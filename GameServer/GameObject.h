#pragma once
#include "GameTypes.h"
#include "Protocol.pb.h"

class GameObject : public std::enable_shared_from_this<GameObject>
{
public:
    GameObject() = default;
    virtual ~GameObject() = default;

    // 식별
    uint64                   GetObjectId()   const { return _objectId; }
    Protocol::GameObjectType GetObjectType() const { return _objectType; }
    int32                    GetTemplateId() const { return _templateId; }
    int32                    GetLevel()      const { return _level; }
    const WString&           GetName()       const { return _name; }
    const string&            GetNameUtf8()   const { return _nameUtf8; }

    // 위치
    Vector3 GetPos() const { return _pos; }
    float   GetYaw() const { return _yaw; }
    void    SetPos(Vector3 pos) { _pos = pos; }
    void    SetYaw(float yaw)   { _yaw = yaw; }

    // 씬 참조
    void         SetGameScene(GameSceneRef scene) { _gameScene = scene; _gameSceneRaw = scene.get(); }
    GameSceneRef GetGameScene()    { return _gameScene.lock(); }
    GameScene*   GetGameSceneRaw() { return _gameSceneRaw; }

    // 패킷 생성
    void MakeSummaryInfo(Protocol::ObjectSummary& info) const;
    void MakeObjectInfo(Protocol::ObjectInfo& info)     const;
    virtual void MakePosInfo(Protocol::PosInfo& info)   const {}
    virtual void MakeStatInfo(Protocol::StatInfo& info) const {}

protected:
    void SetName(const wstring& name)
    {
        _name     = WString(name.begin(), name.end());
        _nameUtf8 = Utils::ToString(name); 
    }

    uint64                   _objectId   = 0;
    Protocol::GameObjectType _objectType = Protocol::GameObjectType::UNKNOWN;
    int32                    _templateId = 0;
    int32                    _level      = 0;

    Vector3 _pos;
    float   _yaw = 0.f;

    GameSceneWeakRef _gameScene;
    GameScene*       _gameSceneRaw = nullptr;

private:
    WString _name;
    string  _nameUtf8;  // protobuf set_name용 캐싱 (SetName() 통해서만 갱신)
};
