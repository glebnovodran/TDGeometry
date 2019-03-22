#include <glm.hpp>

struct GLSysCfg {
	int x;
	int y;
	int w;
	int h;
};

namespace GLSys {
	void init(const GLSysCfg& cfg);
	void reset();
	void stop();
	void swap();
	void loop(void (*pLoop)());
	bool valid();

	//GLuint compile_shader_str(const char* pSrc, size_t srcSize, GLenum kind); // TODO: use std:string
	GLuint compile_shader_str(const std::string& src, GLenum kind);
	GLuint compile_prog_strs(const std::string& srcVtx, const std::string& srcFrag, GLuint* pSIdVtx, GLuint* pSIdFrag);
}

void sys_dbg_msg(const char* pFmt, ...);
bool gl_error();