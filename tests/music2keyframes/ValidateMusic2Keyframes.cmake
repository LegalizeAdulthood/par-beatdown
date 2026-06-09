cmake_minimum_required(VERSION 3.23)

foreach(required_name IN ITEMS
        MUSIC2KEYFRAMES_TOOL
        MUSIC2KEYFRAMES_SOURCE_DIR
        MUSIC2KEYFRAMES_BASE
        MUSIC2KEYFRAMES_TIMELINE
        MUSIC2KEYFRAMES_CONFIG
        MUSIC2KEYFRAMES_EXPECTED_KEYFRAME_COUNT
        MUSIC2KEYFRAMES_EXPECTED
        MUSIC2KEYFRAMES_OUTPUT)
    if(NOT DEFINED ${required_name})
        message(FATAL_ERROR "missing ${required_name}")
    endif()
endforeach()

execute_process(
    COMMAND "${MUSIC2KEYFRAMES_TOOL}"
        "${MUSIC2KEYFRAMES_BASE}"
        "${MUSIC2KEYFRAMES_TIMELINE}"
        "${MUSIC2KEYFRAMES_CONFIG}"
        -o "${MUSIC2KEYFRAMES_OUTPUT}"
    WORKING_DIRECTORY "${MUSIC2KEYFRAMES_SOURCE_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "music2keyframes failed: ${result}\nstdout:\n${stdout}\nstderr:\n${stderr}")
endif()

file(READ "${MUSIC2KEYFRAMES_EXPECTED}" expected)
file(READ "${MUSIC2KEYFRAMES_OUTPUT}" actual)
string(REPLACE "\r\n" "\n" expected "${expected}")
string(REPLACE "\r\n" "\n" actual "${actual}")

function(fail_field_check field message)
    message(FATAL_ERROR "field check failed: ${field}: ${message}")
endfunction()

function(json_get_value output field)
    string(JSON value ERROR_VARIABLE json_error GET "${actual}" ${ARGN})
    if(NOT json_error STREQUAL "NOTFOUND")
        fail_field_check("${field}" "${json_error}")
    endif()
    set(${output} "${value}" PARENT_SCOPE)
endfunction()

function(json_array_length output field)
    string(JSON value ERROR_VARIABLE json_error LENGTH "${actual}" ${ARGN})
    if(NOT json_error STREQUAL "NOTFOUND")
        fail_field_check("${field}" "${json_error}")
    endif()
    set(${output} "${value}" PARENT_SCOPE)
endfunction()

function(assert_json_equal field expected_value)
    json_get_value(actual_value "${field}" ${ARGN})
    if(NOT "${actual_value}" STREQUAL "${expected_value}")
        fail_field_check("${field}"
            "expected '${expected_value}', actual '${actual_value}'")
    endif()
endfunction()

function(assert_json_number_equal field expected_value)
    json_get_value(actual_value "${field}" ${ARGN})
    if(NOT actual_value EQUAL expected_value)
        fail_field_check("${field}"
            "expected ${expected_value}, actual ${actual_value}")
    endif()
endfunction()

function(assert_json_array_empty field)
    json_array_length(actual_length "${field}" ${ARGN})
    if(NOT actual_length EQUAL 0)
        fail_field_check("${field}" "expected empty array")
    endif()
endfunction()

function(assert_json_array_length field expected_value)
    json_array_length(actual_length "${field}" ${ARGN})
    if(NOT actual_length EQUAL expected_value)
        fail_field_check("${field}"
            "expected ${expected_value}, actual ${actual_length}")
    endif()
endfunction()

function(assert_supported_op value field)
    if(NOT "${value}" STREQUAL "add"
            AND NOT "${value}" STREQUAL "multiply"
            AND NOT "${value}" STREQUAL "replace")
        fail_field_check("${field}" "unsupported op '${value}'")
    endif()
endfunction()

function(assert_supported_source value field)
    if(NOT "${value}" STREQUAL "music.rms"
            AND NOT "${value}" STREQUAL "music.peak")
        fail_field_check("${field}" "unsupported source '${value}'")
    endif()
endfunction()

function(assert_keyframes_valid)
    json_array_length(keyframe_count "keyframes" keyframes)
    if(keyframe_count EQUAL 0)
        return()
    endif()

    set(previous_frame -1)
    math(EXPR last_index "${keyframe_count} - 1")
    foreach(index RANGE 0 ${last_index})
        json_get_value(frame "keyframes[${index}].frame"
            keyframes ${index} frame)
        if(frame LESS previous_frame)
            fail_field_check("keyframes[${index}].frame"
                "frames are not sorted")
        endif()
        set(previous_frame ${frame})

        json_get_value(target "keyframes[${index}].target"
            keyframes ${index} target)
        if("${target}" STREQUAL "")
            fail_field_check("keyframes[${index}].target"
                "target must not be empty")
        endif()

        json_get_value(op "keyframes[${index}].op" keyframes ${index} op)
        assert_supported_op("${op}" "keyframes[${index}].op")
        json_get_value(value "keyframes[${index}].value"
            keyframes ${index} value)
        json_get_value(source "keyframes[${index}].source"
            keyframes ${index} source)
        assert_supported_source("${source}" "keyframes[${index}].source")
    endforeach()
endfunction()

assert_json_equal("schema" "par-beatdown.music2keyframes-overlay" schema)
assert_json_number_equal("version" 1 version)
assert_json_equal("generator.name" "music2keyframes" generator name)
assert_json_equal("generator.version" "0.1.0" generator version)
assert_json_equal("source.base_animation" "${MUSIC2KEYFRAMES_BASE}"
    source base_animation)
assert_json_equal("source.timeline" "${MUSIC2KEYFRAMES_TIMELINE}"
    source timeline)
assert_json_equal("source.adapter_config" "${MUSIC2KEYFRAMES_CONFIG}"
    source adapter_config)
assert_json_array_length("keyframes"
    "${MUSIC2KEYFRAMES_EXPECTED_KEYFRAME_COUNT}" keyframes)
assert_keyframes_valid()
assert_json_array_empty("diagnostics.warnings" diagnostics warnings)

if(NOT expected STREQUAL actual)
    message(FATAL_ERROR
        "music2keyframes output differs from gold JSON\n"
        "expected: ${MUSIC2KEYFRAMES_EXPECTED}\n"
        "actual: ${MUSIC2KEYFRAMES_OUTPUT}")
endif()
