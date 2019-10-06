#include <glm.hpp>
#define GLM_FORCE_RADIANS
#include <gtc/matrix_transform.hpp>
#include "TDGeometry.hpp"


struct GLDrawCfg {
	int width;
	int height;
};

struct GLDrawMesh;

namespace GLDraw {
	void init(const GLDrawCfg& cfg);
	void reset();
	void loop(void(*pLoop)());

	void setView(const glm::vec3& pos, const glm::vec3& tgt, const glm::vec3& up = glm::vec3(0, 1, 0));
	void setDegreesFOVY(float deg);
	void setViewRange(float znear, float zfar);

	GLDrawMesh* meshCreate(const TDGeometry& geo);
	void meshDestroy(GLDrawMesh* pMsh);
	void meshDraw(GLDrawMesh* pMsh, const glm::mat4x4& worldMtx);

	void begin();
	void end();

	glm::mat4x4 xformSRTXYZ(const glm::vec3& translate, const glm::vec3& rotateDegrees, const glm::vec3& scale = glm::vec3(1.0f));
	glm::mat4x4 xformSRTXYZ(float tx, float ty, float tz, float rx, float ry, float rz, float sx = 1.0f, float sy = 1.0f, float sz = 1.0f);
} // GLDraw
