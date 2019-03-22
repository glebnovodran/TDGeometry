#include <string> // TODO: remove ?
#include <glm.hpp>
#define GLM_FORCE_RADIANS
#include <gtc/matrix_transform.hpp>
#include <TDGeometry.hpp>

struct GLDrawCfg {
	char* appPath;
	int x;
	int y;
	int w;
	int h;
};

namespace GLDraw {
	bool init(const GLDrawCfg& cfg);
	void reset();
	void begin();
	void end();
	void loop(void(*pLoop)());

	void set_view(const glm::vec3& pos, const glm::vec3& tgt, const glm::vec3& up = glm::vec3(0, 1, 0));
	void set_FOVY_degrees(float deg);
	void set_view_range(float znear, float zfar);

	void set_hemi_light(const glm::vec3& sky, const glm::vec3& ground, const glm::vec3& up);
	void set_spec_light(const glm::vec3& dir, const glm::vec3& clr);

	class Mesh {

	private:
		Mesh() : mNumVtx(0), mNumTri(0), mBuffIdVtx(0), mBuffIdIdx(0), mRoughness(0.001){};

		bool is_idx16() const { return mNumVtx <= (1 << 16); }
		int idx_bytes() const;
		int mNumVtx;
		int mNumTri;
		uint32_t mBuffIdVtx;
		uint32_t mBuffIdIdx;
		float mRoughness;
	public:
		struct Vtx {
			glm::vec3 pos;
			glm::vec3 nrm;
			glm::vec3 clr;
		};

		static Mesh* create(const TDGeometry& geo);
		void destroy();
		void draw(const glm::mat4x4& worldMtx);
		void set_roughness(float roughness) { mRoughness = roughness; }
	};

	glm::mat4x4 xformSRTXYZ(const glm::vec3& translate, const glm::vec3& rotateDegrees, const glm::vec3& scale = glm::vec3(1.0f));
	glm::mat4x4 xformSRTXYZ(float tx, float ty, float tz, float rx, float ry, float rz, float sx = 1.0f, float sy = 1.0f, float sz = 1.0f);
}
