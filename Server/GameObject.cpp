#include "pch.h"
#include "GameObject.h"
#include "DataManager.h"

void GameObject::Init(uint64 objectId, Vector3 pos, float yaw)
{
	_summary->objectId = objectId;
	_stats.pos = pos;
	_stats.yaw = yaw;
}

Vector3 GameObject::GetForward()
{
	float radian = GetYaw() * (3.141592f / 180.0);
	float x = sinf(radian);
	float z = cosf(radian);

	return Vector3(x, 0.0f, z).GetNormalized();
}

