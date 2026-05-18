#pragma once

#include <string>
#include <vector>

namespace localstream {

class FileScanner {
public:
    // Escanea los directorios dados y retorna paths de archivos de audio
    std::vector<std::string> scan(const std::vector<std::string>& directories);

private:
    bool isAudioFile(const std::string& path);
};

} // namespace localstream