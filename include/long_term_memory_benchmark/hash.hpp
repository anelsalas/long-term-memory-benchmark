#pragma once
#include <string>
#include <string_view>

namespace ltm {
std::string sha256(std::string_view bytes);
}
