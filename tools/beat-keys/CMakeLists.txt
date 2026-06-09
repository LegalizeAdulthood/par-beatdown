add_executable(beat-keys
    main.cpp
)
target_link_libraries(beat-keys PRIVATE
    ParBeatDownCore
    nlohmann_json::nlohmann_json)
target_folder(beat-keys "Tools")
