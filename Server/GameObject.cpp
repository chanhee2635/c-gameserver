#include "pch.h"
#include "GameObject.h"
#include "DataManager.h"

void GameObject::MakeSummaryInfo(Protocol::ObjectSummary& info) const
{
	info.set_object_id(_objectId);
	info.set_name(Utils::ws2s(_name));
	info.set_level(_level);
	info.set_template_id(_templateId);
	info.set_object_type(static_cast<Protocol::GameObjectType>(_objectType));
}

void GameObject::MakeObjectInfo(Protocol::ObjectInfo& info) const
{
	MakeSummaryInfo(*info.mutable_summary());
	MakePosInfo(*info.mutable_pos_info());
	MakeStatInfo(*info.mutable_stat_info());
}
