#include "Shader.h"

#include "AppGlobals.h"

Shader::Shader() : BaseShader(GLOBALS.Device, GLOBALS.Hwnd) {
}

Shader::~Shader(){

	if (sampleState)
		sampleState->Release();

	if (matrixBuffer)
		matrixBuffer->Release();

	if (layout)
		layout->Release();
}

///Reference: https://docs.microsoft.com/en-us/windows/uwp/gaming/load-a-game-asset
void Shader::loadSkinVertexShader(WCHAR * filename) {
	if (vertexShader) {
		printf("Error: vertex shader has already been loaded prior!\n");
		return;
	}
	
	/// Load shader -----------------------------------------------------------------------------------------------------------------------

	std::ifstream input(filename, std::ios::binary);
	std::vector<char> bytes((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));

	if (bytes.size() <= 0) {
		std::wstring wfilename(filename);
		printf("Error: vertex shader file %s does not exist...\n", std::string(wfilename.begin(), wfilename.end()).c_str());
		return;
	}

	HRESULT result = GLOBALS.Device->CreateVertexShader(bytes.data(), bytes.size(), nullptr, &vertexShader);
	if (result != S_OK) {
		std::wstring wfilename(filename);
		printf("Error: could not load compiled skinning vertex shader %s...\n", std::string(wfilename.begin(), wfilename.end()).c_str());
		printError(result);
		return;
	}

	/// Create input layout -----------------------------------------------------------------------------------------------------------------------

	const D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Float3 Position
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Float2 Texcoord0
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Float3 Normal
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Float3 Tangent
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Uint4 BlendIndices0
		{ "BLENDINDICES", 1, DXGI_FORMAT_R32G32B32A32_UINT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Uint4 BlendIndices1
		{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Float4 BlendWeight0
		{ "BLENDWEIGHT",  1, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Float4 BlendWeight1
	};

	result = GLOBALS.Device->CreateInputLayout(layoutDesc, ARRAYSIZE(layoutDesc), bytes.data(), bytes.size(), &layout);
	if (result != S_OK) {
		std::wstring wfilename(filename);
		printf("Error: could not create input layout for skinning vertex shader %s...\n", std::string(wfilename.begin(), wfilename.end()).c_str());
		printError(result);
		return;
	}

	//Success! :D
}

void Shader::loadTangentVertexShader(WCHAR * filename) {
	if (vertexShader) {
		printf("Error: vertex shader has already been loaded prior!\n");
		return;
	}

	/// Load shader -----------------------------------------------------------------------------------------------------------------------

	std::ifstream input(filename, std::ios::binary);
	std::vector<char> bytes((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));

	if (bytes.size() <= 0) {
		std::wstring wfilename(filename);
		printf("Error: vertex shader file %s does not exist...\n", std::string(wfilename.begin(), wfilename.end()).c_str());
		return;
	}

	HRESULT result = GLOBALS.Device->CreateVertexShader(bytes.data(), bytes.size(), nullptr, &vertexShader);
	if (result != S_OK) {
		std::wstring wfilename(filename);
		printf("Error: could not load compiled tangent vertex shader %s...\n", std::string(wfilename.begin(), wfilename.end()).c_str());
		printError(result);
		return;
	}

	/// Create input layout -----------------------------------------------------------------------------------------------------------------------

	const D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Float3 Position
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Float2 Texcoord0
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Float3 Normal
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 } // Float3 Tangent
	};

	result = GLOBALS.Device->CreateInputLayout(layoutDesc, ARRAYSIZE(layoutDesc), bytes.data(), bytes.size(), &layout);
	if (result != S_OK) {
		std::wstring wfilename(filename);
		printf("Error: could not create input layout for tangent vertex shader %s...\n", std::string(wfilename.begin(), wfilename.end()).c_str());
		printError(result);
		return;
	}

	//Success! :D
}

void Shader::printError(HRESULT errorCode){
#define ERR(err) case err: printf(#err "\n"); return;
	switch (errorCode) {
		ERR(D3D11_ERROR_FILE_NOT_FOUND)
		ERR(D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS)
		ERR(D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS)
		ERR(D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD)
		ERR(DXGI_ERROR_INVALID_CALL)
		ERR(DXGI_ERROR_WAS_STILL_DRAWING)
		ERR(E_FAIL)
		ERR(E_INVALIDARG)
		ERR(E_OUTOFMEMORY)
		ERR(E_NOTIMPL)
		ERR(S_FALSE)
	default:
		printf("Unknown error\n");
	}
#undef ERR
}

void Shader::initShader(WCHAR* vsFilename, WCHAR* psFilename, bool skin, bool colour, bool tangent) {

	// Load (+ compile) shader files
	if (skin)
		loadSkinVertexShader(vsFilename);
	else if (tangent)
		loadTangentVertexShader(vsFilename);
	else if (colour)
		loadColourVertexShader(vsFilename);
	else
		loadVertexShader(vsFilename);
	loadPixelShader(psFilename);

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	D3D11_BUFFER_DESC matrixBufferDesc;
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);

	//If this is derived from Shader, will setup additional specific buffers
	initBuffers();

}

/// Loads hull and domain for this shader
void Shader::initHullDomain(WCHAR* hsFilename, WCHAR* dsFilename) {
	loadHullShader(hsFilename);
	loadDomainShader(dsFilename);

	//setup dynamic tessellation buffer
	SETUP_SHADER_BUFFER(DynamicTessellationBufferType, dynamicTessellationBuffer);
}

/// Loads geometry for this shader
void Shader::initGeometry(WCHAR* gsFilename) {
	loadGeometryShader(gsFilename);
}

void Shader::setTexture(ID3D11ShaderResourceView * texture, int reg, ID3D11DeviceContext* deviceContext){
	deviceContext->PSSetShaderResources(reg, 1, &texture);
}

void Shader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX &worldMatrix, const XMMATRIX &viewMatrix, const XMMATRIX &projectionMatrix, XMFLOAT3 cameraPosition) {

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// Transpose the matrices to prepare them for the shader.
	XMMATRIX tworld, tview, tproj;
	tworld = XMMatrixTranspose(worldMatrix);
	tview = XMMatrixTranspose(viewMatrix);
	tproj = XMMatrixTranspose(projectionMatrix);
	MatrixBufferType* dataPtr;
	deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = tworld;
	dataPtr->view = tview;
	dataPtr->projection = tproj;
	deviceContext->Unmap(matrixBuffer, 0);
	if (domainShader) {//if there is a domain shader, the matrices will be applied there.
		deviceContext->DSSetConstantBuffers(0, 1, &matrixBuffer);
	}
	else if (geometryShader) {//instead if there is a geometry shader, the matrices will be applied there.
		deviceContext->GSSetConstantBuffers(0, 1, &matrixBuffer);
	}
	else {//otherwise, vertex shader it is
		deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);
	}

	//Send dynamic tessellation buffer if needed
	if (hullShader) {
		DynamicTessellationBufferType* dynPtr;
		deviceContext->Map(dynamicTessellationBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		dynPtr = (DynamicTessellationBufferType*)mappedResource.pData;
		dynPtr->worldMatrix = tworld;
		dynPtr->cameraPosition = cameraPosition;
		dynPtr->oneOverFarPlane = 1 / FAR_PLANE;
		dynPtr->tessellationMax = GLOBALS.TessellationMax;
		dynPtr->tessellationMin = GLOBALS.TessellationMin;
		dynPtr->tessellationRange = GLOBALS.TessellationRange <= 0 ? FLT_MAX : 1.0f/GLOBALS.TessellationRange;
		deviceContext->Unmap(dynamicTessellationBuffer, 0);
		deviceContext->HSSetConstantBuffers(0, 1, &dynamicTessellationBuffer);
	}

	// Set sampler resource in the pixel shader
	deviceContext->PSSetSamplers(0, 1, &sampleState);

}
