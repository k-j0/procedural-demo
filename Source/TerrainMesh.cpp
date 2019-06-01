#include "TerrainMesh.h"

#include "AppGlobals.h"
#include <algorithm>
#include <random>
#include "PerlinNoise.h"
#include <experimental/coroutine>
#include "Utils.h"

#define REINIT_TIMEOUT 1.0f //minimum amount of time between each buffer reinit


TerrainMesh::TerrainMesh(int seed, int x, int z, int size, RuinBlockLibrary* blockLibrary) : seed(seed + 24 * x + 9999 * z), baseX(x), baseZ(z), size(size), blockLibrary(blockLibrary){//the effective seed depends on the base coords

	if (size < 2) return;

	//setup our own light. one for each chunk to enable shadows:)
	light.setAmbientColour(42.f / 255.f, 89.f / 255.f, 109.f / 255.f, 1);
	light.setDiffuseColour(1, 1, 1, 1);
	light.setPosition(x-60, 25, z-60);
	light.setDirection(1, -0.5f, 1);
	light.setAttenuation(1, 0, 0);
	light.setType(DIRECTIONAL_LIGHT);
	light.setupShadows();
	light.updateFov(165.f);
	light.setShadowmapSize(142);//sqrt(100x100 + 100x100), ie the length of the diagonal of a 100x100 chunk
	light.setShadowmapRes(4096);

	initHeightmap();

	initBuffers(GLOBALS.Device);
}

TerrainMesh::~TerrainMesh(){
	if (heightmap) delete heightmap;
	heightmap = nullptr;
	if (ruins) delete ruins;
	ruins = nullptr;
	if (realHeights) {
		for (int y = 0; y < size + 2; ++y)
			delete[] realHeights[y];
		delete[] realHeights;
		realHeights = nullptr;
	}
	vertexBuffer->Release();
	indexBuffer->Release();
	vertexBuffer = nullptr;
	indexBuffer = nullptr;
}

///Returns the actual height at a certain point (-1..size+1) for the chunk. This includes neighbours etc
float TerrainMesh::getRealHeight(int x, int y) const {
	if (x < -1 || x >= size + 1 || y < -1 || y >= size + 1) {
		printf("ArrayOutOfBounds! cannot get real height at index %d %d on a chunk of size %d..\n", x, y, size);
		return 0;
	}
	return realHeights[y + 1][x + 1];
}

float TerrainMesh::computeRealHeight(int x, int y){
	float threshold = 0.2f * size;
	if (x < 0 || y < 0 || x >= size || y >= size) {
		//allow grabbing heights 1 index outside our own size from neighbouring heightmaps
		if (x == -1 && y >= 0 && y < size) {
			//grab this height from our Left neighbour instead
			if (leftNeighbour) {
				return leftNeighbour->getRealHeight(x + size - 1, y);
			}
		}
		else if (y == -1 && x >= 0 && x < size) {
			//grab this height from our below neighbour instead
			if (belowNeighbour) {
				return belowNeighbour->getRealHeight(x, y + size - 1);
			}
		}
		else if (x == size && y >= 0 && y < size) {
			if (rightNeighbour) {
				return rightNeighbour->getRealHeight(1, y);
			}
		}
		else if (y == size) {
			if (topNeighbour) {
				return topNeighbour->getRealHeight(x, 1);
			}
		}
		//printf("Cannot get height at position (%d, %d) on a terrain mesh of size %d.", x, y, size);
		return INFINITY;
	}
	float height = heightmap->getHeight(x, y);
	//factor in neighbours
	float leftContribution = 0;
	float belowContribution = 0;
	float leftHeight = 0;
	float belowHeight = 0;
	float diagonalHeight = -1000;
	if (x < threshold && leftNeighbour && leftNeighbour->heightmap && x + size - 1 < heightmap->getSize()) {//add contribution from left neighbour
		leftHeight = leftNeighbour->heightmap->getHeight(x + size - 1, y);
		leftContribution = 1 - (float)x / threshold;
	}
	if (y < threshold && belowNeighbour && belowNeighbour->heightmap && y + size - 1 < heightmap->getSize()) {//add contribution from below neighbour
		belowHeight = belowNeighbour->heightmap->getHeight(x, y + size - 1);
		belowContribution = 1 - (float)y / threshold;
	}
	if (leftContribution > 0 && belowContribution > 0) {//add contribution from diagonal neighbour
		if (diagonalNeighbour && diagonalNeighbour->heightmap) {
			diagonalHeight = diagonalNeighbour->heightmap->getHeight(x + size - 1, y + size - 1);
		}
		else {
			//printf("No diagonal neighbour! Hole in map will be visible.\n"); // <-- this is unnecessary since the inifinite terrain code makes it inevitable, but only for chunks fast enough that it doesn't matter
		}
	}
	float thisPart = height * (1 - leftContribution) * (1 - belowContribution);
	float leftPart = leftHeight * leftContribution * (1 - belowContribution);
	float belowPart = belowHeight * belowContribution * (1 - leftContribution);
	float diagonalPart = diagonalHeight * leftContribution * belowContribution;
	return thisPart + leftPart + belowPart + diagonalPart;

	/**
	ShaderToy code to model the interpolation. Head over to https://www.shadertoy.com/new and paste to see results.
	(not meant to be good shader code, just a model of the problem).
	(this is also the first time i paste non-cpp code into a cpp file as comment, i think).
	*******



void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	// Normalized pixel coordinates (from 0 to 1)
	vec2 uv = fragCoord/iResolution.xy;    

	vec3 LEFT = vec3(0.9, 0.1, 0.3);
	vec3 BELOW = vec3(0.0, 0.8, 0.8);
	vec3 DIAG = vec3(0.7, 0.6, 0.3);
	vec3 THIS = vec3(0.4, 0.9, 0.3);
    
    float blendDist = 0.4;
    
	uv = 2.0*uv;
	float left = 0.0;
	float below = 0.0;
    bool inDiag = false;
	if(uv.x > 1.0){
		uv.x -= 1.0;
		if(uv.x < blendDist)
			left = 1.0 - uv.x / blendDist;
    }else{
        if(uv.y <= 1.0){inDiag = true;THIS = DIAG;}
        else{THIS = LEFT;BELOW = DIAG;}
    }
	if(uv.y > 1.0){
		uv.y -= 1.0;
		if(uv.y < blendDist)
			below = 1.0 - uv.y / blendDist;
    }else if(!inDiag){
        THIS = BELOW;
        LEFT = DIAG;
    }

    //Blending code:
    
	LEFT *= left * (1.0-below);
	BELOW *= below * (1.0-left);
	THIS *= (1.0-left) * (1.0-below);
	DIAG *= left * below;


	vec3 total = LEFT + BELOW + THIS + DIAG;

    bool showErrors = true;
	if(!showErrors || (total.x <= 1.0 && total.y <= 1.0 && total.z <= 1.0)){
		fragColor = vec4(total, 1.0);
	}
	else fragColor = vec4(1.0, 0.0, 1.0, 1.0);
}


	*/
}

void TerrainMesh::initHeightmap() {
	
	if (heightmap) delete heightmap;
	heightmap = new Heightmap(seed, size * 1.2f);//generating a larger heightmap than the terrain mesh means we can interpolate between heightmaps in between terrains

	if (realHeights) {
		for (int y = 0; y < size + 2; ++y)
			delete[] realHeights[y];
		delete[] realHeights;
		realHeights = nullptr;
	}
	realHeights = new float*[size + 2];//see computeRealHeight(int,int) for size explanation.
	for (int y = 0; y < size + 2; ++y) {
		realHeights[y] = new float[size + 2];
		for (int x = 0; x < size + 2; ++x) {
			realHeights[y][x] = 0;
		}
	}

	heightmap->generate();

}

void TerrainMesh::updateTerrain(const TerrainMesh* leftNeighbour, const TerrainMesh* belowNeighbour, const TerrainMesh* diagonalNeighbour, const TerrainMesh* topNeighbour, const TerrainMesh* rightNeighbour, ID3D11Device* device, ID3D11DeviceContext* deviceContext, float dt) {

	needUpdate = false;
	//only update the mesh if there's been any changes (the updating of the mesh happens within reinitBuffers)
	needUpdate |= leftNeighbour != this->leftNeighbour;
	needUpdate |= belowNeighbour != this->belowNeighbour;
	needUpdate |= diagonalNeighbour != this->diagonalNeighbour;
	needUpdate |= topNeighbour != this->topNeighbour;
	needUpdate |= rightNeighbour != this->rightNeighbour;
	
	this->leftNeighbour = leftNeighbour;
	this->belowNeighbour = belowNeighbour;
	this->diagonalNeighbour = diagonalNeighbour;
	this->topNeighbour = topNeighbour;
	this->rightNeighbour = rightNeighbour;
	
	//update the heightmap (for async generation)
	heightmap->update();
	needUpdate |= heightmap->hasChanged();
	if (leftNeighbour) needUpdate |= leftNeighbour->heightmap->hasChanged();
	if (belowNeighbour) needUpdate |= belowNeighbour->heightmap->hasChanged();
	if (diagonalNeighbour) needUpdate |= diagonalNeighbour->heightmap->hasChanged();
	if (topNeighbour && topNeighbour->heightmap) needUpdate |= topNeighbour->heightmap->hasChanged();
	if (rightNeighbour && rightNeighbour->heightmap) needUpdate |= rightNeighbour->heightmap->hasChanged();

	if (!needUpdate) {
		if (!ruins) {
			ruins = new RuinsMap(seed-1, size-1, [&](int x, int y) {//Slope function
						//given coordinates on the heightmap, returns a slope value between 0..1.
						return 1 - getNormal(x, y).y;
					}, [&](int x, int y) {//Height function
						return getRealHeight(x, y);
					}, blockLibrary);
			needUpdate = true;
		}
	}

}

void TerrainMesh::reinitBuffers(ID3D11Device* device, float dt) {
	meshChanged = false;

	if (needUpdate) {//only update buffers every few millis rather than every single frame to save on resources:
		needsReinitLater = true;
		timeSinceLastBufferUpdate -= dt;
		if (timeSinceLastBufferUpdate <= 0) {
			initBuffers(device);
			needsReinitLater = false;
			meshChanged = true;
			timeSinceLastBufferUpdate = REINIT_TIMEOUT;
		}
	}
	else if (needsReinitLater) {

		initBuffers(device);//do a final one to get the last update regardless of time passed
		needsReinitLater = false;
		meshChanged = true;
	}

	//also update the ruins to make sure they're entirely generated (or re-generated if needed)
	if (ruins) {
		if(ruins->update(needUpdate))
			meshChanged = true;
	}
}

void TerrainMesh::initBuffers(ID3D11Device * device){
	if(vertexBuffer)vertexBuffer->Release();
	vertexBuffer = nullptr;
	if(indexBuffer)indexBuffer->Release();
	indexBuffer = nullptr;

	//compute the current real heights at all points
	for (int y = -1; y < size + 1; ++y) {
		for (int x = -1; x < size + 1; ++x) {
			realHeights[y + 1][x + 1] = computeRealHeight(x, y);
		}
	}


	D3D11_SUBRESOURCE_DATA vertexData, indexData;

	vertexCount = size*size;// size is the number of vertices on one axis
	indexCount = (size-1)*(size-1)*6;// 6 indices per plane

	VertexType_Tangent* vertices = new VertexType_Tangent[vertexCount];
	unsigned long* indices = new unsigned long[indexCount];

	// Load the vertex array with data from the heightmap's data
	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			int index = y * size + x;
			vertices[index].position = XMFLOAT3(x+baseX-size/2, getRealHeight(x, y), y+baseZ-size/2);
			vertices[index].texture = XMFLOAT2((float)x / (size-1), (float)y / (size-1));
			vertices[index].normal = XMFLOAT3(0, 1, 0);
			vertices[index].tangent = XMFLOAT3(1, 0, 0);
		}
	}

	// generate normals for those vertices
	calculateNormals(&vertices);

	// Load the index array with data.
	int index = -1;
	for (int y = 0; y < size - 1; ++y) {
		for (int x = 0; x < size - 1; ++x) {
			int index1 = y*size + x;//bottom left
			int index2 = y*size + x + 1;//bottom right
			int index3 = (y + 1)*size + x;//upper left
			int index4 = (y + 1)*size + x + 1;//upper right

			//diamond pattern
			if (x % 2 == y % 2) std::swap(index2, index4);

			//First tri
			indices[++index] = index4;
			indices[++index] = index3;
			indices[++index] = index1;

			//diamond pattern
			if (x % 2 == y % 2) { std::swap(index2, index4); std::swap(index1, index3); }

			//Second tri
			indices[++index] = index4;
			indices[++index] = index1;
			indices[++index] = index2;

		}
	}

	D3D11_BUFFER_DESC vertexBufferDesc = { sizeof(VertexType_Tangent) * vertexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	vertexData = { vertices, 0 , 0 };
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

	D3D11_BUFFER_DESC indexBufferDesc = { sizeof(unsigned long) * indexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
	indexData = { indices, 0, 0 };
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;
	delete[] indices;
	indices = 0;
}

XMFLOAT3 TerrainMesh::getNormal(int x, int y) {

	// javascript-style inline function to get positions of adjoining vertices on the terrain (or on the adjacent terrains). JS-style inline functions are very ugly in c++, sorry about that :)
	/*struct __inlineFunctionHelper {
		TerrainMesh* t;
		__inlineFunctionHelper(TerrainMesh* t) : t(t) {}
		///returns the vertex position at coords x,y. if invalid, 0 is returned in w (otherwise 1).
		XMFLOAT4 operator() (int x, int y) {
			float height = t->getRealHeight(x, y);
			if (height == INFINITY) return XMFLOAT4(0, 0, 0, 0);//invalid
			return XMFLOAT4(x, height, y, 1);//valid
		}
	} GetVert(this);*/
	/*XMFLOAT4 p = GetVert(x, y);
	XMFLOAT4 left = GetVert(x - 1, y);
	XMFLOAT4 right = GetVert(x + 1, y);
	XMFLOAT4 up = GetVert(x, y + 1);
	XMFLOAT4 down = GetVert(x, y - 1);*/

	//Note: replaced with this macro for optimization. however, please refer to above function as far as clarity goes.
#define GetVert(x, y, name) XMFLOAT4 p##name; {float height = getRealHeight(x, y); if(height == INFINITY) p##name = XMFLOAT4(0,0,0,0); else p##name = XMFLOAT4(x, height, y, 1);}

	//grab adjacent verts using that inline function from above
	GetVert(x, y, p);
	GetVert(x - 1, y, left);
	GetVert(x + 1, y, right);
	GetVert(x, y + 1, up);
	GetVert(x, y - 1, down);

	//accumulate cross produces from all 4 verts to generate summed-up normal then average:
	XMFLOAT3 normals(0, 0, 0);

#define ADD_NORMAL(vert1, vert2) {\
	normals = Utils::add3(normals, Utils::normalize(Utils::cross(Utils::sub3(vert1, pp), Utils::sub3(vert2, pp))));\
}

	if (pleft.w && pup.w) {
		ADD_NORMAL(pleft, pup);
	}
	if (pright.w && pup.w) {
		ADD_NORMAL(pup, pright);
	}
	if (pright.w && pdown.w) {
		ADD_NORMAL(pright, pdown);
	}
	if (pleft.w && pdown.w) {
		ADD_NORMAL(pdown, pleft);
	}

	//average out, and assign
	return Utils::normalize(normals);

#undef ADD_NORMAL
}

//From the initial tutorial example
void TerrainMesh::calculateNormals(VertexType_Tangent** vertices) {

	//iterate over vertices to compute normals for each one
	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			int index = y * size + x;
			
			(*vertices)[index].normal = getNormal(x, y);

		}
	}
}

void TerrainMesh::sendData(ID3D11DeviceContext * deviceContext, D3D_PRIMITIVE_TOPOLOGY top) const{
	if(!vertexBuffer || !indexBuffer) return;
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType_Tangent);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(top);
}
