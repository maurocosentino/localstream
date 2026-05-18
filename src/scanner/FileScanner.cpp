#include "scanner/FileScanner.hpp"
#include <filesystem>
#include <algorithm>

namespace localstream {

namespace fs = std::filesystem;

std::vector<std::string> FileScanner::scan(const std::vector<std::string>& directories)
{
    std::vector<std::string> audio_files;

    for (const auto& dir : directories) {
        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            continue;
        }

        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string path = entry.path().string();

            if (isAudioFile(path)) {
                audio_files.push_back(path);
            }
        }
    }

    return audio_files;
}

bool FileScanner::isAudioFile(const std::string& path)
{
    std::string ext = fs::path(path).extension().string();

    // Convertimos a lowercase para manejar .MP3, .Flac, etc.
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".mp3" || ext == ".flac";
}

} // namespace localstream