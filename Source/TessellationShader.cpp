#include "TessellationShader.h"



TessellationShader::TessellationShader(){
	SETUP_SHADER_TANGENT(tessellated_vs, default_fs);
	SETUP_TESSELATION(default_hs, default_ds);
}


TessellationShader::~TessellationShader(){
}

