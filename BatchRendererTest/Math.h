#pragma once

#include "glm/glm.hpp"

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

glm::mat4 GetRotation(Vec3 rotation);
Vec3 GetForwardVector(Vec3 rotation);
Vec3 GetRightVector(Vec3 rotation);
Vec3 GetUpVector(Vec3 rotation);