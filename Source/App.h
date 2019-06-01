#pragma once

#include "DXF.h"
#include "DefaultShader.h"
#include "ExtendedLight.h"
#include "Material.h"
#include "PostProcessingPass.h"
#include "ColourGradingShader.h"
#include "DepthShader.h"
#include "GaussianBlurShader.h"
#include "BloomShader.h"
#include "CombinationShader.h"
#include "SquareMesh.h"
#include "InfiniteTerrain.h"

class App : public BaseApplication {

public:
	App();
	~App();
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN) override;

	bool frame() override;

	void handleInput(float dt) override;

protected:
	bool render() override;
	void geometry(LitShader* shader, XMMATRIX& world, XMMATRIX& view, XMMATRIX& projection, XMFLOAT3 cameraPosition, bool sendShadowmaps, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	void gui();

	void updateFov();

private:
	///Shaders for geometry
	LitShader* shader = nullptr;
	DepthShader* depthShader = nullptr;

	///Lighting
	bool lighting = true;
	bool showShadowmaps = false;//Debug: show shadow maps
	bool shadows = true;

	///Meshes, materials, textures
	InfiniteTerrain* terrain;
	int terrainSeed;
	bool showDepth = false;//when true, overlays the depth map on top of everything

	///Post processing shaders and passes
	PPTextureShader* textureShader;
	PostProcessingPass depthPass;
	ColourGradingShader* colourGrading;
	PostProcessingPass colourGradingPass;
	BloomShader* bloom;
	GaussianBlurShader* gaussian;
	PostProcessingPass bloomPass;
	PostProcessingPass vGaussianPass;
	CombinationShader* combine;
	PostProcessingPass combinationPass;
	bool showLut = false;//when true, displays the tonemapped LUT used by the colour grading effect
	bool disablePostProcessing = false;
	float fov = 60.0f;
	XMMATRIX projectionMatrix;

	///Whether to show ui
	bool showUi = false;

	///For animating things consistently when needed
	float timeScale = 1;
	float cameraSpeed = 2.5f;

	float totalTime = 0;
	bool freeCam = false;

};
