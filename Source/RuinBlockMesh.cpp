#include "RuinBlockMesh.h"

#include "AppGlobals.h"
#include "Utils.h"


RuinBlockMesh::RuinBlockMesh(std::default_random_engine* randomEngine) : randomEngine(randomEngine){
	initBuffers(GLOBALS.Device);
}


RuinBlockMesh::~RuinBlockMesh(){
}

void RuinBlockMesh::split12(std::vector<VertexType_Tangent>* vertices, std::vector<unsigned long>* indices, XMFLOAT3 scale) {
	splitMesh(vertices, indices, XMFLOAT3(scale.x, scale.y, 0), XMFLOAT3(1, 1, 0));
	splitMesh(vertices, indices, XMFLOAT3(scale.x, -scale.y, 0), XMFLOAT3(1, -1, 0));
	splitMesh(vertices, indices, XMFLOAT3(-scale.x, -scale.y, 0), XMFLOAT3(-1, -1, 0));
	splitMesh(vertices, indices, XMFLOAT3(-scale.x, scale.y, 0), XMFLOAT3(-1, 1, 0));
	splitMesh(vertices, indices, XMFLOAT3(scale.x, 0, scale.z), XMFLOAT3(1, 0, 1));
	splitMesh(vertices, indices, XMFLOAT3(scale.x, 0, -scale.z), XMFLOAT3(1, 0, -1));
	splitMesh(vertices, indices, XMFLOAT3(-scale.x, 0, -scale.z), XMFLOAT3(-1, 0, -1));
	splitMesh(vertices, indices, XMFLOAT3(-scale.x, 0, scale.z), XMFLOAT3(-1, 0, 1));
	splitMesh(vertices, indices, XMFLOAT3(0, scale.y, scale.z), XMFLOAT3(0, 1, 1));
	splitMesh(vertices, indices, XMFLOAT3(0, scale.y, -scale.z), XMFLOAT3(0, 1, -1));
	splitMesh(vertices, indices, XMFLOAT3(0, -scale.y, -scale.z), XMFLOAT3(0, -1, -1));
	splitMesh(vertices, indices, XMFLOAT3(0, -scale.y, scale.z), XMFLOAT3(0, -1, 1));
}

float RuinBlockMesh::rnd(float min, float max) {
	float rand = float((*randomEngine)() % 1000) / 1000.0f;
	return min + (max - min) * rand;
}


void RuinBlockMesh::splitMesh(std::vector<VertexType_Tangent>* vertices, std::vector<unsigned long>* indices, XMFLOAT3 planePt, XMFLOAT3 planeNorm) {

	std::vector<VertexType_Tangent> newVerts;
	std::vector<unsigned long> newIndices;
	struct Edge {
		XMFLOAT3 a, b;
		XMFLOAT2 uvA, uvB;//coordinates within plane of the two verts.
		Edge(XMFLOAT3 a, XMFLOAT3 b) : a(a), b(b) {}
	};
	std::vector<Edge> cutEdges;//the edges along the cut

	//go through the mesh's tris using the indices
	//cut out any triangles that are above the threshold plane
	for (int index = 0; index < indices->size(); index += 3) {
		int vert1 = (*indices)[index];
		int vert2 = (*indices)[index + 1];
		int vert3 = (*indices)[index + 2];

		VertexType_Tangent a = (*vertices)[vert1];
		VertexType_Tangent b = (*vertices)[vert2];
		VertexType_Tangent c = (*vertices)[vert3];

		//figure out which of the three points are above/under the plane
		bool aOk = !isPointAbovePlane(a.position, planeNorm, planePt);
		bool bOk = !isPointAbovePlane(b.position, planeNorm, planePt);
		bool cOk = !isPointAbovePlane(c.position, planeNorm, planePt);

		int underPlane = 0;
		underPlane += aOk ? 1 : 0;
		underPlane += bOk ? 1 : 0;
		underPlane += cOk ? 1 : 0;

		//if all verts in the tri are below the plane, keep the exact same tri.
		if (underPlane == 3) {
			int newIndexA, newIndexB, newIndexC;

			newIndexA = addVert(&newVerts, a);//adds the vertices, or just returns their indices if they're already in there
			newIndexB = addVert(&newVerts, b);
			newIndexC = addVert(&newVerts, c);

			newIndices.push_back(newIndexA);
			newIndices.push_back(newIndexB);
			newIndices.push_back(newIndexC);
		}
		else if (underPlane == 2) {//2 out of 3 underneath the plane
			//name them differently so we know which one gets discarded and which ones we keep
			VertexType_Tangent discard, one, two;
			if (!aOk) { discard = a; one = b; two = c; }
			else if (!bOk) { discard = b; one = c; two = a; }//easy to prove that we need one to be C and two to be A to conserve winding order (easy to prove = 3 mins and a piece of paper)
			else { discard = c; one = a; two = b; }

			//find the point on one->discard which lies on the plane, and on two->discard similarly
			VertexType_Tangent oneD = inBetween(one, discard, planeNorm, planePt);
			VertexType_Tangent twoD = inBetween(two, discard, planeNorm, planePt);

			//now we have a quad (one, oneD, twoD, two) which we can split into two tris (one, oneD, twoD) and (twoD, oneD, two)
			// Since one and two have been selected to be either B->C, C->A or A->B, we can just keep the winding order by going: one->two->twoD->oneD
			int newIndexOne, newIndexTwo, newIndexOneD, newIndexTwoD;

			newIndexOne = addVert(&newVerts, one);//adds the vertices, or just returns their indices if they're already in there
			newIndexTwo = addVert(&newVerts, two);
			newIndexOneD = addVert(&newVerts, oneD);
			newIndexTwoD = addVert(&newVerts, twoD);

			//record new verts for linking later
			cutEdges.push_back(Edge(oneD.position, twoD.position));

			//first tri
			newIndices.push_back(newIndexOne);
			newIndices.push_back(newIndexTwo);
			newIndices.push_back(newIndexOneD);
			//second tri
			newIndices.push_back(newIndexOneD);
			newIndices.push_back(newIndexTwo);
			newIndices.push_back(newIndexTwoD);
		}
		else if (underPlane == 1) {//only onw vert under plane
			//only keep one of the three verts
			VertexType_Tangent keep, one, two;
			if (aOk) { keep = a; one = b; two = c; }
			else if (bOk) { keep = b; one = c; two = a; }
			else { keep = c; one = a; two = b; }

			//find the two points on the plane
			VertexType_Tangent oneD = inBetween(keep, one, planeNorm, planePt);
			VertexType_Tangent twoD = inBetween(keep, two, planeNorm, planePt);

			//make them into a new tri (keep, oneD, twoD) (winding order stays the same)
			int newIndexKeep, newIndexOneD, newIndexTwoD;
			newIndexKeep = addVert(&newVerts, keep);//adds the vertices, or just returns their indices if they're already in there
			newIndexOneD = addVert(&newVerts, oneD);
			newIndexTwoD = addVert(&newVerts, twoD);

			//record new verts for linking later
			cutEdges.push_back(Edge(oneD.position, twoD.position));

			newIndices.push_back(newIndexKeep);
			newIndices.push_back(newIndexOneD);
			newIndices.push_back(newIndexTwoD);

		}
		else {
			//discard them all since the tri is fully above the cutting plane.
		}
	}

	//close off gap left by cutting
	if (cutEdges.size() > 3) {

		//figure out the boundaries of the cut edges on the plane, to make a bounding box lying on the plane (to determine uvs)
		//take cutEdges[0] as arbitrary reference U axis within plane
		XMFLOAT3 uAxis = Utils::normalize(Utils::sub3(cutEdges[0].a, cutEdges[0].b));
		XMFLOAT3 vAxis = Utils::cross(planeNorm, uAxis);//figure out v axis using the normal and the u axis
		XMFLOAT2 uvBoundingMin(INFINITY, INFINITY), uvBoundingMax(-INFINITY, -INFINITY);
		for (Edge& edge : cutEdges) {//figure out the individual u,v coordinates within plane of the cut verts
			//project A onto u axis to determine its u coordinate, then onto v axis to determine v coord
			edge.uvA.x = Utils::dot3(Utils::sub3(edge.a, planePt), uAxis) / Utils::dot3(uAxis, uAxis);
			edge.uvA.y = Utils::dot3(Utils::sub3(edge.a, planePt), vAxis) / Utils::dot3(vAxis, vAxis);
			//same with B
			edge.uvB.x = Utils::dot3(Utils::sub3(edge.b, planePt), uAxis) / Utils::dot3(uAxis, uAxis);
			edge.uvB.y = Utils::dot3(Utils::sub3(edge.b, planePt), vAxis) / Utils::dot3(vAxis, vAxis);
			//compute bounding box in uv coords:
			if (uvBoundingMin.x > edge.uvA.x) uvBoundingMin.x = edge.uvA.x;
			if (uvBoundingMin.y > edge.uvA.y) uvBoundingMin.y = edge.uvA.y;
			if (uvBoundingMin.x > edge.uvB.x) uvBoundingMin.x = edge.uvB.x;
			if (uvBoundingMin.y > edge.uvB.y) uvBoundingMin.y = edge.uvB.y;
			if (uvBoundingMax.x < edge.uvA.x) uvBoundingMax.x = edge.uvA.x;
			if (uvBoundingMax.y < edge.uvA.y) uvBoundingMax.y = edge.uvA.y;
			if (uvBoundingMax.x < edge.uvB.x) uvBoundingMax.x = edge.uvB.x;
			if (uvBoundingMax.y < edge.uvB.y) uvBoundingMax.y = edge.uvB.y;
		}
		XMFLOAT2 uvBoundingSize = Utils::sub2(uvBoundingMax, uvBoundingMin);
		if (uvBoundingSize.x > uvBoundingSize.y) uvBoundingSize.y = uvBoundingSize.x; else uvBoundingSize.x = uvBoundingSize.y;//make it a square.
		//transform individual uv coords on edges to fit into bounding box along 0..1
		for (Edge& edge : cutEdges) {
			edge.uvA = Utils::add2(Utils::sub2(XMFLOAT2(0, 0), uvBoundingMin), Utils::mult2(edge.uvA, 1.f/uvBoundingSize.x));
			edge.uvB = Utils::add2(Utils::sub2(XMFLOAT2(0, 0), uvBoundingMin), Utils::mult2(edge.uvB, 1.f/uvBoundingSize.x));
		}

		//place the actual faces
		XMFLOAT3 start = cutEdges[0].a;//arbitrarily select a start
		for (Edge& edge : cutEdges) {
			if (isAlmostEqual(edge.a, start) || isAlmostEqual(edge.b, start)) {
				continue;//no need for that one (it's adjacent to the start point)
			} else {
				//add the triangle formed by start, edge.a, edge.b
				VertexType_Tangent one, two, three;
				one.position = start;
				one.texture = cutEdges[0].uvA;
				one.normal = planeNorm;
				two.position = edge.a;
				two.texture = edge.uvA;
				two.normal = planeNorm;
				three.position = edge.b;
				three.texture = edge.uvB;
				three.normal = planeNorm;
				//should winding order be inverted?
				XMFLOAT3 norm = Utils::cross(Utils::sub3(two.position, one.position), Utils::sub3(three.position, one.position));
				float dot = Utils::dot3(norm, planeNorm);
				if (dot > 0) {//invert winding.
					VertexType_Tangent cache = three;
					three = two;
					two = cache;
				}
				//add tri
				newIndices.push_back(addVert(&newVerts, one));
				newIndices.push_back(addVert(&newVerts, two));
				newIndices.push_back(addVert(&newVerts, three));
			}
		}
	}

	//finally copy the cached results into the vertex and index arrays
	vertices->clear();
	indices->clear();
	for (int i = 0; i < newVerts.size(); ++i) vertices->push_back(newVerts[i]);
	for (int i = 0; i < newIndices.size(); ++i) indices->push_back(newIndices[i]);
}

void RuinBlockMesh::getRandomSplittingPlane(std::vector<VertexType_Tangent>* vertices, XMFLOAT3 & planePt, XMFLOAT3 & planeNorm) {
	//determine AABB cube for the shape
	XMFLOAT3 aabbMin, aabbMax;
	getBoundingBox(vertices, aabbMin.x, aabbMax.x, aabbMin.y, aabbMax.y, aabbMin.z, aabbMax.z);
	XMFLOAT3 size(aabbMax.x - aabbMin.x, aabbMax.y - aabbMin.y, aabbMax.z - aabbMin.z);

	//find a plane which crosses the AABB
	do {
		planeNorm = XMFLOAT3(rnd(-1, 1), rnd(0, 1)/*greater on y*/, rnd(-1, 1));
		planeNorm = Utils::normalize(planeNorm);
	} while (planeNorm.x + planeNorm.y + planeNorm.z == 0);//make sure its not 0
	planePt = XMFLOAT3(rnd(aabbMin.x + size.x*0.1f, aabbMax.x - size.x*0.1f), rnd(aabbMin.y + size.y*0.1f, aabbMax.y - size.y*0.1f), rnd(aabbMin.z + size.z*0.1f, aabbMax.z - size.z*0.1f));//within an AABB cube 80% the size of the full aabb (centered)
	planePt = XMFLOAT3(rnd(-0.8f, 0.8f), rnd(-0.8f, 0.8f), rnd(-0.8f, 0.8f));// -0.8..0.8
#define SIGMOID(x) 1.f/(1.f+expf(-5*x))
	planePt = XMFLOAT3(SIGMOID(planePt.x), SIGMOID(planePt.y), SIGMOID(planePt.z));//~0..~1 (outcentered from sigmoid)
	planePt = XMFLOAT3(aabbMin.x + planePt.x*size.x, aabbMin.y + planePt.y*size.y, aabbMin.z + planePt.z*size.z);//0..1 -> aabbMin..aabbMax
#undef SIGMOID
}

void RuinBlockMesh::getRandomSplittingPlane(XMFLOAT3 & planePt, XMFLOAT3 & planeNorm, XMFLOAT3& aabbMin, XMFLOAT3& aabbMax) {
	XMFLOAT3 size(aabbMax.x - aabbMin.x, aabbMax.y - aabbMin.y, aabbMax.z - aabbMin.z);

	//find a plane which crosses the AABB
	do {
		planeNorm = XMFLOAT3(rnd(-1, 1), rnd(0, 1)/*greater on y*/, rnd(-1, 1));
		planeNorm = Utils::normalize(planeNorm);
	} while (planeNorm.x + planeNorm.y + planeNorm.z == 0);//make sure its not 0
	planePt = XMFLOAT3(rnd(aabbMin.x + size.x*0.1f, aabbMax.x - size.x*0.1f), rnd(aabbMin.y + size.y*0.1f, aabbMax.y - size.y*0.1f), rnd(aabbMin.z + size.z*0.1f, aabbMax.z - size.z*0.1f));//within an AABB cube 80% the size of the full aabb (centered)
	planePt = XMFLOAT3(rnd(-0.8f, 0.8f), rnd(-0.8f, 0.8f), rnd(-0.8f, 0.8f));// -0.8..0.8
#define SIGMOID(x) 1.f/(1.f+expf(-5*x))
	planePt = XMFLOAT3(SIGMOID(planePt.x), SIGMOID(planePt.y), SIGMOID(planePt.z));//~0..~1 (outcentered from sigmoid)
	planePt = XMFLOAT3(aabbMin.x + planePt.x*size.x, aabbMin.y + planePt.y*size.y, aabbMin.z + planePt.z*size.z);//0..1 -> aabbMin..aabbMax
#undef SIGMOID
}

void RuinBlockMesh::getBoundingBox(std::vector<VertexType_Tangent>* vertices, float & minX, float & maxX, float & minY, float & maxY, float & minZ, float & maxZ, bool overwriteExisting)
{

	if (overwriteExisting) {//setting to false allows to combine bounding boxes of several meshes at once:)
		minX = maxX = (*vertices)[0].position.x;
		minY = maxY = (*vertices)[0].position.y;
		minZ = maxZ = (*vertices)[0].position.z;
	}

	for (int i = 0; i < vertices->size(); ++i) {
		if ((*vertices)[i].position.x < minX) minX = (*vertices)[i].position.x;
		else if ((*vertices)[i].position.x > maxX) maxX = (*vertices)[i].position.x;
		if ((*vertices)[i].position.y < minY) minY = (*vertices)[i].position.y;
		else if ((*vertices)[i].position.y > maxY) maxY = (*vertices)[i].position.y;
		if ((*vertices)[i].position.z < minZ) minZ = (*vertices)[i].position.z;
		else if ((*vertices)[i].position.z > maxZ) maxZ = (*vertices)[i].position.z;
	}
}

bool RuinBlockMesh::isEqual(const VertexType_Tangent& a, const VertexType_Tangent& b) {
	return a.position.x == b.position.x && a.position.y == b.position.y && a.position.z == b.position.z &&
			a.normal.x == b.normal.x && a.normal.y == b.normal.y  && a.normal.z == b.normal.z &&
			a.texture.x == b.texture.x && a.texture.y == b.texture.y;
}

bool RuinBlockMesh::isEqual(XMFLOAT3 a, XMFLOAT3 b) {
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool RuinBlockMesh::isAlmostEqual(const VertexType_Tangent& a, const VertexType_Tangent& b, float err) {
	return isAlmostEqual(a.position, b.position, err) && isAlmostEqual(a.normal, b.normal, err) && isAlmostEqual(a.texture.x, b.texture.x, err) && isAlmostEqual(a.texture.y, b.texture.y, err);
}

bool RuinBlockMesh::isAlmostEqual(XMFLOAT3 a, XMFLOAT3 b, float err) {
	return isAlmostEqual(a.x, b.x, err) && isAlmostEqual(a.y, b.y, err) && isAlmostEqual(a.z, b.z, err);
}

bool RuinBlockMesh::isAlmostEqual(float a, float b, float err) {
	float diff = a - b;
	return diff*diff < err*err;
}

unsigned long RuinBlockMesh::addVert(std::vector<VertexType_Tangent>* vertices, const VertexType_Tangent & vert){
	for (int i = 0; i < vertices->size(); ++i) {
		if (isAlmostEqual(vert, (*vertices)[i]))
			return i;
	}
	//not found yet, so just throw it at the end
	vertices->push_back(vert);
	return vertices->size() - 1;
}

RuinBlockMesh::VertexType_Tangent RuinBlockMesh::inBetween(const VertexType_Tangent & a, const VertexType_Tangent & b, const XMFLOAT3 & planeNorm, const XMFLOAT3 & planeOrigin){
	VertexType_Tangent err;
	err.position = XMFLOAT3(INFINITY, INFINITY, INFINITY);//our error type in this case is just a vert at +INFINITY on all axes
	XMFLOAT3 aToB = Utils::sub3(a.position, b.position);
	//printf("A (%f  %f  %f) to B (%f  %f  %f) is (%f  %f  %f)\n", a.position.x, a.position.y, a.position.z, b.position.x, b.position.y, b.position.z, aToB.x, aToB.y, aToB.z);
	float dot = Utils::dot3(aToB, planeNorm);
	if (dot == 0) { printf("Plane and line are parallel!\n"); return err; }//plane and line are parallel
	float s = -Utils::dot3(Utils::sub3(planeOrigin, a.position), planeNorm) / dot;
	if (s < -0.0001f || s > 1.0001f) { printf("S is not in 0..1! S = %f\n", s); return err; }//the point of contact is too far in either direction
	//linearly interpolate between the two to figure out the midpoint's position, normal and uvs
	VertexType_Tangent ret;
	ret.position = Utils::lerp(a.position, b.position, 1-s);
	ret.normal = Utils::lerp(a.normal, b.normal, 1-s);
	ret.texture = Utils::lerp(a.texture, b.texture, 1-s);
	return ret;
}

bool RuinBlockMesh::isEdgeCutByPlane(const VertexType_Tangent & a, const VertexType_Tangent & b, const XMFLOAT3 & planeNorm, const XMFLOAT3 & planeOrigin)
{
	XMFLOAT3 aToB = Utils::sub3(a.position, b.position);
	float dot = Utils::dot3(aToB, planeNorm);
	if (dot == 0) { return false; }//plane and line are parallel
	float s = Utils::dot3(planeNorm, Utils::sub3(a.position, planeOrigin)) / Utils::dot3(planeNorm, aToB);
	return s >= 0 && s <= 1;
}

bool RuinBlockMesh::isPointAbovePlane(const XMFLOAT3 & point, const XMFLOAT3 & normal, const XMFLOAT3 & planeOrigin){
	XMFLOAT3 toPoint = Utils::sub3(point, planeOrigin);
	//dot product between that and the normal:
	float dot = Utils::dot3(toPoint, normal);
	return dot >= 0;
}



void RuinBlockMesh::scaleVerts(std::vector<VertexType_Tangent>* vertices, XMFLOAT3 scale) {
	for (VertexType_Tangent& vert : *vertices) {
		vert.position = XMFLOAT3(vert.position.x * scale.x, vert.position.y * scale.y, vert.position.z * scale.z);
	}
}

void RuinBlockMesh::translateVerts(std::vector<VertexType_Tangent>* vertices, XMFLOAT3 translation) {
	for (VertexType_Tangent& vert : *vertices) {
		vert.position = Utils::add3(vert.position, translation);
	}
}

void RuinBlockMesh::addFace(std::vector<VertexType_Tangent>* vertices, std::vector<unsigned long>* indices, XMFLOAT3 pos, XMFLOAT3 scale, XMFLOAT3 topLeft, XMFLOAT3 bottomLeft, XMFLOAT3 bottomRight, XMFLOAT3 topRight, bool invertWinding) {
	VertexType_Tangent v1, v2, v3, v4;
	v1.normal = v2.normal = v3.normal = v4.normal = Utils::normalize(Utils::cross(Utils::sub3(bottomRight, bottomLeft), Utils::sub3(topLeft, bottomLeft)));
	v1.position = Utils::add3(Utils::mult3(topLeft, scale), pos);
	v1.texture = XMFLOAT2(0, 0);
	v2.position = Utils::add3(Utils::mult3(bottomLeft, scale), pos);
	v2.texture = XMFLOAT2(0, 1);
	v3.position = Utils::add3(Utils::mult3(bottomRight, scale), pos);
	v3.texture = XMFLOAT2(1, 1);
	v4.position = Utils::add3(Utils::mult3(topRight, scale), pos);
	v4.texture = XMFLOAT2(1, 0);

	/*printf("Adding verts %f  %f  %f       %f  %f  %f      %f  %f  %f       %f  %f  %f\n",
	v1.position.x, v1.position.y, v1.position.z,
	v2.position.x, v2.position.y, v2.position.z,
	v3.position.x, v3.position.y, v3.position.z,
	v4.position.x, v4.position.y, v4.position.z);*/ //comment in to see debug output for each face.

	//invert winding order if needed
	if (!invertWinding) { v1.normal = v2.normal = v3.normal = v4.normal = Utils::sub3(XMFLOAT3(0, 0, 0), v1.normal); }
#define INV(a, b)if(invertWinding){VertexType_Tangent cache = a;a = b;b = cache;}

	//add the two tris
	INV(v2, v3);
	indices->push_back(RuinBlockMesh::addVert(vertices, v1));
	indices->push_back(RuinBlockMesh::addVert(vertices, v2));
	indices->push_back(RuinBlockMesh::addVert(vertices, v3));
	INV(v2, v3); INV(v1, v4);
	indices->push_back(RuinBlockMesh::addVert(vertices, v3));
	indices->push_back(RuinBlockMesh::addVert(vertices, v4));
	indices->push_back(RuinBlockMesh::addVert(vertices, v1));

#undef INV

}

void RuinBlockMesh::addCube(std::vector<VertexType_Tangent>* vertices, std::vector<unsigned long>* indices, XMFLOAT3 position, XMFLOAT3 size, bool bottomFace) {

	//front
	addFace(vertices, indices, position, size, XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(0.5f, 0.5f, -0.5f));
	//back
	addFace(vertices, indices, position, size, XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT3(0.5f, 0.5f, 0.5f), true);
	//left
	addFace(vertices, indices, position, size, XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(-0.5f, 0.5f, -0.5f));
	//right
	addFace(vertices, indices, position, size, XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(0.5f, 0.5f, -0.5f), true);
	//top
	addFace(vertices, indices, position, size, XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT3(0.5f, 0.5f, 0.5f));
	//bottom
	if (bottomFace) addFace(vertices, indices, position, size, XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(0.5f, -0.5f, 0.5f), true);

}

void RuinBlockMesh::addCylinder(std::vector<VertexType_Tangent>* vertices, std::vector<unsigned long>* indices, XMFLOAT3 position, XMFLOAT3 size, int resolution) {

	float previousAngle = 0;
	for (int i = 1; i < resolution; ++i) {
		float angle = 2*PI*(float)i / (float)resolution;
		if (i == resolution - 1) angle = 0;//loop back.

		XMFLOAT3 topLeft(cosf(previousAngle)*size.x*0.5f, size.y*0.5f, sinf(previousAngle)*size.z*0.5f);
		XMFLOAT3 topRight(cosf(angle)*size.x*0.5f, size.y*0.5f, sinf(angle)*size.z*0.5f);
		XMFLOAT3 bottomLeft(cosf(previousAngle)*size.x*0.5f, -size.y*0.5f, sinf(previousAngle)*size.z*0.5f);
		XMFLOAT3 bottomRight(cosf(angle)*size.x*0.5f, -size.y*0.5f, sinf(angle)*size.z*0.5f);

		addFace(vertices, indices, position, XMFLOAT3(1, 1, 1), topLeft, bottomLeft, bottomRight, topRight);

		previousAngle = angle;
	}
	//top and bottom faces (it doesn't matter that they're squares instead of circles, they're just here to be able to split the mesh later)
	addFace(vertices, indices, position, size, XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT3(0.5f, 0.5f, 0.5f));
	addFace(vertices, indices, position, size, XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(0.5f, -0.5f, 0.5f), true);

}

void RuinBlockMesh::combine(std::vector<VertexType_Tangent>* destination, std::vector<unsigned long>* destIndices, std::vector<VertexType_Tangent>* source, std::vector<unsigned long>* srcIndices) {
	for (unsigned long srcIndex : *srcIndices) {
		destIndices->push_back(addVert(destination, (*source)[srcIndex]));
	}
}




void RuinBlockMesh::initBuffers(ID3D11Device * device){

	if (rnd(0, 1) < 0.75f) {
		//Stone cube/slab.

		addCube(&vertices, &indices, XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), true);
		XMFLOAT3 scale = XMFLOAT3(1.4f, rnd(1, 3), 1.4f);
		scaleVerts(&vertices, scale);
		split12(&vertices, &indices, XMFLOAT3(scale.x*0.5f - 0.05f, scale.y*0.5f - 0.05f, scale.z*0.5f - 0.05f));//cut up the edges
		translateVerts(&vertices, XMFLOAT3(0, scale.y*0.5f - 0.1f, 0));

		//add random cuts along a couple planes
		XMFLOAT3 planePt, planeNorm;
		getRandomSplittingPlane(&vertices, planePt, planeNorm);
		splitMesh(&vertices, &indices, planePt, planeNorm);
		if (rnd(0, 1) < 0.5f && vertices.size() > 0) {
			getRandomSplittingPlane(&vertices, planePt, planeNorm);
			splitMesh(&vertices, &indices, planePt, planeNorm);
		}

	} else {
		//Stone column.

		std::vector<VertexType_Tangent> cube1;
		std::vector<unsigned long> cube1Indices;
		std::vector<VertexType_Tangent> cube2;
		std::vector<unsigned long> cube2Indices;

		//bounding box of the whole shape (3 uncombined meshes)
		XMFLOAT3 aabbMin, aabbMax;

		//one cube on each extremity.
		addCube(&cube1, &cube1Indices, XMFLOAT3(0, 3.95f, 0), XMFLOAT3(1, 0.4f, 1), true);
		getBoundingBox(&cube1, aabbMin.x, aabbMax.x, aabbMin.y, aabbMax.y, aabbMin.z, aabbMax.z);
		addCube(&cube2, &cube2Indices, XMFLOAT3(0, -0.05f, 0), XMFLOAT3(1, 0.4f, 1), true);
		getBoundingBox(&cube2, aabbMin.x, aabbMax.x, aabbMin.y, aabbMax.y, aabbMin.z, aabbMax.z, false);

		//the actual column cylinder
		addCylinder(&vertices, &indices, XMFLOAT3(0, 1.95f, 0), XMFLOAT3(0.7f, 4.f, 0.7f), 20);
		getBoundingBox(&vertices, aabbMin.x, aabbMax.x, aabbMin.y, aabbMax.y, aabbMin.z, aabbMax.z, false);

		//split the 3 meshes once along a random plane
		if (rnd(0, 1) < 0.75f) {
			XMFLOAT3 planePt, planeNorm;
			getRandomSplittingPlane(planePt, planeNorm, aabbMin, aabbMax);
			splitMesh(&cube1, &cube1Indices, planePt, planeNorm);
			splitMesh(&cube2, &cube2Indices, planePt, planeNorm);
			splitMesh(&vertices, &indices, planePt, planeNorm);
		}

		//combine into one mesh:
		combine(&vertices, &indices, &cube1, &cube1Indices);
		combine(&vertices, &indices, &cube2, &cube2Indices);
	}
	
	
	vertexCount = vertices.size();
	indexCount = indices.size();

	// push mesh data to gpu buffers

	D3D11_SUBRESOURCE_DATA vertexData, indexData;

	D3D11_BUFFER_DESC vertexBufferDesc = { sizeof(VertexType_Tangent) * vertexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	vertexData = { vertices.data(), 0 , 0 };
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

	D3D11_BUFFER_DESC indexBufferDesc = { sizeof(unsigned long) * indexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
	indexData = { indices.data(), 0, 0 };
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

}

void RuinBlockMesh::sendData(ID3D11DeviceContext * deviceContext, D3D_PRIMITIVE_TOPOLOGY top){
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType_Tangent);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(top);
}

// I/O functions

void RuinBlockMesh::write(FileWriter & w) {
	//write how many verts we need to read
	FileSystem::w_uint16(w(), vertices.size());
	//write verts
	for (int i = 0; i < vertices.size(); ++i) {
		int before = w()->size();
		VertexType_Tangent& vtt = vertices[i];
		FileSystem::w_float3(w(), vtt.position);
		FileSystem::w_float3(w(), vtt.normal);
		FileSystem::w_float2(w(), vtt.texture);
		FileSystem::w_float3(w(), vtt.tangent);
		int space = w()->size() - before;
	}
	//write how many indices we need to read
	FileSystem::w_uint16(w(), indices.size());
	//write indices
	for (int i = 0; i < indices.size(); ++i) {
		unsigned long index = indices[i];
		FileSystem::w_uint32(w(), index);//yeah i'm writing a uint64 as a uint32 but i dont think i'm ever going to reach that many tris:)
	}
}

RuinBlockMesh::RuinBlockMesh(FileReader & r) {

	//read verts
	vertexCount = FileSystem::r_uint16(r());
	for (int i = 0; i < vertexCount; ++i) {
		int space = r()->size();
		VertexType_Tangent vtt;
		vtt.position = FileSystem::r_float3(r());
		vtt.normal = FileSystem::r_float3(r());
		vtt.texture = FileSystem::r_float2(r());
		vtt.tangent = FileSystem::r_float3(r());
		space -= r()->size();
		vertices.push_back(vtt);
	}
	//read indices
	indexCount = FileSystem::r_uint16(r());
	for (int i = 0; i < indexCount; ++i) {
		unsigned long index = FileSystem::r_uint32(r());
		indices.push_back(index);
	}

	// push mesh data to gpu buffers

	D3D11_SUBRESOURCE_DATA vertexData, indexData;

	D3D11_BUFFER_DESC vertexBufferDesc = { sizeof(VertexType_Tangent) * vertexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	vertexData = { vertices.data(), 0 , 0 };
	GLOBALS.Device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

	D3D11_BUFFER_DESC indexBufferDesc = { sizeof(unsigned long) * indexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
	indexData = { indices.data(), 0, 0 };
	GLOBALS.Device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);
	

}


RuinBlockMesh::RuinBlockMesh(int TEST) {
	VertexType_Tangent test;
	test.position = XMFLOAT3(1, 2, 3);
	vertices.push_back(test);
	vertices.push_back(test);
	test.position = XMFLOAT3(4, 5, 6);
	vertices.push_back(test);
	vertices.push_back(test);
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
}