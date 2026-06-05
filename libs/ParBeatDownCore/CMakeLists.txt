add_library(ParBeatDownCore
    include/ParBeatDownCore/version.h
    version.cpp
)
target_include_directories(ParBeatDownCore PUBLIC include)
if(PAR_BEATDOWN_ENABLE_TRACKER_FILE)
    target_link_libraries(ParBeatDownCore PUBLIC libopenmpt::libopenmpt)
    target_compile_definitions(ParBeatDownCore PUBLIC PAR_BEATDOWN_ENABLE_TRACKER_FILE)
endif()
target_folder(ParBeatDownCore "Libraries")
