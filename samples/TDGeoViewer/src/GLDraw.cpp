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
#include <gtx/euler_angles.hpp>

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

std::string load_text(const std::string& path) {
	std::ifstream is(path);
	return std::string((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
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

	struct GPU {
		GLuint shaderIdVtx;
		GLuint shaderIdPix;
		GLuint programId;
		GLint attrLocPos;
		GLint attrLocNrm;
		GLint attrLocClr;
		GLint prmLocWMtx;
		GLint prmLocViewProj;
		GLint prmLocViewPos;
		GLint prmLocHemiSky;
		GLint prmLocHemiGround;
		GLint prmLocHemiUp;
		GLint prmSpecDir;
		GLint prmSpecClr;
		GLint prmSpecRough;
		GLint prmLocInvGamma;
	} mGPU;

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

		void setRange(float znear, float zfar) {
			mNear = znear;
			mFar = zfar;
		}

		void setFOVY(float fovy) {
			mFOVY = fovy;
		}

		void update() {
			mViewMtx = glm::lookAt(mPos, mTgt, mUp);
			mProjMtx = glm::perspective(mFOVY, mAspect, mNear, mFar);
			mViewProjMtx = mProjMtx * mViewMtx;
		}

	} mView;

	struct LIGHT {
		glm::vec3 sky;
		glm::vec3 ground;
		glm::vec3 up;
		glm::vec3 specDir;
		glm::vec3 specClr;
		float specRoughness;
	} mLight;

	glm::vec3 mGamma;

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

	void set_view(const glm::vec3& pos, const glm::vec3& tgt, const glm::vec3& up) {
		s_app.mView.set(pos, tgt, up);
		s_app.mView.update();
	}

	void set_degreesFOVY(float deg) {
		s_app.mView.setFOVY(glm::radians(deg));
		s_app.mView.update();
	}

	void set_view_range(float znear, float zfar) {
		s_app.mView.setRange(znear, zfar);
		s_app.mView.update();
	}

	void set_hemi_light(const glm::vec3& sky, const glm::vec3& ground, const glm::vec3& up) {
		s_app.mLight.sky = sky;
		s_app.mLight.ground = ground;
		s_app.mLight.up = up;
	}

	void set_spec_light(const glm::vec3& dir, const glm::vec3& clr, float roughness) {
		s_app.mLight.specDir = dir;
		s_app.mLight.specClr = clr;
		s_app.mLight.specRoughness = roughness;
	}

	int Mesh::idx_bytes() const { return is_idx16() ? sizeof(GLushort) : sizeof(GLuint); }

	// geo should contain a triangulated geometry
	Mesh* Mesh::create(const TDGeometry& geo) {
		uint32_t vtxNum = geo.get_pnt_num();
		uint32_t triNum = geo.get_poly_num();
		if ((vtxNum == 0) || (triNum == 0)) { return nullptr; }
		Mesh* pMsh = new Mesh();
		pMsh->mNumVtx = vtxNum;
		pMsh->mNumTri = triNum;

		glGenBuffers(2, &pMsh->mBuffIdVtx);

		if (0 != pMsh->mBuffIdVtx) {
			Mesh::Vtx * pVtx = new Vtx[vtxNum];
			for (uint32_t i = 0; i < vtxNum; i++) {
				TDGeometry::Point pnt = geo.get_pnt(i);
				pVtx[i].pos = glm::vec3(pnt.x, pnt.y, pnt.z);
				pVtx[i].nrm = glm::vec3(pnt.nx, pnt.ny, pnt.nz);
				pVtx[i].clr = glm::vec3(pnt.r, pnt.g, pnt.b);
			}
			glBindBuffer(GL_ARRAY_BUFFER, pMsh->mBuffIdVtx);
			glBufferData(GL_ARRAY_BUFFER, vtxNum * sizeof(Vtx), pVtx, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			delete[] pVtx;
		} else { gl_error(); }

		if (0 != pMsh->mBuffIdIdx) {
			size_t sizeIB = triNum * 3 * pMsh->idx_bytes();
			char* pIdx = new char[sizeIB];
			if (pMsh->is_idx16()) {
				uint16_t* pIB16 = (uint16_t*)pIdx;
				for (uint32_t i = 0; i < triNum; i++) {
					TDGeometry::Poly pol = geo.get_poly(i);
					for (uint32_t j = 0; j < 3; j++) {
						*pIB16++ = (uint16_t)pol.ipnt[j];
					}
				}
			} else {
				uint32_t* pIB32 = (uint32_t*)pIdx;
				for (uint32_t i = 0; i < triNum; i++) {
					TDGeometry::Poly pol = geo.get_poly(i);
					for (uint32_t j = 0; j < 3; j++) {
						*pIB32++ = (uint32_t)pol.ipnt[j];
					}
				}
			}
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMsh->mBuffIdIdx);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeIB, pIdx, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			delete[] pIdx;
		} else { gl_error(); }

		return pMsh;
	}

	void Mesh::destroy() {
		if (0 != mBuffIdIdx) {
			glDeleteBuffers(1, &mBuffIdIdx);
			mBuffIdIdx = 0;
		}
		if (0 != mBuffIdVtx) {
			glDeleteBuffers(1, &mBuffIdVtx);
			mBuffIdVtx = 0;
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void Mesh::draw(const glm::mat4x4& worldMtx) {
		if (!s_app.valid_egl()) { return; }
		if (0 == mBuffIdVtx) { return; }
		if (0 == mBuffIdIdx) { return; }

		glUseProgram(s_app.mGPU.programId);

		glUniform3f(s_app.mGPU.prmLocInvGamma, 1.0f / s_app.mGamma.r, 1.0f / s_app.mGamma.g, 1.0f / s_app.mGamma.b);
		glUniform3fv(s_app.mGPU.prmLocHemiSky, 1, (float*)&s_app.mLight.sky);
		glUniform3fv(s_app.mGPU.prmLocHemiGround, 1, (float*)&s_app.mLight.ground);
		glUniform3fv(s_app.mGPU.prmLocHemiUp, 1, (float*)&s_app.mLight.up);

		glUniform3fv(s_app.mGPU.prmSpecDir, 1, (float*)&s_app.mLight.specDir);
		glUniform3fv(s_app.mGPU.prmSpecClr, 1, (float*)&s_app.mLight.specClr);
		glUniform1f(s_app.mGPU.prmSpecRough, s_app.mLight.specRoughness);

		glUniform3fv(s_app.mGPU.prmLocViewPos, 1, (float*)&s_app.mView.mPos);

		glm::mat4x4 tm = glm::transpose(s_app.mView.mViewProjMtx);
		glUniformMatrix4fv(s_app.mGPU.prmLocViewProj, 1, GL_FALSE, (float*)&tm);
		tm = glm::transpose(worldMtx);
		glUniform4fv(s_app.mGPU.prmLocWMtx, 3, (float*)&tm);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
#if 0
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW); // TD
		glCullFace(GL_BACK);
#endif

		const GLsizei vstride = (GLsizei)sizeof(Mesh::Vtx);
		glBindBuffer(GL_ARRAY_BUFFER, mBuffIdVtx);
		glEnableVertexAttribArray(s_app.mGPU.attrLocPos);
		glVertexAttribPointer(s_app.mGPU.attrLocPos, 3, GL_FLOAT, GL_FALSE, vstride, (const void*)offsetof(Mesh::Vtx, pos));
		glEnableVertexAttribArray(s_app.mGPU.attrLocNrm);
		glVertexAttribPointer(s_app.mGPU.attrLocNrm, 3, GL_FLOAT, GL_FALSE, vstride, (const void*)offsetof(Mesh::Vtx, nrm));
		glEnableVertexAttribArray(s_app.mGPU.attrLocClr);
		glVertexAttribPointer(s_app.mGPU.attrLocClr, 3, GL_FLOAT, GL_FALSE, vstride, (const void*)offsetof(Mesh::Vtx, clr));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBuffIdIdx);
		glDrawElements(GL_TRIANGLES, mNumTri * 3, is_idx16() ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, 0);

		glDisableVertexAttribArray(s_app.mGPU.attrLocPos);
		glDisableVertexAttribArray(s_app.mGPU.attrLocNrm);
		glDisableVertexAttribArray(s_app.mGPU.attrLocClr);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	}

	glm::mat4x4 scl(const glm::vec3& s) { return glm::scale(glm::mat4x4(1.0f), s); }

	glm::mat4x4 xformSRTXYZ(const glm::vec3& translate, const glm::vec3& rotateDegrees, const glm::vec3& scale) {
		glm::vec3 rotateRadians = glm::radians(rotateDegrees);
		glm::mat4x4 rm = glm::eulerAngleZ(rotateRadians.z) * glm::eulerAngleY(rotateRadians.y) * glm::eulerAngleX(rotateRadians.x);
		glm::mat4x4 sm = scl(scale);
		glm::mat4x4 mtx = rm * sm;
		mtx[3] = glm::vec4(translate, 1);
		return mtx;
	}

	glm::mat4x4 xformSRTXYZ(float tx, float ty, float tz, float rx, float ry, float rz, float sx, float sy, float sz) {
		return xformSRTXYZ(glm::vec3(tx, ty, tz), glm::vec3(rx, ry, rz), glm::vec3(sx, sy, sz));
	}
}

void GLESApp::init(const GLDrawCfg& cfg) {
	mView.mWidth = cfg.width;
	mView.mHeight = cfg.height;
	mView.mAspect = (float)mView.mWidth / mView.mHeight;
	mView.setFOVY(glm::radians(40.0f));
	mView.setRange(0.1f, 1000.0f);

	mClearColor = glm::vec3(0.33f, 0.44f, 0.55f);
	mGamma = glm::vec3(2.2f);

	init_wnd();
	init_egl();
	init_gpu();
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
	GLint status;
	GLint infoLen = 0;

	if (!valid_egl()) { return; }

	std::string vtxSrc = load_text("../../data/shader/vtx.vert");
	if (vtxSrc.length() < 1) { return; }
	const char* pVtxSrc = vtxSrc.c_str();
	mGPU.shaderIdVtx = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(mGPU.shaderIdVtx, 1, &pVtxSrc, nullptr);
	glCompileShader(mGPU.shaderIdVtx);
	glGetShaderiv(mGPU.shaderIdVtx, GL_COMPILE_STATUS, &status);
	pVtxSrc = nullptr;

	if (!status) {
		sys_dbg_msg("GPU: vertex shader compilation failed.\n");
		glGetShaderiv(mGPU.shaderIdVtx, GL_INFO_LOG_LENGTH, &infoLen);
		char* pInfo = new char[infoLen];
		if (pInfo) {
			glGetShaderInfoLog(mGPU.shaderIdVtx, infoLen, &infoLen, pInfo);
			sys_dbg_msg(pInfo);
			delete[] pInfo;
		}
		return;
	}

	std::string pixSrc = load_text("../../data/shader/hemidir.frag");
	if (pixSrc.length() < 1) return;
	const char* pPixSrc = pixSrc.c_str();
	mGPU.shaderIdPix = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(mGPU.shaderIdPix, 1, &pPixSrc, nullptr);
	glCompileShader(mGPU.shaderIdPix);
	glGetShaderiv(mGPU.shaderIdPix, GL_COMPILE_STATUS, &status);
	pPixSrc = nullptr;

	if (!status) {
		sys_dbg_msg("GPU: pixel shader compilation failed.\n");
		glGetShaderiv(mGPU.shaderIdPix, GL_INFO_LOG_LENGTH, &infoLen);
		char* pInfo = new char[infoLen];
		if (pInfo) {
			glGetShaderInfoLog(mGPU.shaderIdPix, infoLen, &infoLen, pInfo);
			sys_dbg_msg(pInfo);
			delete[] pInfo;
		}
		return;
	}

	sys_dbg_msg("GPU: shaders ok.\n");
	mGPU.programId = glCreateProgram();
	glAttachShader(mGPU.programId, mGPU.shaderIdVtx);
	glAttachShader(mGPU.programId, mGPU.shaderIdPix);
	glLinkProgram(mGPU.programId);
	glGetProgramiv(mGPU.programId, GL_LINK_STATUS, &status);
	if (!status) {
		sys_dbg_msg("GPU: program link failed.\n");
		glGetProgramiv(mGPU.programId, GL_INFO_LOG_LENGTH, &infoLen);
		char* pInfo = new char[infoLen];
		if (pInfo) {
			glGetShaderInfoLog(mGPU.shaderIdPix, infoLen, &infoLen, pInfo);
			sys_dbg_msg(pInfo);
			delete[] pInfo;
		}
		return;
	}
	sys_dbg_msg("GPU: program ok.\n");

	mGPU.attrLocPos = glGetAttribLocation(mGPU.programId, "vtxPos");
	mGPU.attrLocNrm = glGetAttribLocation(mGPU.programId, "vtxNrm");
	mGPU.attrLocClr = glGetAttribLocation(mGPU.programId, "vtxClr");
	mGPU.prmLocWMtx = glGetUniformLocation(mGPU.programId, "prmWMtx");
	mGPU.prmLocViewProj = glGetUniformLocation(mGPU.programId, "prmViewProj");
	mGPU.prmLocViewPos = glGetUniformLocation(mGPU.programId, "prmViewPos");
	mGPU.prmLocHemiSky = glGetUniformLocation(mGPU.programId, "prmHemiSky");
	mGPU.prmLocHemiGround = glGetUniformLocation(mGPU.programId, "prmHemiGround");
	mGPU.prmLocHemiUp = glGetUniformLocation(mGPU.programId, "prmHemiUp");
	mGPU.prmSpecDir = glGetUniformLocation(mGPU.programId, "prmSpecDir");
	mGPU.prmSpecClr = glGetUniformLocation(mGPU.programId, "prmSpecClr");
	mGPU.prmSpecRough = glGetUniformLocation(mGPU.programId, "prmSpecRough");
	mGPU.prmLocInvGamma = glGetUniformLocation(mGPU.programId, "prmInvGamma");
}

void GLESApp::reset_gpu() {
	glDeleteShader(mGPU.shaderIdPix);
	glDeleteShader(mGPU.shaderIdVtx);
	glDeleteProgram(mGPU.programId);
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

void GLESApp::init_wnd() {
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_VREDRAW | CS_HREDRAW;

	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)drwWndProc, &mhInstance)) {
		sys_dbg_msg("Can't obtain instance handle");
		return;
	}

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
	rect.right = mView.mWidth;
	rect.bottom = mView.mHeight;
	int style = WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_GROUP;
	AdjustWindowRect(&rect, style, FALSE);
	int wndW = rect.right - rect.left;
	int wndH = rect.bottom - rect.top;
	TCHAR title[128];
	ZeroMemory(title, sizeof(title));
	_stprintf_s(title, sizeof(title) / sizeof(title[0]), _T("%s: build %s"), _T("drwEGL"), _T(__DATE__));
	mNativeWindow = CreateWindowEx(0, s_drwClassName, title, style, 0, 0, wndW, wndH, NULL, NULL, mhInstance, NULL);
	if (mNativeWindow) {
		ShowWindow(mNativeWindow, SW_SHOW);
		UpdateWindow(mNativeWindow);
		mNativeDisplayHandle = GetDC(mNativeWindow);
	}
}

void GLESApp::reset_wnd() {
	UnregisterClass(s_drwClassName, mhInstance);
}

void GLDraw::loop(void(*pLoop)()) {
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
