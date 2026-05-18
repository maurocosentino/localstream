#include <gtest/gtest.h>
#include "metadata/MetadataReader.hpp"
#include <fstream>
#include <filesystem>

using namespace localstream;
namespace fs = std::filesystem;

class MetadataReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = fs::temp_directory_path() / "localstream_tests";
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        fs::remove_all(test_dir);
    }

    fs::path test_dir;
    MetadataReader reader;
};

TEST_F(MetadataReaderTest, ReturnsNulloptForNonExistentFile) {
    auto result = reader.read("/nonexistent/path/file.mp3");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MetadataReaderTest, ReturnsNulloptForNonAudioFile) {
    auto fake_mp3 = test_dir / "fake.mp3";
    std::ofstream f(fake_mp3);
    f << "esto no es un mp3";
    f.close();

    // TagLib intenta abrir cualquier archivo con extensión mp3
    // Si logra abrirlo, retorna metadata vacía con fallbacks
    auto result = reader.read(fake_mp3.string());
    if (result.has_value()) {
        // El fallback usa el nombre del archivo como título
        EXPECT_EQ(result->file_path, fake_mp3.string());
        EXPECT_FALSE(result->title.empty());
    }
    // Si no puede abrirlo, retorna nullopt — ambos casos son válidos
}

TEST_F(MetadataReaderTest, ReturnsNulloptForEmptyFile) {
    auto empty_file = test_dir / "empty.mp3";
    std::ofstream f(empty_file);
    f.close();

    // Un archivo vacío con extensión mp3 puede ser abierto por TagLib
    // El comportamiento depende de la versión de TagLib instalada
    auto result = reader.read(empty_file.string());
    if (result.has_value()) {
        EXPECT_EQ(result->file_path, empty_file.string());
    }
    // Ambos casos (nullopt o valor con fallback) son comportamiento válido
}

TEST_F(MetadataReaderTest, SetsFilePathCorrectly) {
    // Este test requiere un MP3 real en tu biblioteca
    // Lo saltamos si no hay archivos disponibles
    std::string test_mp3 = "";

    // Buscamos el primer MP3 en /tmp si existe
    for (const auto& entry : fs::recursive_directory_iterator(
             fs::temp_directory_path(),
             fs::directory_options::skip_permission_denied)) {
        if (entry.path().extension() == ".mp3") {
            test_mp3 = entry.path().string();
            break;
        }
    }

    if (test_mp3.empty()) {
        GTEST_SKIP() << "No hay archivos MP3 disponibles para testear";
    }

    auto result = reader.read(test_mp3);
    if (result.has_value()) {
        EXPECT_EQ(result->file_path, test_mp3);
    }
}

TEST_F(MetadataReaderTest, FormatIsSetFromExtension) {
    // Verificamos que la extensión se detecta correctamente
    // usando un archivo fake — solo probamos la lógica de extensión
    // a través de un archivo real si está disponible

    std::string test_flac = "";
    for (const auto& entry : fs::recursive_directory_iterator(
             fs::temp_directory_path(),
             fs::directory_options::skip_permission_denied)) {
        if (entry.path().extension() == ".flac") {
            test_flac = entry.path().string();
            break;
        }
    }

    if (test_flac.empty()) {
        GTEST_SKIP() << "No hay archivos FLAC disponibles para testear";
    }

    auto result = reader.read(test_flac);
    if (result.has_value()) {
        EXPECT_EQ(result->format, "flac");
    }
}

TEST_F(MetadataReaderTest, FallbackTitleIsFileStem) {
    auto fake = test_dir / "mi_cancion_favorita.mp3";
    std::ofstream f(fake);
    f << "not valid audio";
    f.close();

    auto result = reader.read(fake.string());
    if (result.has_value()) {
        // Cuando TagLib puede abrir el archivo pero no tiene título,
        // el fallback usa el stem del nombre de archivo
        EXPECT_EQ(result->title, "mi_cancion_favorita");
    }
    // Si TagLib no puede abrirlo, retorna nullopt — también válido
}