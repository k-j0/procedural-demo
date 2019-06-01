#include "PPTextureShader.h"



PPTextureShader::PPTextureShader(){
	SETUP_SHADER(postprocessing_vs, postprocessing_fs);
}


PPTextureShader::~PPTextureShader(){
}
