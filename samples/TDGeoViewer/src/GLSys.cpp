#define _WIN32_WINNT 0x0500

extern const char* s_applicationName;

#if defined(X11)
	#include <unistd.h>
	#include "X11/Xlib.h"
	#include "X11/Xutil.h"
#endif

#define DYNAMICGLES_NO_NAMESPACE
#define DYNAMICEGL_NO_NAMESPACE

// undefine _CONSOLE for DynamicGles.h include to avoid PVR SDK R2
// compilation problem with win32 console apps
#if defined(_WIN32)
#undef _CONSOLE
#endif
#include <DynamicGles.h>
#if defined(_WIN32)
#define _CONSOLE
#endif

//#include <fstream>
#include <iostream>
#include <cstdarg>
//#include <memory>

#include "GLSys.hpp"

void sys_dbg_msg(const char* pFmt, ...) {
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
	OutputDebugStringA(buf);
#elif defined(UNIX)
	std::cout << buf << std::endl;
#endif
}

bool gl_error() {
#ifdef _DEBUG
	GLenum lastError = glGetError();
	if (lastError != GL_NO_ERROR) {
		sys_dbg_msg("GLES failed with (%x).\n", lastError);
		return true;
	}
#endif
	return false;
}

bool egl_has_extention(EGLDisplay eglDisplay, const char* name) {
	const char* extns = eglQueryString(eglDisplay, EGL_EXTENSIONS);
	return extns != NULL && strstr(extns, name);
}

static struct GLSysGlobal {
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

	int mWndOrgX;
	int mWndOrgY;
	int mWndW;
	int mWndH;
	int mWidth;
	int mHeight;

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

	bool valid_display() const { return mEGL.display != EGL_NO_DISPLAY; }
	bool valid_surface() const { return mEGL.surface != EGL_NO_SURFACE; }
	bool valid_context() const { return mEGL.context != EGL_NO_CONTEXT; }
	bool valid_egl() const { return valid_display() && valid_surface() && valid_context(); }

	void stop_egl() {
		if (valid_display()) {
			eglMakeCurrent(mEGL.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglTerminate(mEGL.display);
		}
	}

	void init_sys();
	void init_wnd();
	void init_egl();

	void reset_sys();
	void reset_wnd();
	void reset_egl();
} s_global;

//s_glb
namespace GLSys {

	bool s_initFlg = false;
	void init(const GLSysCfg& cfg) {
		if (s_initFlg) return;
		s_global.mWndOrgX = cfg.x;
		s_global.mWndOrgY = cfg.y;
		s_global.mWidth = cfg.w;
		s_global.mHeight = cfg.h;
		s_global.init_sys();
		s_global.init_wnd();
		s_global.init_egl();
		s_initFlg = true;
	}

	void reset() {
		if (!s_initFlg) return;
		s_global.reset_sys();
		s_global.reset_egl();
		s_global.reset_wnd();
		s_initFlg = false;
	}

	void stop() {
		s_global.stop_egl();
	}

	void swap() {
		eglSwapBuffers(s_global.mEGL.display, s_global.mEGL.surface);
	}

	bool valid() {
		return s_global.valid_egl();
	}

	GLuint compile_shader_str(const std::string& src, GLenum kind) {
		GLuint sid = 0;
		if (valid() && !src.empty()) {
			sid = glCreateShader(kind);
			if (sid) {
				//GLint len[1] = { (GLint)srcSize };
				const char* pSrc = src.c_str();
				glShaderSource(sid, 1, (const GLchar* const*)&pSrc, NULL);
				glCompileShader(sid);
				GLint status = 0;
				glGetShaderiv(sid, GL_COMPILE_STATUS, &status);
				if (!status) {
					GLint infoLen = 0;
					glGetShaderiv(sid, GL_INFO_LOG_LENGTH, &infoLen);
					if (infoLen > 0) {
						char* pInfo = new char[infoLen];
						if (pInfo) {
							glGetShaderInfoLog(sid, infoLen, &infoLen, pInfo);
							sys_dbg_msg(pInfo);
							delete[] pInfo;
							pInfo = nullptr;
						}
					}
					glDeleteShader(sid);
					sid = 0;
				}
			}
		}
		return sid;
	}

	GLuint compile_prog_strs(const std::string& srcVtx, const std::string& srcFrag, GLuint* pSIdVtx, GLuint* pSIdFrag) {
		GLuint pid = 0;
		GLuint sidVtx = 0;
		GLuint sidFrag = 0;
		union {
			struct {
				uint32_t vtx  :   1;
				uint32_t frag :   1;
				uint32_t prog :   1;
				uint32_t linked : 1;
			};
			uint32_t all;
		} init = { 0 };

		if (GLSys::valid() && !srcVtx.empty() && !srcFrag.empty()) {
			sidVtx = compile_shader_str(srcVtx, GL_VERTEX_SHADER);
			if (sidVtx) {
				init.vtx = 1;
				sidFrag = compile_shader_str(srcFrag, GL_FRAGMENT_SHADER);
				if (sidFrag) {
					init.frag = 1;
					pid = glCreateProgram();
					if (pid) {
						init.prog = 1;
						glAttachShader(pid, sidVtx);
						glAttachShader(pid, sidFrag);
						glLinkProgram(pid);
						GLint status = 0;
						glGetProgramiv(pid, GL_LINK_STATUS, &status);
						if (!status) {
							init.linked = 1;
							GLint infoLen = 0;
							glGetProgramiv(pid, GL_INFO_LOG_LENGTH, &infoLen);
							if (infoLen > 0) {
								char* pInfo = new char[infoLen];
								if (pInfo) {
									glGetProgramInfoLog(pid, infoLen, &infoLen, pInfo);
									delete[] pInfo;
									pInfo = nullptr;
								}
							}
						}
					}
				}
			}

			if (init.all != 0xF) { // if incomplete
				if (init.vtx) {
					glDeleteShader(sidVtx);
					sidVtx = 0;
				}
				if (init.frag) {
					glDeleteShader(sidFrag);
					sidFrag = 0;
				}
				if (init.prog) {
					glDeleteProgram(pid);
					pid = 0;
				}
			}
		}
		if (pSIdVtx) {
			*pSIdVtx = sidVtx;
		}
		if (pSIdFrag) {
			*pSIdFrag = sidFrag;
		}
		return pid;
	}

}

void GLSysGlobal::init_egl() {
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
	bool hasCreateCtxExt = egl_has_extention(mEGL.display, "EGL_KHR_create_context");
	EGLint ctxType = hasCreateCtxExt ? EGL_OPENGL_ES3_BIT_KHR : EGL_OPENGL_ES2_BIT;
	static EGLint cfgAttrs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, ctxType,
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
		EGL_CONTEXT_CLIENT_VERSION, hasCreateCtxExt ? 3 : 2,
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

void GLSysGlobal::reset_egl() {
	stop_egl();
	mEGL.reset();
}

#ifdef _WIN32

static const TCHAR* s_drwClassName = _T("GLDrawWindow");

static LRESULT CALLBACK drwWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	LRESULT res = 0;
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		res = DefWindowProc(hWnd, msg, wParam, lParam);
		break;
	}
	return res;
}

void GLSysGlobal::init_sys() {
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)drwWndProc, &mhInstance)) {
		sys_dbg_msg("Can't obtain instance handle");
	}
}

void GLSysGlobal::reset_sys() {}

void GLSysGlobal::init_wnd() {
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_VREDRAW | CS_HREDRAW;

	wc.hInstance = mhInstance;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = s_drwClassName;
	wc.lpfnWndProc = drwWndProc;
	wc.cbWndExtra = 0x10;
	mClassAtom = RegisterClassEx(&wc);

	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = mWidth;
	rect.bottom = mHeight;
	int style = WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_GROUP;
	AdjustWindowRect(&rect, style, FALSE);
	mWndW = rect.right - rect.left;
	mWndH = rect.bottom - rect.top;
	TCHAR title[128];
	ZeroMemory(title, sizeof(title));
	_stprintf_s(title, sizeof(title) / sizeof(title[0]), _T("%s: build %s"), s_applicationName, _T(__DATE__));
	mNativeWindow = CreateWindowEx(0, s_drwClassName, title, style, mWndOrgX, mWndOrgY, mWndW, mWndH, NULL, NULL, mhInstance, NULL);
	if (mNativeWindow) {
		ShowWindow(mNativeWindow, SW_SHOW);
		UpdateWindow(mNativeWindow);
		mNativeDisplayHandle = GetDC(mNativeWindow);
	}
}

void GLSysGlobal::reset_wnd() {
	if (mNativeDisplayHandle) {
		ReleaseDC(mNativeWindow, mNativeDisplayHandle);
	}
	UnregisterClass(s_drwClassName, mhInstance);
}

void GLSys::loop(void(*pLoop)()) {
	MSG msg;
	bool done = false;
	while (!done) {
		if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)) {
			if (GetMessage(&msg, NULL, 0, 0)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
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

static int wait_for_MapNotify(Display* pDisp, XEvent* pEvt, char* pArg) 
{
	if ((pEvt->type == MapNotify) && (pEvt->xmap.window == (Window)pArg)) { 
		return 1;
	}
	return 0;
}

void GLSysGlobal::init_sys() {}
void GLSysGlobal::reset_sys() {}

void GLSysGlobal::init_wnd() {
	using namespace std;

	sys_dbg_msg("GLSysGlobal::init_wnd()");
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
	mWndW = mWidth;
	mWndH = mHeight;
	mNativeWindow = XCreateWindow(mpNativeDisplay,
								rootWindow,
								0,
								0,
								mWndW,
								mWndH,
								0,
								pVisualInfo->depth,
								InputOutput,
								pVisualInfo->visual,
								CWEventMask | CWColormap,
								&windowAttributes);

	XMapWindow(mpNativeDisplay, mNativeWindow);
	XStoreName(mpNativeDisplay, mNativeWindow, s_applicationName);

	Atom windowManagerDelete = XInternAtom(mpNativeDisplay, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(mpNativeDisplay, mNativeWindow, &windowManagerDelete , 1);

	mNativeDisplayHandle = (EGLNativeDisplayType)mpNativeDisplay;

	XEvent event;
	XIfEvent(mpNativeDisplay, &event, wait_for_MapNotify, (char*)mNativeWindow);
	sys_dbg_msg("finished");
}

void GLSysGlobal::reset_wnd() {
	XDestroyWindow(mpNativeDisplay, mNativeWindow);
	XCloseDisplay(mpNativeDisplay);
}

void GLSys::loop(void(*pLoop)()) {
	XEvent event;
	bool done = false;
	while (!done) {
		KeySym key;
		while (XPending(s_global.mpNativeDisplay)) {
			XNextEvent(s_global.mpNativeDisplay, &event);
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


