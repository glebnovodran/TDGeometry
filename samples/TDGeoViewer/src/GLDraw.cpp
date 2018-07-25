#define _WIN32_WINNT 0x0500
#define DYNAMICGLES_NO_NAMESPACE
#define DYNAMICEGL_NO_NAMESPACE
#include <DynamicGles.h>
#include <fstream>
#include <cstdarg>

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
#endif
}

static struct GLESApp {
#ifdef _WIN32
	HINSTANCE mhInstance;
	ATOM mClassAtom;
	HWND mhWnd;
	HDC mhDC;
#endif

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
	if (!valid_display()) return;
	int verMaj = 0;
	int verMin = 0;
	bool flg = eglInitialize(mEGL.display, &verMaj, &verMin);
	if (!flg) return;
	sys_dbg_msg("EGL %d.%d\n", verMaj, verMin);
	flg = eglBindAPI(EGL_OPENGL_ES_API);
	if (flg != EGL_TRUE) return;

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
	if (!flg) return;
#ifdef _WIN32
	mEGL.surface = eglCreateWindowSurface(mEGL.display, mEGL.config, mhWnd, nullptr);
#else
#endif
	if (!valid_surface()) return;

	static EGLint ctxAttrs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	mEGL.context = eglCreateContext(mEGL.display, mEGL.config, nullptr, ctxAttrs);
	if (!valid_context()) return;
	eglMakeCurrent(mEGL.display, mEGL.surface, mEGL.surface, mEGL.context);
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
	mhWnd = ::CreateWindowEx(0, s_drwClassName, title, style, 0, 0, wndW, wndH, NULL, NULL, mhInstance, NULL);
	if (mhWnd) {
		::ShowWindow(mhWnd, SW_SHOW);
		::UpdateWindow(mhWnd);
		mhDC = ::GetDC(mhWnd);
		if (mhDC) {
			mEGL.display = eglGetDisplay(mhDC);
		}
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
#elif defined(UNIX)
void GLESApp::init_wnd() {
}
void GLESApp::reset_wnd() {
}
void GLDraw::loop(void(*pLoop)()) {
}
#endif
