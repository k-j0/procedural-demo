#pragma once

#include "DXF.h"

class SquareMesh : public BaseMesh{
public:
	SquareMesh();
	~SquareMesh();

	void sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) override;

protected:
	void initBuffers(ID3D11Device* device) override;
};

