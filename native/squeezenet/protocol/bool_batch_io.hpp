#pragma once
// Engineering transport repair only. Preserve native EMP bool byte transcript;
// no OT, randomness, security parameter, arithmetic or public flush is removed.
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace aqcf_bool_batch {
constexpr int packed_chunk_bytes = 16384;
static_assert(sizeof(bool) == 1, "Pinned native byte packing requires byte bool");

template<class IO> void send(IO& io, bool* data, int length) {
    if (length < 0) throw std::invalid_argument("negative bool length");
    void* ptr = data;
    std::size_t space = static_cast<std::size_t>(length);
    const void* aligned = std::align(alignof(uint64_t), sizeof(uint64_t), ptr, space);
    if (!aligned) { io.send_data(data, length); return; }
    const int prefix = length - static_cast<int>(space);
    io.send_data(data, prefix); // Native unaligned prefix, including zero-length call.
    const auto* values = static_cast<const bool*>(aligned);
    const int rest = length - prefix, packed = rest / 8;
    std::array<uint8_t, packed_chunk_bytes> buffer;
    for (int begin = 0; begin < packed; begin += packed_chunk_bytes) {
        const int count = std::min(packed_chunk_bytes, packed - begin);
        for (int j = 0; j < count; ++j) {
            uint8_t byte = 0;
            for (int bit = 0; bit < 8; ++bit)
                byte |= uint8_t(values[8 * (begin + j) + bit]) << bit;
            buffer[j] = byte;
        }
        io.send_data(buffer.data(), count);
    }
    if (8 * packed != rest) io.send_data(values + 8 * packed, rest - 8 * packed);
}

template<class IO> void recv(IO& io, bool* data, int length) {
    if (length < 0) throw std::invalid_argument("negative bool length");
    void* ptr = data;
    std::size_t space = static_cast<std::size_t>(length);
    void* aligned = std::align(alignof(uint64_t), sizeof(uint64_t), ptr, space);
    if (!aligned) { io.recv_data(data, length); return; }
    const int prefix = length - static_cast<int>(space);
    io.recv_data(data, prefix);
    auto* values = static_cast<bool*>(aligned);
    const int rest = length - prefix, packed = rest / 8;
    std::array<uint8_t, packed_chunk_bytes> buffer;
    for (int begin = 0; begin < packed; begin += packed_chunk_bytes) {
        const int count = std::min(packed_chunk_bytes, packed - begin);
        io.recv_data(buffer.data(), count);
        for (int j = 0; j < count; ++j)
            for (int bit = 0; bit < 8; ++bit)
                values[8 * (begin + j) + bit] = (buffer[j] >> bit) & 1U;
    }
    if (8 * packed != rest) io.recv_data(values + 8 * packed, rest - 8 * packed);
}
}
