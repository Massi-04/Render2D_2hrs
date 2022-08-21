#include <iostream>
#include <string>

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

#include <Windows.h>

struct Vec2
{
    float X, Y;

    operator glm::vec2() const
    {
        return glm::vec2(X, Y);
    }
};

struct Vec3
{
    float X, Y, Z;
    
    operator glm::vec3() const
    {
        return glm::vec3(X, Y, Z);
    }

    operator glm::vec4() const
    {
        return glm::vec4(X, Y, Z, 1.0f);
    }

    Vec3 operator-() const
    {
        return { -X, -Y, -Z };
    }
};

struct Vec4
{
    float X, Y, Z, W;

    operator glm::vec4() const
    {
        return glm::vec4(X, Y, Z, W);
    }
};

struct Transform
{
    Vec3 Location;
    Vec3 Rotation;
    Vec3 Scale;
};

struct Vertex
{
    Vec3 Position;
    Vec3 Color;
    Vec2 TextureCoordinates;
    float TextureIndex;
};

struct Camera
{
    Transform Transform;
    float FOV;
    float AspectRatio;
};

struct Quad
{
    Transform Transform;
    Vec3 Color;
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

glm::mat4 GetRotation(Vec3 rotation)
{
    return
    {
        glm::rotate(glm::mat4(1.0f), glm::radians(rotation.X), glm::vec3(1.0f, 0.0f, 0.0f))
        *
        glm::rotate(glm::mat4(1.0f), glm::radians(rotation.Y), glm::vec3(0.0f, 1.0f, 0.0f))
        *
        glm::rotate(glm::mat4(1.0f), glm::radians(rotation.Z), glm::vec3(0.0f, 0.0f, 1.0f))
    };
}

#define WND_WIDTH 1600
#define WND_HEIGHT 900

float totalTime = 0.0f;
float deltaTime = 0.0f;

float rotPerSec = 50.0f;

Quad quad;

#define SUBMENU(MenuName, Code)\
if(ImGui::CollapsingHeader(MenuName, ImGuiTreeNodeFlags_DefaultOpen)) \
{ \
    ImGui::Spacing(); \
    ImGui::Spacing(); \
    Code \
}\
ImGui::Spacing(); \
ImGui::Spacing();

void ImGuiRender()
{
    ImGui::Begin("Settings");

    SUBMENU
    (
        "Tips / Help",
        {
            ImGui::Text("- Tieni premuto \"E\" per rimuovere la texture");
        }
    );

    SUBMENU
    (
        "Texture color",
        {
            ImGui::PushItemWidth(200);
            ImGui::ColorPicker3("no texture color", &quad.Color.X, 0);
            ImGui::PopItemWidth();
        }
    );

    SUBMENU
    (
        "Quad transform",
        {
            ImGui::DragFloat3("Location", &quad.Transform.Location.X, 0.01f);
            ImGui::DragFloat3("Rotation", &quad.Transform.Rotation.X, 0.01f);
            ImGui::DragFloat3("Scale", &(quad.Transform.Scale.X), .01f, 0.01f, 1000.0f, "%.3f", 0);
        }
    );

    SUBMENU
    (
        "Script",
        {
            ImGui::DragFloat("Rotation speed (deg/s)", &rotPerSec);
        }
    );

    SUBMENU
    (
        "Info",
        {
            ImGui::Text((char*)glGetString(GL_VENDOR));
            ImGui::Text((char*)glGetString(GL_RENDERER));
            ImGui::Text((char*)glGetString(GL_VERSION));
            ImGui::Spacing();
            ImGui::Text("Frametime: %.3f ms (%i FPS )", deltaTime * 1000, (int32_t)(1.0f / deltaTime));
        }
    );
    
    ImGui::End();

    
    ImGui::Begin("lol");
    if (ImGui::Button("premimi"))
    {
        MessageBoxA(nullptr, "LOL", "lol", MB_OK | MB_SYSTEMMODAL | MB_ICONERROR);
        abort(); // lol
    }   
    ImGui::End();
}

float textureIndex = 0.0f;

#define VERTEX_COUNT 4
#define INDEX_COUNT 6

Vertex vertexBufferData[VERTEX_COUNT] =
{
    { -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, textureIndex },
    {  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, textureIndex },
    {  0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, textureIndex },
    { -0.5f,  0.5f, 0.0f, 0.0f, 0.6f, 0.6f, 0.0f, 0.0f, textureIndex }
};

uint32_t indexBufferData[INDEX_COUNT]
{
    0, 1, 2,
    2, 3, 0
};

Vertex tmpBufferData[VERTEX_COUNT];

Shader* shader;
VertexBuffer* vBuffer;


void UpdateVertexBufferData(Camera camera, Quad quad)
{

    // view & projection
    glm::mat4 view =
        glm::translate(glm::mat4(1.0f), -(glm::vec3)camera.Transform.Location)
        *
        GetRotation(-camera.Transform.Rotation);

    glm::mat4 proj = glm::perspectiveLH(glm::radians(camera.FOV), camera.AspectRatio, 0.1f, 10000.0f);

    shader->SetUniformMat4("u_View", 1, glm::value_ptr(view), false);
    shader->SetUniformMat4("u_Proj", 1, glm::value_ptr(proj), false);

    // model
    for (int i = 0; i < sizeof(vertexBufferData) / sizeof(Vertex); i++)
    {
        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), (glm::vec3)quad.Transform.Location)
            *
            GetRotation(quad.Transform.Rotation)
            *
            glm::scale(glm::mat4(1.0f), (glm::vec3)quad.Transform.Scale);

        glm::vec4 loc = (glm::vec4)vertexBufferData[i].Position;

        glm::vec3 res = model * loc;

        tmpBufferData[i].Position = { res.x, res.y, res.z };
        tmpBufferData[i].Color = quad.Color;
        tmpBufferData[i].TextureCoordinates = vertexBufferData[i].TextureCoordinates;
        tmpBufferData[i].TextureIndex = quad.TextureIndex;
    }

    vBuffer->SetData((float*)tmpBufferData, sizeof(tmpBufferData), 0);
}

float rot = 0.0f;

int main()
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WND_WIDTH, WND_HEIGHT, "Hello World", NULL, NULL);
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

    shader = Shader::FromFile("res/vertex.txt", "res/fragment.txt");
    shader->Bind();

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

    int samplers[2] = { 0, 1 };
    shader->SetUniform1iv("u_TexSlots", 2, samplers);


    vBuffer = new VertexBuffer((float*)&tmpBufferData, sizeof(tmpBufferData));
    IndexBuffer* iBuffer = new IndexBuffer(indexBufferData, sizeof(indexBufferData));

    vBuffer->SetLayout
    ({
        { ShaderDataType::Float3, false },
        { ShaderDataType::Float3, false },
        { ShaderDataType::Float2, false },
        { ShaderDataType::Float, false }
    });

    VertexArray* vertexArray = new VertexArray(vBuffer, iBuffer);

    glfwSwapInterval(1);

    Camera cam;
    cam.AspectRatio = (float)WND_WIDTH / WND_HEIGHT;
    cam.FOV = 120.0f;
    cam.Transform.Location = { 0.0f, 0.0f, -1.0f };
    cam.Transform.Rotation = { 0.0f, 0.0f, 0.0f };

    quad.Color = { 1.0f, 1.0f, 1.0f };
    quad.TextureIndex = 1.0f;
    quad.Transform.Location = { 0.0f, 0.0f, 0.0f };
    quad.Transform.Rotation = { 0.0f, 0.0f, 0.0f };
    quad.Transform.Scale = { 1.0f, 1.0f, 1.0f };

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        float currentTime = glfwGetTime();
        deltaTime = currentTime - totalTime;
        totalTime = currentTime;

        glfwPollEvents();
        /* Poll for and process events */

        // imgui "rendering"
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuiRender();
        ImGui::Render();

        glClear(GL_COLOR_BUFFER_BIT);

        quad.Transform.Rotation.Y += rotPerSec * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        {
            quad.TextureIndex = 0.0f;
        }
        else
        {
            quad.TextureIndex = 1.0f;
        }

        UpdateVertexBufferData(cam, quad);
        
        glDrawElements(GL_TRIANGLES, INDEX_COUNT, GL_UNSIGNED_INT, nullptr);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}