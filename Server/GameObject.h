#pragma once

class GameObject : public enable_shared_from_this<GameObject>
{
public:
	GameObject() {}
	virtual ~GameObject() {}

	virtual void Init(uint64 objectId, Vector3 pos, float yaw);

	uint64			GetObjectId() { return _summary->objectId; }
	GameObjectType	GetObjectType() { return _summary->objectType; }
	Vector3			GetPos() { return _stats.pos; }
	float			GetYaw() { return _stats.yaw; }
	const StatData&	GetStat() { return _stats; }
	Vector3			GetForward();

	void SetGameScene(GameSceneRef gameScene) { _gameScene = gameScene; }
	GameSceneRef GetGameScene() { return _gameScene.lock(); }

protected:
	SummaryDataRef		_summary;
	StatData			_stats;

private:
	weak_ptr<GameScene> _gameScene;
};
