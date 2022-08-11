#include <iostream>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

struct Vec2
{
    float X, Y;
};

struct Vec3
{
    float X, Y, Z;
};

struct Vec4
{
    float X, Y, Z, W;
};

const char* vertexShaderSrc = "\
        #version 330 core\
        \n\
        layout(location = 0) in vec4 position;\
        \n\
        void main()\n\
        {\n\
            gl_Position = position;\n\
        }";

const char* pixelShaderSrc = "\
        #version 330 core\n\
        \n\
        layout(location = 0) out vec4 color;\n\
        void main()\n\
        {\n\
            color = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n\
        }";

bool GetShaderCompileStatus(uint32_t shaderID, std::string* info)
{
    int compileResult = 0;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileResult);
    if (compileResult == GL_TRUE)
        return true; // no errors
    int msgLen;
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &msgLen);
    char* msg = new char[msgLen];
    glGetShaderInfoLog(shaderID, msgLen, &msgLen, msg);
    if (info != nullptr)
        *info = msg;
    delete[] msg;
    return false;
}

glm::mat4 transform(glm::vec2 const& Orientation, glm::vec3 const& Translate, glm::vec3 const& Up)
{
    glm::mat4 Proj = glm::perspective(glm::radians(45.f), 1.33f, 0.1f, 10.f);
    glm::mat4 ViewTranslate = glm::translate(glm::mat4(1.f), Translate);
    glm::mat4 ViewRotateX = glm::rotate(ViewTranslate, Orientation.y, Up);
    glm::mat4 View = glm::rotate(ViewRotateX, Orientation.x, Up);
    glm::mat4 Model = glm::mat4(1.0f);
    return Proj * View * Model;
}

void _CheckGLError(const char* file, int line)
{
    GLenum err(glGetError());

    while (err != GL_NO_ERROR)
    {
        std::string error;
        switch (err)
        {
        case GL_INVALID_OPERATION:  error = "INVALID_OPERATION";      break;
        case GL_INVALID_ENUM:       error = "INVALID_ENUM";           break;
        case GL_INVALID_VALUE:      error = "INVALID_VALUE";          break;
        case GL_OUT_OF_MEMORY:      error = "OUT_OF_MEMORY";          break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
        }
        std::cout << "GL_" << error.c_str() << " - " << file << ":" << line << std::endl;
        err = glGetError();
    }

    return;
}

void ImGuiRender()
{
    ImGui::Begin("finestra");
    if (ImGui::Button("premimi"))
    {
        ImGui::Text("siiii");
    }
    ImGui::End();
}

int main()
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(900, 900, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        return -1;
    }

    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    Vec2 vertexBufferData[] =
    {
        { -0.5f, -0.5f },
        {  0.5f, -0.5f },
        {  0.0f,  0.5f }
    };

    uint32_t indexBufferData[]
    {
        0,1,2
    };

    uint32_t vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    uint32_t pixelShader = glCreateShader(GL_FRAGMENT_SHADER);
    uint32_t shaderProgram = glCreateProgram();

    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);

    glShaderSource(pixelShader, 1, &pixelShaderSrc, nullptr);
    glCompileShader(pixelShader);

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, pixelShader);
    glLinkProgram(shaderProgram);
    glValidateProgram(shaderProgram);

    glUseProgram(shaderProgram);

    uint32_t vertexBuffer = 0;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, GL_STATIC_DRAW);


    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vec2), 0);

    uint32_t indexBuffer = 0;
    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexBufferData), indexBufferData, GL_STATIC_DRAW);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        /* Poll for and process events */

        // imgui "rendering"
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuiRender();
        ImGui::Render();


        glClear(GL_COLOR_BUFFER_BIT);
        
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}