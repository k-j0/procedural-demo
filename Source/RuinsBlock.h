#pragma once

#include "RuinBlockMesh.h"

class RuinsBlock {

public:
	RuinsBlock(XMFLOAT3 position, XMFLOAT3 rotation, RuinBlockMesh* mesh);

	inline void setPosition(XMFLOAT3 pos) { position = pos; }
	inline void setRotation(XMFLOAT3 rot) { rotation = rot; }

	void render(LitShader* shader, Material* material, D3D* renderer, XMMATRIX& worldMatrix, XMMATRIX& viewMatrix, XMMATRIX& projectionMatrix, XMFLOAT3 cameraPosition);

protected:
	XMFLOAT3 position, rotation;//rotation is in rads
	RuinBlockMesh* mesh;//the shared mesh that we're going to use

};