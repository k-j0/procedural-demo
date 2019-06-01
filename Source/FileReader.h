#pragma once

#include <string>
#include "FileSystem.h"

class FileReader {
public:
	FileReader(std::string name);

	//overloading these two operators allow to do:
	//		while(reader--) byte b = FileSystem::r_uint8(reader());
	//rather than
	//		while(reader.data->size() > 0) byte b = FileSystem::r_uint8(&reader.data));
	inline std::deque<byte>* operator() () { return &data; }
	inline bool operator--(int) { return data.size() > 0; }

	std::deque<byte> data;//left public to be able to read directly from handling code.
};