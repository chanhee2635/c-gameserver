#pragma once
#include <cmath>

inline constexpr float PI     = 3.14159265f;
inline constexpr float DEG2RAD = PI / 180.f;
inline constexpr float RAD2DEG = 180.f / PI;

struct Vector3
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;

    Vector3() = default;
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    // 기본 연산
    Vector3 operator+(const Vector3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vector3 operator-(const Vector3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vector3 operator*(float s)          const { return { x * s,   y * s,   z * s }; }
    bool    operator==(const Vector3& o) const { return x == o.x && y == o.y && z == o.z; }

    // 크기
    float LengthSq() const { return x * x + y * y + z * z; }
    float Length()   const { return sqrtf(LengthSq()); }

    // 거리
    float DistanceSq(const Vector3& o) const { return (*this - o).LengthSq(); }
    float Distance(const Vector3& o)   const { return (*this - o).Length(); }

    // 정규화 
    Vector3 Normalized() const
    {
        float len = Length();
        if (len < 1e-6f) return { 0.f, 0.f, 0.f };
        return { x / len, y / len, z / len };
    }

    // 내적: 두 벡터가 얼마나 같은지 (방향 판단, 시야각 체크)
    float Dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }

    // 외적: 법선 계산 (충돌, 회전 방향 판별)
    Vector3 Cross(const Vector3& o) const
    {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }

    // 영벡터 체크
    bool IsZero() const { return LengthSq() < 1e-12f; }

    // 상수
    static Vector3 Zero()    { return { 0.f, 0.f, 0.f }; }
    static Vector3 Up()      { return { 0.f, 1.f, 0.f }; }
    static Vector3 Forward() { return { 0.f, 0.f, 1.f }; }
};
