///Used to combine two textures into one final image

#pragma once
#include "PostProcessingShader.h"
class CombinationShader : public PostProcessingShader{
public:
	CombinationShader();
	~CombinationShader();

	void setTexture1(ID3D11ShaderResourceView* texture1);
};

