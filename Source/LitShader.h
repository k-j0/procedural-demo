#pragma once
#include "Shader.h"

///Base shader for any lit piece of geometry

#define NUM_LIGHTS 8 //must be same as in fragment, vertex and domain shaders AND needs to be divisible by 4

//modes
#define COLOUR 0
#define DIFFUSE_TEXTURE 1
#define DIFFUSE_AND_NORMAL_MAP 2
#define NORMAL_MAP 3

#define NO_DISPLACEMENT 0
#define DISPLACEMENT 1

class LitShader : public Shader{

protected:

	///passes camera information to VS; should only require one gpu write per frame
	struct CameraBufferType {
		XMFLOAT3 cameraPosition;
		float farPlane;//the far plane's distance from camera
	};

	///passes light information to FS; should only require one gpu write per frame
	struct LightBufferType {
		XMFLOAT4 ambient;
		XMFLOAT4 diffuse[NUM_LIGHTS];
		XMFLOAT4 position[NUM_LIGHTS];//w is type (see ExtendedLight.h)
		XMFLOAT4 direction[NUM_LIGHTS];
		XMFLOAT4 attenuation[NUM_LIGHTS];//w is whether to generate shadows for this light (0 or 1)
		float oneOverFarPlane;
		float shadowmapBias;
		float showShadowmapErrors;
		float oneOverShadowmapSize;
	};

	///passes material information to FS; expected to be written to gpu for each different mesh
	struct MaterialBufferType {
		float mode;//see defined modes at top of file
		XMFLOAT3 colour;
		XMFLOAT3 specularColour;
		float specularPower;
		XMFLOAT3 emissive;
		float pad;
	};

	///passes displacement information to DS if needed
	struct DisplacementBufferType {
		float mode;
		float scale;
		float bias;
		float mapSize;
	};

	///passes light matrices for each light in the scene to vert
	struct ShadowmapMatrixBufferType {
		XMFLOAT4 shadowmapMode[NUM_LIGHTS];//x is 0 for no shadowmap, 1 for shadowmap
		XMMATRIX lightView[NUM_LIGHTS];
		XMMATRIX lightProjection[NUM_LIGHTS];
	};

	///passes additional effects input (per-case basis)
	struct EffectsBufferType {
		float time;
		XMFLOAT3 unused;
	};

public:
	LitShader();
	virtual ~LitShader();
	
	///should be called only once per frame
	void setLightParameters(ID3D11DeviceContext* deviceContext, XMFLOAT3 cameraPosition, ExtendedLight** lights, ID3D11ShaderResourceView** shadowmaps, bool sendShadowmaps, int numLights);
	///setup material parameters for any shader (colour, texture, etc)
	void setMaterialParameters(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* normalMap, ID3D11ShaderResourceView* displacementMap, Material* material);
	///setup effects-related input
	void setEffectParameters(ID3D11DeviceContext* deviceContext, float time);

protected:
	virtual void initBuffers() override;

private:
	ID3D11Buffer* cameraBuffer;			//VS b1 or DS b1
	ID3D11Buffer* lightBuffer;			//PS b0
	ID3D11Buffer* materialBuffer;		//PS b1
	ID3D11Buffer* displacementBuffer;	//DS b2
	ID3D11Buffer* shadowmapMatrixBuffer;//VS b3 or DS b3
	ID3D11Buffer* effectsBuffer;		//PS b5

	ID3D11SamplerState* pointSampler;	//PS s1
};

