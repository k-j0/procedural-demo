#pragma once
#include "GaussianBlurShader.h"

class BloomShader : public GaussianBlurShader {
protected:
	struct BloomBufferType{
		float threshold;//0..3
		float intensity;//0..5
		XMFLOAT2 padding;
	};

public:
	BloomShader();
	~BloomShader();

	void setBloom();

	///Parameters for blooming
	float threshold = 1.75f;
	float intensity = 0.5f;

protected:
	void initBuffers() override;

private:
	ID3D11Buffer* bloomBuffer;
};

