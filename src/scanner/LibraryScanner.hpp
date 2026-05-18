#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include "scanner/FileScanner.hpp"
#include "metadata/MetadataReader.hpp"
#include "db/Database.hpp"

namespace localstream {

class LibraryScanner {
public:
    LibraryScanner(Database& db, const std::vector<std::string>& directories);
    ~LibraryScanner();

    // Escaneo sincrónico — bloquea hasta terminar
    // Usado en el arranque del servidor
    int scan();

    // Escaneo asíncrono — retorna inmediatamente
    // Retorna false si ya hay un scan en curso
    bool scanAsync();

    bool isScanning() const;

private:
    Database&                db_;
    std::vector<std::string> directories_;
    FileScanner              scanner_;
    MetadataReader           reader_;

    std::atomic<bool> is_scanning_{false};
    std::mutex        db_mutex_;
    std::thread       scan_thread_;

    int runScan();
};

} // namespace localstream