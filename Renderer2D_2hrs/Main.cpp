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

#define VSYNC 1

#define MAX_QUAD_BATCH 5000

GLFWwindow* window;

uint32_t MaxQuads = 0;
uint32_t MaxVertices = 0;
uint32_t MaxIndices = 0;

uint32_t quadCount = 0;

uint32_t drawCalls = 0;

Vertex* vertexBufferData;
uint32_t* indexBufferData;

VertexArray* vertexArray;
VertexBuffer* vBuffer;
IndexBuffer* iBuffer;
Shader* shader;
Texture* whiteTexture;

const Vec3 QuadVertices[] =
{
    { -0.5f, -0.5f, 0.0f },
    {  0.5f, -0.5f, 0.0f },
    {  0.5f,  0.5f, 0.0f },
    { -0.5f,  0.5f, 0.0f }
};

const Vec2 QuadTextCoords[] =
{
    { 0.0f, 1.0f },
    { 1.0f, 1.0f },
    { 1.0f, 0.0f },
    { 0.0f, 0.0f }
};

glm::mat4 view;
glm::mat4 proj;

bool Init()
{
    /* Initialize the library */
    if (!glfwInit())
        return false;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WND_WIDTH, WND_HEIGHT, "Mhanz", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return false;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        return false;
    }

    glfwSwapInterval(VSYNC);

    ImGui::CreateContext();

    ImGui::GetIO().IniFilename = nullptr;

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
        return false;

    if (!ImGui_ImplOpenGL3_Init())
        return false;
    
    return true;
}

void Shutdown()
{
    glfwTerminate();
}

void InitRenderer(uint32_t maxQuads)
{
    MaxQuads = maxQuads;
    MaxVertices = MaxQuads * 4;
    MaxIndices = MaxQuads * 6;

    quadCount = 0;

    vertexBufferData = (Vertex*)malloc(sizeof(Vertex) * MaxVertices);
    indexBufferData = (uint32_t*)malloc(sizeof(uint32_t) * MaxIndices);

    for (int i = 0, offset = 0, valOffset = 0; i < MaxQuads; i++, offset += 6, valOffset = 4 * i)
    {
        indexBufferData[0 + offset] = 0 + valOffset;
        indexBufferData[1 + offset] = 1 + valOffset;
        indexBufferData[2 + offset] = 2 + valOffset;
        indexBufferData[3 + offset] = 2 + valOffset;
        indexBufferData[4 + offset] = 3 + valOffset;
        indexBufferData[5 + offset] = 0 + valOffset;
    }

    vBuffer = new VertexBuffer(nullptr, sizeof(Vertex) * MaxVertices);
    iBuffer = new IndexBuffer(indexBufferData, sizeof(uint32_t) * MaxIndices);
    shader = Shader::FromFile("res/vertex.txt", "res/fragment.txt");

    shader->Bind();

    vBuffer->SetLayout
    ({
        { ShaderDataType::Float3, false },
        { ShaderDataType::Float3, false },
        { ShaderDataType::Float2, false },
        { ShaderDataType::Float, false }
    });

    vertexArray = new VertexArray(vBuffer, iBuffer);
    vertexArray->Bind();

    uint32_t rgba = 0xffffffff; // white texture
    unsigned char* ptr = (unsigned char*)&rgba;
    
    whiteTexture = new Texture(1, 1, 4, ptr);
    whiteTexture->Bind(0);

    int samplers[16];
    for (int i = 0; i < 16; i++)
        samplers[i] = i;
    shader->SetUniform1iv("u_TexSlots", 16, samplers);
}

void ShutdownRenderer()
{
    delete[] vertexBufferData;
    delete[] indexBufferData;

    delete vBuffer;
    delete iBuffer;
}

void ImGuiRender();

void BeginScene(Camera camera)
{
    drawCalls = 0;

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    view =  glm::translate(glm::mat4(1.0f), -(glm::vec3)camera.Transform.Location)
            *
            GetRotation(-camera.Transform.Rotation);

    proj = glm::perspectiveLH(glm::radians(camera.FOV), camera.AspectRatio, 0.1f, 10000.0f);
}

void Flush()
{
    uint32_t indexCount = quadCount * 6;

    shader->SetUniformMat4("u_View", 1, glm::value_ptr(view), false);
    shader->SetUniformMat4("u_Proj", 1, glm::value_ptr(proj), false);

    vBuffer->SetData((float*)vertexBufferData, sizeof(Vertex) * 4 * quadCount, 0);

    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

    quadCount = 0;

    drawCalls++;
}

void DrawQuad(Quad quad)
{
    glm::mat4 model =
        glm::translate(glm::mat4(1.0f), (glm::vec3)quad.Transform.Location)
        *
        GetRotation(quad.Transform.Rotation)
        *
        glm::scale(glm::mat4(1.0f), (glm::vec3)quad.Transform.Scale);


    for (int i = 0; i < 4; i++)
    {
        uint32_t vertexIndex = quadCount * 4;

        glm::vec4 loc = QuadVertices[i];
        glm::vec3 res = model * loc;

        vertexBufferData[i + vertexIndex].Position = { res.x, res.y, res.z };
        vertexBufferData[i + vertexIndex].Color = quad.Color;
        vertexBufferData[i + vertexIndex].TextureCoordinates = QuadTextCoords[i];
        vertexBufferData[i + vertexIndex].TextureIndex = quad.TextureIndex;
    }

    quadCount++;

    if (quadCount == MaxQuads)
    {
        Flush();
    }
}

void EndScene()
{
    if (quadCount > 0)
        Flush();

    glDisable(GL_DEPTH_TEST);
    ImGuiRender();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    glfwSwapBuffers(window);
}

float totalTime = 0.0f;
float deltaTime = 0.0f;

float rotPerSec = 50.0f;

Quad quad;

Texture* myTexture;

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

    ImGui::SetWindowPos({ 0, 0 }, ImGuiCond_Once);
    ImGui::SetWindowSize({ 400, WND_HEIGHT }, ImGuiCond_Once);

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
            ImGui::Text("Draw calls: %i", drawCalls);
        }
    );
     
    ImGui::End();

    
    ImGui::Begin("lol");

    ImGui::SetWindowPos({ WND_WIDTH - 200 - 20, 0 + 20 }, ImGuiCond_Once);
    ImGui::SetWindowSize({ 200, 60 }, ImGuiCond_Once);

    if (ImGui::Button("premimi"))
    {
        MessageBoxA(nullptr, "LOL", "lol", MB_OK | MB_SYSTEMMODAL | MB_ICONERROR);
        abort(); // lol
    }   
    ImGui::End();
}

float textureIndex = 0.0f;
float rot = 0.0f;

int main()
{
    if (Init())
    {
        InitRenderer(MAX_QUAD_BATCH);

        myTexture = Texture::FromFile("res/doom.png");
        myTexture->Bind(1);

        quad.Color = { 1.0f, 1.0f, 1.0f };
        quad.TextureIndex = 1.0f;
        quad.Transform.Location = { 0.0f, 0.0f, 0.0f };
        quad.Transform.Rotation = { 0.0f, 0.0f, 0.0f };
        quad.Transform.Scale = { 1.0f, 1.0f, 1.0f };

        Camera cam;
        cam.AspectRatio = (float)WND_WIDTH / WND_HEIGHT;
        cam.FOV = 120.0f;
        cam.Transform.Location = { 0.0f, 0.0f, -1.0f };
        cam.Transform.Rotation = { 0.0f, 0.0f, 0.0f };

        while (!glfwWindowShouldClose(window))
        {
            float currentTime = glfwGetTime();
            deltaTime = currentTime - totalTime;
            totalTime = currentTime;

            quad.Transform.Rotation.Y += rotPerSec * deltaTime;

            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            {
                quad.TextureIndex = 0.0f;
            }
            else
            {
                quad.TextureIndex = 1.0f;
            }

            BeginScene(cam);

            Quad q2;

            q2.Color = quad.Color;
            q2.TextureIndex = quad.TextureIndex;
            q2.Transform = quad.Transform;
            q2.Transform.Location = { 0.0f, 0.0f, 0.0f };

            DrawQuad(quad);
            DrawQuad(q2);


            EndScene();
        }

        ShutdownRenderer();
        Shutdown();
        
        return 0;
    }
    else
    {
        std::cout << "Init error";
        return -1;
    }
}