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

	class Mesh {

	private:
		Mesh() : mNumVtx(0), mNumTri(0), mBuffIdVtx(0), mBuffIdIdx(0) {};

		bool is_idx16() const { return mNumVtx <= (1 << 16); }
		int idx_bytes() const;
		int mNumVtx;
		int mNumTri;
		uint32_t mBuffIdVtx;
		uint32_t mBuffIdIdx;
	public:
		struct Vtx {
			glm::vec3 pos;
			glm::vec3 nrm;
			glm::vec3 clr;
		};

		static Mesh* create(const TDGeometry& geo);
		void destroy();
		void draw(const glm::mat4x4& worldMtx);
	};

	glm::mat4x4 xformSRTXYZ(const glm::vec3& translate, const glm::vec3& rotateDegrees, const glm::vec3& scale = glm::vec3(1.0f));
	glm::mat4x4 xformSRTXYZ1(const glm::vec3& translate, const glm::vec3& rotateDegrees, const glm::vec3& scale);
	glm::mat4x4 xformSRTXYZ(float tx, float ty, float tz, float rx, float ry, float rz, float sx = 1.0f, float sy = 1.0f, float sz = 1.0f);
}
