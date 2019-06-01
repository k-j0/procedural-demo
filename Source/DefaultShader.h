#pragma once

#include "LitShader.h"

using namespace std;
using namespace DirectX;

class DefaultShader : public LitShader{

public:
	DefaultShader();
	~DefaultShader();

protected:
	void initBuffers() override;

};

