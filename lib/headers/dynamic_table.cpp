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

}  // namespace ion
