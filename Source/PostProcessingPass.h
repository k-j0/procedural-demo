#pragma once

#include "DXF.h"
#include "PostProcessingShader.h"

class PostProcessingPass {

public:
	~PostProcessingPass();

	///Initializes the post processing pass with a post processing shader
	void Setup(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int width, int height, PostProcessingShader* shader);
	///Sets up the pass; render the scene after this call
	void Begin(D3D* renderer, ID3D11DeviceContext* deviceContext, XMFLOAT3 clearColour = XMFLOAT3(1.0f, 1.0f, 1.0f));
	void Begin();
	///Ends the pass; all render code should have been done
	void End(D3D* renderer);
	void End();
	///Renders the results onto an ortho quad filling the screen
	void Render(D3D* renderer, ID3D11DeviceContext* deviceContext, XMMATRIX orthoViewMatrix);
	void Render(XMMATRIX orthoViewMatrix);

	bool enabled = true;//if turned to false, will simply stop doing its thing

	void setSize(int width, int height);

	inline OrthoMesh* getOrthoMesh() { return orthoMesh; }
	inline RenderTexture* getRenderTexture() { return renderTexture; }

private:
	PostProcessingShader* shader;
	RenderTexture* renderTexture = nullptr;
	OrthoMesh* orthoMesh = nullptr;

};