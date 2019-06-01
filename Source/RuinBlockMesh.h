#pragma once

#include "DXF.h"
#include "LitShader.h"
#include <vector>
#include <random>
#include "FileSystem.h"
#include "FileReader.h"
#include "FileWriter.h"

class RuinBlockMesh : public BaseMesh {

	///Vertex struct for geometry with position, texture, normals and tangents
	struct VertexType_Tangent {
		XMFLOAT3 position;
		XMFLOAT2 texture;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
	};

public:
	RuinBlockMesh(std::default_random_engine* randomEngine);
	~RuinBlockMesh();

	void sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) override;

	// i/o functions to save/load ruin block meshes
	void write(FileWriter& w);
	RuinBlockMesh(FileReader& r);
	RuinBlockMesh(int TEST);

protected:
	void initBuffers(ID3D11Device* device) override;

	//equality utilities
	static bool isEqual(const VertexType_Tangent& a, const VertexType_Tangent& b);
	static bool isEqual(XMFLOAT3 a, XMFLOAT3 b);
	static bool isAlmostEqual(const VertexType_Tangent& a, const VertexType_Tangent& b, float err = 0.001f);
	static bool isAlmostEqual(XMFLOAT3 a, XMFLOAT3 b, float err = 0.001f);
	static bool isAlmostEqual(float a, float b, float err = 0.001f);

	//utility for transforming directly the vertices of the mesh
	void scaleVerts(std::vector<VertexType_Tangent>* vertices, XMFLOAT3 scale);
	void translateVerts(std::vector<VertexType_Tangent>* vertices, XMFLOAT3 translation);

	//utility for adding primitives to the vertices
	static unsigned long addVert(std::vector<VertexType_Tangent>* vertices, const VertexType_Tangent& vert);//returns the index of vert within vertices, or adds it if it's not in the array.
	void addFace(std::vector<VertexType_Tangent>* vertices, std::vector<unsigned long>* indices, XMFLOAT3 pos, XMFLOAT3 scale, XMFLOAT3 topLeft, XMFLOAT3 bottomLeft, XMFLOAT3 bottomRight, XMFLOAT3 topRight, bool invertWinding = false);
	void addCube(std::vector<VertexType_Tangent>* vertices, std::vector<unsigned long>* indices, XMFLOAT3 position, XMFLOAT3 size, bool bottomFace);
	void addCylinder(std::vector<VertexType_Tangent>* vertices, std::vector<unsigned long>* indices, XMFLOAT3 position, XMFLOAT3 size, int resolution);//adds a cylinder to the verts (no bottom or top)
	void combine(std::vector<VertexType_Tangent>* destination, std::vector<unsigned long>* destIndices, std::vector<VertexType_Tangent>* source, std::vector<unsigned long>* srcIndices);

	//splits the mesh along a random plane, and keeps whichever part is biggest
	void splitMesh(std::vector<VertexType_Tangent>* vertices, std::vector<unsigned long>* indices, XMFLOAT3 planePt, XMFLOAT3 planeNorm);
	void getRandomSplittingPlane(std::vector<VertexType_Tangent>* vertices, XMFLOAT3& planePt, XMFLOAT3& planeNorm);
	void getRandomSplittingPlane(XMFLOAT3& planePt, XMFLOAT3& planeNorm, XMFLOAT3& aabbMin, XMFLOAT3& aabbMax);
	void getBoundingBox(std::vector<VertexType_Tangent>* vertices, float& minX, float& maxX, float& minY, float& maxY, float& minZ, float& maxZ, bool overwriteExisting = true);
	bool isPointAbovePlane(const XMFLOAT3& point, const XMFLOAT3& normal, const XMFLOAT3& planeOrigin);//returns whether the specified point is above the plane defined by planeOrigin and normal
	VertexType_Tangent inBetween(const VertexType_Tangent& a, const VertexType_Tangent& b, const XMFLOAT3& planeNorm, const XMFLOAT3& planeOrigin);//returns the point in between a and b which lies on the plane defined by planeOrigin and planeNorm
	bool isEdgeCutByPlane(const VertexType_Tangent& a, const VertexType_Tangent& b, const XMFLOAT3& planeNorm, const XMFLOAT3& planeOrigin);

	//splitting utilities
	void split12(std::vector<VertexType_Tangent>* vertices, std::vector<unsigned long>* indices, XMFLOAT3 scale);//executes 12 diagonal cuts to the passed vertices.

	float rnd(float min, float max);

	std::default_random_engine* randomEngine;

	//vertex and index buffers
	std::vector<VertexType_Tangent> vertices;
	std::vector<unsigned long> indices;
};

//a helper class allowing to generate a few block meshes once, then grab then out from the library when needed
class RuinBlockLibrary {

public:
	inline RuinBlockLibrary(int amount, int seed) : seed(seed) {//generate a certain amount of meshes
		if (FileSystem::fileExists("saved/blocks-" + std::to_string(seed))) {
			//read from disk instead of generating.
			FileReader r("saved/blocks-" + std::to_string(seed));
			int totalSize = r()->size();
			while (r--) {
				meshes.push_back(new RuinBlockMesh(r));
			}
		} else {

			//generate from scratch
			FileWriter w("saved/blocks-" + std::to_string(seed));
			std::default_random_engine randomEngine(seed);//use one random engine for all those meshes so that they're all different but end up the same when given the same initial seed
			for (int i = 0; i < amount; ++i) {
				meshes.push_back(new RuinBlockMesh(&randomEngine));
				meshes[i]->write(w);//write the blocks library out to disk to speed up next time we use the same seed
			}

		}
	}

	inline ~RuinBlockLibrary() {//free up everything
		for (auto it = meshes.begin(); it != meshes.end();) {
			delete (*it);
			it = meshes.erase(it);
		}
	}

	inline RuinBlockMesh* grab(int index) {//given any random index, returns a mesh (even if the index is way beyond the amount we have, as we use a mod)
		return meshes[index % meshes.size()];
	}

	inline int size() { return meshes.size(); }

protected:
	std::vector<RuinBlockMesh*> meshes;
	int seed;

};