#include <glm.hpp>
#define GLM_FORCE_RADIANS
#include <gtc/matrix_transform.hpp>
#include <TDGeometry.hpp>

struct GLDrawSys {
#ifdef _WIN32
	void* hInstance;
#else
	void* _p_;
#endif
};

struct GLDrawCfg {
	GLDrawSys sys;
	int width;
	int height;
};

namespace GLDraw {
	void init(const GLDrawCfg& cfg);
	void reset();
	void loop(void(*pLoop)());

	void begin();
	void end();
}
