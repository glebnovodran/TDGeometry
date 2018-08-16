#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN 1
#	define NOMINMAX
#	define _WIN32_WINNT 0x0500
#	include <tchar.h>
#	include <windows.h>
#elif defined(X11)
	#include "X11/Xlib.h"
	#include "X11/Xutil.h"
#endif

#include "GLDraw.hpp"

static TDGeometry s_tdgeo;
static GLDraw::Mesh* s_pMesh = nullptr;

static void data_init(const std::string& folder) {
	s_tdgeo.load(folder);
	s_pMesh = GLDraw::Mesh::create(s_tdgeo);
}

static void data_reset() {
	s_pMesh->destroy();
	s_pMesh = nullptr;
}

static void main_loop() {
	GLDraw::begin();
	glm::mat4x4 mtx = glm::mat4x4(1.0f);
	static float rotDY = 0.0f;
	mtx = GLDraw::xformSRTXYZ(0.0f, 0.0f, 0.0f, 0.0f, rotDY, 0.0f);
	rotDY += 1.0f;
	s_pMesh->draw(mtx);
	GLDraw::end();
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
#elif defined(UNIX)
int main(int argc, char **argv) {
#endif
	GLDrawCfg cfg;

	cfg.width = 1024;
	cfg.height = 768;
	GLDraw::init(cfg);
	data_init("../../data/geo");
	TDGeometry::BBox bbox = s_tdgeo.bbox();
	GLDraw::adjust_view_for_bbox(bbox.min, bbox.max);
	GLDraw::loop(main_loop);
	data_reset();
	GLDraw::reset();

	return 0;
}
