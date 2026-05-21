#include "metadata/MetadataReader.hpp"

#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <algorithm>
#include <filesystem>

namespace localstream {

namespace fs = std::filesystem;

std::optional<TrackMetadata> MetadataReader::read(
    const std::string& file_path) {
  // 1. Abrir el archivo con TagLib
  TagLib::FileRef file(file_path.c_str());

  if (file.isNull() || !file.tag()) {
    return std::nullopt;
  }

  TagLib::Tag* tag = file.tag();
  TagLib::AudioProperties* props = file.audioProperties();

  // 2. Extraer metadata
  TrackMetadata metadata;
  metadata.file_path = file_path;
  metadata.title = tag->title().toCString(true);
  std::string full_artist = tag->artist().toCString(true);
  // Tomamos solo el primer artista antes de la coma
  auto comma = full_artist.find(',');
  metadata.artist_name =
      (comma != std::string::npos) ? full_artist.substr(0, comma) : full_artist;
  // Trim de espacios al final
  while (!metadata.artist_name.empty() && metadata.artist_name.back() == ' ')
    metadata.artist_name.pop_back();
  metadata.album_title = tag->album().toCString(true);
  metadata.year = static_cast<int>(tag->year());
  metadata.track_number = static_cast<int>(tag->track());
  metadata.duration_s = props ? props->length() : 0;
  metadata.file_size = static_cast<int>(fs::file_size(file_path));

  // 3. Formato según extensión
  std::string ext = fs::path(file_path).extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  if (ext == ".mp3")
    metadata.format = "mp3";
  else if (ext == ".flac")
    metadata.format = "flac";
  else if (ext == ".ogg")
    metadata.format = "ogg";
  else if (ext == ".opus")
    metadata.format = "opus";
  else if (ext == ".m4a" || ext == ".aac")
    metadata.format = "aac";
  else if (ext == ".wav")
    metadata.format = "wav";
  else
    metadata.format = ext.substr(1);

  // 4. Fallbacks para metadata faltante
  if (metadata.title.empty())
    metadata.title = fs::path(file_path).stem().string();
  if (metadata.artist_name.empty())
    metadata.artist_name = "Artista Desconocido";
  if (metadata.album_title.empty()) metadata.album_title = "Album Desconocido";

  return metadata;
}

}  // namespace localstream