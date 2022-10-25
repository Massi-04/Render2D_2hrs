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

#include <future>

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

    Vec3 operator+(Vec3 other)
    {
        return { X + other.X, Y + other.Y, Z + other.Z };
    }

    Vec3 operator*(Vec3 other)
    {
        return { X * other.X, Y * other.Y, Z * other.Z };
    }

    Vec3 operator+(float value)
    {
        return { X + value, Y + value, Z + value };
    }

    Vec3 operator*(float value)
    {
        return { X * value, Y * value, Z * value };
    }

    Vec3& operator+=(Vec3 other)
    {
        *this = *this + other;
        return *this;
    }

    Vec3& operator*=(Vec3 other)
    {
        *this = *this * other;
        return *this;
    }

    Vec3& operator+=(float value)
    {
        *this = *this + value;
        return *this;
    }

    Vec3& operator*=(float value)
    {
        *this = *this * value;
        return *this;
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

struct TexturedQuad
{
    Transform Transform;
    Vec3 ColorTint;
    float TextureIndex;
    float TilingFactor;
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
bool Fullscreen = false;

#define VSYNC 1

#define IMGUI 1

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
Vec3 clearColor = { 0.321f, 0.058f, 0.784f };

TexturedQuad* quadBatch = 0;

int ThreadCount = 0;
std::future<void>* threads = 0;

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
    ThreadCount = std::thread::hardware_concurrency();
    if (ThreadCount < 1)
    {
        return false;
    }
    // NO, IT USES THE CONSTRUCTOR FOR REF COUNT AND WE NEED TO INITIALIZE IT, we could loop and init it ourself but lets use the new operator
    // threads = (std::future<void>*)malloc(sizeof(std::future<void>) * ThreadCount);
    threads = new std::future<void>[ThreadCount];

    /* Initialize the library */
    if (!glfwInit())
        return false;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WndWidth, WndHeight, "Mhanz", Fullscreen ? glfwGetPrimaryMonitor() : 0, NULL);
    
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

#if IMGUI

    ImGui::CreateContext();

    ImGui::GetIO().IniFilename = nullptr;

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
        return false;

    if (!ImGui_ImplOpenGL3_Init())
        return false;

#endif

    return true;
}

void Shutdown()
{
    delete[] threads;
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

    quadBatch = (TexturedQuad*)malloc(sizeof(TexturedQuad) * MAX_QUAD_BATCH);

    glClearColor(clearColor.X, clearColor.Y, clearColor.Z, 1.0f);
}

void ShutdownRenderer()
{
    free(vertexBufferData);
    free(indexBufferData);

    delete vBuffer;
    delete iBuffer;

    free(textureSlots);

    free(quadBatch);
}

void ImGuiRender();

void BeginScene(Camera camera)
{
    drawCalls = 0;

    glfwPollEvents();

#if IMGUI

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

#endif

    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    view =
        GetRotation(-camera.Transform.Rotation)
        *
        glm::translate(glm::mat4(1.0f), -(glm::vec3)camera.Transform.Location);

    proj = glm::perspectiveLH(glm::radians(camera.FOV), camera.AspectRatio, 0.1f, 10000.0f);
}

void CalcVertices(TexturedQuad* batchData, Vertex* vertexData, uint32_t count)
{
    Vec2 quadTextCoords[4];

    for (uint32_t i = 0; i < count; i++)
    {
        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), (glm::vec3)(batchData->Transform.Location))
            *
            GetRotation(batchData->Transform.Rotation)
            *
            glm::scale(glm::mat4(1.0f), (glm::vec3)(batchData->Transform.Scale));

        GetTextCoordinates((float*)quadTextCoords, batchData->TilingFactor);

        for (uint32_t j = 0; j < 4; j++)
        {
            uint32_t vertexIndex = quadCount * 4;

            glm::vec4 loc = QuadVertices[j];
            glm::vec3 res = model * loc;

            vertexData->Position = { res.x, res.y, res.z };
            vertexData->Color = batchData->ColorTint;
            vertexData->TextureCoordinates = quadTextCoords[j];
            vertexData->TextureIndex = batchData->TextureIndex;
            vertexData++;
        }

        batchData++;
    }
}

void BuildVertexBuffer()
{
    int32_t quadsPerThread = quadCount / ThreadCount;
    TexturedQuad* batchData = quadBatch;
    Vertex* vertexData = vertexBufferData;

    int32_t i;
    for (i = 0; i < ThreadCount - 1; i++)
    {
        threads[i] = std::async(std::launch::async, CalcVertices, batchData, vertexData, quadsPerThread);

        batchData += quadsPerThread;
        vertexData += quadsPerThread * 4;
    }
    threads[i] = std::async(std::launch::async, CalcVertices, batchData, vertexData, quadCount - quadsPerThread * i);

    for (int32_t j = 0; j < i + 1; j++)
    {
        threads[j].wait();
    }
}

void Flush()
{
    BuildVertexBuffer();

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

void PushTexturedQuad(const TexturedQuad& quad)
{
    memcpy(quadBatch + quadCount, &quad, sizeof(TexturedQuad));
}

void DrawQuad(Transform transform, Vec3 color)
{
    TexturedQuad desc;
    desc.Transform = transform;
    desc.TextureIndex = 0.0f; // white texture
    desc.TilingFactor = 1.0f;
    desc.ColorTint = color;

    PushTexturedQuad(desc);

    quadCount++;
    totalQuadCount++;

    if (quadCount == MaxQuads)
    {
        Flush();
    }
}

void DrawQuadTextured(const Transform& transform, Texture* texture, float tilingFactor = 1.0f, Vec3 colorTint = { 1.0f, 1.0f, 1.0f })
{
    float textureLoc = FindTexture(texture);
    if (textureLoc == -1)
    {
        PushTexture(texture);
        textureLoc = textureIndex - 1;
    }

    TexturedQuad desc;
    desc.Transform = transform;
    desc.TextureIndex = textureLoc;
    desc.TilingFactor = tilingFactor;
    desc.ColorTint = colorTint;

    PushTexturedQuad(desc);

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

#if IMGUI
    ImGuiRender();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

    totalQuadCount = 0;
    totalTextures = 1; // white texture

    glfwSwapBuffers(window);
}

float totalTime = 0.0f;
float deltaTime = 0.0f;

float rotPerSec = 50.0f;
float tilingFactor = 1.0f;

float cameraMoveSpeed = 3.0f;
float cameraScroolMultiplier = 100.0f;
float mouseSens = 0.2f;

Transform mainQuadTransform = { { 0.0f, 0.0f, -0.2f }, { 0.0f, 0.0f, 0.0f }, { 1.5f, 1.5f, 1.0f} };
Vec3 mainQuadColor = { 0.2f, 0.92f, 0.52f };
Vec3 checkerboardQuadScale = { 1.0f, 1.0f, 1.0f };

Texture* myTexture;

Camera cam;

float imguiPanelWidth = -1.0f;

#define SUBMENU(MenuName, Code)\
if(ImGui::CollapsingHeader(MenuName, ImGuiTreeNodeFlags_DefaultOpen)) \
{ \
    ImGui::Spacing(); \
    ImGui::Spacing(); \
    Code \
}\
ImGui::Spacing(); \
ImGui::Spacing();

#if IMGUI
void ImGuiRender()
{
    ImGui::Begin("Settings");

    ImGui::SetWindowPos({ 0, 0 }, ImGuiCond_Once);
    ImGui::SetWindowSize({ (float)400, (float)WndHeight }, ImGuiCond_Once);
    imguiPanelWidth = ImGui::GetWindowWidth();

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
        "Clear color",
        {
            ImGui::PushItemWidth(200);
            ImGui::ColorPicker3("Background color", &clearColor.X, 0);
            ImGui::PopItemWidth();
            if (ImGui::Button("Apply"))
            {
                glClearColor(clearColor.X, clearColor.Y, clearColor.Z, 1.0f);
            }
        }
    );

    SUBMENU
    (
        "Quad transform",
        {
            ImGui::DragFloat3("Q_Location", &mainQuadTransform.Location.X, 0.01f);
            ImGui::DragFloat3("Q_Rotation", &mainQuadTransform.Rotation.X, 0.01f);
            ImGui::DragFloat3("Q_Scale", &mainQuadTransform.Scale.X, .01f, 0.01f, 1000.0f, "%.3f", 0);
        }
    );

    SUBMENU
    (
        "Camera transform and FOV",
        {
            ImGui::DragFloat3("C_Location", &cam.Transform.Location.X, 0.01f);
            ImGui::DragFloat3("C_Rotation", &cam.Transform.Rotation.X, 0.01f);
            ImGui::DragFloat("C_FOV", &cam.FOV, .01f, 0.01f, 1000.0f, "%.3f");
        }
    );

    SUBMENU
    (
        "Script",
        {
            ImGui::DragInt("Checherboard size", &checherboardSize, 0.01f);
            ImGui::DragFloat3("Checherboard quad scale", &checkerboardQuadScale.X, 0.01f);
            ImGui::DragFloat("Rotation speed (deg/s)", &rotPerSec, 0.01f);
            ImGui::DragFloat("Texture tiling factor", &tilingFactor, 0.5f);
            ImGui::DragFloat("Camera speed (units/s)", &cameraMoveSpeed, 0.01f);
            ImGui::DragFloat("Camera scroll multiplier", &cameraScroolMultiplier, 0.01f);
            ImGui::DragFloat("Mouse sensitivity", &mouseSens, 0.01f);
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
#endif

void OnWindowResize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    WndWidth = width;
    WndHeight = height;
}

float rot = 0.0f;

float mouseX = 0.0f;
float mouseY = 0.0f;

Vec3 GetForwardVector(Vec3 rotation)
{
    Vec3 forward;

    forward.X = cos(glm::radians(rotation.X)) * sin(glm::radians(rotation.Y));
    forward.Y = -sin(glm::radians(rotation.X));
    forward.Z = cos(glm::radians(rotation.X)) * cos(glm::radians(rotation.Y));

    return forward;
}

Vec3 GetRightVector(Vec3 rotation)
{
    Vec3 right;

    right.X = cos(glm::radians(rotation.Y));
    right.Y = 0.0f;
    right.Z = -sin(glm::radians(rotation.Y));

    return right;
}

Vec3 GetUpVector(Vec3 rotation)
{
    glm::vec3 up = glm::cross((glm::vec3)GetForwardVector(rotation), (glm::vec3)GetRightVector(rotation));

    return { up.x, up.y, up.z };
}



void MoveCameraForward(float direction)
{
    Vec3 forward = GetForwardVector(cam.Transform.Rotation);

    float finalSpeed = cameraMoveSpeed * deltaTime * direction;

    cam.Transform.Location += forward * finalSpeed;
}

void MoveCameraRight(float direction)
{
    Vec3 right = GetRightVector(cam.Transform.Rotation);

    float finalSpeed = cameraMoveSpeed * deltaTime * direction;

    cam.Transform.Location += right * finalSpeed;
}

inline void MoveCameraUp(float direction)
{
    Vec3 up = GetUpVector(cam.Transform.Rotation);

    float finalSpeed = cameraMoveSpeed * deltaTime * direction;

    cam.Transform.Location += up * finalSpeed;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    if (mouseX > imguiPanelWidth)
    {
        MoveCameraForward(yoffset * cameraScroolMultiplier);
    }
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
    Transform quadTransform = {};
    quadTransform.Scale = checkerboardQuadScale;

    bool white = !(checherboardSize % 2);

    for (uint32_t height = 0; height < checherboardSize; height++)
    {
        if (!(checherboardSize % 2))
        {
            white = !white;
        }
        for (uint32_t width = 0; width < checherboardSize; width++)
        {
            quadTransform.Location = { quadTransform.Scale.X * width, quadTransform.Scale.Y * height, 0.0f };
            DrawQuadTextured(quadTransform, (white = !white) ? whiteTexture : myTexture, tilingFactor, { 1.0f, 1.0f, 1.0f });
        }
    }
}

#define GLFW_TIMER 0

#if GLFW_TIMER == 0

LARGE_INTEGER freq, startTicks, currentTicks;

void InitTimer()
{
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&startTicks);
}

double GetTime()
{
    QueryPerformanceCounter(&currentTicks);
    return ((double)currentTicks.QuadPart - (double)startTicks.QuadPart) / freq.QuadPart;
}

#else

void InitTimer()
{
}

double GetTime()
{
    return glfwGetTime();
}

#endif

int main()
{
    if (Init())
    {
        InitRenderer(MAX_QUAD_BATCH);


        myTexture = Texture::FromFile("res/doom.png");

        cam.FOV = 120.0f;
        cam.Transform.Location = { 0.0f, 0.0f, -1.0f };
        cam.Transform.Rotation = { 0.0f, 0.0f, 0.0f };

        InitTimer();

        while (!glfwWindowShouldClose(window))
        {
            double currentTime = GetTime();
            deltaTime = currentTime - totalTime;
            totalTime = currentTime;

            cam.AspectRatio = (float)WndWidth / WndHeight;

            mainQuadTransform.Rotation.Z += rotPerSec * deltaTime;

            UpdateCameraLocation();
            UpdateCameraRotation();

            BeginScene(cam);

            DrawQuad(mainQuadTransform, mainQuadColor);

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