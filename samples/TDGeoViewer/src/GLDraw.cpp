#define _WIN32_WINNT 0x0500

#if defined(X11)
	#include <unistd.h>
	#include "X11/Xlib.h"
	#include "X11/Xutil.h"
#endif

#define DYNAMICGLES_NO_NAMESPACE
#define DYNAMICEGL_NO_NAMESPACE

#include <fstream>
#include <iostream>
#include <cstdarg>
#include <memory>

#include <DynamicGles.h>

#include "GLDraw.hpp"

static void sys_dbg_msg(const char* pFmt, ...) {
	char buf[1024];
	va_list lst;
	va_start(lst, pFmt);
#ifdef _MSC_VER
	vsprintf_s(buf, sizeof(buf), pFmt, lst);
#else
	vsprintf(buf, pFmt, lst);
#endif
	va_end(lst);
#ifdef _WIN32
	::OutputDebugStringA(buf);
#elif defined(UNIX)
	std::cout << buf << std::endl;
#endif
}

static struct GLESApp {
#ifdef _WIN32
	HINSTANCE mhInstance;
	ATOM mClassAtom;
	HWND mNativeWindow;
#elif defined(UNIX)
	#if defined(X11)
		Display* mpNativeDisplay;
		Window mNativeWindow;
	#endif
#endif
	EGLNativeDisplayType mNativeDisplayHandle; // Win : HDC; X11 : Display

	struct EGL {
		EGLDisplay display;
		EGLSurface surface;
		EGLContext context;
		EGLConfig config;

		void reset() {
			display = EGL_NO_DISPLAY;
			surface = EGL_NO_SURFACE;
			context = EGL_NO_CONTEXT;
		}
	} mEGL;


	struct VIEW {
		glm::mat4x4 mViewMtx;
		glm::mat4x4 mProjMtx;
		glm::mat4x4 mViewProjMtx;
		glm::vec3 mPos;
		glm::vec3 mTgt;
		glm::vec3 mUp;
		int mWidth;
		int mHeight;
		float mAspect;
		float mFOVY;
		float mNear;
		float mFar;

		void set(const glm::vec3& pos, const glm::vec3& tgt, const glm::vec3& up) {
			mPos = pos;
			mTgt = tgt;
			mUp = up;
		}		
	} mView;

	glm::vec3 mClearColor;

	void init_wnd();
	void init_egl();
	void init_gpu();

	void reset_wnd();
	void reset_egl();
	void reset_gpu();

	void init(const GLDrawCfg& cfg);
	void reset();

	bool valid_display() const { return mEGL.display != EGL_NO_DISPLAY; }
	bool valid_surface() const { return mEGL.surface != EGL_NO_SURFACE; }
	bool valid_context() const { return mEGL.context != EGL_NO_CONTEXT; }
	bool valid_egl() const { return valid_display() && valid_surface() && valid_context(); }

	void frame_clear() const {
		glColorMask(true, true, true, true);
		glDepthMask(true);
		glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

} s_app;

static bool s_initFlg = false;

namespace GLDraw {

	void init(const GLDrawCfg& cfg) {
		if (s_initFlg) return;
		::memset(&s_app, 0, sizeof(GLESApp));
		s_app.init(cfg);
		s_initFlg = true;
	}

	void reset() {
		if (!s_initFlg) return;
		s_app.reset();
		::memset(&s_app, 0, sizeof(GLESApp	));
		s_initFlg = false;
	}

	void loop(void(*pLoop)());

	void begin() {
		if (!s_app.valid_egl()) { return; }
		s_app.frame_clear();
	}

	void end() {
		if (!s_app.valid_egl()) { return; }
		eglSwapBuffers(s_app.mEGL.display, s_app.mEGL.surface);
	}
}

void GLESApp::init(const GLDrawCfg& cfg) {
#ifdef _WIN32
	mhInstance = (HINSTANCE)cfg.sys.hInstance;
#endif
	mView.mWidth = cfg.width;
	mView.mHeight = cfg.height;
	mView.mAspect = (float)mView.mWidth / mView.mHeight;
	init_wnd();
	init_egl();
	init_gpu();

	mClearColor = glm::vec3(0.33f, 0.44f, 0.55f);
}

void GLESApp::reset() {
	reset_gpu();
	reset_egl();
	reset_wnd();
}

void GLESApp::init_egl() {
	using namespace std;
	sys_dbg_msg("init_egl()");
	if (mNativeDisplayHandle) {
		mEGL.display = eglGetDisplay(mNativeDisplayHandle);
	}
	
	if (!valid_display()) {
		sys_dbg_msg("Failed to get and EGLDisplay");
		return;
	}

	int verMaj = 0;
	int verMin = 0;
	bool flg = eglInitialize(mEGL.display, &verMaj, &verMin);
	if (!flg) return;
	sys_dbg_msg("EGL %d.%d\n", verMaj, verMin);
	flg = eglBindAPI(EGL_OPENGL_ES_API);
	if (flg != EGL_TRUE) {
		sys_dbg_msg("eglBindAPI failed");
		return;
	}

	static EGLint cfgAttrs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLint ncfg = 0;
	flg = eglChooseConfig(mEGL.display, cfgAttrs, &mEGL.config, 1, &ncfg);
	if (flg) flg = ncfg == 1;
	if (!flg) {
		sys_dbg_msg("eglChooseConfig failed");
		return;
	}

	mEGL.surface = eglCreateWindowSurface(mEGL.display, mEGL.config, (EGLNativeWindowType)mNativeWindow, nullptr);
	if (!valid_surface()) {
		sys_dbg_msg("eglCreateWindowSurface failed");
		return;
	}

	static EGLint ctxAttrs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	mEGL.context = eglCreateContext(mEGL.display, mEGL.config, nullptr, ctxAttrs);
	if (!valid_context()) {
		sys_dbg_msg("eglCreateContext failed");
		return;
	}
	if (!eglMakeCurrent(mEGL.display, mEGL.surface, mEGL.surface, mEGL.context)) {
		sys_dbg_msg("eglMakeCurrent failed");
	}

	sys_dbg_msg("finished");
}

void GLESApp::reset_egl() {
	if (valid_display()) {
		eglMakeCurrent(mEGL.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglTerminate(mEGL.display);
	}

	mEGL.reset();
}

void GLESApp::init_gpu() {
}

void GLESApp::reset_gpu() {
}

#ifdef _WIN32
static const TCHAR* s_drwClassName = _T("GLDrawWindow");

static LRESULT CALLBACK drwWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	LRESULT res = 0;
	switch (msg) {
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
	default:
		res = ::DefWindowProc(hWnd, msg, wParam, lParam);
		break;
	}
	return res;
}

void GLESApp::init_wnd() {
	WNDCLASSEX wc;
	::ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.hInstance = mhInstance;
	wc.hCursor = ::LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = s_drwClassName;
	wc.lpfnWndProc = drwWndProc;
	wc.cbWndExtra = 0x10;
	mClassAtom = ::RegisterClassEx(&wc);

	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = mView.mWidth;
	rect.bottom = mView.mHeight;
	int style = WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_GROUP;
	::AdjustWindowRect(&rect, style, FALSE);
	int wndW = rect.right - rect.left;
	int wndH = rect.bottom - rect.top;
	TCHAR title[128];
	::ZeroMemory(title, sizeof(title));
	::_stprintf_s(title, sizeof(title) / sizeof(title[0]), _T("%s: build %s"), _T("drwEGL"), _T(__DATE__));
	mNativeWindow = ::CreateWindowEx(0, s_drwClassName, title, style, 0, 0, wndW, wndH, NULL, NULL, mhInstance, NULL);
	if (mNativeWindow) {
		::ShowWindow(mNativeWindow, SW_SHOW);
		::UpdateWindow(mNativeWindow);
		mNativeDisplayHandle = ::GetDC(mNativeWindow);
	}
}

void GLESApp::reset_wnd() {
	::UnregisterClass(s_drwClassName, mhInstance);
}

void GLDraw::loop(void(*pLoop)()) {
	MSG msg;
	bool done = false;
	while (!done) {
		if (::PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)) {
			if (::GetMessage(&msg, NULL, 0, 0)) {
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			} else {
				done = true;
				break;
			}
		} else {
			if (pLoop) {
				pLoop();
			}
		}
	}
}
#elif defined(X11)
static const char* s_applicationName = "TDGeoViewer";

static int wait_for_MapNotify(Display* pDisp, XEvent* pEvt, char* pArg) 
{ 
    if ((pEvt->type == MapNotify) && (pEvt->xmap.window == (Window)pArg)) { 
    	return 1; 
    } 
    return 0; 
} 

void GLESApp::init_wnd() {
	using namespace std;

	sys_dbg_msg("GLESApp::init_wnd()");
	mpNativeDisplay = XOpenDisplay(0);
	if (mpNativeDisplay == 0) {
		sys_dbg_msg("ERROR: can't open X display");
		return;
	}
	
	int defaultScreen = XDefaultScreen(mpNativeDisplay);
	int defaultDepth = DefaultDepth(mpNativeDisplay, defaultScreen);

	XVisualInfo* pVisualInfo = new XVisualInfo();
	XMatchVisualInfo(mpNativeDisplay, defaultScreen, defaultDepth, TrueColor, pVisualInfo);

	if (pVisualInfo == nullptr) {
		sys_dbg_msg("ERROR: can't aquire visual info");
		return;
	}

	Window rootWindow = RootWindow(mpNativeDisplay, defaultScreen);
	Colormap colorMap = XCreateColormap(mpNativeDisplay, rootWindow, pVisualInfo->visual, AllocNone);

	XSetWindowAttributes windowAttributes;
	windowAttributes.colormap = colorMap;
	windowAttributes.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | KeyPressMask;
	
	mNativeWindow = XCreateWindow(mpNativeDisplay,              // The display used to create the window
	                              rootWindow,                   // The parent (root) window - the desktop
	                              0,                            // The horizontal (x) origin of the window
	                              0,                            // The vertical (y) origin of the window
	                              mView.mWidth,                 // The width of the window
	                              mView.mHeight,                // The height of the window
	                              0,                            // Border size - set it to zero
	                              pVisualInfo->depth,           // Depth from the visual info
	                              InputOutput,                  // Window type - this specifies InputOutput.
	                              pVisualInfo->visual,          // Visual to use
	                              CWEventMask | CWColormap,     // Mask specifying these have been defined in the window attributes
	                              &windowAttributes);           // Pointer to the window attribute structure

	XMapWindow(mpNativeDisplay, mNativeWindow);
	XStoreName(mpNativeDisplay, mNativeWindow, s_applicationName);

	Atom windowManagerDelete = XInternAtom(mpNativeDisplay, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(mpNativeDisplay, mNativeWindow, &windowManagerDelete , 1);

	mNativeDisplayHandle = (EGLNativeDisplayType)mpNativeDisplay;

	XEvent event;
	XIfEvent(mpNativeDisplay, &event, wait_for_MapNotify, (char*)mNativeWindow);
	sys_dbg_msg("finished");
}

void GLESApp::reset_wnd() {
	XDestroyWindow(mpNativeDisplay, mNativeWindow);
	XCloseDisplay(mpNativeDisplay);
}

void GLDraw::loop(void(*pLoop)()) {
	XEvent event;
	bool done = false;
	while (!done) {
		KeySym key;
		while (XPending(s_app.mpNativeDisplay)) {
			XNextEvent(s_app.mpNativeDisplay, &event);
			switch (event.type) {
				case KeyPress:
					done = true;
			}
		}

		if (pLoop) {
			pLoop();
		}

	}
}
#endif
