#pragma once
#include "Shader.h"

class PostProcessingShader : public Shader {

public:
	PostProcessingShader();
	virtual ~PostProcessingShader();

	///Sends texture data (result from previous rendering pass)
	void setTextureData(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* texture);

protected:
	virtual void initBuffers() override;

private:


};

