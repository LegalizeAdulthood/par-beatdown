include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-ParBeatDownCore test.cpp)
target_link_libraries(test-ParBeatDownCore PUBLIC
    GTest::gtest_main
    ParBeatDownCore)
target_folder(test-ParBeatDownCore "Tests")

gtest_discover_tests(test-ParBeatDownCore)
