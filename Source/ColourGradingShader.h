#pragma once
#include "PostProcessingShader.h"
#include "TonemappingShader.h"
#include "PostProcessingPass.h"
#include "PPTextureShader.h"

#define LUT_WIDTH 256
#define LUT_HEIGHT 16

class ColourGradingShader : public PostProcessingShader {

protected:
	struct ColourGradingType {
		XMFLOAT3 fog;
		float strength;//0..1
		XMFLOAT3 logLutParams;//x: 1/lut_width, y: 1/lut_height, z: lut_height-1
		float vignette;//0..1
		float chromaticAberrationStrength;//0..1
		float chromaticAberrationDistance;//0..1
		XMFLOAT2 pad;
	};

public:
	ColourGradingShader();
	~ColourGradingShader();

	///setup the colour grading
	void setColourGrading(D3D* renderer, ID3D11DeviceContext* deviceContext, XMMATRIX orthoViewMatrix, ID3D11ShaderResourceView* depthTexture);

	///debug: show GUI for params
	void gui();

	///debug: show tonemapped LUT output
	void showLut(D3D* renderer, XMMATRIX orthoViewMatrix);

	///init the lut
	inline void setLut(ID3D11ShaderResourceView* view) { lut = view; retonemap = true; }

	///Parameters
	float strength = 1;//0..1
	float vignette = 1;//0..1
	float chromaticAberrationStrength = 1;//0..1
	float chromaticAberrationDistance = 0.05f;//0..1
	XMFLOAT3 fogColour = XMFLOAT3(0.5f, 0.5f, 0.5f);
	//setters and getters for tonemapping properties; anytime setter is called, the LUT will be retonemapped on the next frame before rendering
#define SG(name, set, get) inline void set (float param){ name = param; retonemap = true; } inline float get (){ return name; }
	SG(tonemapping, setTonemapping, getTonemapping)
	SG(exposure, setExposure, getExposure)
	SG(brightness, setBrightness, getBrightness)
	SG(contrast, setContrast, getContrast)
	SG(hue, setHue, getHue)
	SG(saturation, setSaturation, getSaturation)
	SG(value, setValue, getValue)
#undef SG

protected:
	void initBuffers() override;

	///Parameters for tonemapping (should be changed via setters; changing one re-runs tonemapping shader on this frame)
	float tonemapping = TONEMAP_NONE;
	float exposure = 1;//0..2
	float brightness = 0;//-1..1
	float contrast = 0;//-1..1
	float hue = 0;//0..1
	float saturation = 0.5f;//0..1
	float value = 1;//0..2

private:
	ID3D11Buffer* colourGradingBuffer = nullptr;
	ID3D11ShaderResourceView* lut = nullptr;
	ID3D11ShaderResourceView* tonemappedLut = nullptr;//result of post-processed lut
	ID3D11SamplerState* lutSampler = nullptr;

	bool retonemap = false;//set to true each time one of the params changes; redoes tonemapping to LUT on this frame when true

	TonemappingShader* tonemapShader;
	PPTextureShader* textureShader;
	OrthoMesh* orthoMesh, * orthoMeshSmall;//first one is screen size, second is 256x16 (LUT_WIDTH x LUT_HEIGHT)
	RenderTexture* renderTexture;

};

