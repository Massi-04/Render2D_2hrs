#include <iostream>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include "stb/stb_image.h"

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

const char* vertexShaderSrc = "\
        #version 330 core\n\
        \n\
        layout(location = 0) in vec3 position;\n\
        layout(location = 1) in vec3 color;\n\
        layout(location = 2) in vec2 texCoords;\n\
        layout(location = 3) in float texIndex;\n\
        out vec3 v_Color;\n\
        out vec2 v_TexCoord;\n\
        out float v_TexIndex;\n\
        uniform mat4 u_View;\n\
        uniform mat4 u_Proj;\n\
        void main()\n\
        {\n\
            gl_Position = u_Proj * u_View * vec4(position, 1.0f);\n\
            v_Color = color;\n\
            v_TexCoord = texCoords;\n\
            v_TexIndex = texIndex;\n\
        }";

const char* pixelShaderSrc = "\
        #version 330 core\n\
        \n\
        layout(location = 0) out vec4 color;\n\
        in vec3 v_Color;\n\
        in vec2 v_TexCoord;\n\
        in float v_TexIndex;\n\
        uniform sampler2D u_TexSlots[32];\n\
        void main()\n\
        {\n\
            color = texture(u_TexSlots[int(v_TexIndex)], v_TexCoord);\n\
        }";

const char* textureSrc = "res/doom.png";

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
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, .5f))
        *
        GetRotation(0.0f, 0.0f, 0.0f);

    glm::mat4 proj = glm::perspectiveLH(glm::radians(120.0f), 1.0f, 0.1f, 10000.0f);

    Vertex vertexBufferData[] =
    {
        { -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },
        {  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f },
        {  0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f },
        { -0.5f,  0.5f, 0.0f, 0.0f, 0.6f, 0.6f, 0.0f, 0.0f, 0.0f }
    };

    for (int i = 0; i < sizeof(vertexBufferData) / sizeof(Vertex); i++)
    {
        glm::mat4 model = 
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f))
            *
            GetRotation(0.0f, 0.0f, 0.0f)
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

    uint32_t vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    int x, y, channels;
    stbi_uc* textureData = stbi_load(textureSrc, &x, &y, &channels, 4);

    uint32_t texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

    stbi_image_free(textureData);

    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    uint32_t pixelShader = glCreateShader(GL_FRAGMENT_SHADER);
    uint32_t shaderProgram = glCreateProgram();

    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);

    std::string vErr;
    GetShaderCompileStatus(vertexShader, &vErr);

    glShaderSource(pixelShader, 1, &pixelShaderSrc, nullptr);
    glCompileShader(pixelShader);

    std::string pErr;
    GetShaderCompileStatus(pixelShader, &pErr);

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, pixelShader);
    glLinkProgram(shaderProgram);
    glValidateProgram(shaderProgram);

    glUseProgram(shaderProgram);

    auto loc = glGetUniformLocation(shaderProgram, "u_TexSlots");
    int samplers[2] = { 0, 1, };
    glUniform1iv(loc, 2, samplers);

    auto loc2 = glGetUniformLocation(shaderProgram, "u_View");
    glUniformMatrix4fv(loc2, 1, GL_FALSE, glm::value_ptr(view));

    auto loc3 = glGetUniformLocation(shaderProgram, "u_Proj");
    glUniformMatrix4fv(loc3, 1, GL_FALSE, glm::value_ptr(proj));

    uint32_t vertexBuffer = 0;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, GL_STATIC_DRAW);


    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(Vec3));
    
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (sizeof(Vec3) + sizeof(Vec3)));
    
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(Vec3) + sizeof(Vec3) + sizeof(Vec2)));

    auto err = glGetError();

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
        
        glDrawElements(GL_TRIANGLES, sizeof(indexBufferData) / sizeof(uint32_t), GL_UNSIGNED_INT, nullptr);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}