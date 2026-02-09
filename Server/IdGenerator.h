#pragma once
class IdGenerator
{
public:
	static uint64 GenerateId(GameObjectType type)
	{
		static atomic<uint32> _counter = 1;

		uint64 id = static_cast<uint64>(type) << 56;
		id |= static_cast<uint64>(_counter.fetch_add(1));

		return id;
	}
};

