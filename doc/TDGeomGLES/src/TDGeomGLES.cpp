#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN 1
#	define NOMINMAX
#	define _WIN32_WINNT 0x0500
#	include <tchar.h>
#	include <windows.h>
#endif

#include <algorithm>
#include "GLDraw.hpp"

static TDGeometry s_geo;
static GLDrawMesh* s_pMesh = nullptr;

static void data_init() {
	//s_geo.load("../../data/m_head");
	s_geo.load("../../data/f_dummy");
	s_pMesh = GLDraw::meshCreate(s_geo);
}

static void data_reset() {
	GLDraw::meshDestroy(s_pMesh);
	s_pMesh = nullptr;
	s_geo.unload();
}

static void main_loop() {
	TDGeometry::BBox geoBBox = s_geo.bbox();
	glm::vec3 vmin(geoBBox.min[0], geoBBox.min[1], geoBBox.min[2]);
	glm::vec3 vmax(geoBBox.max[0], geoBBox.max[1], geoBBox.max[2]);
	glm::vec3 vsize = vmax - vmin;
	glm::vec3 vc = (vmin + vmax) * 0.5f;
	glm::vec3 tgt = vc;
	glm::vec3 pos = vc + glm::vec3(0, vsize.y * 0.2f, std::max(std::max(vsize.x, vsize.y), vsize.z) * 1.75f);

	GLDraw::begin();
	GLDraw::setView(pos, tgt);
	glm::mat4x4 mtx = glm::mat4x4(1.0f);
	static float rotDY = 0.0f;
	mtx = GLDraw::xformSRTXYZ(0.0f, 0.0f, 0.0f,  0.0f, rotDY, 0.0f);
	rotDY += 1.0f;
	GLDraw::meshDraw(s_pMesh, mtx);
	GLDraw::end();
}

#include <gtx/euler_angles.hpp>

glm::mat4x4 test_rot_xyz(float dx, float dy, float dz) {
	//return glm::eulerAngleZ(glm::radians(dz)) * glm::eulerAngleXY(glm::radians(dx), glm::radians(dy));
	return glm::eulerAngleZ(glm::radians(dz)) * glm::eulerAngleYX(glm::radians(dy), glm::radians(dx));
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	glm::mat4x4 rm = test_rot_xyz(10, 20, 30);

	GLDrawCfg cfg;
	cfg.width = 1024;
	cfg.height = 768;
	GLDraw::init(cfg);
	data_init();
	GLDraw::loop(main_loop);
	data_reset();
	GLDraw::reset();
	return 0;
}
#endif
