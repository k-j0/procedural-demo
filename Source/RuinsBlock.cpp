#include "RuinsBlock.h"

RuinsBlock::RuinsBlock(XMFLOAT3 position, XMFLOAT3 rotation, RuinBlockMesh* mesh) : position(position), rotation(rotation), mesh(mesh) {

}


void RuinsBlock::render(LitShader* shader, Material* material, D3D* renderer, XMMATRIX& worldMatrix, XMMATRIX& viewMatrix, XMMATRIX& projectionMatrix, XMFLOAT3 cameraPosition) {

	mesh->sendData(renderer->getDeviceContext());							// unfortunately we need to perform yaw rotation last so cant use the handy XMMatrixRotationRollPitchYaw :'(
	shader->setShaderParameters(renderer->getDeviceContext(), XMMatrixRotationY(rotation.y) * XMMatrixRotationX(rotation.x) * XMMatrixRotationZ(rotation.z) * XMMatrixTranslation(position.x, position.y, position.z) * worldMatrix, viewMatrix, projectionMatrix, cameraPosition);
	//shader->setMaterialParameters(renderer->getDeviceContext(), sandTex, sandNormalsTex, NULL, material);
	shader->render(renderer->getDeviceContext(), mesh->getIndexCount());

}