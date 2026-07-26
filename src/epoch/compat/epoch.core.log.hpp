#pragma once

#include <string_view>

namespace epoch::core {
void log_info(std::string_view message) noexcept;
void log_error(std::string_view message) noexcept;
}
