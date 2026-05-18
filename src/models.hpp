#pragma once

#include <string>

namespace localstream {

struct Artist {
    int         id;
    std::string name;
};

struct Album {
    int         id;
    std::string title;
    int         artist_id;
    int         year;
};

struct Track {
    int         id;
    std::string title;
    int         artist_id;
    int         album_id;
    std::string file_path;
    int         duration_s;
    int         track_number;
    int         file_size;
    std::string format;
};

} // namespace localstream