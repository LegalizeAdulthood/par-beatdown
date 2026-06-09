function(add_beat_keys_gold_test test_name)
    cmake_parse_arguments(gold "" "CONFIG;KEYFRAME_COUNT;TIMELINE" "" ${ARGN})
    string(REPLACE "." "-" output_name "${test_name}")
    if(NOT DEFINED gold_CONFIG)
        set(gold_CONFIG tests/beat-keys/adapter.beat-keys.json)
    endif()
    if(NOT DEFINED gold_KEYFRAME_COUNT)
        set(gold_KEYFRAME_COUNT 0)
    endif()
    if(NOT DEFINED gold_TIMELINE)
        set(gold_TIMELINE tests/par-beatdown/gold-write-tracker-source.json)
    endif()
    add_test(NAME beat-keys.${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DBEAT_KEYS_TOOL=$<TARGET_FILE:beat-keys>
            -DBEAT_KEYS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DBEAT_KEYS_BASE=tests/beat-keys/base-animation.json
            -DBEAT_KEYS_TIMELINE=${gold_TIMELINE}
            -DBEAT_KEYS_CONFIG=${gold_CONFIG}
            -DBEAT_KEYS_EXPECTED_KEYFRAME_COUNT=${gold_KEYFRAME_COUNT}
            -DBEAT_KEYS_EXPECTED=${CMAKE_CURRENT_LIST_DIR}/gold-${output_name}.json
            -DBEAT_KEYS_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${output_name}.actual.json
            -P ${CMAKE_CURRENT_LIST_DIR}/ValidateBeatKeys.cmake)
    set_tests_properties(beat-keys.${test_name} PROPERTIES
        LABELS beat-keys)
endfunction()

function(add_beat_keys_merge_test test_name)
    cmake_parse_arguments(merge ""
        "BASE;CONFIG;EXPECTED_GENERATED_SOURCE;EXPECTED_TRACK_COUNT;TIMELINE"
        "" ${ARGN})
    string(REPLACE "." "-" output_name "${test_name}")
    if(NOT DEFINED merge_BASE)
        set(merge_BASE tests/beat-keys/base-merge-animation.json)
    endif()
    add_test(NAME beat-keys.${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DBEAT_KEYS_TOOL=$<TARGET_FILE:beat-keys>
            -DBEAT_KEYS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DBEAT_KEYS_BASE=${merge_BASE}
            -DBEAT_KEYS_TIMELINE=${merge_TIMELINE}
            -DBEAT_KEYS_CONFIG=${merge_CONFIG}
            -DBEAT_KEYS_EXPECTED_GENERATED_SOURCE=${merge_EXPECTED_GENERATED_SOURCE}
            -DBEAT_KEYS_EXPECTED_TRACK_COUNT=${merge_EXPECTED_TRACK_COUNT}
            -DBEAT_KEYS_EXPECTED=${CMAKE_CURRENT_LIST_DIR}/gold-${output_name}.json
            -DBEAT_KEYS_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${output_name}.actual.json
            -P ${CMAKE_CURRENT_LIST_DIR}/ValidateBeatKeysMerge.cmake)
    set_tests_properties(beat-keys.${test_name} PROPERTIES
        LABELS beat-keys)
endfunction()

function(add_beat_keys_error_test test_name expected_stderr)
    cmake_parse_arguments(error "" "" "ARGUMENTS" ${ARGN})
    if(DEFINED error_ARGUMENTS)
        add_test(NAME beat-keys.${test_name}
            COMMAND ${CMAKE_COMMAND}
                -DBEAT_KEYS_TOOL=$<TARGET_FILE:beat-keys>
                -DBEAT_KEYS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                "-DBEAT_KEYS_EXPECTED_STDERR=${expected_stderr}"
                "-DBEAT_KEYS_ARGUMENTS=${error_ARGUMENTS}"
                -P ${CMAKE_CURRENT_LIST_DIR}/ValidateToolFailure.cmake)
    else()
        add_test(NAME beat-keys.${test_name}
            COMMAND ${CMAKE_COMMAND}
                -DBEAT_KEYS_TOOL=$<TARGET_FILE:beat-keys>
                -DBEAT_KEYS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                "-DBEAT_KEYS_EXPECTED_STDERR=${expected_stderr}"
                -P ${CMAKE_CURRENT_LIST_DIR}/ValidateToolFailure.cmake)
    endif()
    set_tests_properties(beat-keys.${test_name} PROPERTIES
        LABELS beat-keys)
endfunction()

add_beat_keys_gold_test(write-empty-overlay)
add_beat_keys_gold_test(write-rms-keyframes
    CONFIG tests/beat-keys/rms.beat-keys.json
    KEYFRAME_COUNT 9
    TIMELINE tests/beat-keys/timeline-features.json)
add_beat_keys_gold_test(write-peak-keyframes
    CONFIG tests/beat-keys/peak.beat-keys.json
    KEYFRAME_COUNT 3
    TIMELINE tests/beat-keys/timeline-features.json)
add_beat_keys_gold_test(write-note-pulses
    CONFIG tests/beat-keys/note-pulses.beat-keys.json
    KEYFRAME_COUNT 8
    TIMELINE tests/beat-keys/timeline-events.json)
add_beat_keys_gold_test(write-row-pulses
    CONFIG tests/beat-keys/row-pulses.beat-keys.json
    KEYFRAME_COUNT 4
    TIMELINE tests/beat-keys/timeline-events.json)
add_beat_keys_merge_test(merge-rms-overlay
    CONFIG tests/beat-keys/merge-rms.beat-keys.json
    EXPECTED_GENERATED_SOURCE music.rms
    EXPECTED_TRACK_COUNT 2
    TIMELINE tests/beat-keys/timeline-features.json)
add_beat_keys_merge_test(merge-event-overlay
    CONFIG tests/beat-keys/merge-event.beat-keys.json
    EXPECTED_GENERATED_SOURCE music.note_pulse
    EXPECTED_TRACK_COUNT 2
    TIMELINE tests/beat-keys/timeline-events.json)

add_beat_keys_error_test(fail-missing-base-animation-path
    "missing base animation path")
add_beat_keys_error_test(fail-missing-timeline-path
    "missing timeline path"
    ARGUMENTS
        tests/beat-keys/base-animation.json)
add_beat_keys_error_test(fail-missing-adapter-config-path
    "missing adapter config path"
    ARGUMENTS
        tests/beat-keys/base-animation.json
        tests/par-beatdown/gold-write-tracker-source.json)
add_beat_keys_error_test(fail-missing-base-animation
    "could not open base animation"
    ARGUMENTS
        tests/beat-keys/missing-base-animation.json
        tests/par-beatdown/gold-write-tracker-source.json
        tests/beat-keys/adapter.beat-keys.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/missing-base-animation.actual.json)
add_beat_keys_error_test(fail-missing-timeline
    "could not open timeline"
    ARGUMENTS
        tests/beat-keys/base-animation.json
        tests/beat-keys/missing-timeline.json
        tests/beat-keys/adapter.beat-keys.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/missing-timeline.actual.json)
add_beat_keys_error_test(fail-missing-adapter-config
    "could not open adapter config"
    ARGUMENTS
        tests/beat-keys/base-animation.json
        tests/par-beatdown/gold-write-tracker-source.json
        tests/beat-keys/missing-adapter.beat-keys.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/missing-adapter-config.actual.json)
add_beat_keys_error_test(fail-invalid-adapter-config-schema
    "unsupported adapter config schema"
    ARGUMENTS
        tests/beat-keys/base-animation.json
        tests/par-beatdown/gold-write-tracker-source.json
        tests/beat-keys/invalid-schema.beat-keys.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-schema.actual.json)
add_beat_keys_error_test(fail-invalid-adapter-config-version
    "unsupported adapter config version"
    ARGUMENTS
        tests/beat-keys/base-animation.json
        tests/par-beatdown/gold-write-tracker-source.json
        tests/beat-keys/invalid-version.beat-keys.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-version.actual.json)
add_beat_keys_error_test(fail-invalid-continuous-source
    "unsupported continuous source"
    ARGUMENTS
        tests/beat-keys/base-animation.json
        tests/beat-keys/timeline-features.json
        tests/beat-keys/invalid-source.beat-keys.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-source.actual.json)
add_beat_keys_error_test(fail-invalid-event-source
    "unsupported event source"
    ARGUMENTS
        tests/beat-keys/base-animation.json
        tests/beat-keys/timeline-events.json
        tests/beat-keys/invalid-event-source.beat-keys.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-event-source.actual.json)
add_beat_keys_error_test(fail-generated-id-conflict
    "generated track id conflict"
    ARGUMENTS
        tests/beat-keys/base-id-conflict-animation.json
        tests/beat-keys/timeline-features.json
        tests/beat-keys/merge-rms.beat-keys.json
        -o ${CMAKE_CURRENT_BINARY_DIR}/generated-id-conflict.actual.json)
