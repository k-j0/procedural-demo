#include "SquareMesh.h"

#include "AppGlobals.h"

SquareMesh::SquareMesh(){
	initBuffers(GLOBALS.Device);
}


SquareMesh::~SquareMesh(){
}

void SquareMesh::initBuffers(ID3D11Device* device){
	D3D11_SUBRESOURCE_DATA vertexData, indexData;

	vertexCount = 4;
	indexCount = 6;

	VertexType* vertices = new VertexType[vertexCount];
	unsigned long* indices = new unsigned long[indexCount];

	// Load the vertex array with data.
	vertices[0].position = XMFLOAT3(-0.5f, 1.0f, 0.0f);  // top left
	vertices[0].normal = XMFLOAT3(0, 0, 1);
	vertices[0].texture = XMFLOAT2(0, 0);

	vertices[1].position = XMFLOAT3(-0.5f, 0.0f, 0.0f);  // bottom left.
	vertices[1].normal = XMFLOAT3(0, 0, 1);
	vertices[1].texture = XMFLOAT2(0, 1);

	vertices[2].position = XMFLOAT3(0.5f, 0.0f, 0.0f);  // bottom right.
	vertices[2].normal = XMFLOAT3(0, 0, 1);
	vertices[2].texture = XMFLOAT2(1, 1);

	vertices[3].position = XMFLOAT3(0.5f, 1.0f, 0.0f);  // top right.
	vertices[3].normal = XMFLOAT3(0, 0, 1);
	vertices[3].texture = XMFLOAT2(1, 0);

	// Load the index array with data.
	indices[0] = 0;  // Top left
	indices[1] = 1;  // Bottom left
	indices[2] = 2;  // Bottom right
	indices[3] = 2;  // Bottom right
	indices[4] = 3;  // Top right
	indices[5] = 0;  // Top left

	D3D11_BUFFER_DESC vertexBufferDesc = { sizeof(VertexType) * vertexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	vertexData = { vertices, 0 , 0 };
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

	D3D11_BUFFER_DESC indexBufferDesc = { sizeof(unsigned long) * indexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
	indexData = { indices, 0, 0 };
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;
	delete[] indices;
	indices = 0;
}

void SquareMesh::sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top){
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(top);
}