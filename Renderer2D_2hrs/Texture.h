#pragma once

#include <stdint.h>

class Texture
{
public:
	Texture(uint32_t width, uint32_t height, uint32_t channels, unsigned char* data);
	~Texture();

	void Bind(uint32_t slot);

	static Texture* FromFile(const char* path);

private:
	uint32_t m_RendererID;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_InternalFormat;
	uint32_t m_DataFormat;
};

