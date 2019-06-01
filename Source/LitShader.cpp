#include "LitShader.h"

#include "AppGlobals.h"

LitShader::LitShader() {
}

LitShader::~LitShader(){
	cameraBuffer->Release();
	lightBuffer->Release();
	materialBuffer->Release();
}

void LitShader::initBuffers() {

	//Setup camera buffer
	SETUP_SHADER_BUFFER(CameraBufferType, cameraBuffer);

	//Setup light buffer
	SETUP_SHADER_BUFFER(LightBufferType, lightBuffer);

	//Setup material buffer
	SETUP_SHADER_BUFFER(MaterialBufferType, materialBuffer);

	//Setup displacement buffer
	SETUP_SHADER_BUFFER(DisplacementBufferType, displacementBuffer);

	//Setup shadowmap matrix buffer
	SETUP_SHADER_BUFFER(ShadowmapMatrixBufferType, shadowmapMatrixBuffer);

	//Setup effects buffer
	SETUP_SHADER_BUFFER(EffectsBufferType, effectsBuffer);

	// Create sampler states
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &sampleState);

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	renderer->CreateSamplerState(&samplerDesc, &pointSampler);
}

void LitShader::setEffectParameters(ID3D11DeviceContext* deviceContext, float time) {

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	
	EffectsBufferType* effectsPtr;
	deviceContext->Map(effectsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	effectsPtr = (EffectsBufferType*)mappedResource.pData;
	effectsPtr->time = time;
	deviceContext->Unmap(effectsBuffer, 0);
	deviceContext->PSSetConstantBuffers(5, 1, &effectsBuffer);

}

void LitShader::setLightParameters(ID3D11DeviceContext* deviceContext, XMFLOAT3 cameraPosition, ExtendedLight** lights, ID3D11ShaderResourceView** shadowmaps, bool sendShadowmaps, int numLights){

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// Send camera data to vertex shader
	CameraBufferType* cameraPtr;
	deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	cameraPtr = (CameraBufferType*)mappedResource.pData;
	cameraPtr->cameraPosition = cameraPosition;
	cameraPtr->farPlane = FAR_PLANE;
	deviceContext->Unmap(cameraBuffer, 0);
	if(domainShader)//send to domain shader
		deviceContext->DSSetConstantBuffers(1, 1, &cameraBuffer);
	else//send to vertex shader instead
		deviceContext->VSSetConstantBuffers(1, 1, &cameraBuffer);

	// Send light data to pixel shader
	LightBufferType* lightPtr;
	deviceContext->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	lightPtr = (LightBufferType*)mappedResource.pData;
	//ambient is the first light's Ambient component only.
	if (numLights > 0)
		lightPtr->ambient = (*lights)[0].getAmbientColour();
	else
		lightPtr->ambient = XMFLOAT4(1, 1, 1, 1);
	//add all the lights:
	for (int light = 0; light < NUM_LIGHTS; ++light) {
		if (light < numLights) {
			lightPtr->diffuse[light] = (*lights)[light].getDiffuseColour();
			lightPtr->position[light] = XMFLOAT4((*lights)[light].getPosition().x, (*lights)[light].getPosition().y, (*lights)[light].getPosition().z, (*lights)[light].getType());
			lightPtr->direction[light] = (*lights)[light].getFormattedDirection();
			lightPtr->attenuation[light] = (*lights)[light].getAttenuation();
			lightPtr->attenuation[light].w = (*lights)[light].shouldBypassShadows() || !sendShadowmaps ? 0 : 1;//w of attenuation is whether we want shadows from this light
		}
		else {
			lightPtr->position[light] = XMFLOAT4(UNUSED_SHADER_PARAM, UNUSED_SHADER_PARAM, UNUSED_SHADER_PARAM, INACTIVE_LIGHT);//w component is 0 meaning light is inactive.
		}
	}
	lightPtr->oneOverFarPlane = 1.0f / FAR_PLANE;
	lightPtr->shadowmapBias = GLOBALS.ShadowmapBias;
	lightPtr->showShadowmapErrors = GLOBALS.ShadowmapSeeErrors ? 1 : 0;
	lightPtr->oneOverShadowmapSize = numLights > 0 ? 1.0f / lights[0]->getShadowmapRes() : 1;
	deviceContext->Unmap(lightBuffer, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &lightBuffer);

	// Send shadowmap data to vertex or domain shader
	ShadowmapMatrixBufferType* smbPtr;
	deviceContext->Map(shadowmapMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	smbPtr = (ShadowmapMatrixBufferType*)mappedResource.pData;
	for (int light = 0; light < NUM_LIGHTS; ++light) {
		if (light < numLights && !(*lights)[light].shouldBypassShadows()) {//pass this light's matrices
			smbPtr->shadowmapMode[light] = XMFLOAT4(1, UNUSED_SHADER_PARAM, UNUSED_SHADER_PARAM, UNUSED_SHADER_PARAM);
			smbPtr->lightView[light] = XMMatrixTranspose((*lights)[light].getView());// transpose matrices to prepare for mul in shaders
			smbPtr->lightProjection[light] = XMMatrixTranspose((*lights)[light].getProjection());
		}
		else {//ignore this light for shadowmaps
			smbPtr->shadowmapMode[light] = XMFLOAT4(0, UNUSED_SHADER_PARAM, UNUSED_SHADER_PARAM, UNUSED_SHADER_PARAM);
		}
	}
	deviceContext->Unmap(shadowmapMatrixBuffer, 0);
	if (domainShader)//send to domain
		deviceContext->DSSetConstantBuffers(3, 1, &shadowmapMatrixBuffer);
	else//send to vertex
		deviceContext->VSSetConstantBuffers(3, 1, &shadowmapMatrixBuffer);

	// Send shadowmaps to fragment
	if(sendShadowmaps)
		deviceContext->PSSetShaderResources(2, numLights, shadowmaps);
}

void LitShader::setMaterialParameters(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* normalMap, ID3D11ShaderResourceView* displacementMap, Material* material) {

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// Send material data to pixel shader
	if (material != nullptr) {//if it's null, just keep the previous one; allows for optimization if we're drawing multiple meshes with the same mat
		MaterialBufferType* materialPtr;
		deviceContext->Map(materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		materialPtr = (MaterialBufferType*)mappedResource.pData;
		if (texture)
			materialPtr->mode = normalMap && GLOBALS.normalMapping ? DIFFUSE_AND_NORMAL_MAP : DIFFUSE_TEXTURE;
		else
			materialPtr->mode = normalMap && GLOBALS.normalMapping ? NORMAL_MAP : COLOUR;
		materialPtr->colour = material->colour;
		materialPtr->specularColour = material->specularColour;
		materialPtr->specularPower = material->specularPower;
		materialPtr->emissive = material->emissive;
		deviceContext->Unmap(materialBuffer, 0);
		deviceContext->PSSetConstantBuffers(1, 1, &materialBuffer);
	}

	// Set shader texture resources in the pixel shader.
	if (texture)
		deviceContext->PSSetShaderResources(0, 1, &texture);
	if (normalMap)
		deviceContext->PSSetShaderResources(1, 1, &normalMap);

	//Set displacement map resources in domain shader
	if (domainShader) {

		//send displacement buffer:
		DisplacementBufferType* dispPtr;
		deviceContext->Map(displacementBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		dispPtr = (DisplacementBufferType*)mappedResource.pData;
		if (displacementMap)
			dispPtr->mode = DISPLACEMENT;
		else
			dispPtr->mode = NO_DISPLACEMENT;
		dispPtr->scale = GLOBALS.DisplacementScale;
		dispPtr->bias = -GLOBALS.DisplacementScale/2.0f;
		dispPtr->mapSize = 1024;
		deviceContext->Unmap(displacementBuffer, 0);
		deviceContext->DSSetConstantBuffers(2, 1, &displacementBuffer);

		if (displacementMap) {
			deviceContext->DSSetShaderResources(0, 1, &displacementMap);
		}
	}


	// Set sampler resources in the pixel shader
	deviceContext->PSSetSamplers(0, 1, &sampleState);
	deviceContext->PSSetSamplers(1, 1, &pointSampler);
}