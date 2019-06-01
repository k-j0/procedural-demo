#include "CombinationShader.h"

#include "AppGlobals.h"

CombinationShader::CombinationShader(){
	SETUP_SHADER(postprocessing_vs, combine_fs);
}


CombinationShader::~CombinationShader(){
}


void CombinationShader::setTexture1(ID3D11ShaderResourceView* texture1) {
	GLOBALS.DeviceContext->PSSetShaderResources(1, 1, &texture1);
}