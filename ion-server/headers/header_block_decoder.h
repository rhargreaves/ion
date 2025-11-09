#pragma once
#include <span>
#include <vector>

struct HttpHeader {
    std::string name;
    std::string value;
};

class HeaderBlockDecoder {
   public:
    std::vector<HttpHeader> decode(std::span<const uint8_t> data);

   private:
    std::vector<HttpHeader> dynamic_table_;
};
