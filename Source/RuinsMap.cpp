#include "RuinsMap.h"

#include "Shader.h"
#include <chrono>

RuinsMap::RuinsMap(int seed, int size, std::function<float(int, int)> slopeFunction, std::function<float(int, int)> heightFunction, RuinBlockLibrary* blockLibrary) : 
			seed(seed), size(size), slopeFunction(slopeFunction), heightFunction(heightFunction), blockLibrary(blockLibrary) {
	
	generate();
}

RuinsMap::~RuinsMap() {
	release();
}

void RuinsMap::placeKernel(int baseX, int baseY) {
	int kernelSize = randomEngine() % 8 + 3;//room size, within 3..10

	//place the kernel as a mask
	for (int y = 0; y < kernelSize; ++y) {
		for (int x = 0; x < kernelSize; ++x) {
			int trueX = x + baseX - kernelSize;
			int trueY = y + baseY - kernelSize;
			if (trueX >= 0 && trueX < size && trueY >= 0 && trueY < size) {
				map[trueY][trueX] = x == 0 || x == kernelSize-1 || y == 0 || y == kernelSize-1;//only the walls, filling in the rest with emptiness
			}
		}
	}
}


void RuinsMap::generate() {

	release();//start over in case we need to

	randomEngine = std::default_random_engine(seed + 1);//always use a slightly different seed than whoever called us to get different values (but always the same with the same seed)
	map = new bool*[size];
	for (int y = 0; y < size; ++y) {
		map[y] = new bool[size];
		for (int x = 0; x < size; ++x) {
			map[y][x] = false;
		}
	}

	currentOperation = new RuinsEnumerator(asyncGeneration());//start the async generation

	hasChanged = true;
}

bool RuinsMap::update(bool redo) {

	if (redo) {//start over..
		generate();
	}

	if (currentOperation) {//we're currently working on something
		hasChanged = true;
		if (!currentOperation->IsFinished()) {
			currentOperation->Continue();//keep going
		}
		else {//move on to the next operation
			RuinsEnumerator* next = currentOperation->NextOperation();
			delete currentOperation;
			currentOperation = next;//note that in this case we're supporting moving on to the next but since in practice we don't use this, no need to bloat the code up with the kind of things we do in Heightmap::update() :)
		}
	}
	else {//no more operations
		hasChanged = false;
	}

	return hasChanged;
}

//using https://docs.microsoft.com/en-us/windows/desktop/direct3d11/overviews-direct3d-11-resources-textures-create
ID3D11ShaderResourceView* RuinsMap::asTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext) {

	if (!hasChanged && debugView) return debugView;

	///Fill in the texture's pixel data

	uint32_t* pixels = new uint32_t[size*size];
	for (int index = 0; index < size*size; ++index) {
		int x = index % size;
		int y = int(index / size);
		if (map[y][x]) {
			pixels[index] = 0xffffffff;
		}
		else {
			pixels[index] = 0xff000000;
		}
	}
	
	D3D11_SUBRESOURCE_DATA initData = { pixels, size*sizeof(uint32_t), 0 };

	///Fill in the texture description

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = desc.Height = size;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	if (debugTexture) {
		debugTexture->Release();
	}
	debugTexture = NULL;

	///Create the texture:

	HRESULT result = device->CreateTexture2D(&desc, &initData, &debugTexture);
	delete[] pixels;//release that
	if(result != S_OK){
		printf("Error creating ruins debug texture (CreateTexture2D): ");
		Shader::printError(result);
		return NULL;
	}

	if (debugView) {
		debugView->Release();
	}
	debugView = NULL;

	///Use the created texture to generate the shader resource view

	result = device->CreateShaderResourceView(debugTexture, NULL, &debugView);
	if (result != S_OK) {
		printf("Error creating ruins debug texture (CreateShaderResourceView): ");
		Shader::printError(result);
		return NULL;
	}

	return debugView;
}

void RuinsMap::renderRuins(LitShader* shader, Material* material, D3D* renderer, XMMATRIX& worldMatrix, XMMATRIX& viewMatrix, XMMATRIX& projectionMatrix, XMFLOAT3 cameraPosition) const {
	for (RuinsBlock* block : blocks) {
		block->render(shader, material, renderer, worldMatrix, viewMatrix, projectionMatrix, cameraPosition);
	}
}

void RuinsMap::release(){
	if (map) {
		for (int y = 0; y < size; ++y) {
			delete[] map[y];
		}
		delete[] map;
		map = NULL;
	}

	if (debugTexture) {
		debugTexture->Release();
		debugTexture = NULL;
	}

	if (debugView) {
		debugView->Release();
		debugView->Release();
	}

	//release all the blocks
	for (auto it = blocks.begin(); it != blocks.end();) {
		delete (*it);
		it = blocks.erase(it);
	}

	if (currentOperation) delete currentOperation;
	currentOperation = NULL;
}



//coroutine utility macros
#define MAX_TIME_IN_COROUTINE 4
#define INITIAL_CHECKPOINT std::chrono::high_resolution_clock clock; auto time = clock.now();
#define CHECKPOINT {auto now = clock.now(); int millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - time).count(); if(millis >= MAX_TIME_IN_COROUTINE){ co_yield NULL; time = clock.now(); }}

RuinsMap::RuinsEnumerator RuinsMap::asyncGeneration() {

	INITIAL_CHECKPOINT

	//random blind agent-based generation
	XMINT2 rob(randomEngine() % size, randomEngine() % size);
	uint8_t dir = randomEngine() % 4;//0 left - 1 up - 2 right - 3 down
	int covered = 0;
	while (covered < size * size * 0.2f) {//keep walking until a good portion (~20%) is covered
		++covered;
		//place a wall
		map[rob.y][rob.x] = true;
		//potentially place a premade room kernel
		if (randomEngine() % 30 == 0) {
			placeKernel(rob.x, rob.y);
		}
		dir = randomEngine() % 20 == 0 ? randomEngine() % 4 : dir;//each step, 1 in 20 chances of changing direction
																  //step forward
		rob.x += dir == 0 ? -1 : dir == 2 ? +1 : 0;
		rob.y += dir == 1 ? +1 : dir == 3 ? -1 : 0;
		//check that we haven't stepped outside the bounds, and if so go back in and change direction
		if (rob.x < 0 || rob.x >= size) {
			rob.x = rob.x < 0 ? 0 : size - 1;
			dir = randomEngine() % 2 == 0 ? 1 : 3;//go up or down now
		}
		if (rob.y < 0 || rob.y >= size) {
			rob.y = rob.y < 0 ? 0 : size - 1;
			dir = randomEngine() % 2 == 0 ? 0 : 2;//go left or right now
		}

		CHECKPOINT
	}

	//clean out the slopes
	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			float slope = slopeFunction(x, y) + slopeFunction(x + 1, y) + slopeFunction(x + 1, y + 1) + slopeFunction(x, y + 1);
			if (map[y][x] && slope > 0.15f) {//too big a slope here. clean out any ruins from here.
				map[y][x] = false;
			}
		}
		CHECKPOINT
	}

	//generate the blocks' meshes we need
	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			if (map[y][x]) {//compute position and rotation of the block:
				float heightTopLeft = heightFunction(x, y + 1);
				float heightTopRight = heightFunction(x + 1, y + 1);
				float heightBottomLeft = heightFunction(x, y);
				float heightBottomRight = heightFunction(x + 1, y);
				float height = (heightTopLeft + heightBottomLeft + heightBottomRight + heightTopRight) * 0.25f;//the height of the block is just the avg
																											   //compute pitch and roll from the adjacent heights:
				float heightLeft = (heightTopLeft + heightBottomLeft) * 0.5f;
				float heightRight = (heightTopRight + heightBottomRight) * 0.5f;
				float rightVector = heightRight - heightLeft;//a vector from the left part to the right part of the underlying terrain right underneath the block (only the Y component, since X is 1 always)
				float roll = atan2(rightVector, 1);
				float heightTop = (heightTopLeft + heightTopRight) * 0.5f;
				float heightBottom = (heightBottomLeft + heightBottomRight) * 0.5f;
				float forwardVector = heightTop - heightBottom;//a vector from the back part to the front part of the underlying terrain right underneath the block. again, only y component, as X=1.
				float pitch = -atan2(forwardVector, 1);
				float yaw = float(randomEngine() % 1000) / 1000.0f * 2 * 3.1415f;//random yaw between 0..360
				blocks.push_back(new RuinsBlock(XMFLOAT3(x, height, y), XMFLOAT3(pitch, yaw, roll), blockLibrary->grab(randomEngine())));//push back one of the pre-generated meshes, with the given position and rotation

				CHECKPOINT
			}
		}
	}

	co_return NULL;
}
#undef MAX_TIME_IN_COUROUTINE
#undef INITIAL_CHECKPOINT
#undef CHECKPOINT