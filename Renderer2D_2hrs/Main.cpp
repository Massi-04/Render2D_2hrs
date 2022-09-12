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
}

void OnWindowResize(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


int WndWidth = 1600;
int WndHeight = 900;

#define VSYNC 0

#define MAX_QUAD_BATCH 10000
#define MAX_TEXTURE_SLOTS 16

GLFWwindow* window;

uint32_t MaxQuads = 0;
uint32_t MaxVertices = 0;
uint32_t MaxIndices = 0;

uint32_t quadCount = 0;
uint32_t totalQuadCount = 0;

uint32_t drawCalls = 0;

Vertex* vertexBufferData;
uint32_t* indexBufferData;

VertexArray* vertexArray;
VertexBuffer* vBuffer;
IndexBuffer* iBuffer;
Shader* shader;
Texture* whiteTexture;

Texture** textureSlots;
uint32_t textureIndex = 1;
uint32_t totalTextures = 1;

int32_t checherboardSize = 50;

int32_t FindTexture(Texture* texture)
{
    for (uint32_t i = 0; i < textureIndex; i++)
    {
        if (textureSlots[i] == texture)
        {
            return i;
        }
    }
    return -1;
}

inline bool IsTextureBound(Texture* texture)
{
    return FindTexture(texture) != -1;
}

void PushTexture(Texture* texture)
{
    textureSlots[textureIndex++] = texture;
    totalTextures++;
}

void BindAllTextures()
{
    for (uint32_t i = 0; i < textureIndex; i++)
    {
        textureSlots[i]->Bind(i);
    }
}

void ClearTextures()
{
    textureIndex = 1;
    for (uint32_t i = 1; i < MAX_TEXTURE_SLOTS; i++)
    {
        textureSlots[i] = nullptr;
    }
}

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

    glfwSetScrollCallback(window, scroll_callback);

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

    uint32_t whitePixel = 0xffffffff;
    whiteTexture = new Texture(1, 1, 4, (unsigned char*)&whitePixel);

    int samplers[MAX_TEXTURE_SLOTS];
    for (int i = 0; i < MAX_TEXTURE_SLOTS; i++)
        samplers[i] = i;
    shader->SetUniform1iv("u_TexSlots", MAX_TEXTURE_SLOTS, samplers);

    textureSlots = (Texture**)malloc(sizeof(Texture*) * MAX_TEXTURE_SLOTS);
    textureSlots[0] = whiteTexture;
}

void ShutdownRenderer()
{
    free(vertexBufferData);
    free(indexBufferData);

    delete vBuffer;
    delete iBuffer;

    free(textureSlots);
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

    view =
        GetRotation(-camera.Transform.Rotation)
        *
        glm::translate(glm::mat4(1.0f), -(glm::vec3)camera.Transform.Location);

    proj = glm::perspectiveLH(glm::radians(camera.FOV), camera.AspectRatio, 0.1f, 10000.0f);
}

void Flush()
{
    uint32_t indexCount = quadCount * 6;

    shader->SetUniformMat4("u_View", 1, glm::value_ptr(view), false);
    shader->SetUniformMat4("u_Proj", 1, glm::value_ptr(proj), false);

    vBuffer->SetData((float*)vertexBufferData, sizeof(Vertex) * 4 * quadCount, 0);

    BindAllTextures();

    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

    quadCount = 0;
    ClearTextures();

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
        vertexBufferData[i + vertexIndex].TextureIndex = 0.0f;
        vertexBufferData[i + vertexIndex].TextureCoordinates = QuadTextCoords[i];
    }

    quadCount++;
    totalQuadCount++;

    if (quadCount == MaxQuads)
    {
        Flush();
    }
}

void DrawQuadTextured(Transform transform, Texture* texture, float tilingFactor = 1.0f, Vec3 colorTint = { 1.0f, 1.0f, 1.0f })
{
    glm::mat4 model =
        glm::translate(glm::mat4(1.0f), (glm::vec3)transform.Location)
        *
        GetRotation(transform.Rotation)
        *
        glm::scale(glm::mat4(1.0f), (glm::vec3)transform.Scale);

    GetTextCoordinates((float*)QuadTextCoords, tilingFactor);

    float textureLoc = FindTexture(texture);
    if (textureLoc == -1)
    {
        PushTexture(texture);
        textureLoc = textureIndex - 1;
    }

    for (int i = 0; i < 4; i++)
    {
        uint32_t vertexIndex = quadCount * 4;

        glm::vec4 loc = QuadVertices[i];
        glm::vec3 res = model * loc;

        vertexBufferData[i + vertexIndex].Position = { res.x, res.y, res.z };
        vertexBufferData[i + vertexIndex].Color = colorTint;
        vertexBufferData[i + vertexIndex].TextureCoordinates = QuadTextCoords[i];
        vertexBufferData[i + vertexIndex].TextureIndex = textureLoc;
    }

    quadCount++;
    totalQuadCount++;

    if (quadCount == MaxQuads || textureIndex == MAX_TEXTURE_SLOTS)
    {
        Flush();
    }
}

void EndScene()
{
    if (quadCount > 0 || textureIndex > 1)
        Flush();

    glDisable(GL_DEPTH_TEST);
    ImGuiRender();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    totalQuadCount = 0;
    totalTextures = 1; // white texture
    
    glfwSwapBuffers(window);
}

float totalTime = 0.0f;
float deltaTime = 0.0f;

float rotPerSec = 50.0f;
float tilingFactor = 3.0f;

float cameraMoveSpeed = 3.0f;
float cameraScroolMultiplier = 100.0f;
float mouseSens = 0.5f;

Transform mainQuadTransform = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f} };
Vec3 mainQuadColor = { 1.0f, 1.0f, 1.0f };

Texture* myTexture;

Camera cam;

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
            ImGui::Text("-WASD and QE to move the camera");
            ImGui::Text("-Hold right mouse btn to rotate the camera");
            ImGui::Text("-Mouse wheel to zoom the camera in/out");
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
            ImGui::DragInt("Checherboard size", &checherboardSize, 0);
            ImGui::DragFloat("Rotation speed (deg/s)", &rotPerSec);
            ImGui::DragFloat("Texture tiling factor", &tilingFactor, 0.5f);
            ImGui::DragFloat("Camera speed (units/s)", &cameraMoveSpeed);
            ImGui::DragFloat("Camera scroll multiplier", &cameraScroolMultiplier);
            ImGui::DragFloat("Mouse sensitivity", &mouseSens);
            if (ImGui::Button("Reset camera transform"))
            {
                cam.Transform = {};
                cam.Transform.Location.Z = -1.0f;
            }
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
            ImGui::Text("Quad count: %i", totalQuadCount);
            ImGui::Text("Texture count: %i", totalTextures);
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

float rot = 0.0f;

float mouseX = 0.0f;
float mouseY = 0.0f;

inline void MoveCameraForward(float direction)
{
    cam.Transform.Location.Z += direction * cameraMoveSpeed * deltaTime;
}

inline void MoveCameraRight(float direction)
{
    cam.Transform.Location.X += direction * cameraMoveSpeed * deltaTime;
}

inline void MoveCameraUp(float direction)
{
    cam.Transform.Location.Y += direction * cameraMoveSpeed * deltaTime;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    MoveCameraForward(yoffset * cameraScroolMultiplier);
}

void UpdateCameraLocation()
{
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        MoveCameraRight(-1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        MoveCameraRight(1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        MoveCameraUp(-1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        MoveCameraUp(1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        MoveCameraForward(-1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        MoveCameraForward(1.0f);
    }
}

void UpdateCameraRotation()
{
    double newX, newY;
    glfwGetCursorPos(window, &newX, &newY);
    
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        float deltaX = mouseX - newX;
        float deltaY = mouseY - newY;

        if (deltaX != 0.0f)
        {
            cam.Transform.Rotation.Y += -deltaX * mouseSens;
        }
        if (deltaY != 0.0f)
        {
            cam.Transform.Rotation.X += -deltaY * mouseSens;
        }
    }
    
    mouseX = newX;
    mouseY = newY;
}

void DrawCheckerboard()
{
    Transform tmp = {};
    tmp.Scale = { 1.0f, 1.0f, 1.0f };
    
    bool white = !(checherboardSize % 2);
    
    for (uint32_t height = 0; height < checherboardSize; height++)
    {
        if (!(checherboardSize % 2))
        {
            white = !white;
        }
        for (uint32_t width = 0; width < checherboardSize; width++)
        {
            tmp.Location = { 1.0f * width, 1.0f * height, 0.0f };
            DrawQuadTextured(tmp, (white = !white) ? whiteTexture : myTexture, 1.0f, mainQuadColor);
        }
    }
}

int main()
{
    if (Init())
    {
        InitRenderer(MAX_QUAD_BATCH);

        myTexture = Texture::FromFile("res/doom.png");

        Texture* selectedTexture = myTexture;

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

            UpdateCameraLocation();
            UpdateCameraRotation();

            BeginScene(cam);

            DrawQuadTextured(mainQuadTransform, selectedTexture, tilingFactor, mainQuadColor);


            DrawCheckerboard();

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