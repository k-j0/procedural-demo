#include "DepthShader.h"



DepthShader::DepthShader(){
	SETUP_SHADER(depth_vs, depth_fs);
}


DepthShader::~DepthShader(){
}