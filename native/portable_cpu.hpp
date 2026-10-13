#pragma once
#include <array>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sched.h>

namespace aqcf_portable {
// CPU numbering is deployment metadata; it does not select a protocol path.
inline std::array<int, 4> cpu_allocation(std::array<int, 4> legacy) {
    const char* text = std::getenv("AQCF_CPU_LIST");
    if (!text) return legacy;
    std::array<int, 4> result{};
    cpu_set_t allowed;
    CPU_ZERO(&allowed);
    if (sched_getaffinity(0, sizeof(allowed), &allowed))
        throw std::runtime_error("Cannot read allowed CPUs");
    std::istringstream stream(text);
    for (unsigned lane = 0; lane < result.size(); ++lane) {
        if (!(stream >> result[lane]) || result[lane] < 0 ||
            result[lane] >= CPU_SETSIZE || !CPU_ISSET(result[lane], &allowed) ||
            (lane && result[lane] <= result[lane - 1]))
            throw std::runtime_error("AQCF_CPU_LIST requires four distinct allowed CPUs in increasing order");
        if (lane + 1 < result.size()) {
            char separator = 0;
            if (!(stream >> separator) || separator != ',')
                throw std::runtime_error("AQCF_CPU_LIST requires four comma-separated CPUs");
        }
    }
    stream >> std::ws;
    if (!stream.eof()) throw std::runtime_error("Trailing value in AQCF_CPU_LIST");
    return result;
}
}
