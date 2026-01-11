#include "header_block_encoder.h"

#include <algorithm>
#include <ranges>

#include "bit_writer.h"
#include "header_block_decoder.h"
#include "header_static_table.h"
#include "huffman_codes.h"
#include "int_encoder.h"

namespace ion {

static constexpr size_t MAX_PLAIN_TEXT_STRING_LENGTH = 3;

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

        auto encoded_length = IntegerEncoder::encode(bitstream.size(), 7);
        bytes.push_back(0x80 | encoded_length[0]);
        for (const auto& byte : encoded_length | std::ranges::views::drop(1)) {
            bytes.push_back(byte);
        }

        bytes.insert(bytes.end(), bitstream.begin(), bitstream.end());
    } else {
        // non huffman
        auto encoded_length = IntegerEncoder::encode(str.size(), 7);
        bytes.push_back(encoded_length[0]);
        for (const auto& byte : encoded_length | std::ranges::views::drop(1)) {
            bytes.push_back(byte);
        }

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

        // check dynamic headers too
        if (auto dyn_table_index = dynamic_table_.find(hdr)) {
            const size_t index = STATIC_TABLE.size() + dyn_table_index.value() + 1;
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

            // insert into dynamic table
            dynamic_table_.insert(hdr);
            continue;
        }

        // is dynamic field header name?
        if (auto dyn_table_index = dynamic_table_.find_name(hdr.name)) {
            const size_t index = STATIC_TABLE.size() + dyn_table_index.value() + 1;
            bytes.push_back(static_cast<uint8_t>(index | 0x40));

            // encode string
            const auto len_and_str_bytes = write_length_and_string(hdr.value);
            bytes.insert(bytes.end(), len_and_str_bytes.begin(), len_and_str_bytes.end());

            // insert into dynamic table
            dynamic_table_.insert(hdr);
            continue;
        }

        // new name
        const auto name_len_and_str_bytes = write_length_and_string(hdr.name);
        const auto value_len_and_str_bytes = write_length_and_string(hdr.value);
        bytes.push_back(0x40);
        bytes.insert(bytes.end(), name_len_and_str_bytes.begin(), name_len_and_str_bytes.end());
        bytes.insert(bytes.end(), value_len_and_str_bytes.begin(), value_len_and_str_bytes.end());

        // insert into dynamic table
        dynamic_table_.insert(hdr);
    }
    return bytes;
}

}  // namespace ion
