#include "App.h"

#include "AppGlobals.h"
#include "Utils.h"

#define LOWCOST_STARTUP true //set to true to disable post-processing by default
#define WIREFRAME_STARTUP false
#define RES_PATH "res/"
//#define DEBUG_D3D11 //define to recreate device and device context with flag D3D11_CREATE_DEVICE_DEBUG

App::App(){
}

App::~App(){

	if (terrain)
		delete terrain;

	if (shader)
		delete shader;
	if (depthShader)
		delete depthShader;
	if (textureShader)
		delete textureShader;

	if (colourGrading)
		delete colourGrading;
	if (gaussian)
		delete gaussian;
	if (bloom)
		delete bloom;
	if (combine)
		delete combine;

	//release fbx sdk objects (we've been keeping some around for animation purposes)
#ifdef FBX_SDK
	FBXScene::Release();
#endif
}

void App::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN){
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

#ifdef DEBUG_D3D11
	renderer->device = NULL;
	HRESULT result = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, 0, D3D11_SDK_VERSION, &renderer->device, NULL, &renderer->deviceContext);
	if (result != S_OK) {
		printf("Could not recreate device: ");
		Shader::printError(result);
	}
#endif

	printf("Starting up.\n");

	terrainSeed = 1552766176;//could use time(0) but i like this one :)

	//setup globals access to certain pointers & values
	GLOBALS.TextureManager = textureMgr;
	GLOBALS.Renderer = renderer;
	GLOBALS.Device = renderer->getDevice();
	GLOBALS.DeviceContext = renderer->getDeviceContext();
	GLOBALS.ScreenWidth = screenWidth;
	GLOBALS.ScreenHeight = screenHeight;
	GLOBALS.Hwnd = hwnd;

	//materials and textures
	textureMgr->loadTexture("lut", (WCHAR*)L"" RES_PATH "LUTs/Lut_blue.png");
	//terrain-specific textures
	textureMgr->loadTexture("caustics", (WCHAR*)L"" RES_PATH "caustics.png");
	textureMgr->loadTexture("rock", (WCHAR*)L"" RES_PATH "RockAlbedo.png");
	textureMgr->loadTexture("rockN", (WCHAR*)L"" RES_PATH "RockNormals.png");
	textureMgr->loadTexture("sand", (WCHAR*)L"" RES_PATH "SandAlbedo.png");
	textureMgr->loadTexture("sandN", (WCHAR*)L"" RES_PATH "SandNormals.png");
	//textureMgr->loadTexture("ruin", (WCHAR*)L"" RES_PATH "stonefloor_albedo.png");
	textureMgr->loadTexture("ruin", (WCHAR*)L"" RES_PATH "marbleAlbedo.png");
	textureMgr->loadTexture("ruinN", (WCHAR*)L"" RES_PATH "stonefloor_normal.png");

	//initialize shaders
	wireframeToggle = WIREFRAME_STARTUP;
	shader = new DefaultShader;
	depthShader = new DepthShader;
	textureShader = new PPTextureShader;
	colourGrading = new ColourGradingShader;
	colourGrading->fogColour = XMFLOAT3(6.f/255.f, 67.f/255.f, 89.f/255.f);//cool dark-ish blue (light blue looks awesome but then its broken by bloom so whatever)
	colourGrading->setLut(textureMgr->getTexture("lut"));
	colourGrading->setTonemapping(1);
	colourGrading->setExposure(1.441f);
	colourGrading->setBrightness(0.159f);
	colourGrading->setContrast(0.169f);
	colourGrading->setHue(0);
	colourGrading->setSaturation(0.718f);
	colourGrading->setValue(1);
	colourGrading->vignette = 0.7f;
	colourGrading->chromaticAberrationDistance = 0.022f;
	bloom = new BloomShader;
	bloom->horizontal = true;
	bloom->distance = 200;
	bloom->threshold = 1.954f;
	bloom->intensity = 0.564f;
	gaussian = new GaussianBlurShader;
	gaussian->horizontal = false;
	gaussian->distance = 200;
	combine = new CombinationShader;
	colourGradingPass.Setup(GLOBALS.Device, GLOBALS.DeviceContext, GLOBALS.ScreenWidth, GLOBALS.ScreenHeight, colourGrading);
	bloomPass.Setup(GLOBALS.Device, GLOBALS.DeviceContext, GLOBALS.ScreenWidth * 0.7f, GLOBALS.ScreenHeight * 0.7f, bloom);
	vGaussianPass.Setup(GLOBALS.Device, GLOBALS.DeviceContext, GLOBALS.ScreenWidth * 0.7f, GLOBALS.ScreenHeight * 0.7f, gaussian);
	depthPass.Setup(GLOBALS.Device, GLOBALS.DeviceContext, GLOBALS.ScreenWidth, GLOBALS.ScreenHeight, textureShader);
	combinationPass.Setup(GLOBALS.Device, GLOBALS.DeviceContext, GLOBALS.ScreenWidth, GLOBALS.ScreenHeight, combine);
#if LOWCOST_STARTUP
	disablePostProcessing = true;//bloom & pp is expensive, so i don't necessarily want it by default (especially on my own laptop hehe)
#endif

	//initialize meshes
	terrain = new InfiniteTerrain(textureMgr, terrainSeed);

	//place camera
	camera->setPosition(50, 20, -65);
	camera->setRotation(0, 0, 0);
	updateFov();

}

///Change fov of projection matrix
void App::updateFov() {
	projectionMatrix = Utils::changeFov(renderer->getProjectionMatrix(), fov, (float)GLOBALS.ScreenWidth / GLOBALS.ScreenHeight);
}

bool App::frame(){
	//update base app
	if (!BaseApplication::frame())
		return false;
	float dt = timer->getTime();

	totalTime += dt;

	//update anything that requires animation
	terrain->update(camera->getPosition(), renderer->getDevice(), renderer->getDeviceContext(), dt);

	//render to screen
	if (!render())
		return false;
	return true;
}

void App::handleInput(float dt) {
	//let the base application control the camera, but 5 times as fast
	BaseApplication::handleInput(dt*cameraSpeed);

	if (!freeCam) {
		//basic collision detection for camera.
		XMFLOAT3 camPos = terrain->handleCamera(camera->getPosition());
		camera->setPosition(camPos.x, camPos.y, camPos.z);
	}

	//right click to show debug ui
	if (input->isRightMouseDown()) {
		input->setRightMouse(false);
		showUi = !showUi;
	}
}

///Render geometry with any custom shaders
void App::geometry(LitShader* shader, XMMATRIX& worldMatrix, XMMATRIX& viewMatrix, XMMATRIX& projectionMatrix, XMFLOAT3 cameraPosition, bool sendShadowmaps, D3D_PRIMITIVE_TOPOLOGY top) {

	terrain->getShader()->setEffectParameters(renderer->getDeviceContext(), totalTime);
	terrain->render(lighting, shadows, renderer, worldMatrix, viewMatrix, projectionMatrix, cameraPosition);

}

bool App::render(){
	XMMATRIX worldMatrix;
	
	//update camera position and get matrices
	camera->update();
	worldMatrix = renderer->getWorldMatrix();
	GLOBALS.ViewMatrix = camera->getViewMatrix();


// ** shadow mapping passes ** //

	if (shadows) {
		terrain->shadowmappingPass(depthShader, renderer, worldMatrix);
	}


// ** depth pass ** //

	if (!wireframeToggle && !disablePostProcessing) {//no need to grab depths if we're not going to render fog
		depthPass.Begin(renderer, GLOBALS.DeviceContext, XMFLOAT3(0, 0, 0));

		//Render geometry to depth
		terrain->depthPass(depthShader, renderer, worldMatrix, GLOBALS.ViewMatrix, projectionMatrix, camera->getPosition());
		
		depthPass.End(renderer);
	}


// ** render geometry ** //

	//clear scene
	if (!wireframeToggle && !disablePostProcessing) {
		if(colourGradingPass.enabled)
			colourGradingPass.Begin(renderer, renderer->getDeviceContext(), XMFLOAT3(0.2f, 0.2f, 0.3f));
		else if(bloomPass.enabled)
			bloomPass.Begin(renderer, renderer->getDeviceContext(), XMFLOAT3(0.2f, 0.2f, 0.3f));
		else renderer->beginScene(0.2f, 0.2f, 0.3f, 1);

	} else //in wireframe mode, we don't want any post processing at all.
		renderer->beginScene(0.2f, 0.2f, 0.3f, 1);

	// Geometry
	geometry(NULL, worldMatrix, GLOBALS.ViewMatrix, projectionMatrix, camera->getPosition(), shadows);


// ** post processing ** //

	//only do post processing and stuff if not in wireframe mode
	if (!wireframeToggle && !disablePostProcessing) {

		//end whichever is enabled (or none)
		colourGradingPass.End();
		bloomPass.End();

		// Finish up colour grading if it's enabled
		if(colourGradingPass.enabled){

			//Apply tonemapping
			bloomPass.Begin();
			colourGrading->setColourGrading(renderer, renderer->getDeviceContext(), camera->getOrthoViewMatrix(), depthPass.getRenderTexture()->getShaderResourceView());
			colourGradingPass.Render(camera->getOrthoViewMatrix());
			bloomPass.End();
		}

		//Finish up with bloom if it's enabled
		if (bloomPass.enabled) {

			//Blur horizontally and only keep brightest pixels
			vGaussianPass.Begin();
			bloom->setBlur();
			bloom->setBloom();
			bloomPass.Render(camera->getOrthoViewMatrix());
			vGaussianPass.End();

			//Blur vertically...
			combinationPass.Begin();
			gaussian->setBlur();
			vGaussianPass.Render(camera->getOrthoViewMatrix());
			combinationPass.End();

			//And combine results of bloom and what we had before starting bloom (ie colour grading or nothing)
			combine->setTexture1(bloomPass.getRenderTexture()->getShaderResourceView());
			combinationPass.Render(camera->getOrthoViewMatrix());
		}

		//Show depth if we need to
		if (showDepth) {
			depthPass.Render(camera->getOrthoViewMatrix());
		}

	}//!wireframe
	

// ** UI ** //

	//Show colour grading LUT
	if (showLut) colourGrading->showLut(renderer, camera->getOrthoViewMatrix());

	//Show shadowmaps
	if (showShadowmaps) {
		terrain->showShadowmaps(textureShader, renderer, camera->getOrthoViewMatrix());
	}

	//Show ui
	gui();

	//swap buffers
	renderer->endScene();

	return true;
}

void App::gui()
{
	// Force turn off unnecessary shader stages.
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->HSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->DSSetShader(NULL, NULL, 0);

	// Display FPS
	ImGui::Text("FPS: %.2f", timer->getFPS());
	int newSeed = terrainSeed;
	ImGui::DragInt("Seed", &newSeed);
	if (newSeed != terrainSeed) {
		terrainSeed = newSeed;
		//Regenerate all terrains!
		delete terrain;
		terrain = new InfiniteTerrain(textureMgr, terrainSeed);
	}

	// Enabling/disabling stuff
	if (ImGui::CollapsingHeader("Scene parameters")) {
		ImGui::Checkbox("Wireframe mode", &wireframeToggle);
		ImGui::Checkbox("Free camera", &freeCam);
		ImGui::Checkbox("Disable post processing", &disablePostProcessing);
		ImGui::Checkbox("Show depth", &showDepth);
		float newFov = fov;
		ImGui::SliderFloat("Fov", &newFov, 1, 180);
		if (newFov != fov) {
			fov = newFov;
			updateFov();
		}
		ImGui::SliderFloat("Far plane", &GLOBALS.FarPlane, 1, 300);
		ImGui::Text("Camera pos %f %f %f", camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);
		ImGui::SliderFloat("Camera speed", &cameraSpeed, 0, 10);
		ImGui::SliderFloat("Timescale", &timeScale, 0, 1);
	}

	//Lighting params
	if (ImGui::CollapsingHeader("Lighting")) {
		ImGui::Checkbox("Apply lighting", &lighting);
		ImGui::Checkbox("Normal mapping", &GLOBALS.normalMapping);
		ImGui::Checkbox("Apply shadowing", &shadows);
		ImGui::Checkbox("Show shadowmaps", &showShadowmaps);
		ImGui::Checkbox("Show out of range shadowmaps", &GLOBALS.ShadowmapSeeErrors);
		ImGui::SliderFloat("Shadowmap bias", &GLOBALS.ShadowmapBias, 0, 0.002f, "%.4f");
	}

	// Colour grading params
	if (ImGui::CollapsingHeader("Colour Grading")) {
		ImGui::Checkbox("Apply colour grading", &colourGradingPass.enabled);
		colourGrading->gui();
		ImGui::Checkbox("Show tonemapped LUT", &showLut);
	}

	// Bloom params
	if (ImGui::CollapsingHeader("Bloom")) {
		ImGui::Checkbox("Apply bloom", &bloomPass.enabled);
		ImGui::SliderFloat("Threshold", &bloom->threshold, 0, 3);
		ImGui::SliderFloat("Intensity", &bloom->intensity, 0, 5);
		int blurDist = gaussian->distance;
		ImGui::SliderInt("Blur distance", &blurDist, 1, MAX_NEIGHBOURS);
		gaussian->distance = blurDist;
		bloom->distance = blurDist;
	}

	// Render UI
	ImGui::Render();
	if(showUi) ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}