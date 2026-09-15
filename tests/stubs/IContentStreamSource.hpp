#pragma once

#include <string>

class Stream {
public:
    virtual ~Stream() = default;
};

class IContentStreamSource {
public:
    virtual ~IContentStreamSource() = default;
    virtual Stream* OpenRead(std::string assetPath) = 0;
};
