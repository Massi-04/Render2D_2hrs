#include "Buffer.h"

#include "GL/glew.h"

////////////////////////////////////////////////
/////////////// VERTEX BUFFER //////////////////
////////////////////////////////////////////////

VertexBuffer::VertexBuffer(size_t size)
	: m_Size(size), m_Layout()
{
	glCreateBuffers(1, &m_RendererID);

	glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);

	glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
}

VertexBuffer::VertexBuffer(float* vertices, size_t size)
	: m_Size(size)
{
	glCreateBuffers(1, &m_RendererID);

	glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);

	glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_DYNAMIC_DRAW);
}

VertexBuffer::~VertexBuffer()
{
	glDeleteBuffers(1, &m_RendererID);
}

void VertexBuffer::Bind()
{
	glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
}

void VertexBuffer::Unbind()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::SetData(float* vertices, size_t size, size_t offset)
{
	// assert size <= m_Size - offset

	glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
	glBufferSubData(GL_ARRAY_BUFFER, offset, size, vertices);
}

////////////////////////////////////////////////
/////////////// INDEX BUFFER ///////////////////
////////////////////////////////////////////////

IndexBuffer::IndexBuffer(size_t size)
	: m_Size(size)
{
	glCreateBuffers(1, &m_RendererID);

	// we are binding as GL_ARRAY_BUFFER because GL_ELEMENT_ARRAY_BUFFER needs is directly tied
	// to the currently bound VAO, so we temporarly bind this as an ARRAY_BUFFER to initialize it

	glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
	glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);
}

IndexBuffer::IndexBuffer(uint32_t* indices, size_t size)
	: m_Size(size)
{
	glCreateBuffers(1, &m_RendererID);

	// we are binding as GL_ARRAY_BUFFER because GL_ELEMENT_ARRAY_BUFFER needs is directly tied
	// to the currently bound VAO, so we temporarly bind this as an ARRAY_BUFFER to initialize it

	glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
	glBufferData(GL_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
}

void IndexBuffer::Bind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
}

void IndexBuffer::Unbind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void IndexBuffer::SetData(uint32_t* indices, size_t size, size_t offset)
{
	// assert size <= m_Size - offset

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, indices);
}

////////////////////////////////////////////////
/////////////// VERTEX ARRAY ///////////////////
////////////////////////////////////////////////

VertexArray::VertexArray(VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer)
	: m_VertexBuffer(nullptr), m_IndexBuffer(nullptr)
{
	glGenVertexArrays(1, &m_RendererID);

	if(vertexBuffer)
		SetVertexBuffer(vertexBuffer);
	if(indexBuffer)
		SetIndexBuffer(indexBuffer);
}

VertexArray::~VertexArray()
{
	delete m_VertexBuffer;
	delete m_IndexBuffer;
}

void VertexArray::SetVertexBuffer(VertexBuffer* vertexBuffer)
{
	delete m_VertexBuffer;

	glBindVertexArray(m_RendererID);
	vertexBuffer->Bind();

	const VertexLayout& layout = vertexBuffer->GetLayout();
	const std::vector<VertexAttribute>& attributes = layout.GetAttributes();

	for (int i = 0; i < attributes.size(); i++)
	{
		glEnableVertexAttribArray(i);
		glVertexAttribPointer
		(
			i, GetDataTypeCount(attributes[i].Type),
			GetDataTypeBaseType(attributes[i].Type),
			attributes[i].Normalized ? GL_TRUE : GL_FALSE, layout.GetStride(),
			(const void*)attributes[i].Offset
		);
	}

	m_VertexBuffer = vertexBuffer;
}

void VertexArray::SetIndexBuffer(IndexBuffer* indexBuffer)
{
	delete m_IndexBuffer;

	glBindVertexArray(m_RendererID);

	indexBuffer->Bind();

	m_IndexBuffer = indexBuffer;
}

void VertexArray::Bind()
{
	glBindVertexArray(m_RendererID);
}

void VertexArray::Unbind()
{
	glBindVertexArray(0);
}

////////////////////////////////////////////////
/////////////// VERTEX LAYOUT //////////////////
////////////////////////////////////////////////

VertexLayout::VertexLayout(std::initializer_list<VertexAttribute> attributes)
	: m_Attributes(attributes), m_Stride(0)
{
	int32_t offset = 0;
	for (int32_t i = 0; i < m_Attributes.size(); i++)
	{
		size_t size = GetDataTypeSize(m_Attributes[i].Type);
		m_Attributes[i].Size = size;
		m_Attributes[i].Offset = offset;
		offset += size;
		m_Stride += size;
	}
}
