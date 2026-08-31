#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <array>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <iostream>
#include <fstream>
#include <sstream>
#include <format>
#include <cmath>
#include <unordered_map>

using namespace std::chrono_literals;
