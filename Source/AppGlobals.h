#pragma once

///allows easy access to global pointers throughout code base without having to pass around stuff in too many function calls

///singleton class; access using GLOBALS
#define GLOBALS AppGlobals::Instance()

extern class TextureManager;
extern class D3D;
extern class ID3D11Device;
extern class ID3D11DeviceContext;

class AppGlobals {

private:
	inline AppGlobals() { }

public:
	inline static AppGlobals& Instance() {
		static AppGlobals instance;//instanciated on first use
		return instance;
	}

	//get rid of those two to guarantee there's only one instance
	AppGlobals(AppGlobals const&) = delete;
	void operator=(AppGlobals const&) = delete;

	
	/** The following are the global pointers and values that should be accessible once App's constructor has been called. */
	TextureManager* TextureManager;
	D3D* Renderer;
	ID3D11Device* Device;
	ID3D11DeviceContext* DeviceContext;
	HWND Hwnd;

	XMMATRIX ViewMatrix;
	int ScreenWidth;
	int ScreenHeight;
	float FarPlane = 170.0f;
	bool normalMapping = true;

	float TessellationMin = 1.f;
	float TessellationMax = 16.f;
	float TessellationRange = 0.5f;
	float DisplacementScale = 1.0f;
	float ShadowmapBias = 0.0008f;
	bool ShadowmapSeeErrors = false;
	
};
