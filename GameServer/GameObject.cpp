#include "pch.h"
#include "GameObject.h"

void GameObject::MakeSummaryInfo(Protocol::ObjectSummary& info) const
{
    info.set_object_id(_objectId);
    info.set_name(_nameUtf8);
    info.set_level(_level);
    info.set_template_id(_templateId);
    info.set_object_type(_objectType);
}

void GameObject::MakeObjectInfo(Protocol::ObjectInfo& info) const
{
    MakeSummaryInfo(*info.mutable_summary());
    MakePosInfo(*info.mutable_pos_info());
    MakeStatInfo(*info.mutable_stat_info());
}