#include "native_file_store.h"

#include "file_reader.h"

namespace ion {

std::optional<std::vector<uint8_t>> NativeFileStore::read(const std::string& path) const {
    return FileReader::read_file(path);
}

std::optional<size_t> NativeFileStore::size(const std::string& path) const {
    return FileReader::get_file_size(path);
}

bool NativeFileStore::exists(const std::string& relative_path) const {
    return FileReader::is_readable(relative_path);
}

}  // namespace ion
