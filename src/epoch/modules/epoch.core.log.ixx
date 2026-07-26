module;
#include <string_view>

export module epoch.core.log;

export namespace epoch::core {
void log_info(std::string_view message) noexcept;
void log_error(std::string_view message) noexcept;
}
