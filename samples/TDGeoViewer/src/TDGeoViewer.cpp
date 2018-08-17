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

static void view_light_init() {
	TDGeometry::BBox bbox = s_tdgeo.bbox();
	glm::vec3 vmin(bbox.min[0], bbox.min[1], bbox.min[2]);
	glm::vec3 vmax(bbox.max[0], bbox.max[1], bbox.max[2]);
	glm::vec3 tgt = (vmin + vmax) * 0.5f;
	float shift = (vmax - vmin).z;
	glm::vec3 pos = tgt;
	pos.z += shift * 4;
	GLDraw::set_view(pos, tgt);

	glm::vec3 sky(2.14318f, 1.971372f, 1.862601f);
	glm::vec3 ground(0.15f, 0.1f, 0.075f);
	glm::vec3 up = glm::vec3(0, 1, 0);
	glm::vec3 specDir = glm::normalize(tgt - (pos + glm::vec3(0.0f, 1.0f, 0.0f)));
	glm::vec3 specClr(1, 0.9f, 0.85f);
	float roughness = 0.45f;

	GLDraw::set_hemi_light(sky, ground, up);
	GLDraw::set_spec_light(specDir, specClr , roughness);
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
	view_light_init();

	GLDraw::loop(main_loop);

	data_reset();
	GLDraw::reset();

	return 0;
}
