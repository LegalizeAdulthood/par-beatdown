add_library(ParBeatDownCore
    include/ParBeatdown/Version.h
    Version.cpp
)
if(PAR_BEATDOWN_ENABLE_TRACKER_FILE)
    target_sources(ParBeatDownCore PRIVATE
        include/ParBeatdown/TrackerTimeline.h
        TrackerTimeline.cpp
    )
endif()
target_include_directories(ParBeatDownCore PUBLIC include)
if(PAR_BEATDOWN_ENABLE_TRACKER_FILE)
    target_link_libraries(ParBeatDownCore PUBLIC
        libopenmpt::libopenmpt
        nlohmann_json::nlohmann_json)
    target_compile_definitions(ParBeatDownCore PUBLIC PAR_BEATDOWN_ENABLE_TRACKER_FILE)
endif()
target_folder(ParBeatDownCore "Libraries")
