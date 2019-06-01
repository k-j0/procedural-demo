#include "DefaultShader.h"

DefaultShader::DefaultShader(){
	SETUP_SHADER_TANGENT(default_vs, default_fs);
}

DefaultShader::~DefaultShader(){
}

void DefaultShader::initBuffers() {
	LitShader::initBuffers();
}