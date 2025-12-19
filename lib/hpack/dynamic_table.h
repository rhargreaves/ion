

#pragma once
#include <deque>
#include <optional>

#include "http_header.h"

namespace ion {

class DynamicTable {
   public:
    size_t size();
    HttpHeader get(size_t index);
    void insert(const HttpHeader& header);
    std::optional<size_t> find(const HttpHeader& header);
    std::optional<size_t> find_name(const std::string& name);
    void log_contents() const;

   private:
    std::deque<HttpHeader> table_{};
};

}  // namespace ion
