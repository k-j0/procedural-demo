#pragma once

#include "DXF.h"
#include <random>
#include "Heightmap.h"
#include "RuinsMap.h"
#include "ExtendedLight.h"
#include "FileSystem.h"
#include "FileReader.h"
#include "FileWriter.h"

//#define SEND_DEBUG_RUINS_MAP//uncomment to send debug ruins map to terrain shader - note that terrain_fs needs an additional define to show the texture.

class TerrainMesh : public BaseMesh {

	///Vertex struct for geometry with position, texture, normals and tangents
	struct VertexType_Tangent {
		XMFLOAT3 position;
		XMFLOAT2 texture;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
	};

public:
	TerrainMesh(int seed, int x, int z, int size, RuinBlockLibrary* blockLibrary);
	~TerrainMesh();

	void sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) const;

	inline XMINT2 getBaseCoords() const { return XMINT2(baseX, baseZ); }

	inline const Heightmap* getHeightmap() { return heightmap; }

	//allow updating the buffers using data from surrounding neighbours
	void updateTerrain(const TerrainMesh* leftNeighbour, const TerrainMesh* belowNeighbour, const TerrainMesh* diagonalNeighbour, const TerrainMesh* topNeighbour, const TerrainMesh* rightNeighbour, ID3D11Device* device, ID3D11DeviceContext* deviceContext, float dt);
	void reinitBuffers(ID3D11Device* device, float dt);//call each frame, this will handle renewing the buffers.

	float getRealHeight(int x, int y) const;//uses the neighbouring heightmaps to get the actual height of the terrain mesh. x and y are between 0 - size.

	///grab a pointer to the debug texture of the map of the ruins
#ifdef SEND_DEBUG_RUINS_MAP
	inline ID3D11ShaderResourceView* getRuinMapView(ID3D11Device* device, ID3D11DeviceContext* deviceContext) { if (ruins) return ruins->asTexture(device, deviceContext); else return NULL; }
#else
	inline ID3D11ShaderResourceView* getRuinMapView(ID3D11Device* device, ID3D11DeviceContext* deviceContext) { return NULL; }//save on resources by never creating the debug texture.
#endif

	inline const RuinsMap* getRuins() const { return ruins; }

	inline ExtendedLight* getLight() { return &light; }
	inline ID3D11ShaderResourceView* getShadowmap() { return light.getShadowmap(); }

	inline bool hasMeshChanged() { return meshChanged; }

	inline const TerrainMesh* getLeftNeighbour() { return leftNeighbour; }
	inline const TerrainMesh* getBelowNeighbour() { return belowNeighbour; }
	inline const TerrainMesh* getDiagonalNeighbour() { return diagonalNeighbour; }

	inline int getIndexCount() const { return indexCount; }//hide base member for added const.

protected:
	void initHeightmap();
	void initBuffers(ID3D11Device* device) override;
	void calculateNormals(VertexType_Tangent** vertices);
	XMFLOAT3 getNormal(int x, int y);//returns the normal for a particular vert
	float computeRealHeight(int x, int y);

	int seed;
	int baseX;
	int baseZ;
	int size;

	RuinBlockLibrary* blockLibrary;

	Heightmap* heightmap = nullptr;//this is just an indication of the real heights
	RuinsMap* ruins = nullptr;//the ruins laid onto this terrain chunk
	float** realHeights;//a 2d array representing the real heights of all verts in this chunk, with one additional row/column on either side. this includes heights post-inclusion of neighbouring heightmaps
	
	//lighting
	ExtendedLight light;//each chunk has its own light, to support shadowmapping as best as possible on an infinite map

	/// The heightmaps belonging to our neighbours, which we can use to interpolate at the edges of the terrain, and to compute normals throughout
	const TerrainMesh* leftNeighbour = nullptr;// x-1 , z
	const TerrainMesh* belowNeighbour = nullptr;// x , z-1
	const TerrainMesh* diagonalNeighbour = nullptr;// x-1 , z-1
	const TerrainMesh* topNeighbour = nullptr;// x , z+1         <- for these two we need the terrain mesh ref rather than the raw heightmap, since their heightmaps will be influenced by us potentially
	const TerrainMesh* rightNeighbour = nullptr;// x+1 , z

	//these fields allow a delay between re initializing the buffers rather than do it each frame:
	float timeSinceLastBufferUpdate = 0;//allows us to only reinit buffers at certain intervals rather than each frame.
	bool needsReinitLater = false;//turns to true whenever we need to update buffers; when we don't need to anymore, this tells us to do one final update anyways!
	bool needUpdate;

	bool meshChanged = true;//true on frames when the mesh changed
};

#undef SEND_DEBUG_RUINS_MAP