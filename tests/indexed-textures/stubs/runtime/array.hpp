#pragma once
#include <vector>
template<class T> class Array {
public:
    std::vector<T> Values;
    explicit Array(std::vector<T> values) : Values(values) {}
    int get_Length() const { return static_cast<int>(Values.size()); }
    T& operator[](size_t i) { return Values[i]; }
    const T& operator[](size_t i) const { return Values[i]; }
};
