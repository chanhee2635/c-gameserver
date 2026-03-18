#pragma once

// pos
struct Vector3
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	Vector3() {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

	float SqrtMagnitude() const
	{
		return (x * x + y * y + z * z);
	}

	float Dot(const Vector3& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

	void Normalize()
	{
		float sqrtMag = SqrtMagnitude();
		if (sqrtMag < 0.0001f) return;

		// CPU의 SSE/AVX 특수 연산 장치 사용(근사치 계산으로 훨씬 적은 사이클)
		float invMag = 1.0f / std::sqrt(sqrtMag);

		x *= invMag;
		y *= invMag;
		z *= invMag;
	}

	Vector3 GetNormalized() const
	{
		Vector3 v = *this;
		v.Normalize();
		return v;
	}

	float LengthSquared() const
	{
		return x * x + y * y + z * z;
	}
	static float DistanceSquared(const Vector3& v1, const Vector3& v2)
	{
		float dx = v1.x - v2.x;
		float dy = v1.y - v2.y;
		float dz = v1.z - v2.z;

		return (dx * dx + dy * dy + dz * dz);
	}
	float DistanceSquared(const Vector3& other) const
	{
		return DistanceSquared(*this, other);
	}

	Vector3 operator+(const Vector3& other) const { return { x + other.x, y + other.y, z + other.z }; }
	Vector3& operator+=(const Vector3& other) { x += other.x; y += other.y; z += other.z; return *this; }
	Vector3 operator-(const Vector3& other) const { return { x - other.x, y - other.y, z - other.z }; }
	Vector3 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar }; }
	Vector3 operator/(float scalar) const { if (::abs(scalar) < 0.0001f) return Vector3(0.f, 0.f, 0.f); return Vector3(x / scalar, y / scalar, z / scalar); }
};

// 아주 가끔 변하는 데이터
struct PlayerSummaryData {
	uint64	dbId;
	wstring	name;
	int32	templateId;
	int32	level;
};

struct PlayerLoadData {
	Vector3 pos;
	float	yaw;
	int32	hp, mp;
	int64	exp;
};

// 기획 데이터
struct MonsterData
{
	int32 id;
	int32 level;
	wstring name;
	int32 maxHp;
	int32 attack;
	int32 defense;
	int64 rewardExp;
	float speed;
	float searchRange;
	float maxSearchRange;
	float attackRange;
	float attackSpeed;
};

struct PlayerData
{
	int32 id;
	int32 level;
	int32 maxHp;
	int32 maxMp;
	int32 attack;
	int32 defense;
	int64 reqExp;
	float speed;
	float attackRange;
	float attackSpeed;
	float attackAngle;
};

struct PrefabData
{
	int32 id;
	wstring name;
	wstring prefabPath;
	int32 maxCombo;
	vector<int32> comboHitDelays;
	int32 deathDuration;
};

struct SpawnData
{
	int32 id;
	Vector3 pos;
	float yaw;
	uint64 respawnTick;
};
