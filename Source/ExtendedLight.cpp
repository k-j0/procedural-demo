#include "ExtendedLight.h"

#include "AppGlobals.h"
#include "PostProcessingPass.h"
#include "PPTextureShader.h"
#include "Utils.h"

ExtendedLight::ExtendedLight() {
}

ExtendedLight::~ExtendedLight() {
	if (shadowsSetup) {
		delete texShader;
		delete depthPass;
	}
}

///Prepares this light for shadow mapping
void ExtendedLight::setupShadows() {
	if (shadowsSetup) {
		delete texShader;
		delete depthPass;
	}
	texShader = new PPTextureShader;
	depthPass = new PostProcessingPass;
	depthPass->Setup(GLOBALS.Device, GLOBALS.DeviceContext, shadowMapRes, shadowMapRes, texShader);
	setShadowmapSize(shadowmapWorldSize);//generate projection or ortho matrix
	shadowsSetup = true;
}

///We're assuming the ortho/projection matrix has been generated on this light!
bool ExtendedLight::StartRecordingShadowmap() {
	if (shouldBypassShadows()) return false;//cannot have shadowmaps for point lights or spotlights yet

	generateViewMatrix();
	depthPass->Begin(GLOBALS.Renderer, GLOBALS.DeviceContext, XMFLOAT3(0,0,0));

	return true;
}

ID3D11ShaderResourceView* ExtendedLight::StopRecordingShadowmap() {
	if (shouldBypassShadows()) return nullptr;

	depthPass->End();

	shadowMap = depthPass->getRenderTexture()->getShaderResourceView();

	return shadowMap;
}

void ExtendedLight::updateFov(float fov){
	projectionFov = fov;
	projection = Utils::changeFov(getProjectionMatrix(), projectionFov);
}

void ExtendedLight::setShadowmapRes(int res) {
	shadowMapRes = res;
	if (shouldBypassShadows()) return;
	depthPass->setSize(res, res);
}

void ExtendedLight::setShadowmapSize(float sz) {
	shadowmapWorldSize = sz;
	if (type == DIRECTIONAL_LIGHT)
		generateOrthoMatrix(sz, sz, 0.1f, 1000.f);
	else {
		generateProjectionMatrix(0.1f, 1000.f);
		updateFov(projectionFov);
	}
}
