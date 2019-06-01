#include "ColourGradingShader.h"

#include "AppGlobals.h"


ColourGradingShader::ColourGradingShader(){
	SETUP_SHADER(postprocessing_vs, colourgrading_fs);

	orthoMesh = new OrthoMesh(GLOBALS.Device, GLOBALS.DeviceContext, GLOBALS.ScreenWidth, GLOBALS.ScreenHeight);
	orthoMeshSmall = new OrthoMesh(GLOBALS.Device, GLOBALS.DeviceContext, LUT_WIDTH, LUT_HEIGHT, -GLOBALS.ScreenWidth/2+LUT_WIDTH/2, GLOBALS.ScreenHeight/2-LUT_HEIGHT/2);
	renderTexture = new RenderTexture(GLOBALS.Device, LUT_WIDTH, LUT_HEIGHT, SCREEN_NEAR, SCREEN_DEPTH);
	tonemapShader = new TonemappingShader;
	textureShader = new PPTextureShader;
}


ColourGradingShader::~ColourGradingShader(){
	colourGradingBuffer->Release();
	lutSampler->Release();
	delete tonemapShader;
	delete textureShader;
	delete renderTexture;
	delete orthoMesh;
	delete orthoMeshSmall;
}

void ColourGradingShader::initBuffers() {
	PostProcessingShader::initBuffers();

	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = 0;
	renderer->CreateSamplerState(&samplerDesc, &lutSampler);

	SETUP_SHADER_BUFFER(ColourGradingType, colourGradingBuffer);
}

void ColourGradingShader::setColourGrading(D3D* renderer, ID3D11DeviceContext* deviceContext, XMMATRIX orthoViewMatrix, ID3D11ShaderResourceView* depthTexture) {
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// Retonemap LUT if needed
	if (retonemap && lut != nullptr) {
		retonemap = false;
		renderer->setZBuffer(false);
		//render the lut to rendertexture using tonemapping shader
		renderTexture->setRenderTarget(deviceContext);
		renderTexture->clearRenderTarget(deviceContext, 1, 1, 1, 1);
		orthoMesh->sendData(deviceContext);
		tonemapShader->setShaderParameters(deviceContext, renderer->getWorldMatrix(), orthoViewMatrix, renderer->getOrthoMatrix(), XMFLOAT3(0,0,0));
		tonemapShader->setTextureData(deviceContext, lut);
		//pass params
		tonemapShader->tonemapping = getTonemapping();
		tonemapShader->exposure = getExposure();
		tonemapShader->brightness = getBrightness();
		tonemapShader->contrast = getContrast();
		tonemapShader->hue = getHue();
		tonemapShader->saturation = getSaturation();
		tonemapShader->value = getValue();
		tonemapShader->setTonemapping(deviceContext);
		tonemapShader->render(deviceContext, orthoMesh->getIndexCount());
		renderer->setBackBufferRenderTarget();
		renderer->setZBuffer(true);
		//grab the tonemapped lut
		tonemappedLut = renderTexture->getShaderResourceView();
	}

	// Send data to fragment shader
	ColourGradingType* cgPtr;
	deviceContext->Map(colourGradingBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	cgPtr = (ColourGradingType*)mappedResource.pData;
	cgPtr->strength = tonemappedLut == nullptr ? 0 : strength;
	cgPtr->logLutParams = XMFLOAT3(1.0f/LUT_WIDTH, 1.0f/LUT_HEIGHT, LUT_HEIGHT-1.0f);
	cgPtr->vignette = vignette;
	cgPtr->chromaticAberrationStrength = chromaticAberrationStrength;
	cgPtr->chromaticAberrationDistance = chromaticAberrationDistance;
	cgPtr->fog = fogColour;
	deviceContext->Unmap(colourGradingBuffer, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &colourGradingBuffer);

	// Send LUT
	if (tonemappedLut != nullptr) {
		deviceContext->PSSetShaderResources(1, 1, &tonemappedLut);
		deviceContext->PSSetSamplers(1, 1, &lutSampler);
	}
	else {
		printf("Error: LUT was not tonemapped.");
	}

	// Send depth texture
	deviceContext->PSSetShaderResources(2, 1, &depthTexture);
}


void ColourGradingShader::gui() {

	//ImGui::SliderFloat("Strength", &strength, 0, 1);
	ImGui::SliderFloat("Vignette", &vignette, 0, 1);
	ImGui::SliderFloat("Chromatic aberration strength", &chromaticAberrationStrength, 0, 1);
	ImGui::SliderFloat("Chromatic aberration distance", &chromaticAberrationDistance, 0, 1);
	float fog[3] = { fogColour.x, fogColour.y, fogColour.z };
	ImGui::ColorEdit3("Fog colour", fog);
	ImGui::Text("Note: fog strength is controlled by far plane.");
	fogColour = XMFLOAT3(fog[0], fog[1], fog[2]);
	int tonemapping = getTonemapping();
	if (tonemapping == TONEMAP_NONE)
		ImGui::SliderInt("Tonemapping (None)", &tonemapping, 0, 2);
	else if (tonemapping == TONEMAP_ACES)
		ImGui::SliderInt("Tonemapping (Filmic)", &tonemapping, 0, 2);
	else if (tonemapping == TONEMAP_NEUTRAL)
		ImGui::SliderInt("Tonemapping (Neutral)", &tonemapping, 0, 2);
	if(tonemapping != getTonemapping())
		setTonemapping(tonemapping);

	//Show ui for each parameter; if user changes the param on this frame, call the setter.
#define SHOWGUI(name, set, get, min, max) do{ float param = get (); ImGui::SliderFloat(#name, &param, min, max); if(param != get ()) set (param); }while(false)
	SHOWGUI(Exposure, setExposure, getExposure, 0, 2);
	SHOWGUI(Brightness, setBrightness, getBrightness, -1, 1);
	SHOWGUI(Contrast, setContrast, getContrast, -1, 1);
	SHOWGUI(Hue, setHue, getHue, 0, 1);
	SHOWGUI(Saturation, setSaturation, getSaturation, 0, 2);
	SHOWGUI(Value, setValue, getValue, 0, 2);
#undef SHOWGUI

}

void ColourGradingShader::showLut(D3D* renderer, XMMATRIX orthoViewMatrix) {

	renderer->setZBuffer(false);
	orthoMeshSmall->sendData(renderer->getDeviceContext());
	textureShader->setShaderParameters(renderer->getDeviceContext(), renderer->getWorldMatrix(), orthoViewMatrix, renderer->getOrthoMatrix(), XMFLOAT3(0,0,0));
	textureShader->setTextureData(renderer->getDeviceContext(), tonemappedLut);
	textureShader->render(renderer->getDeviceContext(), orthoMeshSmall->getIndexCount());
	renderer->setZBuffer(true);

}