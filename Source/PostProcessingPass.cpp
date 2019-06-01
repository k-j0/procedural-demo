#include "PostProcessingPass.h"

#include "AppGlobals.h"

PostProcessingPass::~PostProcessingPass() {
	if (renderTexture)
		delete renderTexture;
	if (orthoMesh)
		delete orthoMesh;
}

void PostProcessingPass::Setup(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int width, int height, PostProcessingShader* shader) {
	this->shader = shader;
	renderTexture = new RenderTexture(device, width, height, SCREEN_NEAR, SCREEN_DEPTH);
	orthoMesh = new OrthoMesh(device, deviceContext, GLOBALS.ScreenWidth, GLOBALS.ScreenHeight);
}

///Call this to change the size once Setup has already been called
void PostProcessingPass::setSize(int width, int height) {
	//Can't change size of RenderTexture once it's already been created so we need a new one entirely.
	delete renderTexture;
	renderTexture = new RenderTexture(GLOBALS.Device, width, height, SCREEN_NEAR, SCREEN_DEPTH);
}

void PostProcessingPass::Begin(D3D* renderer, ID3D11DeviceContext* deviceContext, XMFLOAT3 clearColour) {
	if (!enabled) {
		renderer->beginScene(clearColour.x, clearColour.y, clearColour.z, 1);
		return;
	}
	renderTexture->setRenderTarget(deviceContext);
	renderTexture->clearRenderTarget(deviceContext, clearColour.x, clearColour.y, clearColour.z, 1);
}

void PostProcessingPass::Begin(){
	Begin(GLOBALS.Renderer, GLOBALS.DeviceContext);
}

void PostProcessingPass::End(D3D* renderer) {
	renderer->setBackBufferRenderTarget();
	renderer->resetViewport();
}

void PostProcessingPass::End(){
	End(GLOBALS.Renderer);
}

void PostProcessingPass::Render(D3D* renderer, ID3D11DeviceContext* deviceContext, XMMATRIX orthoViewMatrix) {
	if (!enabled) {
		return;
	}
	renderer->setZBuffer(false);
	orthoMesh->sendData(deviceContext);
	shader->setShaderParameters(deviceContext, renderer->getWorldMatrix(), orthoViewMatrix, renderer->getOrthoMatrix(), XMFLOAT3(0,0,0));
	shader->setTextureData(deviceContext, renderTexture->getShaderResourceView());
	shader->render(deviceContext, orthoMesh->getIndexCount());
	renderer->setZBuffer(true);
}

void PostProcessingPass::Render(XMMATRIX orthoViewMatrix) {
	Render(GLOBALS.Renderer, GLOBALS.DeviceContext, orthoViewMatrix);
}