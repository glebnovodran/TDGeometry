#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN 1
#	define NOMINMAX
#	define _WIN32_WINNT 0x0500
#	include <tchar.h>
#	include <windows.h>
#endif

#include "GLDraw.hpp"

static void main_loop() {
	GLDraw::begin();
	GLDraw::end();
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	GLDrawCfg cfg;
	cfg.sys.hInstance = hInstance;
	cfg.width = 1024;
	cfg.height = 768;

	GLDraw::init(cfg);
	GLDraw::loop(main_loop);
	GLDraw::reset();

	return 0;
}
#else

#endif
