#pragma once

#include "TerrainMesh.h"
#include "DefaultShader.h"

class InfiniteTerrain{
public:
	InfiniteTerrain(TextureManager* textureMgr, int seed, int chunkSize = 100);
	~InfiniteTerrain();

	void update(XMFLOAT3 cameraPosition, ID3D11Device* device, ID3D11DeviceContext* deviceContext, float dt);
	void render(bool lighting, bool shadowing, D3D* renderer, XMMATRIX& worldMatrix, XMMATRIX& viewMatrix, XMMATRIX& projectionMatrix, XMFLOAT3 cameraPosition);
	void depthPass(LitShader* depthShader, D3D* renderer, XMMATRIX& worldMatrix, XMMATRIX& viewMatrix, XMMATRIX& projectionMatrix, XMFLOAT3 cameraPosition, const TerrainMesh* specificChunk = NULL);
	void shadowmappingPass(LitShader* depthShader, D3D* renderer, XMMATRIX& worldMatrix);

	//debug: render shadowmaps to screen
	void showShadowmaps(PPTextureShader* textureShader, D3D* renderer, XMMATRIX orthoViewMatrix);

	inline LitShader* getShader() { return shader; }

	//basic collision detection for camera
	XMFLOAT3 handleCamera(XMFLOAT3 cameraPosition);

protected:
	int seed;
	int chunkSize;

	std::vector<TerrainMesh*> chunks;

	LitShader* shader;//for the terrain meshes
	LitShader* blockShader;//for the ruins

	RuinBlockLibrary* ruinBlockLibrary;//contains a bunch of pre-generated ruin elements

	//textures:
	ID3D11ShaderResourceView* causticsTex;
	ID3D11ShaderResourceView* rockTex;
	ID3D11ShaderResourceView* rockNormalsTex;
	ID3D11ShaderResourceView* sandTex;
	ID3D11ShaderResourceView* sandNormalsTex;
	ID3D11ShaderResourceView* ruinsTex;
	ID3D11ShaderResourceView* ruinsNormalsTex;
	Material* material;
};

