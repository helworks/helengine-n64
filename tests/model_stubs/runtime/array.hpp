#pragma once
#include <cstdint>
#include <vector>

// Only the generated array operations used by the real Nintendo64RuntimeModel.
template<class T> class Array {
    std::vector<T> Values;
public:
    explicit Array(int32_t length) : Values(static_cast<std::size_t>(length)) {}
    int32_t get_Length() const { return static_cast<int32_t>(Values.size()); }
    T& operator[](int32_t index) { return Values[static_cast<std::size_t>(index)]; }
    const T& operator[](int32_t index) const { return Values[static_cast<std::size_t>(index)]; }
};
