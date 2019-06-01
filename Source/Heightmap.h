#pragma once

/** Represents a heightmap that can belong to a TerrainMesh. Typically the heightmap extends beyond the terrain mesh by 120% on positive X and Z to allow smooth blending with neighbouring meshes 
    A heightmap created from the same seed and size will always look the exact same to allow recreation of deleted heightmaps
*/

#include "DXF.h"
#include <random>
#include "PerlinNoise.h"
#include <experimental/coroutine>
#include "FileSystem.h"
#include "FileReader.h"
#include "FileWriter.h"

//#define TIME_HEIGHTMAP_GENERATION // if defined, will time how long it takes to generate the heightmap and write it to a csv file

#ifdef TIME_HEIGHTMAP_GENERATION
#include <chrono>
#endif

class Heightmap {

public:
	Heightmap(int seed, int size);
	~Heightmap();

	float getHeight(int x, int y) const;
	float getSize() const;

	void generate();
	void update();
	inline bool hasChanged() const { return changed; }

protected:
	int seed;
	int size;
	bool changed = true;//whether the heightmap was changed over the last call to update()

	float** heightmap = nullptr;

	std::default_random_engine randomEngine;

	float randomFloat(float max = 1, float min = 0);
	int randomInt(int max, int min = 0);
	float gauss(float distFromCenter, float standardDeviation);
	void voronoiFaulting(int numPoints, float heightRange);
	void perlinNoise(float scale, float heightRange);
	void smoothe(float amount);

	//async generation using coroutines
	struct HeightEnumerator {
		struct promise_type {
			HeightEnumerator* next = NULL;
			inline promise_type(){}
			inline ~promise_type(){}
			inline auto initial_suspend(){ return std::experimental::suspend_always{}; }//suspend initially
			inline auto final_suspend() { return std::experimental::suspend_always{}; }//suspend once done to be able to use Continue()
			//inline auto return_void() { return std::experimental::suspend_never{}; }//never suspend on return (will suspend after)
			inline auto return_value(HeightEnumerator* val) { next = val; return std::experimental::suspend_never{}; }//record the next operation (can be NULL, in which case we're done)
			inline auto yield_value(int v) { return std::experimental::suspend_always{}; }//always suspend on yield (otherwise whats the point of yielding)
			inline auto get_return_object() { return HeightEnumerator{ handle::from_promise(*this) }; }
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
		inline HeightEnumerator* NextOperation() { return coroutineHandle.promise().next; }

		inline ~HeightEnumerator() { if (coroutineHandle) coroutineHandle.destroy(); }

		//make enumerator movable but not copyable
		inline HeightEnumerator(const HeightEnumerator &) = delete;
		inline HeightEnumerator(HeightEnumerator &&g) : coroutineHandle(g.coroutineHandle) { g.coroutineHandle = nullptr; }
		inline HeightEnumerator &operator = (const HeightEnumerator &) = delete;
		inline HeightEnumerator &operator = (HeightEnumerator &&h) {
			coroutineHandle = h.coroutineHandle;
			h.coroutineHandle = nullptr;
			return *this;
		}
	private:
		inline HeightEnumerator(handle h) : coroutineHandle(h) {}
		handle coroutineHandle = nullptr;
		bool finished = false;
	};

	HeightEnumerator asyncVoronoiFaulting(int numPoints, float heightRange);
	HeightEnumerator asyncSmoothe(float amount);
	HeightEnumerator asyncPerlinNoise(float scale, float heightRange);

	//the operation we're currently applying to generate the heightmap, or nullptr if we're done
	HeightEnumerator* currentOperation;

	//i/o operations
	bool read();
	void write();

#ifdef TIME_HEIGHTMAP_GENERATION
	std::chrono::high_resolution_clock timingClock;
	std::deque<float> times;//the times it takes, one for each frame
#endif

};