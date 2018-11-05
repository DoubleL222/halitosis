#pragma once

#include <chrono>

using ms_clock = std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;

