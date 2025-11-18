#pragma once
#include <vector>

#include "dynamic_table.h"

namespace ion {

class HeaderBlockEncoder {
   public:
    explicit HeaderBlockEncoder(DynamicTable& dynamic_table);
    std::vector<uint8_t> encode(const std::vector<HttpHeader>& headers);

   private:
    std::vector<uint8_t> write_length_and_string(const std::string& str);

    DynamicTable& dynamic_table_;
};

}  // namespace ion
