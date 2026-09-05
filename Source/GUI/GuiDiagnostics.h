#pragma once

#include <atomic>

namespace aeriform::gui
{
/** Counts controls that were created with a parameter id that does not exist (tests assert this stays 0). */
inline std::atomic<int>& unboundControlCount()
{
    static std::atomic<int> count { 0 };
    return count;
}
} // namespace aeriform::gui
