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

void GetTextCoordinates(float* coords, float tilingFactor = 1.0f)
{
    coords[0] = 0.0f;
    coords[1] = tilingFactor;

    coords[2] = tilingFactor;
    coords[3] = tilingFactor;

    coords[4] = tilingFactor;
    coords[5] = 0.0f;

    coords[6] = 0.0f;
    coords[7] = 0.0f;

    /*struct Vec2Pack
    {
        float x, y;
    };

    Vec2Pack* data = (Vec2Pack*)coords;
    
    data[0] = { 0.0f, tilingFactor };
    data[1] = { tilingFactor, tilingFactor };
    data[2] = { tilingFactor, 0.0f };
    data[3] = { 0.0f, 0.0f };*/
}

void OnWindowResize(GLFWwindow* window, int width, int height);


int WndWidth = 1600;
int WndHeight = 900;

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

Vec2 QuadTextCoords[4];

glm::mat4 view;
glm::mat4 proj;

bool Init()
{
    /* Initialize the library */
    if (!glfwInit())
        return false;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WndWidth, WndHeight, "Mhanz", NULL, NULL);
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

    glfwSetWindowSizeCallback(window, OnWindowResize);

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

void DrawQuad(Transform transform, Vec3 color)
{
    glm::mat4 model =
        glm::translate(glm::mat4(1.0f), (glm::vec3)transform.Location)
        *
        GetRotation(transform.Rotation)
        *
        glm::scale(glm::mat4(1.0f), (glm::vec3)transform.Scale);

   // GetTextCoordinates((float*)QuadTextCoords, tilingFactor);

    for (int i = 0; i < 4; i++)
    {
        uint32_t vertexIndex = quadCount * 4;

        glm::vec4 loc = QuadVertices[i];
        glm::vec3 res = model * loc;

        vertexBufferData[i + vertexIndex].Position = { res.x, res.y, res.z };
        vertexBufferData[i + vertexIndex].Color = color;
        vertexBufferData[i + vertexIndex].TextureIndex = -1.0f;
        //vertexBufferData[i + vertexIndex].TextureCoordinates = QuadTextCoords[i];
    }

    quadCount++;

    if (quadCount == MaxQuads)
    {
        Flush();
    }
}

void DrawQuadTextured(Transform transform, float textureIndex, float tilingFactor = 1.0f, Vec3 colorTint = { 1.0f, 1.0f, 1.0f })
{
    glm::mat4 model =
        glm::translate(glm::mat4(1.0f), (glm::vec3)transform.Location)
        *
        GetRotation(transform.Rotation)
        *
        glm::scale(glm::mat4(1.0f), (glm::vec3)transform.Scale);

    GetTextCoordinates((float*)QuadTextCoords, tilingFactor);

    for (int i = 0; i < 4; i++)
    {
        uint32_t vertexIndex = quadCount * 4;

        glm::vec4 loc = QuadVertices[i];
        glm::vec3 res = model * loc;

        vertexBufferData[i + vertexIndex].Position = { res.x, res.y, res.z };
        vertexBufferData[i + vertexIndex].Color = colorTint;
        vertexBufferData[i + vertexIndex].TextureCoordinates = QuadTextCoords[i];
        vertexBufferData[i + vertexIndex].TextureIndex = textureIndex;
    }

    // todo texture binding, finding ecc.. if we exeed 16 slots bla bla

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
float tilingFactor = 1.0f;

Transform mainQuadTransform = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f} };
Vec3 mainQuadColor = { 1.0f, 1.0f, 1.0f };

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
    ImGui::SetWindowSize({ (float)400, (float) WndHeight }, ImGuiCond_Once);

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
            ImGui::ColorPicker3("no texture color", &mainQuadColor.X, 0);
            ImGui::PopItemWidth();
        }
    );

    SUBMENU
    (
        "Quad transform",
        {
            ImGui::DragFloat3("Location", &mainQuadTransform.Location.X, 0.01f);
            ImGui::DragFloat3("Rotation", &mainQuadTransform.Rotation.X, 0.01f);
            ImGui::DragFloat3("Scale", &mainQuadTransform.Scale.X, .01f, 0.01f, 1000.0f, "%.3f", 0);
        }
    );

    SUBMENU
    (
        "Script",
        {
            ImGui::DragFloat("Rotation speed (deg/s)", &rotPerSec);
            ImGui::DragFloat("Texture tiling factor", &tilingFactor, 0.5f);
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

    ImGui::SetWindowPos({ (float)WndWidth - 200 - 20, 0 + 20 }, ImGuiCond_Once);
    ImGui::SetWindowSize({ 200, 60 }, ImGuiCond_Once);

    if (ImGui::Button("premimi"))
    {
        MessageBoxA(nullptr, "LOL", "lol", MB_OK | MB_SYSTEMMODAL | MB_ICONERROR);
        abort(); // lol
    }   
    ImGui::End();
}

void OnWindowResize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    WndWidth = width;
    WndHeight = height;
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

        Camera cam;
        cam.FOV = 120.0f;
        cam.Transform.Location = { 0.0f, 0.0f, -1.0f };
        cam.Transform.Rotation = { 0.0f, 0.0f, 0.0f };

        while (!glfwWindowShouldClose(window))
        {
            float currentTime = glfwGetTime();
            deltaTime = currentTime - totalTime;
            totalTime = currentTime;

            cam.AspectRatio = (float)WndWidth / WndHeight;

            mainQuadTransform.Rotation.Y += rotPerSec * deltaTime;

            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            {
                textureIndex = 0.0f;
            }
            else
            {
                textureIndex = 1.0f;
            }

            BeginScene(cam);

            DrawQuadTextured(mainQuadTransform, textureIndex, tilingFactor, mainQuadColor);
            Transform tmp = mainQuadTransform;
            tmp.Location = { 0.0f, 0.0f, 0.005f };
            DrawQuad(tmp, mainQuadColor);

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