#include "header_block_encoder.h"

#include "header_block_decoder.h"
#include "header_static_table.h"

namespace ion {

HeaderBlockEncoder::HeaderBlockEncoder(DynamicTable& dynamic_table)
    : dynamic_table_(dynamic_table) {}

std::vector<uint8_t> HeaderBlockEncoder::encode(const std::vector<HttpHeader>& headers) {
    std::vector<uint8_t> bytes{};
    for (auto& hdr : headers) {
        const auto st_it = std::find_if(
            STATIC_TABLE.begin(), STATIC_TABLE.end(), [hdr](const StaticHttpHeader& header) {
                return header.name == hdr.name && header.value == hdr.value;
            });

        if (st_it != STATIC_TABLE.end()) {
            // found static header
            const size_t table_index = std::distance(STATIC_TABLE.begin(), st_it);
            const size_t index = table_index + 1;
            bytes.push_back(static_cast<uint8_t>(index | 0x80));
        }
    }
    return bytes;
}

}  // namespace ion
