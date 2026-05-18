#include <gtest/gtest.h>
#include "db/Database.hpp"

using namespace localstream;

// Fixture — crea una DB en memoria antes de cada test
class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_unique<Database>(":memory:");
    }

    std::unique_ptr<Database> db;
};

// ── insertArtist
TEST_F(DatabaseTest, InsertArtistReturnsPositiveId) {
    int id = db->insertArtist("The Beatles");
    EXPECT_GT(id, 0);
}

TEST_F(DatabaseTest, InsertSameArtistReturnsSameId) {
    int id1 = db->insertArtist("The Beatles");
    int id2 = db->insertArtist("The Beatles");
    EXPECT_EQ(id1, id2);
}

TEST_F(DatabaseTest, InsertDifferentArtistsReturnDifferentIds) {
    int id1 = db->insertArtist("The Beatles");
    int id2 = db->insertArtist("Pink Floyd");
    EXPECT_NE(id1, id2);
}

// ── insertAlbum
TEST_F(DatabaseTest, InsertAlbumReturnsPositiveId) {
    int artist_id = db->insertArtist("The Beatles");
    int album_id  = db->insertAlbum("Abbey Road", artist_id, 1969);
    EXPECT_GT(album_id, 0);
}

TEST_F(DatabaseTest, InsertSameAlbumReturnsSameId) {
    int artist_id = db->insertArtist("The Beatles");
    int id1 = db->insertAlbum("Abbey Road", artist_id, 1969);
    int id2 = db->insertAlbum("Abbey Road", artist_id, 1969);
    EXPECT_EQ(id1, id2);
}

TEST_F(DatabaseTest, TwoArtistsCanHaveSameAlbumTitle) {
    int artist1 = db->insertArtist("Artist One");
    int artist2 = db->insertArtist("Artist Two");
    int album1  = db->insertAlbum("Greatest Hits", artist1, 2000);
    int album2  = db->insertAlbum("Greatest Hits", artist2, 2001);
    EXPECT_NE(album1, album2);
}

// ── insertTrack
TEST_F(DatabaseTest, InsertTrackReturnsPositiveId) {
    int artist_id = db->insertArtist("The Beatles");
    int album_id  = db->insertAlbum("Abbey Road", artist_id, 1969);

    Track track;
    track.title        = "Come Together";
    track.artist_id    = artist_id;
    track.album_id     = album_id;
    track.file_path    = "/music/come_together.mp3";
    track.duration_s   = 259;
    track.track_number = 1;
    track.file_size    = 1024000;
    track.format       = "mp3";

    bool inserted = false;
    int id = db->insertTrack(track, inserted);

    EXPECT_GT(id, 0);
    EXPECT_TRUE(inserted);
}

TEST_F(DatabaseTest, InsertDuplicateTrackNotInserted) {
    int artist_id = db->insertArtist("The Beatles");
    int album_id  = db->insertAlbum("Abbey Road", artist_id, 1969);

    Track track;
    track.title        = "Come Together";
    track.artist_id    = artist_id;
    track.album_id     = album_id;
    track.file_path    = "/music/come_together.mp3";
    track.duration_s   = 259;
    track.track_number = 1;
    track.file_size    = 1024000;
    track.format       = "mp3";

    bool inserted1 = false;
    bool inserted2 = false;
    db->insertTrack(track, inserted1);
    db->insertTrack(track, inserted2);

    EXPECT_TRUE(inserted1);
    EXPECT_FALSE(inserted2);
}

// ── getArtists
TEST_F(DatabaseTest, GetArtistsReturnsAllArtists) {
    db->insertArtist("The Beatles");
    db->insertArtist("Pink Floyd");
    db->insertArtist("Radiohead");

    auto artists = db->getArtists();
    EXPECT_EQ(artists.size(), 3u);
}

TEST_F(DatabaseTest, GetArtistsEmptyWhenNoArtists) {
    auto artists = db->getArtists();
    EXPECT_TRUE(artists.empty());
}

TEST_F(DatabaseTest, GetArtistsReturnsSortedByName) {
    db->insertArtist("Radiohead");
    db->insertArtist("The Beatles");
    db->insertArtist("Pink Floyd");

    auto artists = db->getArtists();
    EXPECT_EQ(artists[0].name, "Pink Floyd");
    EXPECT_EQ(artists[1].name, "Radiohead");
    EXPECT_EQ(artists[2].name, "The Beatles");
}

// ── getTrackById
TEST_F(DatabaseTest, GetTrackByIdReturnsCorrectTrack) {
    int artist_id = db->insertArtist("The Beatles");
    int album_id  = db->insertAlbum("Abbey Road", artist_id, 1969);

    Track track;
    track.title        = "Come Together";
    track.artist_id    = artist_id;
    track.album_id     = album_id;
    track.file_path    = "/music/come_together.mp3";
    track.duration_s   = 259;
    track.track_number = 1;
    track.file_size    = 1024000;
    track.format       = "mp3";

    bool inserted = false;
    int id = db->insertTrack(track, inserted);

    auto result = db->getTrackById(id);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->title, "Come Together");
    EXPECT_EQ(result->duration_s, 259);
    EXPECT_EQ(result->format, "mp3");
}

TEST_F(DatabaseTest, GetTrackByIdReturnsNulloptForMissingTrack) {
    auto result = db->getTrackById(9999);
    EXPECT_FALSE(result.has_value());
}

// ── searchTracks
TEST_F(DatabaseTest, SearchTracksFindsMatchingTitle) {
    int artist_id = db->insertArtist("The Beatles");
    int album_id  = db->insertAlbum("Abbey Road", artist_id, 1969);

    Track track;
    track.title        = "Come Together";
    track.artist_id    = artist_id;
    track.album_id     = album_id;
    track.file_path    = "/music/come_together.mp3";
    track.duration_s   = 259;
    track.track_number = 1;
    track.file_size    = 1024000;
    track.format       = "mp3";

    bool inserted = false;
    db->insertTrack(track, inserted);

    auto results = db->searchTracks("together");
    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].title, "Come Together");
}

TEST_F(DatabaseTest, SearchTracksIsCaseInsensitive) {
    int artist_id = db->insertArtist("The Beatles");
    int album_id  = db->insertAlbum("Abbey Road", artist_id, 1969);

    Track track;
    track.title        = "Come Together";
    track.artist_id    = artist_id;
    track.album_id     = album_id;
    track.file_path    = "/music/come_together.mp3";
    track.duration_s   = 259;
    track.track_number = 1;
    track.file_size    = 1024000;
    track.format       = "mp3";

    bool inserted = false;
    db->insertTrack(track, inserted);

    auto results = db->searchTracks("TOGETHER");
    EXPECT_EQ(results.size(), 1u);
}

TEST_F(DatabaseTest, SearchTracksReturnsEmptyForNoMatch) {
    auto results = db->searchTracks("xyznotfound");
    EXPECT_TRUE(results.empty());
}