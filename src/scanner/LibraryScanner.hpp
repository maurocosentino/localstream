#pragma once

#include <vector>
#include <string>
#include "scanner/FileScanner.hpp"
#include "metadata/MetadataReader.hpp"
#include "db/Database.hpp"

namespace localstream {

class LibraryScanner {
public:
    LibraryScanner(Database& db, const std::vector<std::string>& directories);

    // Ejecuta el pipeline completo de escaneo
    // Retorna la cantidad de tracks nuevos agregados
    int scan();

private:
    Database&                db_;
    std::vector<std::string> directories_;
    FileScanner              scanner_;
    MetadataReader           reader_;
};

} // namespace localstream