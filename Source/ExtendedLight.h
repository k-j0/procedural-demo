#pragma once

/// Adds functionality to the Light class for light types, attenuation, shadow mapping, etc.

#include "DXF.h"

#define INACTIVE_LIGHT 0
#define POINT_LIGHT 1
#define DIRECTIONAL_LIGHT 2
#define SPOTLIGHT 3

#define SHADOWMAP_RES 2048 //default shadowmap res

extern class PPTextureShader;
extern class PostProcessingPass;

class ExtendedLight : public Light {

protected:
	XMFLOAT4 attenuation = XMFLOAT4(1, 0.125f, 0, 0);
	float type = POINT_LIGHT;

	//Shadow mapping:
	PostProcessingPass* depthPass = nullptr;
	PPTextureShader* texShader = nullptr;
	bool shadowsSetup = false;
	ID3D11ShaderResourceView* shadowMap = nullptr;
	float shadowmapWorldSize = 50;
	float projectionFov = 60;
	float shadowMapRes = SHADOWMAP_RES;
	XMMATRIX projection;

public:
	ExtendedLight();
	~ExtendedLight();

	inline void setAttenuation(float constant = 1, float linear = 0.125f, float quadratic = 0) { attenuation = XMFLOAT4(constant, linear, quadratic, 0); }
	inline XMFLOAT4 getAttenuation() { return attenuation; }

	inline void setType(float t) { type = t; }
	inline float getType() { return type; }

	///returns the normalized, inverted direction, so as to not do it for each fragment of each mesh
	inline XMFLOAT4 getFormattedDirection() {
		XMFLOAT3 direction = getDirection();
		float length = direction.x * direction.x + direction.y * direction.y + direction.z * direction.z;
		length = sqrt(length);
		return XMFLOAT4(direction.x / length, direction.y / length, direction.z / length, 0);
	}



	//Shadowmap functionality

	///Returns false if this light shouldn't cast shadows
	inline bool shouldBypassShadows() { return !shadowsSetup; }// || type == POINT_LIGHT; }

	///Sets up for shadowmap generation
	void setupShadows();//call this once after creating the light

	///returns whether a shadow map will be created for this light, and starts recording the map if true
	bool StartRecordingShadowmap();

	///call after StartRecordingShadowmap(), once all geometry has been rendered to the shadowmap
	ID3D11ShaderResourceView* StopRecordingShadowmap();

	///use this to get projection/ortho matrix and view matrix from this light
	inline XMMATRIX getProjection() { return type == DIRECTIONAL_LIGHT ? getOrthoMatrix() : projection; }//this projection matrix has had its fov changed
	inline XMMATRIX getView() { return getViewMatrix(); }

	void updateFov(float fov);

	///Getters and setters for shadowmaps
	inline float getShadowmapRes() { return shadowMapRes; }
	void setShadowmapRes(int res);
	inline float getShadowmapSize() { return shadowmapWorldSize; }
	void setShadowmapSize(float sz);
	inline ID3D11ShaderResourceView* getShadowmap() { return shadowMap; }
	inline float getProjectionFov() { return projectionFov; }

};