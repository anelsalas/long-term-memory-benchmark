#include "long_term_memory_benchmark/longmemeval.hpp"
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
namespace fs = std::filesystem;

std::string read_dataset(const fs::path& path) {
    const auto size = fs::file_size(path);
    if (size > 256u * 1024u * 1024u) throw std::invalid_argument("Dataset too large");
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::invalid_argument("Dataset unavailable");
    std::string bytes(static_cast<std::size_t>(size), '\0');
    if (size && !file.read(bytes.data(), static_cast<std::streamsize>(size))) throw std::runtime_error("Dataset read failed");
    if (file.peek() != std::char_traits<char>::eof()) throw std::runtime_error("Dataset changed during read");
    return bytes;
}

void write_new_file(const fs::path& path, const std::string& bytes) {
    const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("Output unavailable or already exists");
    bool success = true;
    for (std::size_t offset = 0; offset < bytes.size();) {
        DWORD written = 0;
        const auto count = static_cast<DWORD>(std::min<std::size_t>(bytes.size() - offset, 65536));
        if (!WriteFile(file, bytes.data() + offset, count, &written, nullptr) || !written) { success = false; break; }
        offset += written;
    }
    if (!CloseHandle(file)) success = false;
    if (!success) {
        DeleteFileW(path.c_str());
        throw std::runtime_error("Output write failed");
    }
}
}

int main(int argc, char** argv) {
    try {
        std::map<std::string, std::string> options;
        for (int i = 1; i < argc; i += 2) {
            if (i + 1 >= argc || !options.emplace(argv[i], argv[i + 1]).second)
                throw std::invalid_argument("Invalid arguments");
        }
        for (const auto* name : {"--dataset", "--sha256", "--question", "--namespace", "--out"})
            if (!options.count(name) || options.at(name).empty()) throw std::invalid_argument("Missing argument");
        if (options.size() != 5) throw std::invalid_argument("Unknown argument");
        const auto bytes = read_dataset(fs::u8path(options.at("--dataset")));
        const auto data = ltm::load_longmemeval_case(bytes, options.at("--question"),
            {"explicit-local-dataset", "pinned-input", options.at("--sha256")});
        const auto payload = ltm::memory_ingestion_json(data.candidate, options.at("--namespace"));
        write_new_file(fs::u8path(options.at("--out")), payload);
        std::cout << "Exported " << data.candidate.sessions.size() << " sessions\n";
        return 0;
    } catch (const std::exception&) {
        std::cerr << "Memory export failed; verify inputs, checksum and output path.\n";
        return 2;
    }
}
