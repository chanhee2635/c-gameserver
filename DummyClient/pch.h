#pragma once

#define WIN32_LEAN_AND_MEAN

#include "CorePch.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

using LoginSessionRef = std::shared_ptr<class DummySession>;