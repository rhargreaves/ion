#pragma once
#include "dynamic_table.h"

namespace ion {

class HeaderBlockEncoder {
   public:
    explicit HeaderBlockEncoder(DynamicTable& dynamic_table);
    std::vector<uint8_t> encode(const std::vector<HttpHeader>& headers);

   private:
    DynamicTable& dynamic_table_;
};

}  // namespace ion
