cmake_minimum_required(VERSION 3.14)
project(localstream VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(TAGLIB REQUIRED taglib)

include(FetchContent)
# Asio standalone (requerido por Crow)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-28-0
)
FetchContent_MakeAvailable(asio)

# Le decimos a Crow que use Asio standalone, no Boost
set(CROW_FEATURES standalone)
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include)

FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.2.0
)
FetchContent_MakeAvailable(crow)

FetchContent_Declare(
    sqlitecpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.3.1
)
FetchContent_MakeAvailable(sqlitecpp)

# nlohmann/json (header-only, un solo archivo)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(localstream
    src/main.cpp
    src/config/AppConfig.cpp
    src/db/Database.cpp
    src/scanner/FileScanner.cpp
    src/scanner/LibraryScanner.cpp
    src/metadata/MetadataReader.cpp
    src/api/ApiRouter.cpp
    src/api/WebHandler.cpp
    src/streaming/StreamHandler.cpp
)
target_include_directories(localstream PRIVATE
    src/
    ${SQLITE3_INCLUDE_DIRS}
    ${TAGLIB_INCLUDE_DIRS}
)

target_link_libraries(localstream PRIVATE
    Crow::Crow
    SQLiteCpp
    ${SQLITE3_LIBRARIES}
    ${TAGLIB_LIBRARIES}
    nlohmann_json::nlohmann_json
)