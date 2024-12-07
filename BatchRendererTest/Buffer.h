#pragma once

#include <stdint.h>

#include <vector>
#include "ShaderDataType.h"

struct VertexAttribute
{
	ShaderDataType Type;
	bool Normalized;
	size_t Size;
	int32_t Offset;
};

class VertexLayout
{
public:
	VertexLayout() {}
	VertexLayout(std::initializer_list<VertexAttribute> attributes);

public:
	inline const std::vector<VertexAttribute>& GetAttributes() const { return m_Attributes; }
	inline int32_t GetStride() const { return m_Stride; }

private:
	std::vector<VertexAttribute> m_Attributes;
	int32_t m_Stride;
};

class VertexBuffer
{
public:
	VertexBuffer(size_t size);
	VertexBuffer(float* vertices, size_t size);
	~VertexBuffer();

	void Bind();
	void Unbind();

	void SetData(float* vertices, size_t size, size_t offset = 0);

	inline void SetLayout(const VertexLayout& layout) { m_Layout = layout; }
	inline const VertexLayout& GetLayout() { return m_Layout; }

	uint32_t GetID() const { return m_RendererID; }

private:
	uint32_t m_RendererID;
	uint32_t m_Size;
	VertexLayout m_Layout;
};

class IndexBuffer
{
public:
	IndexBuffer(size_t size);
	IndexBuffer(uint32_t* indices, size_t size);

	void Bind();
	void Unbind();

	void SetData(uint32_t* indices, size_t size, size_t offset);

	uint32_t GetID() const { return m_RendererID; }

private:
	uint32_t m_RendererID;
	uint32_t m_Size;
};

class VertexArray
{
public:
	VertexArray(VertexBuffer* vertexBuffer = nullptr, IndexBuffer* indexBuffer = nullptr);
	~VertexArray();

	void SetVertexBuffer(VertexBuffer* vertexBuffer);
	void SetIndexBuffer(IndexBuffer* indexBuffer);

	void Bind();
	void Unbind();

private:
	uint32_t m_RendererID;
	VertexBuffer* m_VertexBuffer;
	IndexBuffer* m_IndexBuffer;
};