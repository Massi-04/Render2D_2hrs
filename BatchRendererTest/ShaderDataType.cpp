#include "ShaderDataType.h"
#include "GL/glew.h"

size_t GetDataTypeSize(ShaderDataType type)
{
	switch (type)
	{
		case ShaderDataType::Float: return sizeof(float);
		case ShaderDataType::Float2: return sizeof(float) * 2;
		case ShaderDataType::Float3: return sizeof(float) * 3;
		case ShaderDataType::Float4: return sizeof(float) * 4;
		case ShaderDataType::Int: return sizeof(int);
		case ShaderDataType::Int2: return sizeof(int) * 2;
		case ShaderDataType::Int3: return sizeof(int) * 3;
		case ShaderDataType::Int4: return sizeof(int) * 4;
		case ShaderDataType::Mat3: return sizeof(float) * 3 * 3;
		case ShaderDataType::Mat4: return sizeof(float) * 4 * 4;
		case ShaderDataType::Bool: return sizeof(bool);
	}
	// asssert 
	return 0;
}

uint32_t GetDataTypeCount(ShaderDataType type)
{
	switch (type)
	{
		case ShaderDataType::Float: return 1;
		case ShaderDataType::Float2: return 2;
		case ShaderDataType::Float3: return 3;
		case ShaderDataType::Float4: return 4;
		case ShaderDataType::Int: return 1;
		case ShaderDataType::Int2: return 2;
		case ShaderDataType::Int3: return 3;
		case ShaderDataType::Int4: return 4;
		case ShaderDataType::Mat3: return 3;
		case ShaderDataType::Mat4: return 4;
		case ShaderDataType::Bool: return 1;
	}
	// asssert 
	return 0;
}

int32_t GetDataTypeBaseType(ShaderDataType type)
{
	switch (type)
	{
		case ShaderDataType::Float: return GL_FLOAT;
		case ShaderDataType::Float2: return GL_FLOAT;
		case ShaderDataType::Float3: return GL_FLOAT;
		case ShaderDataType::Float4: return GL_FLOAT;
		case ShaderDataType::Int: return GL_INT;
		case ShaderDataType::Int2: return GL_INT;
		case ShaderDataType::Int3: return GL_INT;
		case ShaderDataType::Int4: return GL_INT;
		case ShaderDataType::Mat3: return GL_FLOAT;
		case ShaderDataType::Mat4: return GL_FLOAT;
		case ShaderDataType::Bool: return GL_BOOL;
	}
	// asssert 
	return 0;
}
