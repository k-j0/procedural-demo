#include "InfiniteTerrain.h"
#include "LitShader.h"
#include "AppGlobals.h"
#include "PPTextureShader.h"
#include "Utils.h"

#define MAXIMUM_CHUNKS_AT_ONCE 20 //beyond this limit, some chunks will be unloaded from RAM
//#define SMALL_AMOUNT_OF_CHUNKS //define this to only see a small amount of chunks at a time, rather than the full set
//#define NO_INFINITY //when defined, only one chunk is produced instead of an infinite amount :)
//#define CHECK_BLOCKS //when defined, also renders the block library underneath the terrain


InfiniteTerrain::InfiniteTerrain(TextureManager* textureMgr, int seed, int chunkSize) : seed(seed), chunkSize(chunkSize){
	shader = new LitShader;
	shader->SETUP_SHADER_TANGENT(default_vs, terrain_fs);
	blockShader = new LitShader;
	blockShader->SETUP_SHADER_TANGENT(default_vs, ruinblock_fs);

	//load textures
	causticsTex = textureMgr->getTexture("caustics");
	rockTex = textureMgr->getTexture("rock");
	rockNormalsTex = textureMgr->getTexture("rockN");
	sandTex = textureMgr->getTexture("sand");
	sandNormalsTex = textureMgr->getTexture("sandN");
	ruinsTex = textureMgr->getTexture("ruin");
	ruinsNormalsTex = textureMgr->getTexture("ruinN");

	//initialize materials
	material = new Material;
	material->specularPower = 10.0f;
	material->specularColour = XMFLOAT3(0.1f, 0.1f, 0.1f);
	//material->emissive = XMFLOAT3(0.1f, 0.3f, 0.3f);

	//initialize a bunch of pre-generated blocks
	ruinBlockLibrary = new RuinBlockLibrary(25, seed);

#ifdef NO_INFINITY
	chunks.push_back(new TerrainMesh(seed, 0*chunkSize, 0*chunkSize, chunkSize + 1, ruinBlockLibrary));
#endif
	
}


InfiniteTerrain::~InfiniteTerrain(){
	for (auto it = chunks.begin(); it != chunks.end();) {
		delete *it;
		it = chunks.erase(it);
	}
	delete ruinBlockLibrary;
	delete shader;
	delete blockShader;
	delete material;
}

void InfiniteTerrain::update(XMFLOAT3 cameraPosition, ID3D11Device* device, ID3D11DeviceContext* deviceContext, float dt){
#ifndef NO_INFINITY

	//at all times, make sure we have all chunks adjacent to cameraPosition
	std::vector<XMINT2> requiredChunks;
	int X = floor((float)cameraPosition.x / chunkSize);
	int Z = floor((float)cameraPosition.z / chunkSize);
	//the four closest (the camera being in between their own centers) + their neighbours (if SMALL_AMOUNT_OF_CHUNKS is undefined)

#ifdef SMALL_AMOUNT_OF_CHUNKS
	requiredChunks.push_back(XMINT2(X, Z));
	requiredChunks.push_back(XMINT2(X+1, Z));
	requiredChunks.push_back(XMINT2(X+1, Z+1));
	requiredChunks.push_back(XMINT2(X, Z+1));
#else //the following will only be displayed if SMALL_AMOUNT_OF_CHUNKS is undefined, otherwise we only keep the closest 4 chunks.
	for (int xx = -1; xx <= 2; ++xx) {
		for (int yy = -1; yy <= 2; ++yy) {
			requiredChunks.push_back(XMINT2(X + xx, Z + yy));
		}
	}
#endif

	//delete any chunks we dont need
	for (auto it = chunks.begin(); it != chunks.end();) {
		XMINT2 coords = (*it)->getBaseCoords();
		coords.x /= chunkSize;
		coords.y /= chunkSize;
		//does requiredChunks contain the value?
		bool contains = false;
		for (auto c = requiredChunks.begin(); c != requiredChunks.end();) {
			if (c->x == coords.x && c->y == coords.y) {
				//ok, we have it!
				contains = true;
				requiredChunks.erase(c);//we can stop recording that one since we already have it, and dont want to recreate the mesh
				break;
			}
			else {
				c++;
			}
		}
		//so should we delete it?
		if (!contains && chunks.size() >= MAXIMUM_CHUNKS_AT_ONCE) {//only unload from RAM if we've reached the limit
			delete *it;
			it = chunks.erase(it);
		}
		else {
			it++;
		}
	}

	//add any chunks we do need
	for (XMINT2 required : requiredChunks) {
		chunks.push_back(new TerrainMesh(seed, required.x*chunkSize, required.y*chunkSize, chunkSize + 1, ruinBlockLibrary));
	}
#endif

	//update the terrains we currently have loaded in
	for (TerrainMesh* terrain : chunks) {
		const TerrainMesh* leftNeighbour = nullptr;
		const TerrainMesh* belowNeighbour = nullptr;
		const TerrainMesh* diagonalNeighbour = nullptr;
		const TerrainMesh* rightNeighbour = nullptr;
		const TerrainMesh* topNeighbour = nullptr;
		//figure out if we do have the neighbours
		for (TerrainMesh* neighbour : chunks) {
			if (terrain->getBaseCoords().x == neighbour->getBaseCoords().x && terrain->getBaseCoords().y == neighbour->getBaseCoords().y + chunkSize) {
				belowNeighbour = neighbour;
			}
			else if (terrain->getBaseCoords().x == neighbour->getBaseCoords().x + chunkSize && terrain->getBaseCoords().y == neighbour->getBaseCoords().y) {
				leftNeighbour = neighbour;
			}
			else if (terrain->getBaseCoords().x == neighbour->getBaseCoords().x + chunkSize && terrain->getBaseCoords().y == neighbour->getBaseCoords().y + chunkSize) {
				diagonalNeighbour = neighbour;
			}
			else if (terrain->getBaseCoords().x == neighbour->getBaseCoords().x - chunkSize && terrain->getBaseCoords().y == neighbour->getBaseCoords().y) {
				rightNeighbour = neighbour;
			}
			else if (terrain->getBaseCoords().x == neighbour->getBaseCoords().x && terrain->getBaseCoords().y == neighbour->getBaseCoords().y - chunkSize) {
				topNeighbour = neighbour;
			}
		}
		terrain->updateTerrain(leftNeighbour, belowNeighbour, diagonalNeighbour, topNeighbour, rightNeighbour, device, deviceContext, dt);
	}
	//go through those terrains we have again to check whether any of them need to reinit their buffers now
	for (TerrainMesh* terrain : chunks) {
		terrain->reinitBuffers(device, dt);
	}

}

void InfiniteTerrain::render(bool lighting, bool shadowing, D3D* renderer, XMMATRIX& worldMatrix, XMMATRIX& viewMatrix, XMMATRIX& projectionMatrix, XMFLOAT3 cameraPosition){
	for (TerrainMesh* chunk : chunks) {
		//render the terrain mesh
		chunk->sendData(renderer->getDeviceContext());
		shader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, cameraPosition);
		ExtendedLight* lights = chunk->getLight();
		ID3D11ShaderResourceView** shadowmaps = new ID3D11ShaderResourceView*[1]{ chunk->getShadowmap() };
		shader->setLightParameters(renderer->getDeviceContext(), cameraPosition, lighting? &lights : NULL, lighting && shadowing ? shadowmaps : NULL, lighting && shadowing, lighting?1:0/*only one light*/);
		delete[] shadowmaps;
		shader->setMaterialParameters(renderer->getDeviceContext(), sandTex, sandNormalsTex, NULL, material);
		shader->setTexture(chunk->getRuinMapView(renderer->getDevice(), renderer->getDeviceContext()), 16, renderer->getDeviceContext());
		shader->setTexture(causticsTex, 13, renderer->getDeviceContext());
		shader->setTexture(rockTex, 14, renderer->getDeviceContext());
		shader->setTexture(rockNormalsTex, 15, renderer->getDeviceContext());
		shader->render(renderer->getDeviceContext(), chunk->getIndexCount());

		//render the ruins as well once they're generated
		if (chunk->getRuins()) {
			blockShader->setMaterialParameters(renderer->getDeviceContext(), ruinsTex, ruinsNormalsTex, NULL, material);
			chunk->getRuins()->renderRuins(blockShader, material, renderer, XMMatrixTranslation(chunk->getBaseCoords().x - chunkSize / 2 + 0.5f, 0, chunk->getBaseCoords().y - chunkSize / 2 + 0.5f) * worldMatrix, viewMatrix, projectionMatrix, cameraPosition);
		}
	}

#ifdef CHECK_BLOCKS
	//render the blocks to check them
	for (int i = 0; i < ruinBlockLibrary->size(); ++i) {
		RuinBlockMesh* mesh = ruinBlockLibrary->grab(i);
		mesh->sendData(renderer->getDeviceContext());
		blockShader->setShaderParameters(renderer->getDeviceContext(), XMMatrixTranslation(i * 2, 20, 0) * worldMatrix, viewMatrix, projectionMatrix, cameraPosition);
		blockShader->setMaterialParameters(renderer->getDeviceContext(), ruinsTex, ruinsNormalsTex, NULL, material);
		blockShader->render(renderer->getDeviceContext(), mesh->getIndexCount());
	}
#endif
}

void InfiniteTerrain::depthPass(LitShader* depthShader, D3D* renderer, XMMATRIX& worldMatrix, XMMATRIX& viewMatrix, XMMATRIX& projectionMatrix, XMFLOAT3 cameraPosition, const TerrainMesh* specificChunk) {
	if (depthShader) {

		//a macro cos i dont want to create a real function or to copy paste code :P
#define DEPTHPASS(chunk) {chunk->sendData(renderer->getDeviceContext());\
							depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, cameraPosition);\
							depthShader->setLightParameters(renderer->getDeviceContext(), cameraPosition, NULL, NULL, false, 0);\
							depthShader->render(renderer->getDeviceContext(), chunk->getIndexCount());\
							\
							if (chunk->getRuins())\
								chunk->getRuins()->renderRuins(depthShader, material, renderer, XMMatrixTranslation(chunk->getBaseCoords().x - chunkSize / 2 + 0.5f, 0, chunk->getBaseCoords().y - chunkSize / 2 + 0.5f) * worldMatrix, viewMatrix, projectionMatrix, cameraPosition);}



		if (specificChunk) DEPTHPASS(specificChunk)
		else for (TerrainMesh* chunk : chunks) DEPTHPASS(chunk)


#undef DEPTHPASS
	}
}

void InfiniteTerrain::shadowmappingPass(LitShader* depthShader, D3D* renderer, XMMATRIX& worldMatrix) {
	for (TerrainMesh* chunk : chunks) {
		if (chunk->hasMeshChanged()) {//no need to recapture shadowmap when the mesh has stayed the exact same
			ExtendedLight* light = chunk->getLight();
			if (light->StartRecordingShadowmap()) {

				XMMATRIX lightViewMatrix = light->getView();
				XMMATRIX lightProjectionMatrix = light->getProjection();

				depthPass(depthShader, renderer, worldMatrix, lightViewMatrix, lightProjectionMatrix, light->getPosition(), chunk);
				if(chunk->getBelowNeighbour()) depthPass(depthShader, renderer, worldMatrix, lightViewMatrix, lightProjectionMatrix, light->getPosition(), chunk->getBelowNeighbour());
				if (chunk->getLeftNeighbour()) depthPass(depthShader, renderer, worldMatrix, lightViewMatrix, lightProjectionMatrix, light->getPosition(), chunk->getLeftNeighbour());
				if (chunk->getDiagonalNeighbour()) depthPass(depthShader, renderer, worldMatrix, lightViewMatrix, lightProjectionMatrix, light->getPosition(), chunk->getDiagonalNeighbour());

				light->StopRecordingShadowmap();
			}
		}
	}
}

void InfiniteTerrain::showShadowmaps(PPTextureShader* textureShader, D3D* renderer, XMMATRIX orthoViewMatrix) {
	
	int i = 0;
	for (TerrainMesh* chunk : chunks) {
		++i;
		ID3D11ShaderResourceView* map = chunk->getShadowmap();
		if (map != nullptr) {
#define MAP_SIZE 200.0f
			OrthoMesh ortho(GLOBALS.Device, GLOBALS.DeviceContext, MAP_SIZE, MAP_SIZE, -GLOBALS.ScreenWidth / 2 + MAP_SIZE / 2 + i*MAP_SIZE, -GLOBALS.ScreenHeight / 2 + MAP_SIZE / 2);
			ortho.sendData(GLOBALS.DeviceContext);
			textureShader->setShaderParameters(GLOBALS.DeviceContext, XMMatrixTranslation(0, 0, 0), orthoViewMatrix, renderer->getOrthoMatrix(), XMFLOAT3(0, 0, 0));
			textureShader->setTextureData(GLOBALS.DeviceContext, map);
			textureShader->render(GLOBALS.DeviceContext, ortho.getIndexCount());
#undef MAP_SIZE
		}
	}
}

XMFLOAT3 InfiniteTerrain::handleCamera(XMFLOAT3 cameraPosition) {

	float camX = cameraPosition.x + chunkSize*0.5f;
	float camZ = cameraPosition.z + chunkSize*0.5f;

	//find the current chunk
	int X = floor((float)(camX) / chunkSize);
	int Z = floor((float)(camZ) / chunkSize);

	for (auto* chunk : chunks) {
		if (chunk->getBaseCoords().x == X*chunkSize && chunk->getBaseCoords().y == Z*chunkSize) {
			float x = camX - (X*chunkSize);
			float y = camZ - (Z*chunkSize);

			//find average height at this position
			int intX = int(x);
			int intY = int(y);

			//Turns out that not even lerping but using a bigger window (4*4 samples) gives better results as far as smooth movement goes:)
			float height = 0;
			for (int dX = 0; dX < 4; ++dX) {
				for (int dY = 0; dY < 4; ++dY) {
					int sampleX = intX + dX - 1;
					int sampleY = intY + dY - 1;
					height += chunk->getRealHeight(sampleX, sampleY);
				}
			}
			height /= 16.0f;

			cameraPosition.y = height + 3.f;
		}
	}

	return cameraPosition;
}
