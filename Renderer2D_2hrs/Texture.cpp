#include "Texture.h"

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "stb/stb_image.h"

Texture::Texture(uint32_t width, uint32_t height, uint32_t channels, unsigned char* data)
	: m_Width(width), m_Height(height)
{
	if (channels == 3)
	{
		m_InternalFormat = GL_RGB8;
		m_DataFormat = GL_RGB;
	}
	else if (channels == 4)
	{
		m_InternalFormat = GL_RGBA8;
		m_DataFormat = GL_RGBA;
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
	glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

	glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
}

Texture::~Texture()
{
	glDeleteTextures(1, &m_RendererID);
}

void Texture::Bind(uint32_t slot)
{
	glBindTextureUnit(slot, m_RendererID);
}

Texture* Texture::FromFile(const char* path)
{
	Texture* result;

	int width, height, channels;
	stbi_uc* textureData = stbi_load(path, &width, &height, &channels, 0);

	result = new Texture(width, height, channels, textureData);

	stbi_image_free(textureData);

	return result;
}
