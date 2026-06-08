if(PAR_BEATDOWN_ENABLE_TRACKER_FILE)
    function(add_par_beatdown_gold_test test_name include_name)
        cmake_parse_arguments(gold "" "FPS;OFFSET;FEATURE_HOP" "" ${ARGN})
        string(REPLACE "." "-" output_name "${test_name}")
        set(option_definitions)
        if(DEFINED gold_FPS)
            list(APPEND option_definitions -DPAR_BEATDOWN_FPS=${gold_FPS})
        endif()
        if(DEFINED gold_OFFSET)
            list(APPEND option_definitions -DPAR_BEATDOWN_OFFSET=${gold_OFFSET})
        endif()
        if(DEFINED gold_FEATURE_HOP)
            list(APPEND option_definitions
                -DPAR_BEATDOWN_FEATURE_HOP=${gold_FEATURE_HOP})
        endif()
        add_test(NAME par-beatdown.${test_name}
            COMMAND ${CMAKE_COMMAND}
                -DPAR_BEATDOWN_TOOL=$<TARGET_FILE:par-beatdown>
                -DPAR_BEATDOWN_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                -DPAR_BEATDOWN_INPUT=data/my_neighbors_kid_is_an_internet_addict.xm
                -DPAR_BEATDOWN_INCLUDE=${include_name}
                -DPAR_BEATDOWN_EXPECTED=${CMAKE_CURRENT_LIST_DIR}/gold-${output_name}.json
                -DPAR_BEATDOWN_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${output_name}.actual.json
                ${option_definitions}
                -P ${CMAKE_CURRENT_LIST_DIR}/ValidateTrackerTimeline.cmake)
        set_tests_properties(par-beatdown.${test_name} PROPERTIES
            LABELS par-beatdown)
    endfunction()

    add_par_beatdown_gold_test(write-tracker-source source)
    add_par_beatdown_gold_test(write-module-structure module)
    add_par_beatdown_gold_test(write-timeline-clock timeline)
    add_par_beatdown_gold_test(write-pattern-events events)
    add_par_beatdown_gold_test(write-feature-frames features)
    add_par_beatdown_gold_test(write-custom-fps source FPS 24)
    add_par_beatdown_gold_test(write-timeline-custom-fps timeline FPS 24)
    add_par_beatdown_gold_test(write-sync-offset source OFFSET 1.25)
    add_par_beatdown_gold_test(write-custom-feature-hop features
        FEATURE_HOP 0.5)
endif()
