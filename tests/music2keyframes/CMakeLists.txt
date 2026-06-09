function(add_music2keyframes_gold_test test_name)
    cmake_parse_arguments(gold "" "CONFIG;KEYFRAME_COUNT;TIMELINE" "" ${ARGN})
    string(REPLACE "." "-" output_name "${test_name}")
    if(NOT DEFINED gold_CONFIG)
        set(gold_CONFIG tests/music2keyframes/adapter.music2keyframes.json)
    endif()
    if(NOT DEFINED gold_KEYFRAME_COUNT)
        set(gold_KEYFRAME_COUNT 0)
    endif()
    if(NOT DEFINED gold_TIMELINE)
        set(gold_TIMELINE tests/par-beatdown/gold-write-tracker-source.json)
    endif()
    add_test(NAME music2keyframes.${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DMUSIC2KEYFRAMES_TOOL=$<TARGET_FILE:music2keyframes>
            -DMUSIC2KEYFRAMES_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DMUSIC2KEYFRAMES_BASE=tests/music2keyframes/base-animation.json
            -DMUSIC2KEYFRAMES_TIMELINE=${gold_TIMELINE}
            -DMUSIC2KEYFRAMES_CONFIG=${gold_CONFIG}
            -DMUSIC2KEYFRAMES_EXPECTED_KEYFRAME_COUNT=${gold_KEYFRAME_COUNT}
            -DMUSIC2KEYFRAMES_EXPECTED=${CMAKE_CURRENT_LIST_DIR}/gold-${output_name}.json
            -DMUSIC2KEYFRAMES_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${output_name}.actual.json
            -P ${CMAKE_CURRENT_LIST_DIR}/ValidateMusic2Keyframes.cmake)
    set_tests_properties(music2keyframes.${test_name} PROPERTIES
        LABELS music2keyframes)
endfunction()

function(add_music2keyframes_error_test test_name expected_stderr)
    cmake_parse_arguments(error "" "" "ARGUMENTS" ${ARGN})
    if(DEFINED error_ARGUMENTS)
        add_test(NAME music2keyframes.${test_name}
            COMMAND ${CMAKE_COMMAND}
                -DMUSIC2KEYFRAMES_TOOL=$<TARGET_FILE:music2keyframes>
                -DMUSIC2KEYFRAMES_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                "-DMUSIC2KEYFRAMES_EXPECTED_STDERR=${expected_stderr}"
                "-DMUSIC2KEYFRAMES_ARGUMENTS=${error_ARGUMENTS}"
                -P ${CMAKE_CURRENT_LIST_DIR}/ValidateToolFailure.cmake)
    else()
        add_test(NAME music2keyframes.${test_name}
            COMMAND ${CMAKE_COMMAND}
                -DMUSIC2KEYFRAMES_TOOL=$<TARGET_FILE:music2keyframes>
                -DMUSIC2KEYFRAMES_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                "-DMUSIC2KEYFRAMES_EXPECTED_STDERR=${expected_stderr}"
                -P ${CMAKE_CURRENT_LIST_DIR}/ValidateToolFailure.cmake)
    endif()
    set_tests_properties(music2keyframes.${test_name} PROPERTIES
        LABELS music2keyframes)
endfunction()

add_music2keyframes_gold_test(write-empty-overlay)
add_music2keyframes_gold_test(write-rms-keyframes
    CONFIG tests/music2keyframes/rms.music2keyframes.json
    KEYFRAME_COUNT 9
    TIMELINE tests/music2keyframes/timeline-features.json)
add_music2keyframes_gold_test(write-peak-keyframes
    CONFIG tests/music2keyframes/peak.music2keyframes.json
    KEYFRAME_COUNT 3
    TIMELINE tests/music2keyframes/timeline-features.json)

add_music2keyframes_error_test(fail-missing-base-animation-path
    "missing base animation path")
add_music2keyframes_error_test(fail-missing-timeline-path
    "missing timeline path"
    ARGUMENTS
        tests/music2keyframes/base-animation.json)
add_music2keyframes_error_test(fail-missing-adapter-config-path
    "missing adapter config path"
    ARGUMENTS
        tests/music2keyframes/base-animation.json
        tests/par-beatdown/gold-write-tracker-source.json)
add_music2keyframes_error_test(fail-missing-base-animation
    "could not open base animation"
    ARGUMENTS
        tests/music2keyframes/missing-base-animation.json
        tests/par-beatdown/gold-write-tracker-source.json
        tests/music2keyframes/adapter.music2keyframes.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/missing-base-animation.actual.json)
add_music2keyframes_error_test(fail-missing-timeline
    "could not open timeline"
    ARGUMENTS
        tests/music2keyframes/base-animation.json
        tests/music2keyframes/missing-timeline.json
        tests/music2keyframes/adapter.music2keyframes.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/missing-timeline.actual.json)
add_music2keyframes_error_test(fail-missing-adapter-config
    "could not open adapter config"
    ARGUMENTS
        tests/music2keyframes/base-animation.json
        tests/par-beatdown/gold-write-tracker-source.json
        tests/music2keyframes/missing-adapter.music2keyframes.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/missing-adapter-config.actual.json)
add_music2keyframes_error_test(fail-invalid-adapter-config-schema
    "unsupported adapter config schema"
    ARGUMENTS
        tests/music2keyframes/base-animation.json
        tests/par-beatdown/gold-write-tracker-source.json
        tests/music2keyframes/invalid-schema.music2keyframes.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-schema.actual.json)
add_music2keyframes_error_test(fail-invalid-adapter-config-version
    "unsupported adapter config version"
    ARGUMENTS
        tests/music2keyframes/base-animation.json
        tests/par-beatdown/gold-write-tracker-source.json
        tests/music2keyframes/invalid-version.music2keyframes.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-version.actual.json)
add_music2keyframes_error_test(fail-invalid-continuous-source
    "unsupported continuous source"
    ARGUMENTS
        tests/music2keyframes/base-animation.json
        tests/music2keyframes/timeline-features.json
        tests/music2keyframes/invalid-source.music2keyframes.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-source.actual.json)
