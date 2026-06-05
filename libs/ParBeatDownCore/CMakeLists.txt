add_library(ParBeatDownCore
    include/ParBeatDownCore/version.h
    version.cpp
)
target_include_directories(ParBeatDownCore PUBLIC include)
target_folder(ParBeatDownCore "Libraries")
