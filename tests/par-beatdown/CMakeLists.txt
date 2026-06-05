if(PAR_BEATDOWN_ENABLE_TRACKER_FILE)
    add_test(NAME par-beatdown.writeTrackerTimeline
        COMMAND ${CMAKE_COMMAND}
            -DPAR_BEATDOWN_TOOL=$<TARGET_FILE:par-beatdown>
            -DPAR_BEATDOWN_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DPAR_BEATDOWN_INPUT=data/my_neighbors_kid_is_an_internet_addict.xm
            -DPAR_BEATDOWN_EXPECTED=${CMAKE_CURRENT_LIST_DIR}/gold-write-tracker-timeline.json
            -DPAR_BEATDOWN_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/write-tracker-timeline.actual.json
            -P ${CMAKE_CURRENT_LIST_DIR}/ValidateTrackerTimeline.cmake)
    set_tests_properties(par-beatdown.writeTrackerTimeline PROPERTIES
        LABELS par-beatdown)
endif()
