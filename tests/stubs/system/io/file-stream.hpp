#pragma once

#include <cstddef>

#include "IContentStreamSource.hpp"

class FileStream final : public Stream {
public:
    FileStream(const void*, std::size_t) {
    }
};
