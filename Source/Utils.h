#pragma once

#include <vector>
#include <string>
#include <sstream>
#include "DXF.h"
#ifdef FBX_SDK
#include <fbxsdk.h>
#endif

#define PI 3.14159265f

class Utils {

private:
	Utils() {};//can't instantiate

public:

	///splits a string into a vector of substrings using any delimiter char
	static inline void split(const std::string& s, char delimiter, std::vector<std::string>* out_result) {
		std::string token;
		std::istringstream strm(s);
		while (std::getline(strm, token, delimiter)) {
			out_result->push_back(token);
		}
	}

	///Changes a projection matrix's fov
	static inline XMMATRIX changeFov(XMMATRIX& projectionMatrix, float fov, float widthOverHeight = 1) {
		XMFLOAT4X4 proj;
		XMStoreFloat4x4(&proj, projectionMatrix);
		proj._11 = 1.0f / tan(fov*PI / 360.f);
		proj._22 = proj._11 * widthOverHeight;
		return XMLoadFloat4x4(&proj);
	}

	///clamps value between a and b
	static inline float clamp(float value, float a, float b) {
		return value <= a ? a : value >= b ? b : value;
	}

	///loops the value t so that it is never larger than length and never smaller than 0; from Unity's source code.
	static inline float repeat(float t, float length) {
		return clamp(t - (int)(t / length) * length, 0, length);
	}

	///linear interpolation for floats
	static inline float lerp(float one, float two, float t) {
		return one*t + two*(1.0f - t);
	}

	///linear interpolation for XMFLOAT3's
	static inline XMFLOAT3 lerp(XMFLOAT3 one, XMFLOAT3 two, float t) {
		return XMFLOAT3(lerp(one.x, two.x, t), lerp(one.y, two.y, t), lerp(one.z, two.z, t));
	}

	///linear interpolation for XMFLOAT2's
	static inline XMFLOAT2 lerp(XMFLOAT2 one, XMFLOAT2 two, float t) {
		return XMFLOAT2(lerp(one.x, two.x, t), lerp(one.y, two.y, t));
	}

	///linear interpolation between angles for floats; angles in Degrees
	static inline float lerpAngleDegrees(float one, float two, float t) {
		float delta = repeat((two - one), 360);
		if (delta > 180)
			delta -= 360;
		return one + delta * t;
	}

	///linear interpolation between angles for XMFLOAT3's; angles in Degrees
	static inline XMFLOAT3 lerpAngleDegrees(XMFLOAT3 one, XMFLOAT3 two, float t) {
		return XMFLOAT3(lerpAngleDegrees(one.x, two.x, t), lerpAngleDegrees(one.y, two.y, t), lerpAngleDegrees(one.z, two.z, t));
	}

	///Returns the same angle but between min and min+2*PI
	static inline float mapAngleRad(float angle, float min = 0) {
		while (angle < min) angle += 2 * PI;
		while (angle >= min + 2 * PI) angle -= 2 * PI;
		return angle;
	}

	///converts degrees into radians
	static inline float degToRad(float degrees) {
		return degrees * PI / 180.f;
	}

	///converts radians into degrees
	static inline float radToDeg(float radians) {
		return radians * 180.f / PI;
	}

	///converts float3 degrees into radians
	static inline XMFLOAT3 degToRad(XMFLOAT3 degrees) {
		return XMFLOAT3(degToRad(degrees.x), degToRad(degrees.y), degToRad(degrees.z));
	}

#ifdef FBX_SDK
	///converts an fbxsdk::FbxPropertyT<FbxDouble3> to XMFLOAT3
	static inline XMFLOAT3 toFloat3(fbxsdk::FbxPropertyT<FbxDouble3> fbxDouble3) {
		FbxDouble3 d3 = fbxDouble3.Get();
		return XMFLOAT3(d3[0], d3[1], d3[2]);
	}

	///converts an fbxsdk::FbxVector4 to XMFLOAT3
	static inline XMFLOAT3 toFloat3(const fbxsdk::FbxVector4 fbxVector4) {
		return XMFLOAT3(fbxVector4[0], fbxVector4[1], fbxVector4[2]);
	}
#endif

	///outputs an XMMATRIX to cout
	static inline void printMatrix(XMMATRIX m) {
		XMFLOAT4X4 f;
		XMStoreFloat4x4(&f, m);
		printf("[ %f , %f , %f , %f ]\n", f._11, f._12, f._13, f._14);
		printf("[ %f , %f , %f , %f ]\n", f._21, f._22, f._23, f._24);
		printf("[ %f , %f , %f , %f ]\n", f._31, f._32, f._33, f._34);
		printf("[ %f , %f , %f , %f ]\n", f._41, f._42, f._43, f._44);
	}

	///rng between two floats
	static inline float random(float min, float max) {
		return float(rand() % 1000) / 1000.f * (max - min) + min;
	}

	///cross product (works on any type with .x .y .z so XMFLOAT3 or XMFLOAT4 interchangeably)
	template<typename T>
	static inline XMFLOAT3 cross(T a, T b) {
		return XMFLOAT3(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		);
	}

	///dot product (works on any type with .x .y .z so XMFLOAT3 or XMFLOAT4 interchangeably)
	template<typename T>
	static inline float dot3(T a, T b) {
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	///Add two vector3s
	static inline XMFLOAT2 add2(XMFLOAT2 a, XMFLOAT2 b) {
		return XMFLOAT2(a.x + b.x, a.y + b.y);
	}

	///Add two vector3s
	static inline XMFLOAT3 add3(XMFLOAT3 a, XMFLOAT3 b) {
		return XMFLOAT3(a.x + b.x, a.y + b.y, a.z + b.z);
	}

	///Oppose a vector (any type with .x .y .z defined)
	template<typename T>
	static inline XMFLOAT3 opp3(T a) {
		return XMFLOAT3(-a.x, -a.y, -a.z);
	}

	///Oppose a vector4 (any type with .x .y .z .w defined)
	template<typename T>
	static inline XMFLOAT4 opp4(T a) {
		return XMFLOAT4(-a.x, -a.y, -a.z, -a.w);
	}

	///Subtracts a float2 from another
	static inline XMFLOAT2 sub2(XMFLOAT2 a, XMFLOAT2 b) {
		return XMFLOAT2(a.x - b.x, a.y - b.y);
	}

	///Subtracts a float3 from another
	static inline XMFLOAT3 sub3(XMFLOAT3 a, XMFLOAT3 b) {
		return XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z);
	}

	///Subtract a float4 from another (only on x y and z)
	static inline XMFLOAT3 sub3(XMFLOAT4 a, XMFLOAT4 b) {
		return XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z);
	}

	///Normalize a vector3
	static inline XMFLOAT3 normalize(XMFLOAT3 a) {
		float len = sqrtf((a.x * a.x) + (a.y * a.y) + (a.z * a.z));
		return XMFLOAT3(a.x / len, a.y / len, a.z / len);
	}

	//Return magnitude of a vector3
	static inline float magnitude3(XMFLOAT3 a) {
		return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
	}

	///Mutliply individual components of a vector by a float
	static inline XMFLOAT2 mult2(XMFLOAT2 a, float b) {
		return XMFLOAT2(a.x * b, a.y * b);
	}

	///Mutliply individual components of two float3s
	static inline XMFLOAT3 mult3(XMFLOAT3 a, XMFLOAT3 b) {
		return XMFLOAT3(a.x * b.x, a.y * b.y, a.z * b.z);
	}

	///Mutliply individual components of a vector by a float
	static inline XMFLOAT3 mult3(XMFLOAT3 a, float b) {
		return XMFLOAT3(a.x * b, a.y * b, a.z * b);
	}

};