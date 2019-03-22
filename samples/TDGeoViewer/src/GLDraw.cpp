#define DYNAMICGLES_NO_NAMESPACE
#define DYNAMICEGL_NO_NAMESPACE

#include <fstream>
#include <iostream>
#include <cstdarg>
#include <memory>

// undefine _CONSOLE for DynamicGles.h include to avoid PVR SDK R2
// compilation problem with win32 console apps
#if defined(_WIN32)
#undef _CONSOLE
#endif
#include <DynamicGles.h>
#if defined(_WIN32)
#define _CONSOLE
#endif

#include "GLSys.hpp"
#include "GLDraw.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/euler_angles.hpp>


#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif


std::string load_text(const std::string& path) {
	std::ifstream is(path);
	return std::string((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
}

static struct GLESApp {
	char* mAppPath;

	struct GPU {
		GLuint shaderIdVtx;
		GLuint shaderIdFrag;
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
		float mFOVY;
		float mNear;
		float mFar;

		void set(const glm::vec3& pos, const glm::vec3& tgt, const glm::vec3& up) {
			mPos = pos;
			mTgt = tgt;
			mUp = up;
		}

		void set_range(float znear, float zfar) {
			mNear = znear;
			mFar = zfar;
		}

		void set_FOVY(float fovy) {
			mFOVY = fovy;
		}

		float get_aspect() const {
			return (float)mWidth / (float)mHeight;
		}

		void update() {
			mViewMtx = glm::lookAt(mPos, mTgt, mUp);
			mProjMtx = glm::perspective(mFOVY, get_aspect(), mNear, mFar);
			mViewProjMtx = mProjMtx * mViewMtx;
		}

	} mView;

	struct LIGHT {
		glm::vec3 sky;
		glm::vec3 ground;
		glm::vec3 up;
		glm::vec3 specDir;
		glm::vec3 specClr;
	} mLight;

	glm::vec3 mGamma;

	glm::vec3 mClearColor;

	bool init_gpu() {
		using namespace std;
		GLint status;
		GLint infoLen = 0;

		if (!GLSys::valid()) { return false; }

		string appPath = string(mAppPath);
		string wkFolder = appPath.substr(0, appPath.rfind(PATH_SEPARATOR));

		string vtxPath = wkFolder + PATH_SEPARATOR + "vtx.vert";
		std::string srcVtx = load_text(vtxPath);

		if (srcVtx.length() < 1) { return false; }
		std::string fragPath = wkFolder + PATH_SEPARATOR + "hemidir.frag";
		std::string srcFrag = load_text(fragPath);
		if (srcFrag.length() < 1) { return false; }

		mGPU.programId = GLSys::compile_prog_strs(srcVtx, srcFrag, &mGPU.shaderIdVtx, &mGPU.shaderIdFrag);

		if (mGPU.programId) {
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
		return mGPU.programId != 0;
	}
	void reset_gpu() {
		glDeleteShader(mGPU.shaderIdFrag);
		glDeleteShader(mGPU.shaderIdVtx);
		glDeleteProgram(mGPU.programId);
	}

	bool init(const GLDrawCfg& cfg)  {
		GLSysCfg oglCfg;
		::memset(&oglCfg, 0, sizeof(oglCfg));
		oglCfg.x = cfg.x;
		oglCfg.y = cfg.y;
		oglCfg.w = cfg.w;
		oglCfg.h = cfg.h;
		GLSys::init(oglCfg);

		if (!GLSys::valid()) { return false; }

		mAppPath = cfg.appPath;
		if (!init_gpu()) {
			sys_dbg_msg("GPU initialization failed\n");
			return false;
		}

		mView.mWidth = cfg.w;
		mView.mHeight = cfg.h;
		mView.set_FOVY(glm::radians(40.0f));
		mView.set_range(0.1f, 1000.0f);

		mClearColor = glm::vec3(0.33f, 0.44f, 0.55f);
		mGamma = glm::vec3(2.2f);
		return true;
	}

	void reset() {
		GLSys::stop();
		reset_gpu();
		GLSys::reset();
	}

	void frame_clear() const {
		glColorMask(true, true, true, true);
		glDepthMask(true);
		glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

} s_app;

static bool s_initFlg = false;

static glm::vec3 geo_pnt_pos(const TDGeometry& geo, int ipnt) {
	TDGeometry::Point p = geo.get_pnt(ipnt);
	return glm::vec3(p.x, p.y, p.z);
}

static int get_pol_tris(uint32_t vlst[6], const TDGeometry& geo, int polIdx) {
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

	bool init(const GLDrawCfg& cfg) {
		if (s_initFlg) { return true ; }
		::memset(&s_app, 0, sizeof(GLESApp));
		s_initFlg = s_app.init(cfg);
		return s_initFlg;
	}

	void reset() {
		if (!s_initFlg) { return; }
		s_app.reset();
		::memset(&s_app, 0, sizeof(GLESApp	));
		s_initFlg = false;
	}

	void loop(void(*pLoop)()) {
		GLSys::loop(pLoop);
	}

	void begin() {
		if (!s_initFlg) { return; }
		s_app.frame_clear();
	}

	void end() {
		if (!s_initFlg) { return; }
		GLSys::swap();
	}

	void set_view(const glm::vec3& pos, const glm::vec3& tgt, const glm::vec3& up) {
		s_app.mView.set(pos, tgt, up);
		s_app.mView.update();
	}

	void set_FOVY_degrees(float deg) {
		s_app.mView.set_FOVY(glm::radians(deg));
		s_app.mView.update();
	}

	void set_view_range(float znear, float zfar) {
		s_app.mView.set_range(znear, zfar);
		s_app.mView.update();
	}

	void set_hemi_light(const glm::vec3& sky, const glm::vec3& ground, const glm::vec3& up) {
		s_app.mLight.sky = sky;
		s_app.mLight.ground = ground;
		s_app.mLight.up = up;
	}

	void set_spec_light(const glm::vec3& dir, const glm::vec3& clr) {
		s_app.mLight.specDir = dir;
		s_app.mLight.specClr = clr;
	}

	int Mesh::idx_bytes() const { return is_idx16() ? sizeof(GLushort) : sizeof(GLuint); }

	// Geo should contain triangles or quads. Quads will be triangulated.
	Mesh* Mesh::create(const TDGeometry& geo) {
		uint32_t vtxNum = geo.get_pnt_num();
		if (vtxNum == 0) { return nullptr; }
		uint32_t polNum = geo.get_poly_num();
		if (polNum == 0) { return nullptr; }

		uint32_t triNum = 0;
		for (uint32_t i = 0; i < polNum; ++i) {
			TDGeometry::Poly pol = geo.get_poly(i);
			if (pol.nvtx == 3) {
				triNum += 1;
			} else if (pol.nvtx == 4) {
				triNum += 2;
			}
		}
		if (triNum <= 0) { return nullptr; }

		uint32_t id[2];
		glGenBuffers(2, id);
		if ((0 == id[0]) || (0 == id[1])) {
			gl_error();
			return nullptr;
		}

		Mesh* pMsh = new Mesh();
		pMsh->mNumVtx = vtxNum;
		pMsh->mNumTri = triNum;
		pMsh->mBuffIdVtx = id[0];
		pMsh->mBuffIdIdx = id[1];

		Mesh::Vtx* pVtx = new Vtx[vtxNum];
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

		uint32_t vlst[6];
		size_t sizeIB = triNum * 3 * pMsh->idx_bytes();
		char* pIdx = new char[sizeIB];
		if (pMsh->is_idx16()) {
			uint16_t* pIB16 = (uint16_t*)pIdx;
			for (uint32_t i = 0; i < polNum; i++) {
				uint16_t n = get_pol_tris(vlst, geo, i);
				for (uint16_t j = 0; j < 3 * n; j++) {
					*pIB16++ = (uint16_t)vlst[j];
				}
			}
		} else {
			uint32_t* pIB32 = (uint32_t*)pIdx;
			for (uint16_t i = 0; i < polNum; i++) {
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
		if (!GLSys::valid()) { return; }
		if (0 == mBuffIdVtx) { return; }
		if (0 == mBuffIdIdx) { return; }

		glUseProgram(s_app.mGPU.programId);

		glUniform3f(s_app.mGPU.prmLocInvGamma, 1.0f / s_app.mGamma.r, 1.0f / s_app.mGamma.g, 1.0f / s_app.mGamma.b);
		glUniform3fv(s_app.mGPU.prmLocHemiSky, 1, (float*)&s_app.mLight.sky);
		glUniform3fv(s_app.mGPU.prmLocHemiGround, 1, (float*)&s_app.mLight.ground);
		glUniform3fv(s_app.mGPU.prmLocHemiUp, 1, (float*)&s_app.mLight.up);

		glUniform3fv(s_app.mGPU.prmSpecDir, 1, (float*)&s_app.mLight.specDir);
		glUniform3fv(s_app.mGPU.prmSpecClr, 1, (float*)&s_app.mLight.specClr);
		glUniform1f(s_app.mGPU.prmSpecRough, mRoughness);

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