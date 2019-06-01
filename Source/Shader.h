#pragma once

#include "DXF.h"
#include "ExtendedLight.h"
#include "Material.h"

#define SHADER_PATH "Debug/" // Use this for debugging in IDE
//#define SHADER_PATH "" // Use this for production build

///use the following macro in any deriving class' constructor, passing it vertex and fragment shader filenames, as such:
///  SETUP_SHADER(default_vs, default_fs);
#define SETUP_SHADER(vert, frag) initShader((WCHAR*)L"" SHADER_PATH #vert ".cso", (WCHAR*)L"" SHADER_PATH #frag ".cso")
///use SETUP_SHADER_SKIN(default_vs, default_fs); for skinned shaders instead
#define SETUP_SHADER_SKIN(vert, frag) initShader((WCHAR*)L"" SHADER_PATH #vert ".cso", (WCHAR*)L"" SHADER_PATH #frag ".cso", true)
///use SETUP_SHADER_COLOUR(default_vs, default_fs); for colour shaders instead
#define SETUP_SHADER_COLOUR(vert, frag) initShader((WCHAR*)L"" SHADER_PATH #vert ".cso", (WCHAR*)L"" SHADER_PATH #frag ".cso", false, true)
///use SETUP_SHADER_TANGENT(default_vs, default_fs); for tangent access in shader
#define SETUP_SHADER_TANGENT(vert, frag) initShader((WCHAR*)L"" SHADER_PATH #vert ".cso", (WCHAR*)L"" SHADER_PATH #frag ".cso", false, false, true)
///use the following to also setup a hull and domain shader
#define SETUP_TESSELATION(hull, domain) initHullDomain((WCHAR*)L"" SHADER_PATH #hull ".cso", (WCHAR*)L"" SHADER_PATH #domain ".cso")
///use the following to also setup a geometry shader
#define SETUP_GEOMETRY(geom) initGeometry((WCHAR*)L"" SHADER_PATH #geom ".cso")

///use the following macro in initBuffer() to setup a shader buffer
#define SETUP_SHADER_BUFFER(type, buffer) do{D3D11_BUFFER_DESC desc;\
											desc.Usage = D3D11_USAGE_DYNAMIC;\
											desc.ByteWidth = sizeof(type);\
											desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;\
											desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;\
											desc.MiscFlags = 0;\
											desc.StructureByteStride = 0;\
											renderer->CreateBuffer(&desc, NULL, &buffer);}while(false)

#define UNUSED_SHADER_PARAM 0

#define FAR_PLANE GLOBALS.FarPlane //SCREEN_DEPTH // <-- changed because SCREEN_DEPTH is constant for some reason and its too high for this demo

using namespace DirectX;

class Shader : public BaseShader {
protected:
	//sent to Hull Shader in register b0 if there is a hull
	struct DynamicTessellationBufferType {
		XMMATRIX worldMatrix;
		XMFLOAT3 cameraPosition;
		float oneOverFarPlane;
		float tessellationMin;//minimum amount to apply (furthest from camera); should default to 1
		float tessellationMax;//maxmimum amount to apply (closest to camera); for example 64
		float tessellationRange;//between 1..infinity, how far we should keep applying tessellation at all (if set to 1, tessellation will happen all the way to far plane; at 2, only up to half distance; at infinity, no tessellation)
		float padding;
	};

public:
	Shader();
	virtual ~Shader();

	///setup the defaults parameters for any shader (matrices)
	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX &world, const XMMATRIX &view, const XMMATRIX &projection, XMFLOAT3 cameraPosition);

	/// Don't call these directly! When needed, use the SETUP_SHADER macros instead.
	///initializes the base buffers we need
	inline void initShader(WCHAR* vsFilename, WCHAR* psFilename) override { initShader(vsFilename, psFilename, false); }//<--need this to override pure virtual in BaseShader
	void initShader(WCHAR* vsFilename, WCHAR* psFilename, bool skin, bool colour = false, bool tangent = false);
	void initHullDomain(WCHAR* hsFilename, WCHAR* dsFilename);
	void initGeometry(WCHAR* gsFilename);

	void setTexture(ID3D11ShaderResourceView* texture, int reg, ID3D11DeviceContext* deviceContext);

	static void printError(HRESULT errorCode);

protected:

	///override this in deriving classes to setup additional buffers
	inline virtual void initBuffers() = 0;

	///similar to loadVertexShader(), loadColourVertexShader() and loadTextureVertexShader(), for skinned vertex shaders.
	void loadSkinVertexShader(WCHAR* filename);
	///same thing, for vertex shader with tangent input
	void loadTangentVertexShader(WCHAR* filename);

private:
	ID3D11Buffer* dynamicTessellationBuffer;//this will only be setup if SETUP_TESSELATION() is called!
};

