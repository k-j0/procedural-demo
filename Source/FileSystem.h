#pragma once

#include <deque>
#include <string>
#include <sys/stat.h>

#include "DXF.h"

typedef uint8_t byte;

class FileSystem {
public:

	//Writing utilities

	static inline void w_uint8(std::deque<byte>* bytes, uint8_t val) {
		bytes->push_back(val);
	}

	static inline void w_sint8(std::deque<byte>* bytes, int8_t val) {
		union {//this will even work on non-2s complement
			uint8_t u;
			int8_t s;
		} u;
		u.s = val;
		w_uint8(bytes, u.u);
	}

	static inline void w_uint16(std::deque<byte>* bytes, uint16_t val) {
		w_uint8(bytes, uint8_t(val >> 8));
		w_uint8(bytes, uint8_t(val & 0xff));
	}

	static inline void w_sint16(std::deque<byte>* bytes, int16_t val) {
		w_sint8(bytes, int8_t(val >> 8));
		w_sint8(bytes, int8_t(val & 0xff));
	}

	static inline void w_uint32(std::deque<byte>* bytes, uint32_t val) {
		w_uint16(bytes, uint16_t(val >> 16));
		w_uint16(bytes, uint16_t(val & 0xffff));
	}

	static inline void w_sint32(std::deque<byte>* bytes, int32_t val) {
		w_sint16(bytes, int16_t(val >> 16));
		w_sint16(bytes, int16_t(val & 0xffff));
	}

	static inline void w_float(std::deque<byte>* bytes, float val) {
		union {
			float f;
			uint32_t i;
		} u;
		u.f = val;
		w_uint32(bytes, u.i);
	}

	static inline void w_float2(std::deque<byte>* bytes, XMFLOAT2 val) {
		w_float(bytes, val.x);
		w_float(bytes, val.y);
	}

	static inline void w_float3(std::deque<byte>* bytes, XMFLOAT3 val) {
		w_float(bytes, val.x);
		w_float(bytes, val.y);
		w_float(bytes, val.z);
	}

	static inline void w_float4(std::deque<byte>* bytes, XMFLOAT4 val) {
		w_float2(bytes, XMFLOAT2(val.x, val.y));
		w_float2(bytes, XMFLOAT2(val.z, val.w));
	}

	//Reading utilities

	static inline uint8_t r_uint8(std::deque<byte>* bytes) {
		if(bytes->size() >= 1){
			uint8_t v = (*bytes)[0];
			bytes->pop_front();
			return v;
		} else {
			printf("Cannot read uint8: size = %d.\n", bytes->size());
			return 0;
		}
	}

	static inline int8_t r_sint8(std::deque<byte>* bytes) {
		if (bytes->size() >= 1) {
			union {//see w_sint8()
				uint8_t u;
				int8_t s;
			} u;
			u.u = r_uint8(bytes);
			return u.s;
		} else {
			printf("Cannot read sint8: size = %d.\n", bytes->size());
			return 0;
		}
	}

	static inline uint16_t r_uint16(std::deque<byte>* bytes) {
		return (uint16_t(r_uint8(bytes)) << 8) | (uint16_t(r_uint8(bytes)));
	}

	static inline int16_t r_sint16(std::deque<byte>* bytes) {
		return (int16_t(r_sint8(bytes)) << 8) | (int16_t(r_sint8(bytes)));
	}

	static inline uint32_t r_uint32(std::deque<byte>* bytes) {
		return (uint32_t(r_uint16(bytes)) << 16) | (uint32_t(r_uint16(bytes)));
	}

	static inline int32_t r_sint32(std::deque<byte>* bytes) {
		return (int32_t(r_sint16(bytes)) << 16) | (int32_t(r_sint16(bytes)));
	}

	static inline float r_float(std::deque<byte>* bytes) {
		union {
			float f;
			uint32_t i;
		} u;
		u.i = r_uint32(bytes);
		return u.f;
	}

	static inline XMFLOAT2 r_float2(std::deque<byte>* bytes) {
		if (bytes->size() >= 4 * 4) {
			float x = r_float(bytes);
			float y = r_float(bytes);
			return XMFLOAT2(x, y);
		} else {
			printf("Cannot read float2: size = %d.\n", bytes->size());
			return XMFLOAT2(0, 0);
		}
	}

	static inline XMFLOAT3 r_float3(std::deque<byte>* bytes) {
		if (bytes->size() >= 4 * 3) {
			float x = r_float(bytes);
			float y = r_float(bytes);
			float z = r_float(bytes);
			return XMFLOAT3(x, y, z);
		} else {
			printf("Cannot read float3: size = %d.\n", bytes->size());
			return XMFLOAT3(0, 0, 0);
		}
	}

	static inline XMFLOAT4 r_float4(std::deque<byte>* bytes) {
		XMFLOAT2 one = r_float2(bytes);
		XMFLOAT2 two = r_float2(bytes);
		return XMFLOAT4(one.x, one.y, two.x, two.y);
	}

	//Other file utilities

	static inline bool fileExists(std::string name) {
		struct stat buf;
		return stat(name.c_str(), &buf) == 0;
	}

};