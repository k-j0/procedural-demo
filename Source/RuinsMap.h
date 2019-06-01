#pragma once

///basically a wrapper for a 2d array of bools to generate walls on a terrain mesh

#include <random>
#include "DXF.h"
#include <functional>
#include "RuinBlockMesh.h"
#include "RuinsBlock.h"
#include <experimental/coroutine>

class RuinsMap {

public:
	///Creates and generates a Ruins map. Seed is whatever seed needed for the specific map (in practice, the same as the parent terrainmesh's seed), size is the size of the map, and the slope function is a lambda supposed to return a 0..1 value for slope of the underlying heightmap
	RuinsMap(int seed, int size, std::function<float(int, int)> slopeFunction, std::function<float(int, int)> heightFunction, RuinBlockLibrary* blockLibrary);
	~RuinsMap();

	///create texture as debug view for the map (white pixel for true, black for false)
	ID3D11ShaderResourceView* asTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext);

	///Render all the ruin blocks on the map
	void renderRuins(LitShader* shader, Material* material, D3D* renderer, XMMATRIX& worldMatrix, XMMATRIX& viewMatrix, XMMATRIX& projectionMatrix, XMFLOAT3 cameraPosition) const;

	//updates the ruins. returns true if something has changed visually.
	bool update(bool redo);//if redo is true, we'll restart generating from the top (only pass true if one of the neighbouring chunks has been discovered)

protected:
	int seed;
	int size;
	std::function<float(int, int)> slopeFunction;//returns the slope on 0..1 of the underlying heightmap.
	std::function<float(int, int)> heightFunction;//returns the height in world units of the underlying heightmap.

	bool** map;//they all start at true and get progressively erased out to form holes in the walls

	void generate();
	void placeKernel(int x, int y);//place a room kernel onto the map at the determined location

	bool hasChanged = false;//set to false each time the map changes.

	std::default_random_engine randomEngine;

	//a texture containing the map as White-Black pixels, for passing to terrain shader
	ID3D11Texture2D* debugTexture = nullptr;
	ID3D11ShaderResourceView* debugView = nullptr;

	//the bricks and columns that should be displayed on the terrain
	std::vector<RuinsBlock*> blocks;

	RuinBlockLibrary* blockLibrary;


	//async generation using coroutines
	struct RuinsEnumerator {
		struct promise_type {
			RuinsEnumerator* next = NULL;
			inline promise_type() {}
			inline ~promise_type() {}
			inline auto initial_suspend() { return std::experimental::suspend_always{}; }//suspend initially
			inline auto final_suspend() { return std::experimental::suspend_always{}; }//suspend once done to be able to use Continue()
																					   //inline auto return_void() { return std::experimental::suspend_never{}; }//never suspend on return (will suspend after)
			inline auto return_value(RuinsEnumerator* val) { next = val; return std::experimental::suspend_never{}; }//record the next operation (can be NULL, in which case we're done)
			inline auto yield_value(int v) { return std::experimental::suspend_always{}; }//always suspend on yield (otherwise whats the point of yielding)
			inline auto get_return_object() { return RuinsEnumerator{ handle::from_promise(*this) }; }
			inline void unhandled_exception() { printf("An exception occured...\n"); std::exit(100); }
		};
		using handle = std::experimental::coroutine_handle<promise_type>;

		/// Continues execution of the current coroutine
		inline bool Continue() {
			coroutineHandle.resume();
			finished = coroutineHandle.done();
			return finished;
		}

		/// Returns whether the current coroutine is finished or not
		inline bool IsFinished() { return finished; }

		/// Returns the next operation that should be executed after this coroutine is over
		inline RuinsEnumerator* NextOperation() { return coroutineHandle.promise().next; }

		inline ~RuinsEnumerator() { if (coroutineHandle) coroutineHandle.destroy(); }

		//make enumerator movable but not copyable
		inline RuinsEnumerator(const RuinsEnumerator &) = delete;
		inline RuinsEnumerator(RuinsEnumerator &&g) : coroutineHandle(g.coroutineHandle) { g.coroutineHandle = nullptr; }
		inline RuinsEnumerator &operator = (const RuinsEnumerator &) = delete;
		inline RuinsEnumerator &operator = (RuinsEnumerator &&h) {
			coroutineHandle = h.coroutineHandle;
			h.coroutineHandle = nullptr;
			return *this;
		}
	private:
		inline RuinsEnumerator(handle h) : coroutineHandle(h) {}
		handle coroutineHandle = nullptr;
		bool finished = false;
	};

	RuinsEnumerator asyncGeneration();

	RuinsEnumerator* currentOperation = NULL;

	void release();//releases all resources used for this map

};