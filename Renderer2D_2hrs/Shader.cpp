#include "Shader.h"

#include "GL/glew.h"

#include <fstream>
#include <string>

Shader::Shader(const char* vertexShaderSrc, const char* fragmentShaderSrc)
{
    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    m_RendererID = glCreateProgram();

    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);

    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);

    glAttachShader(m_RendererID, vertexShader);
    glAttachShader(m_RendererID, fragmentShader);
    glLinkProgram(m_RendererID);
    glValidateProgram(m_RendererID);

    glDetachShader(m_RendererID, vertexShader);
    glDeleteShader(vertexShader);

    glDetachShader(m_RendererID, fragmentShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
    glDeleteProgram(m_RendererID);
}

void Shader::Bind()
{
    glUseProgram(m_RendererID);
}

void Shader::UnBind()
{
    glUseProgram(0);
}

Shader* Shader::FromFile(const char* vertexPath, const char* fragmentPath)
{
    std::ifstream vertexFile(vertexPath);
    std::ifstream fragmentFile(fragmentPath);

    std::string vertexSrc;
    if (vertexFile.is_open())
    {
        std::string line;
        while (getline(vertexFile, line))
            vertexSrc += line + '\n';
        vertexFile.close();
    }

    std::string fragmentSrc;
    if (fragmentFile.is_open())
    {
        std::string line;
        while (getline(fragmentFile, line))
            fragmentSrc += line + '\n';
        fragmentFile.close();
    }

    return new Shader(vertexSrc.c_str(), fragmentSrc.c_str());
}

void Shader::SetUniform1iv(const char* name, size_t count, int32_t* value)
{
    int32_t location = glGetUniformLocation(m_RendererID, name);
    if (location > -1)
    {
        glUniform1iv(location, count, value);
    }
}

void Shader::SetUniformMat4(const char* name, size_t count, float* value, bool transpose)
{
    int32_t location = glGetUniformLocation(m_RendererID, name);
    if (location > -1)
    {
        glUniformMatrix4fv(location, count, transpose ? GL_TRUE : GL_FALSE, value);
    }
}
