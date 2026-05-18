add_executable(localstream_tests
    test_database.cpp
    test_metadata_reader.cpp
    ${CMAKE_SOURCE_DIR}/src/db/Database.cpp
    ${CMAKE_SOURCE_DIR}/src/metadata/MetadataReader.cpp
)

target_include_directories(localstream_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SQLITE3_INCLUDE_DIRS}
    ${TAGLIB_INCLUDE_DIRS}
)

target_link_libraries(localstream_tests PRIVATE
    GTest::gtest_main
    SQLiteCpp
    ${SQLITE3_LIBRARIES}
    ${TAGLIB_LIBRARIES}
)

include(GoogleTest)
gtest_discover_tests(localstream_tests)