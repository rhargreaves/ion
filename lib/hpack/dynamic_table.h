

#pragma once
#include <deque>
#include <optional>

#include "http_header.h"

namespace ion {

constexpr size_t DEFAULT_MAX_TABLE_SIZE = 4096;

class DynamicTable {
   public:
    explicit DynamicTable(size_t max_table_size = DEFAULT_MAX_TABLE_SIZE);

    size_t count() const;
    HttpHeader get(size_t index);
    void insert(const HttpHeader& header);
    std::optional<size_t> find(const HttpHeader& header);
    std::optional<size_t> find_name(const std::string& name);
    void log_contents() const;
    int size() const;
    void set_max_table_size(size_t new_sz);

   private:
    std::deque<HttpHeader> table_{};
    size_t table_size_{};
    size_t max_table_size_{};
    static size_t get_header_entry_size(const HttpHeader& header);
};

}  // namespace ion
