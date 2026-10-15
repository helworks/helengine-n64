#pragma once

#include <string>

// Models the generated managed string ABI without an implicit native string conversion.
class HeCppString {
    std::string Storage;
public:
    HeCppString(const char* value) : Storage(value == nullptr ? "" : value) {}
    const std::string& raw() const { return Storage; }
};

class Stream {
public:
    virtual ~Stream() = default;
};

class IContentStreamSource {
public:
    virtual ~IContentStreamSource() = default;
    virtual Stream* OpenRead(HeCppString assetPath) = 0;
};
