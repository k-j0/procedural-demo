#include "FileWriter.h"

#include <fstream>
#include "Utils.h"
#include <vector>

FileWriter::FileWriter(std::string name) : name(name) {
}

FileWriter::~FileWriter() {
	//create directory if needed
	std::vector<std::string> splitName;
	Utils::split(name, '/', &splitName);
	std::string folder = "";
	for (int i = 0; i < splitName.size() - 1; ++i) folder += splitName[i] + "/";
	if (folder.size() > 0) {
		CreateDirectory(folder.c_str(), NULL);
	}
	//write contents to disk.
	ofstream f(name, std::ios::out | std::ios::trunc | std::ios::binary);
	if (f.is_open()) {
		for (byte& b : data) {
			f << b;
		}
		f.close();
	} else {
		printf("File %s could not be opened for writing.\n", name.c_str());
	}
}
