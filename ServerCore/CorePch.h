#pragma once

#define NOMINMAX

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <intrin.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <mutex>
#include <shared_mutex> 
#include <atomic>
#include <thread>       
#include <functional>   
#include <span>         
#include <algorithm>
#include <numeric>
#include <utility>

#include "Types.h"
#include "CoreConfig.h"
#include "CoreMacro.h"
#include "CoreTLS.h"
#include "Utils.h"

#include "Memory.h"
#include "Container.h"

#include "Logger.h" 
#include "ServerStats.h"
#include "ThreadManager.h"

#include "SocketUtils.h"
#include "SendBuffer.h"

#include "JobQueue.h"
#include "JobTimer.h"
#include "GlobalQueue.h"
#include "CoreGlobal.h"

#include "DbConnection.h"
#include "DbConnectionPool.h"
#include "DbBind.h"