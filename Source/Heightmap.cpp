#include "Heightmap.h"

#include <chrono>

//#define QUICKGEN //define this to generate a quick, bad heightmap
#define ASYNC //if defined, the heightmaps will be generated asynchronously

#ifdef TIME_HEIGHTMAP_GENERATION
#include <chrono>
#endif

Heightmap::Heightmap(int seed, int size) : seed(seed), size(size) {

	//init random engine using seed; this way however we get to this point, we'll always generate the same sequence of numbers which in turn will result in the exact same data being generated.
	randomEngine = std::default_random_engine(seed);

	//initialize heights to 0
	heightmap = new float*[size];
	for (int y = 0; y < size; ++y) {
		heightmap[y] = new float[size];
		for (int x = 0; x < size; ++x) {
			heightmap[y][x] = 0;
		}
	}

}

Heightmap::~Heightmap() {

	if (heightmap) {
		for (int y = 0; y < size; ++y) {
			delete[] heightmap[y];
			heightmap[y] = nullptr;
		}
		delete[] heightmap;
		heightmap = nullptr;
	}

	if (currentOperation) delete currentOperation;

}

float Heightmap::getHeight(int x, int y) const {
	if (x < 0 || y < 0 || x >= size || y >= size) {
		printf("Error: cannot access coordinates (%d, %d) on a heightmap of size %d.", x, y, size);
		return -1;
	}
	return heightmap[y][x];
}

float Heightmap::getSize() const {
	return size;
}

void Heightmap::generate() {


	//have we got it saved already? - in which case, no need to re-generate it at all!
#if !defined( TIME_HEIGHTMAP_GENERATION )
	if (read()) return;
#else
	//run it 1000 times to time it
	times.clear();
	for (int i = 0; i < 100; ++i) {
		std::chrono::high_resolution_clock::time_point before = timingClock.now();
		read();
		std::chrono::high_resolution_clock::duration duration = timingClock.now() - before;
		times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
	}
	//write it out to file
	CreateDirectory("TimingResults", LPSECURITY_ATTRIBUTES());
	std::ofstream stream("TimingResults/loadTimings-" + std::to_string(seed) + ".csv");
	stream << "Times to generate heightmap,(microseconds)\n";
	stream << ",,Min:,\"=QUARTILE(A3:A" + std::to_string(times.size() + 3) + ", 0)\",";
	stream << "First Quartile:,\"=QUARTILE(A3:A" + std::to_string(times.size() + 3) + ", 1)\",";
	stream << "Median:,\"=QUARTILE(A3:A" + std::to_string(times.size() + 3) + ", 2)\",";
	stream << "Third Quartile:,\"=QUARTILE(A3:A" + std::to_string(times.size() + 3) + ", 3)\",";
	stream << "Max:,\"=QUARTILE(A3:A" + std::to_string(times.size() + 3) + ", 4)\"\n";
	for (int i = 0; i < times.size(); ++i) {
		stream << std::to_string(times[i]) << "\n";
	}
	stream.close();
	printf("Wrote %d timing results to %s\n", times.size(), std::string("TimingResults/loadTimings-" + std::to_string(seed) + ".csv").c_str());
	times.clear();
	if (read()) return;//default behaviour
#endif

#ifdef ASYNC
	currentOperation = new HeightEnumerator(asyncVoronoiFaulting(100.f/16.f, 16));
#else
	//fractal voronoi
	for (float amount = 16; amount > 1; amount *= 0.3f) {
		voronoiFaulting(100 / amount, amount);
	}

	smoothe(1.5f);

	//fractal perlin noise
	for (float amount = 5; amount > 0.3f; amount *= 0.5f) {
		perlinNoise(0.5f / amount, amount);
	}
#endif

}

void Heightmap::update() {
	//continue generating heightmap
	changed = false;
	if (currentOperation) {//we're currently working on something
		changed = true;
#ifdef TIME_HEIGHTMAP_GENERATION
		std::chrono::high_resolution_clock::time_point before = timingClock.now();
#endif
		if (!currentOperation->IsFinished()) {
			currentOperation->Continue();//keep going
		}
		else {//move on to the next operation
			HeightEnumerator* next = currentOperation->NextOperation();
			delete currentOperation;
			currentOperation = next;
			if (currentOperation) {
				currentOperation->Continue();//start this new operation immediately, no need to wait
			}
		}
#ifdef TIME_HEIGHTMAP_GENERATION
		std::chrono::high_resolution_clock::duration duration = timingClock.now() - before;
		times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());//add the time as microseconds
#endif
	}
	else {//no more operations
		changed = false;
#ifdef TIME_HEIGHTMAP_GENERATION //output the results to a file
		if (times.size() > 10) {
			CreateDirectory("TimingResults", LPSECURITY_ATTRIBUTES());
			std::ofstream stream("TimingResults/timings-" + std::to_string(seed) + ".csv");
			stream << "Times to generate heightmap,(microseconds)\n";
			stream << ",,Min:,\"=QUARTILE(A3:A" + std::to_string(times.size() + 3) + ", 0)\",";
			stream << "First Quartile:,\"=QUARTILE(A3:A" + std::to_string(times.size() + 3) + ", 1)\",";
			stream << "Median:,\"=QUARTILE(A3:A" + std::to_string(times.size() + 3) + ", 2)\",";
			stream << "Third Quartile:,\"=QUARTILE(A3:A" + std::to_string(times.size() + 3) + ", 3)\",";
			stream << "Max:,\"=QUARTILE(A3:A" + std::to_string(times.size() + 3) + ", 4)\"\n";
			for (int i = 0; i < times.size(); ++i) {
				stream << std::to_string(times[i]) << "\n";
			}
			stream.close();
			printf("Wrote %d timing results to %s\n", times.size(), std::string("TimingResults/timings-" + std::to_string(seed) + ".csv").c_str());
		}
		times.clear();
#endif
	}
}

float Heightmap::randomFloat(float max, float min) {
	if (max < min) std::swap(max, min);//this lets us call it using randomFloat(min, max) instead when using 2 args

	float r = randomInt(10000);//using 10000 means we'll only get up to 4 decimal places but i'd argue thats pretty good already
	r /= 10000;

	return min + r*(max - min);

}

int Heightmap::randomInt(int max, int min) {
	if (max < min) std::swap(max, min);//this lets us call it using randomInt(min, max) instead when using 2 args

	return min + (randomEngine() % (max - min));

}

float Heightmap::gauss(float distFromCenterSqr, float standardDeviation) {
	return exp(-distFromCenterSqr / (2 * standardDeviation*standardDeviation));
}

void Heightmap::voronoiFaulting(int numPoints, float heightRange) {

	XMFLOAT3* points = new XMFLOAT3[numPoints];//z coord will be the amount we fault
	for (int i = 0; i < numPoints; ++i) {
		points[i].x = randomFloat(size);
		points[i].y = randomFloat(size);
		points[i].z = randomFloat(-heightRange / 2, heightRange / 2);
	}

	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			//find the closest point to (x, y)
			float minDistanceSqr = INFINITY;
			int closestPoint = 0;
			for (int i = 0; i < numPoints; ++i) {
				float vectX = float(x) - points[i].x;
				float vectY = float(y) - points[i].y;
				float distSqr = vectX*vectX + vectY*vectY;
				if (distSqr < minDistanceSqr) {//this point is closer than the last one
					minDistanceSqr = distSqr;
					closestPoint = i;
				}
			}
			//fault using closest point
			heightmap[y][x] += points[closestPoint].z;

			//find the second closest point
			minDistanceSqr = INFINITY;
			int previousClosest = closestPoint;
			closestPoint = 0;
			for (int i = 0; i < numPoints; ++i) {
				if (i != previousClosest) {
					float vectX = float(x) - points[i].x;
					float vectY = float(y) - points[i].y;
					float distSqr = vectX*vectX + vectY*vectY;
					if (distSqr < minDistanceSqr) {//this point is closer than the last one
						minDistanceSqr = distSqr;
						closestPoint = i;
					}
				}
			}
			//fault using closest point
			heightmap[y][x] += points[closestPoint].z;
		}
	}

	delete[] points;
}

void Heightmap::smoothe(float amount) {

	//the original values will be stored in the copy of the heightmap
	float** heightmapCopy = new float*[size];
	for (int y = 0; y < size; ++y) {
		heightmapCopy[y] = new float[size];
		for (int x = 0; x < size; ++x) {
			heightmapCopy[y][x] = heightmap[y][x];
		}
	}

	int neighbours = (int)(3 * amount) + 1;//can easily prove that at a distance > 3*standardDeviation, gaussian function (a=1,b=0) evaluates to less than exp(-4.5) (=0.01111)

	//read the values from heightmapCopy and use them to smoothe out heightmap
	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {

			float sum = 0;
			float totalWeight = 0;

			//sample neighbouring cells
			for (int ny = y - neighbours / 2; ny <= y + neighbours / 2; ++ny) {
				for (int nx = x - neighbours / 2; nx <= x + neighbours / 2; ++nx) {
					if (ny >= 0 && ny < size && nx >= 0 && nx < size) {
						float distSqr = (nx - x)*(nx - x) + (ny - y)*(ny - y);
						float weight = gauss(distSqr, amount);
						sum += heightmapCopy[ny][nx] * weight;
						totalWeight += weight;
					}
				}
			}

			//use the averaged value as the new height
			if (totalWeight != 0) {
				heightmap[y][x] = sum / totalWeight;
			}
			else heightmap[y][x] = 0;

		}
	}

	//free memory
	for (int y = 0; y < size; ++y)
		delete[] heightmapCopy[y];
	delete[] heightmapCopy;
}

void Heightmap::perlinNoise(float scale, float heightRange) {
	PerlinNoise noise(randomEngine());
	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			heightmap[y][x] += noise.noise(scale*x, scale*y, 0) * heightRange - heightRange / 2;
		}
	}
}




//some coroutine utility macros
#define MAX_TIME_IN_COROUTINE 4
#define INITIAL_CHECKPOINT std::chrono::high_resolution_clock clock; auto time = clock.now();
#define CHECKPOINT {auto now = clock.now(); int millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - time).count(); if(millis >= MAX_TIME_IN_COROUTINE){ co_yield NULL; time = clock.now(); }}

Heightmap::HeightEnumerator Heightmap::asyncVoronoiFaulting(int numPoints, float heightRange){
	
	INITIAL_CHECKPOINT

	std::vector<XMFLOAT3> points;//z coord will be the amount we fault - cant use an array anymore, cos a vector will automatically release its data even if the coroutine never ends
	for (int i = 0; i < numPoints; ++i) {
		points.push_back(XMFLOAT3(
			randomFloat(size),
			randomFloat(size),
			randomFloat(-heightRange / 2, heightRange / 2)
		));
	}

	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			//find the closest point to (x, y)
			float minDistanceSqr = INFINITY;
			int closestPoint = 0;
			for (int i = 0; i < numPoints; ++i) {
				float vectX = float(x) - points[i].x;
				float vectY = float(y) - points[i].y;
				float distSqr = vectX*vectX + vectY*vectY;
				if (distSqr < minDistanceSqr) {//this point is closer than the last one
					minDistanceSqr = distSqr;
					closestPoint = i;
				}
			}
			//fault using closest point
			heightmap[y][x] += points[closestPoint].z;

			//find the second closest point
			minDistanceSqr = INFINITY;
			int previousClosest = closestPoint;
			closestPoint = 0;
			for (int i = 0; i < numPoints; ++i) {
				if (i != previousClosest) {
					float vectX = float(x) - points[i].x;
					float vectY = float(y) - points[i].y;
					float distSqr = vectX*vectX + vectY*vectY;
					if (distSqr < minDistanceSqr) {//this point is closer than the last one
						minDistanceSqr = distSqr;
						closestPoint = i;
					}
				}
			}
			//fault using closest point
			heightmap[y][x] += points[closestPoint].z;
		}
		//if(y%6 == 0) co_yield NULL;
		CHECKPOINT
	}

#ifdef QUICKGEN //in quickgen mode, only the very first step counts.
	co_return NULL;
#else
	//prepare the next step voronoi faulting if needed
	if (heightRange > 5) {
		heightRange *= 0.3f;
		co_return new HeightEnumerator(asyncVoronoiFaulting(100.f / heightRange, heightRange));
	}
	else {
		co_return new HeightEnumerator(asyncSmoothe(1.5f));//move on to next step - smoothe out the resulting heights
	}
#endif
}

Heightmap::HeightEnumerator Heightmap::asyncSmoothe(float amount) {

	INITIAL_CHECKPOINT

	//the original values will be stored in the copy of the heightmap
	std::vector<std::vector<float>> heightmapCopy;
	for (int y = 0; y < size; ++y) {
		heightmapCopy.push_back(std::vector<float>());
		for (int x = 0; x < size; ++x) {
			heightmapCopy[y].push_back(heightmap[y][x]);
		}
	}

	int neighbours = (int)(3 * amount);//can easily prove that at a distance > 3*standardDeviation, gaussian function (a=1,b=0) evaluates to less than exp(-4.5) (=0.01111)

	//read the values from heightmapCopy and use them to smoothe out heightmap
	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {

			float sum = 0;
			float totalWeight = 0;

			//sample neighbouring cells
			for (int ny = y - neighbours / 2; ny <= y + neighbours / 2; ++ny) {
				for (int nx = x - neighbours / 2; nx <= x + neighbours / 2; ++nx) {
					if (ny >= 0 && ny < size && nx >= 0 && nx < size) {
						float distSqr = (nx - x)*(nx - x) + (ny - y)*(ny - y);
						float weight = gauss(distSqr, amount);
						sum += heightmapCopy[ny][nx] * weight;
						totalWeight += weight;
					}
				}
			}

			//use the averaged value as the new height
			if (totalWeight != 0) {
				heightmap[y][x] = sum / totalWeight;
			}
			else heightmap[y][x] = 0;

		}
		//if(y % 10 == 0) co_yield NULL;
		CHECKPOINT
	}

	co_return new HeightEnumerator(asyncPerlinNoise(0.25f/1.25f, 0.7f));//next step - a few passes of fractal perlin noise

}

Heightmap::HeightEnumerator Heightmap::asyncPerlinNoise(float scale, float heightRange) {

	INITIAL_CHECKPOINT

		PerlinNoise noise(randomEngine());
	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			heightmap[y][x] += noise.noise(scale*x, scale*y, 0) * heightRange - heightRange / 2;
		}
		//if (y % 20 == 0) co_yield NULL;
		CHECKPOINT
	}

	if (heightRange > 0.3f) {//keep going in fractal pattern
		heightRange *= 0.5f;
		co_return new HeightEnumerator(asyncPerlinNoise(0.25f / heightRange, heightRange));
	}
	else {
		//done! save it for later.
		write();
		co_return NULL;
	}
}

#undef MAX_TIME_IN_COUROUTINE
#undef INITIAL_CHECKPOINT
#undef CHECKPOINT

void Heightmap::write() {
	if (!FileSystem::fileExists("saved/heightmap-" + std::to_string(seed))) {

		FileWriter w("saved/heightmap-" + std::to_string(seed));
		for (int y = 0; y < size; ++y) {
			for (int x = 0; x < size; ++x) {
				FileSystem::w_float(w(), heightmap[y][x]);
			}
		}

	}
}

bool Heightmap::read() {
	if (FileSystem::fileExists("saved/heightmap-" + std::to_string(seed))) {

		FileReader r("saved/heightmap-" + std::to_string(seed));
		for (int y = 0; y < size; ++y) {
			for (int x = 0; x < size; ++x) {
				heightmap[y][x] = FileSystem::r_float(r());
			}
		}

		changed = true;

		return true;
	}
	else return false;
}
