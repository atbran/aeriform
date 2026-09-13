#pragma once
#include <cstring>
namespace aeriform {
// Edge-triggered host commands retain their live parity across preset/state edits.
inline bool isRackCommand(const char* id) noexcept {
    return std::strncmp(id,"rack",4)==0
        && (std::strstr(id,"_sf_capture")!=nullptr || std::strstr(id,"_sf_release")!=nullptr);
}
}
