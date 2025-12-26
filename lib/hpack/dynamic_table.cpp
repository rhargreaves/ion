#include "dynamic_table.h"

#include <spdlog/spdlog.h>

#include "header_static_table.h"

namespace ion {

DynamicTable::DynamicTable(size_t max_table_size) : max_table_size_(max_table_size) {}

size_t DynamicTable::count() const {
    return table_.size();
}

HttpHeader DynamicTable::get(size_t index) {
    return table_[index];
}

void DynamicTable::insert(const HttpHeader& header) {
    const size_t entry_size = get_header_entry_size(header);
    if (entry_size > max_table_size_) {
        spdlog::debug("dynamic table: entry too big (sz: {}, max: {}), table wiped", entry_size,
                      max_table_size_);
        table_.clear();
        table_size_ = 0;
        return;
    }

    while (table_size_ + entry_size > max_table_size_) {
        table_size_ -= get_header_entry_size(table_.back());
        table_.pop_back();
    }

    table_.push_front(header);
    table_size_ += entry_size;
    spdlog::debug("dynamic table: entry too big (sz: {}, max: {}), table wiped", entry_size,
                  max_table_size_);
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
    spdlog::debug("dynamic table (size: {}, max: {}):", table_size_, max_table_size_);
    int pos = 0;
    for (const auto& header : table_) {
        spdlog::debug(" - ({}) {}: {}", pos++, header.name, header.value);
    }
}

int DynamicTable::size() const {
    return table_size_;
}

void DynamicTable::set_max_table_size(size_t new_sz) {
    max_table_size_ = new_sz;
    while (table_size_ > max_table_size_) {
        assert(!table_.empty());
        table_size_ -= get_header_entry_size(table_.back());
        table_.pop_back();
    }
}

size_t DynamicTable::get_header_entry_size(const HttpHeader& header) {
    return header.name.size() + header.value.size() + 32;
}

}  // namespace ion
