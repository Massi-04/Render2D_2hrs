#include <iostream>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include "Shader.h"
#include "Texture.h"
#include "Buffer.h"

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

struct Vertex
{
    Vec3 Position;
    Vec3 Color;
    Vec2 TextureCoordinates;
    float TextureIndex;
};

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

glm::mat4 GetRotation(float x, float y, float z)
{
    return
    {
        glm::rotate(glm::mat4(1.0f), glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f))
        *
        glm::rotate(glm::mat4(1.0f), glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f))
        *
        glm::rotate(glm::mat4(1.0f), glm::radians(z), glm::vec3(0.0f, 0.0f, 1.0f))
    };
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

    glm::mat4 view =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f))
        *
        GetRotation(0.0f, 0.0f, 0.0f);

    glm::mat4 proj = glm::perspectiveLH(glm::radians(120.0f), 1.0f, 0.1f, 10000.0f);

    float textureIndex = 1.0f;

    Vertex vertexBufferData[] =
    {
        { -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, textureIndex },
        {  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, textureIndex },
        {  0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, textureIndex },
        { -0.5f,  0.5f, 0.0f, 0.0f, 0.6f, 0.6f, 0.0f, 0.0f, textureIndex }
    };

    for (int i = 0; i < sizeof(vertexBufferData) / sizeof(Vertex); i++)
    {
        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f))
            *
            GetRotation(0.0f, 25.0f, 0.0f)
            *
            glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));

        glm::vec4 loc;

        loc.x = vertexBufferData[i].Position.X;
        loc.y = vertexBufferData[i].Position.Y;
        loc.z = vertexBufferData[i].Position.Z;
        loc.w = 1.0f;

        glm::vec3 res = model * loc;

        vertexBufferData[i].Position.X = res.x;
        vertexBufferData[i].Position.Y = res.y;
        vertexBufferData[i].Position.Z = res.z;
    }

    uint32_t indexBufferData[]
    {
        0, 1, 2,
        2, 3, 0
    };

    Shader* s = Shader::FromFile("res/vertex.txt", "res/fragment.txt");
    s->Bind();

    Texture* whiteTexture;

    { // default white texture, if you want no texture you use the white texture and only the color will be shown
        unsigned char red = 0b11111111;
        unsigned char green = 0b11111111;
        unsigned char blue = 0b11111111;
        unsigned char alpha = 0b11111111;

        uint32_t rgba = alpha << 24 | blue << 16 | green << 8 | red;
        unsigned char* ptr = (unsigned char*)&rgba;
        whiteTexture = new Texture(1, 1, 4, ptr);
    }

    whiteTexture->Bind(0);

    Texture* t = Texture::FromFile("res/doom.png");
    t->Bind(1);

    int samplers[2] = { 0, 1, };
    s->SetUniform1iv("u_TexSlots", 2, samplers);

    s->SetUniformMat4("u_View", 1, glm::value_ptr(view), false);
    s->SetUniformMat4("u_Proj", 1, glm::value_ptr(proj), false);


    VertexBuffer* vertexBuffer = new VertexBuffer((float*)&vertexBufferData, sizeof(vertexBufferData));
    IndexBuffer* indexBuffer = new IndexBuffer(indexBufferData, sizeof(indexBufferData));

    vertexBuffer->SetLayout
    ({
        { ShaderDataType::Float3, false },
        { ShaderDataType::Float3, false },
        { ShaderDataType::Float2, false },
        { ShaderDataType::Float, false }
    });

    VertexArray* vertexArray = new VertexArray(vertexBuffer, indexBuffer);

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
        
        glDrawElements(GL_TRIANGLES, sizeof(indexBufferData) / sizeof(uint32_t), GL_UNSIGNED_INT, nullptr);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}