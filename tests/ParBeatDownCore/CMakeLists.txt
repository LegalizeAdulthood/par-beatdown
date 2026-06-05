include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-ParBeatDownCore test.cpp)
target_link_libraries(test-ParBeatDownCore PUBLIC
    GTest::gtest_main
    ParBeatDownCore)
target_compile_definitions(test-ParBeatDownCore PRIVATE
    PAR_BEATDOWN_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/data")
target_folder(test-ParBeatDownCore "Tests")

gtest_discover_tests(test-ParBeatDownCore)
