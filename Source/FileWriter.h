#pragma once

#include <string>
#include "FileSystem.h"

class FileWriter {
	std::string name;//filename to write to
public:
	FileWriter(std::string name);
	~FileWriter();

	//overloading this operator allows to do
	//		FileSystem::r_uint8(writer(), 0);
	//rather than
	//		FileSystem::r_uint8(&writer.data, 0);
	inline std::deque<byte>* operator() () { return &data; }

	std::deque<byte> data;//data to write out to the file (left public cos as this allows handling code to fill it in however needed)

};