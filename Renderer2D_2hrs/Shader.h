#pragma once

#include <stdint.h>

class Shader
{
public:
	Shader(const char* vertexShaderSrc, const char* fragmentShaderSrc);
	~Shader();

	void Bind();
	void UnBind();

	inline uint32_t GetID() const { return m_RendererID; }

	static Shader* FromFile(const char* vertexPath, const char* fragmentPath);

	void SetUniform1iv(const char* name, size_t count, int32_t* value);
	void SetUniformMat4(const char* name, size_t count, float* value, bool transpose);

private:
	uint32_t m_RendererID;
};

