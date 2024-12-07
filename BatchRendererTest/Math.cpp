#include "Math.h"
#include "glm/ext.hpp"

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