#pragma once
class GameUtil
{
public:
	static Protocol::Vector3 ToProto(Vector3 v)
	{
		Protocol::Vector3 pv;
		pv.set_x(v.x);
		pv.set_y(v.y);
		pv.set_z(v.z);
		return pv;
	}
	static Vector3 ToServer(Protocol::Vector3 v)
	{
		return Vector3(v.x(), v.y(), v.z());
	}
};

