#pragma once
// Texture
// Loads and stores a texture ready for rendering.
// Handles mipmap generation on load.

#ifndef _TEXTUREMANAGER_H_
#define _TEXTUREMANAGER_H_

#include <d3d11.h>
//#include "../DirectXTK/Inc/DDSTextureLoader.h"
//#include "../DirectXTK/Inc/WICTextureLoader.h"
#include "DTK\DDSTextureLoader.h"
#include "DTK\WICTextureLoader.h"
#include <string>
#include <fstream>
#include <vector>
#include <map>
//#include "Texture.h"

using namespace DirectX;

class TextureManager
{
public:
	TextureManager(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	~TextureManager();

	void loadTexture(std::string uid, WCHAR* filename);
	ID3D11ShaderResourceView* getTexture(std::string uid);

private:
	bool does_file_exist(const WCHAR *fileName);

	ID3D11ShaderResourceView* texture;
	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;

	std::map<std::string, ID3D11ShaderResourceView*> textureMap;
};

#endif