#pragma once

#include <atomic>
#include <functional>
#include <string_view>

using callback_type = std::function<void(std::string_view)>;

void SetLogFatalCallback(callback_type callback);

void SetLogErrorCallback(callback_type callback);

void SetLogIgnoreCallback(callback_type callback);

void Log(bool condition, std::string_view message);
