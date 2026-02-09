#pragma once

#ifdef _DEBUG
#pragma comment(lib, "RecastNDetour\\Debug\\Detour-d.lib")
#pragma comment(lib, "RecastNDetour\\Debug\\Recast-d.lib")
#else
#pragma comment(lib, "RecastNDetour\\Release\\Detour.lib")
#pragma comment(lib, "RecastNDetour\\Release\\Recast.lib")
#endif
