#include "FileReader.h"

#include <fstream>

FileReader::FileReader(std::string name) {
	//read data into the data deque
	ifstream f(name, std::ios::in | std::ios::binary | std::ios::ate);
	if (f.is_open()) {
		int size = f.tellg();
		f.seekg(0);
		for (int i = 0; i < size; ++i) {
			char b;
			f.read(&b, 1);
			data.push_back(b);
		}
		f.close();
	} else {
		printf("File %s could not be opened for reading.\n", name.c_str());
	}
}
