#define _WIN32_WINNT 0x0500
#define DYNAMICGLES_NO_NAMESPACE
#define DYNAMICEGL_NO_NAMESPACE
#include <DynamicGles.h>
#include <fstream>

#include "GLDraw.hpp"

static struct GLB {
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

	struct GPU {
		GLuint shaderIdVtx;
		GLuint shaderIdPix;
		GLuint programId;
		GLint attrLocPos;
		GLint attrLocNrm;
		GLint attrLocClr;
		GLint prmLocWMtx;
		GLint prmLocViewProj;
		GLint prmLocHemiSky;
		GLint prmLocHemiGround;
		GLint prmLocHemiUp;
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
#if 0
			glm::vec3 dir = glm::normalize(mTgt - mPos);
			glm::vec3 side = glm::normalize(glm::cross(mUp, dir));
			glm::vec3 up = glm::cross(side, dir);
			mViewMtx = glm::transpose(glm::mat4x4(
				glm::vec4(-side, 0),
				glm::vec4(-up, 0),
				glm::vec4(-dir, 0),
				glm::vec4(0, 0, 0, 1)));
			glm::vec4 org = mViewMtx * glm::vec4(-mPos, 1);
			mViewMtx[3] = org;

			float h = mFOVY * 0.5f;
			float c = ::cosf(h) / ::sinf(h);
			float q = mFar / (mFar - mNear);
			mProjMtx = glm::mat4x4(
				glm::vec4(c / mAspect, 0, 0, 0),
				glm::vec4(0, c, 0, 0),
				glm::vec4(0, 0, -q, -1),
				glm::vec4(0, 0, -q * mNear, 0)
			);
#else
			mViewMtx = glm::lookAt(mPos, mTgt, mUp);
			mProjMtx = glm::perspective(mFOVY, mAspect, mNear, mFar);
#endif
			mViewProjMtx = mProjMtx * mViewMtx;
		}
	} mView;

	struct LIGHT {
		glm::vec3 sky;
		glm::vec3 ground;
		glm::vec3 up;
	} mLight;

	glm::vec3 mGamma;

	glm::vec3 mClearColor;

	void init_sys();
	void init_wnd();
	void init_egl();
	void init_gpu();
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
} G;

static bool s_initFlg = false;

static void sys_dbg_msg(const char* pFmt, ...) {
	char buf[1024];
	va_list lst;
	va_start(lst, pFmt);
#ifdef _MSC_VER
	vsprintf_s(buf, sizeof(buf), pFmt, lst);
#else
	vsprintf(pBuf, pFmt, lst);
#endif
	va_end(lst);
#ifdef _WIN32
	::OutputDebugStringA(buf);
#endif
}


std::string load_text(const std::string& path) {
	std::ifstream is(path);
	return std::string((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
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
#endif

void GLB::init_sys() {
#ifdef _WIN32
	::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)drwWndProc, &mhInstance);
#endif
}

void GLB::init_wnd() {
#ifdef _WIN32
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
#endif
}

void GLB::init_egl() {
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

	mEGL.surface = eglCreateWindowSurface(mEGL.display, mEGL.config, mhWnd, nullptr);
	if (!valid_surface()) return;

	static EGLint ctxAttrs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	mEGL.context = eglCreateContext(mEGL.display, mEGL.config, nullptr, ctxAttrs);
	if (!valid_context()) return;
	eglMakeCurrent(mEGL.display, mEGL.surface, mEGL.surface, mEGL.context);
}

void GLB::init_gpu() {
	GLint status;
	GLint infoLen = 0;
	if (!valid_egl()) return;

	std::string vtxSrc = load_text("../../data/obj_vtx.vert");
	if (vtxSrc.length() < 1) return;
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

	std::string pixSrc = load_text("../../data/obj_pix.frag");
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

	sys_dbg_msg("GPU: shaders ok!\n");

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
	mGPU.attrLocPos = glGetAttribLocation(mGPU.programId, "vtxPos");
	mGPU.attrLocNrm = glGetAttribLocation(mGPU.programId, "vtxNrm");
	mGPU.attrLocClr = glGetAttribLocation(mGPU.programId, "vtxClr");
	mGPU.prmLocWMtx = glGetUniformLocation(mGPU.programId, "prmWMtx");
	mGPU.prmLocViewProj = glGetUniformLocation(mGPU.programId, "prmViewProj");
	mGPU.prmLocHemiSky = glGetUniformLocation(mGPU.programId, "prmHemiSky");
	mGPU.prmLocHemiGround = glGetUniformLocation(mGPU.programId, "prmHemiGround");
	mGPU.prmLocHemiUp = glGetUniformLocation(mGPU.programId, "prmHemiUp");
	mGPU.prmLocInvGamma = glGetUniformLocation(mGPU.programId, "prmInvGamma");
}

void GLB::init(const GLDrawCfg& cfg) {
	mView.mWidth = cfg.width;
	mView.mHeight = cfg.height;
	mView.mAspect = (float)mView.mWidth / mView.mHeight;
	init_sys();
	init_wnd();
	init_egl();
	init_gpu();

	mClearColor = glm::vec3(0.33f, 0.44f, 0.55f);

	mGamma = glm::vec3(2.2f);

	mView.set(glm::vec3(0.0f, 1.75f, 1.5f), glm::vec3(0.0f, 1.5f, 0.0f), glm::vec3(0, 1, 0));
	mView.setFOVY(glm::radians(40.0f));
	mView.setRange(0.1f, 1000.0f);
	mView.update();

	mLight.sky = glm::vec3(2.14318f, 1.971372f, 1.862601f) * 0.5f;
	mLight.ground = glm::vec3(0.15f, 0.1f, 0.075f) * 0.5f;
	mLight.up = glm::vec3(0, 1, 0);
}

void GLB::reset() {
	if (valid_display()) {
		eglMakeCurrent(mEGL.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglTerminate(mEGL.display);
	}
	glDeleteShader(mGPU.shaderIdPix);
	glDeleteShader(mGPU.shaderIdVtx);
	glDeleteProgram(mGPU.programId);
	mEGL.reset();
#ifdef _WIN32
	if (mhDC) {
		::ReleaseDC(mhWnd, mhDC);
	}
	::UnregisterClass(s_drwClassName, mhInstance);
#endif
}

struct GLDrawLight {
	glm::vec3 sky;
	glm::vec3 ground;
	glm::vec3 up;
};

struct GLDrawMesh {
	struct Vtx {
		glm::vec3 pos;
		glm::vec3 nrm;
		glm::vec3 clr;
	};

	int mNumVtx;
	int mNumTri;
	GLuint mBuffIdVtx;
	GLuint mBuffIdIdx;

	bool is_idx16() const { return mNumVtx <= (1 << 16); }
	int idx_bytes() const { return is_idx16() ? sizeof(GLushort) : sizeof(GLuint); }
};

static glm::vec3 geo_pnt_pos(const TDGeometry& geo, int ipnt) {
	glm::vec3 v;
	TDGeometry::Point p = geo.get_pnt(ipnt);
	return glm::vec3(p.x, p.y, p.z);
}

static int get_pol_tris(int vlst[6], const TDGeometry& geo, int polIdx) {
	using namespace glm;
	TDGeometry::Poly pol = geo.get_poly(polIdx);
	int ntri = 0;
	if (pol.nvtx == 3) {
		for (int i = 0; i < 3; ++i) {
			vlst[i] = pol.ipnt[i];
		}
		ntri = 1;
	} else if (pol.nvtx == 4) {
		vec3 v[4];
		for (int i = 0; i < 4; ++i) {
			v[i] = geo_pnt_pos(geo, pol.ipnt[i]);
		}
		vec3 e0 = v[0] - v[1];
		vec3 e1 = v[1] - v[2];
		vec3 e2 = v[2] - v[3];
		vec3 e3 = v[3] - v[0];
		int idiv = 0;
		if (dot(cross(e1, e2), cross(e3, e0)) > 0) idiv = 1;
		static int div[2][6] = {
			{0, 1, 2,  0, 2, 3},
			{0, 1, 3,  1, 2, 3}
		};
		for (int i = 0; i < 6; ++i) {
			vlst[i] = pol.ipnt[div[idiv][i]];
		}
		ntri = 2;
	}
	return ntri;
}

namespace GLDraw {

	void init(const GLDrawCfg& cfg) {
		if (s_initFlg) return;
		::memset(&G, 0, sizeof(G));
		G.init(cfg);
		s_initFlg = true;
	}

	void reset() {
		if (!s_initFlg) return;
		G.reset();
		::memset(&G, 0, sizeof(G));
		s_initFlg = false;
	}

	void loop(void(*pLoop)()) {
#ifdef _WIN32
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
#endif
	}

	void setView(const glm::vec3& pos, const glm::vec3& tgt, const glm::vec3& up) {
		G.mView.set(pos, tgt, up);
		G.mView.update();
	}

	void setDegreesFOVY(float deg) {
		G.mView.setFOVY(glm::radians(deg));
		G.mView.update();
	}

	void setViewRange(float znear, float zfar) {
		G.mView.setRange(znear, zfar);
		G.mView.update();
	}

	GLDrawMesh* meshCreate(const TDGeometry& geo) {
		int nvtx = geo.get_pnt_num();
		if (nvtx <= 0) return nullptr;
		int npol = geo.get_poly_num();
		if (npol <= 0) return nullptr;
		int ntri = 0;
		for (int i = 0; i < npol; ++i) {
			TDGeometry::Poly pol = geo.get_poly(i);
			if (pol.nvtx == 3) {
				ntri += 1;
			} else if (pol.nvtx == 4) {
				ntri += 2;
			}
		}
		if (ntri <= 0) return nullptr;
		GLDrawMesh* pMsh = new GLDrawMesh();
		pMsh->mNumVtx = nvtx;
		pMsh->mNumTri = ntri;
		glGenBuffers(1, &pMsh->mBuffIdVtx);
		if (0 != pMsh->mBuffIdVtx) {
			GLDrawMesh::Vtx* pVtx = new GLDrawMesh::Vtx[nvtx];
			for (int i = 0; i < nvtx; i++) {
				TDGeometry::Point pnt = geo.get_pnt(i);
				pVtx[i].pos = glm::vec3(pnt.x, pnt.y, pnt.z);
				pVtx[i].nrm = glm::vec3(pnt.nx, pnt.ny, pnt.nz);
				pVtx[i].clr = glm::vec3(pnt.r, pnt.g, pnt.b);
			}
			glBindBuffer(GL_ARRAY_BUFFER, pMsh->mBuffIdVtx);
			glBufferData(GL_ARRAY_BUFFER, nvtx * sizeof(GLDrawMesh::Vtx), pVtx, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			delete[] pVtx;
		}
		glGenBuffers(1, &pMsh->mBuffIdIdx);
		if (0 != pMsh->mBuffIdIdx) {
			int vlst[6];
			size_t sizeIB = ntri * 3 * pMsh->idx_bytes();
			char* pIdx = new char[sizeIB];
			if (pMsh->is_idx16()) {
				uint16_t* pIB16 = (uint16_t*)pIdx;
				for (int i = 0; i < npol; i++) {
					int n = get_pol_tris(vlst, geo, i);
					for (int j = 0; j < 3 * n; j++) {
						*pIB16++ = (uint16_t)vlst[j];
					}
				}
			} else {
				uint32_t* pIB32 = (uint32_t*)pIdx;
				for (int i = 0; i < npol; i++) {
					int n = get_pol_tris(vlst, geo, i);
					for (int j = 0; j < 3 * n; j++) {
						*pIB32++ = (uint32_t)vlst[j];
					}
				}
			}
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMsh->mBuffIdIdx);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeIB, pIdx, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			delete[] pIdx;
		}
		return pMsh;
	}

	void meshDestroy(GLDrawMesh* pMsh) {
		if (!pMsh) return;
		glDeleteBuffers(1, &pMsh->mBuffIdIdx);
		glDeleteBuffers(1, &pMsh->mBuffIdVtx);
		delete pMsh;
	}

	void meshDraw(GLDrawMesh* pMsh, const glm::mat4x4& worldMtx) {
		if (!G.valid_egl()) return;
		if (!pMsh) return;
		if (0 == pMsh->mBuffIdVtx) return;
		if (0 == pMsh->mBuffIdIdx) return;

		glUseProgram(G.mGPU.programId);

		glUniform3f(G.mGPU.prmLocInvGamma, 1.0f / G.mGamma.r, 1.0f / G.mGamma.g, 1.0f / G.mGamma.b);
		glUniform3fv(G.mGPU.prmLocHemiSky, 1, (float*)&G.mLight.sky);
		glUniform3fv(G.mGPU.prmLocHemiGround, 1, (float*)&G.mLight.ground);
		glUniform3fv(G.mGPU.prmLocHemiUp, 1, (float*)&G.mLight.up);

		glm::mat4x4& tm = glm::transpose(G.mView.mViewProjMtx);
		glUniformMatrix4fv(G.mGPU.prmLocViewProj, 1, GL_FALSE, (float*)&tm);
		tm = glm::transpose(worldMtx);
		glUniform4fv(G.mGPU.prmLocWMtx, 3, (float*)&tm);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
#if 0
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW); // TD
		glCullFace(GL_BACK);
#endif

		const GLsizei vstride = (GLsizei)sizeof(GLDrawMesh::Vtx);
		glBindBuffer(GL_ARRAY_BUFFER, pMsh->mBuffIdVtx);
		glEnableVertexAttribArray(G.mGPU.attrLocPos);
		glVertexAttribPointer(G.mGPU.attrLocPos, 3, GL_FLOAT, GL_FALSE, vstride, (const void*)offsetof(GLDrawMesh::Vtx, pos));
		glEnableVertexAttribArray(G.mGPU.attrLocNrm);
		glVertexAttribPointer(G.mGPU.attrLocNrm, 3, GL_FLOAT, GL_FALSE, vstride, (const void*)offsetof(GLDrawMesh::Vtx, nrm));
		glEnableVertexAttribArray(G.mGPU.attrLocClr);
		glVertexAttribPointer(G.mGPU.attrLocClr, 3, GL_FLOAT, GL_FALSE, vstride, (const void*)offsetof(GLDrawMesh::Vtx, clr));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMsh->mBuffIdIdx);
		glDrawElements(GL_TRIANGLES, pMsh->mNumTri * 3, pMsh->is_idx16() ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, 0);

		glDisableVertexAttribArray(G.mGPU.attrLocPos);
		glDisableVertexAttribArray(G.mGPU.attrLocNrm);
		glDisableVertexAttribArray(G.mGPU.attrLocClr);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void begin() {
		if (!G.valid_egl()) return;
		G.frame_clear();
	}

	void end() {
		if (!G.valid_egl()) return;
		eglSwapBuffers(G.mEGL.display, G.mEGL.surface);
	}

	glm::mat4x4 degX(float deg) { return glm::rotate(glm::mat4x4(1.0f), glm::radians(deg), glm::vec3(1, 0, 0)); }
	glm::mat4x4 degY(float deg) { return glm::rotate(glm::mat4x4(1.0f), glm::radians(deg), glm::vec3(0, 1, 0)); }
	glm::mat4x4 degZ(float deg) { return glm::rotate(glm::mat4x4(1.0f), glm::radians(deg), glm::vec3(0, 0, 1)); }
	glm::mat4x4 scl(const glm::vec3& s) { return glm::scale(glm::mat4x4(1.0f), s); }

	glm::mat4x4 xformSRTXYZ(const glm::vec3& translate, const glm::vec3& rotateDegrees, const glm::vec3& scale) {
		float dx = rotateDegrees.x;
		float dy = rotateDegrees.y;
		float dz = rotateDegrees.z;
		glm::mat4x4 rm = degZ(dz) * degY(dy) * degX(dx);
		glm::mat4x4 sm = scl(scale);
		glm::mat4x4 mtx = rm * sm;
		mtx[3] = glm::vec4(translate, 1);
		return mtx;
	}

	glm::mat4x4 xformSRTXYZ(float tx, float ty, float tz, float rx, float ry, float rz, float sx, float sy, float sz) {
		return xformSRTXYZ(glm::vec3(tx, ty, tz), glm::vec3(rx, ry, rz), glm::vec3(sx, sy, sz));
	}

} // GLDraw
