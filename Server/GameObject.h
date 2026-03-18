#pragma once

class GameObject : public enable_shared_from_this<GameObject>
{
public:
	GameObject() = default;
	virtual ~GameObject() = default;

	uint64			GetObjectId()	const { return _objectId; }
	GameObjectType	GetObjectType() const { return _objectType; }
	const wstring&	GetName()		const { return _name; }
	int32			GetLevel()		const { return _level; }
	int32			GetTemplateId() const { return _templateId; }

	Vector3	GetPos()	 const  { return _pos; }
	void	SetPos(Vector3 pos) { _pos = pos; }

	void			SetGameScene(GameSceneRef gameScene) { _gameScene = gameScene; }
	GameSceneRef	GetGameScene()						 { return _gameScene.lock(); }

	void MakeSummaryInfo(Protocol::ObjectSummary& info) const;
	virtual void MakePosInfo(Protocol::PosInfo& info)	const {}
	virtual void MakeStatInfo(Protocol::StatInfo& info) const {}
	void MakeObjectInfo(Protocol::ObjectInfo& info)		const;

protected:
	uint64			_objectId	= 0;
	GameObjectType	_objectType = GameObjectType::UnKnown;
	int32			_templateId = 0;
	int32			_level		= 0;
	wstring			_name;

	Vector3	_pos;
	float	_yaw = 0.f;

	weak_ptr<GameScene> _gameScene;
};
