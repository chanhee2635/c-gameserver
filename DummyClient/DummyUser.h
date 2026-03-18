#pragma once
struct Vector3
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	Vector3() {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

	float LengthSquared() const { return x * x + y * y + z * z; }

	float Dot(const Vector3& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

	void Normalize()
	{
		float lenSq = LengthSquared();
		if (lenSq < 0.0001f) return;

		float invMag = 1.0f / std::sqrt(lenSq);
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

	static float DistanceSquared(const Vector3& v1, const Vector3& v2)
	{
		float dx = v1.x - v2.x;
		float dy = v1.y - v2.y;
		float dz = v1.z - v2.z;
		return dx * dx + dy * dy + dz * dz;
	}
	float DistanceSquared(const Vector3& other) const
	{
		return DistanceSquared(*this, other);
	}

	Vector3 operator+(const Vector3& other) const { return { x + other.x, y + other.y, z + other.z }; }
	Vector3& operator+=(const Vector3& other) { x += other.x; y += other.y; z += other.z; return *this; }
	Vector3 operator-(const Vector3& other) const { return { x - other.x, y - other.y, z - other.z }; }
	Vector3 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar }; }
};

class DummyUser : public enable_shared_from_this<DummyUser>
{
public:
	void ConnectToGame();
	void ConnectToChat();
	void Update(int64 now);

	void ContinuousMove(float deltaTime);

	void SendRandomChat();
	void SendMovePacket();

	void ConnectCompleted() { isConnected.store(true); }
	bool IsChatConnected() { return isConnected.load(); }

	uint64 GetObjectId();
	ChatSessionRef GetChatSession() { return _chatSession; }

private:
	GameSessionRef _gameSession;
	ChatSessionRef _chatSession;

	atomic<bool> isConnected = false;

	int64 _nextChatTick = 0;
	int64 _nextMoveTick = 0;
	int64 _lastMoveSendTick = 0; 

	Vector3 _moveDir;

	static constexpr float MOVE_SEND_INTERVAL_MS = 100.f;
	static constexpr float MOVE_SPEED = 2.0f;
};