#include "dynamic_table.h"

#include <spdlog/spdlog.h>

#include "header_static_table.h"

namespace ion {

size_t DynamicTable::count() const {
    return table_.size();
}

HttpHeader DynamicTable::get(size_t index) {
    return table_[index];
}

void DynamicTable::insert(const HttpHeader& header) {
    table_.push_front(header);
    spdlog::debug("dynamic table: inserted header: {}: {}", header.name, header.value);
}

std::optional<size_t> DynamicTable::find(const HttpHeader& header) {
    const auto it = std::find_if(table_.begin(), table_.end(), [header](const HttpHeader& hdr) {
        return header.name == hdr.name && header.value == hdr.value;
    });
    if (it == table_.end()) {
        return std::nullopt;
    }
    return std::distance(table_.begin(), it);
}

std::optional<size_t> DynamicTable::find_name(const std::string& name) {
    const auto it = std::find_if(table_.begin(), table_.end(),
                                 [name](const HttpHeader& hdr) { return name == hdr.name; });
    if (it == table_.end()) {
        return std::nullopt;
    }
    return std::distance(table_.begin(), it);
}

void DynamicTable::log_contents() const {
    spdlog::debug("dynamic table:");
    int pos = 0;
    for (const auto& header : table_) {
        spdlog::debug(" - ({}) {}: {}", pos++, header.name, header.value);
    }
}

}  // namespace ion
