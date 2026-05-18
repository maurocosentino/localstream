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
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    static const std::vector<std::string> audio_extensions = {
        ".mp3", ".flac", ".ogg", ".opus", ".wav",
        ".aac", ".m4a", ".wma", ".aiff", ".ape"
    };

    for (const auto& valid_ext : audio_extensions) {
        if (ext == valid_ext) return true;
    }

    return false;
}

} // namespace localstream