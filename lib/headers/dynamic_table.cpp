#include "dynamic_table.h"

namespace ion {

size_t DynamicTable::size() {
    return table_.size();
}

HttpHeader DynamicTable::get(size_t index) {
    return table_[index];
}

void DynamicTable::insert(const HttpHeader& header) {
    table_.push_back(header);
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

}  // namespace ion
