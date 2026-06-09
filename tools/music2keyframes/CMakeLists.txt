add_executable(music2keyframes
    main.cpp
)
target_link_libraries(music2keyframes PRIVATE
    ParBeatDownCore
    nlohmann_json::nlohmann_json)
target_folder(music2keyframes "Tools")
