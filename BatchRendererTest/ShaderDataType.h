#pragma once

#include <stdint.h>

enum ShaderDataType
{
	Float, Float2, Float3, Float4,
	Int, Int2, Int3, Int4,
	Mat3, Mat4,
	Bool
};

size_t GetDataTypeSize(ShaderDataType type);
uint32_t GetDataTypeCount(ShaderDataType type);
int32_t GetDataTypeBaseType(ShaderDataType type);