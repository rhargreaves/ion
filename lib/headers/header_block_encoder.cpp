#include "header_block_encoder.h"

#include "bit_writer.h"
#include "header_block_decoder.h"
#include "header_static_table.h"
#include "huffman_codes.h"

namespace ion {

constexpr size_t MAX_PLAIN_TEXT_STRING_LENGTH = 3;

HeaderBlockEncoder::HeaderBlockEncoder(DynamicTable& dynamic_table)
    : dynamic_table_(dynamic_table) {}

std::vector<uint8_t> HeaderBlockEncoder::write_length_and_string(const std::string& str) {
    std::vector<uint8_t> bytes{};

    if (str.size() > MAX_PLAIN_TEXT_STRING_LENGTH) {
        // use huffman coding
        BitWriter bw{};
        for (const char c : str) {
            auto huffman_code = HUFFMAN_CODES[c];
            bw.write_bits(huffman_code.lsb_aligned_code, huffman_code.code_len);
        }
        auto bitstream = bw.finalize();
        bytes.push_back(0x80 | (bitstream.size() & 0x7F));
        bytes.insert(bytes.end(), bitstream.begin(), bitstream.end());
    } else {
        // non huffman
        const auto length = str.size() & 0x7F;  // no huffman encoding
        bytes.push_back(static_cast<uint8_t>(length));
        for (const char c : str) {
            bytes.push_back(c);
        }
    }
    return bytes;
}

std::vector<uint8_t> HeaderBlockEncoder::encode(const std::vector<HttpHeader>& headers) {
    std::vector<uint8_t> bytes{};
    for (auto& hdr : headers) {
        // is static header?
        const auto st_it = std::find_if(
            STATIC_TABLE.begin(), STATIC_TABLE.end(), [hdr](const StaticHttpHeader& header) {
                return header.name == hdr.name && header.value == hdr.value;
            });

        if (st_it != STATIC_TABLE.end()) {
            // found static header
            const size_t table_index = std::distance(STATIC_TABLE.begin(), st_it);
            const size_t index = table_index + 1;
            bytes.push_back(static_cast<uint8_t>(index | 0x80));
            continue;
        }

        // is static field header name?
        const auto st_name_it =
            std::find_if(STATIC_TABLE.begin(), STATIC_TABLE.end(),
                         [hdr](const StaticHttpHeader& header) { return header.name == hdr.name; });

        if (st_name_it != STATIC_TABLE.end()) {
            const size_t table_index = std::distance(STATIC_TABLE.begin(), st_name_it);
            const size_t index = table_index + 1;
            bytes.push_back(static_cast<uint8_t>(index | 0x40));

            // encode string
            const auto len_and_str_bytes = write_length_and_string(hdr.value);
            bytes.insert(bytes.end(), len_and_str_bytes.begin(), len_and_str_bytes.end());
        }
    }
    return bytes;
}

}  // namespace ion
