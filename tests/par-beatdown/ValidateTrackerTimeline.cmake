cmake_minimum_required(VERSION 3.23)

foreach(required_name IN ITEMS
        PAR_BEATDOWN_TOOL
        PAR_BEATDOWN_SOURCE_DIR
        PAR_BEATDOWN_INPUT
        PAR_BEATDOWN_EXPECTED
        PAR_BEATDOWN_OUTPUT)
    if(NOT DEFINED ${required_name})
        message(FATAL_ERROR "missing ${required_name}")
    endif()
endforeach()

set(command_arguments "${PAR_BEATDOWN_INPUT}" -o "${PAR_BEATDOWN_OUTPUT}")
if(DEFINED PAR_BEATDOWN_INCLUDE)
    foreach(include_name IN LISTS PAR_BEATDOWN_INCLUDE)
        list(APPEND command_arguments --include "${include_name}")
    endforeach()
endif()
if(DEFINED PAR_BEATDOWN_FPS)
    list(APPEND command_arguments --fps "${PAR_BEATDOWN_FPS}")
endif()
if(DEFINED PAR_BEATDOWN_OFFSET)
    list(APPEND command_arguments --offset "${PAR_BEATDOWN_OFFSET}")
endif()
if(DEFINED PAR_BEATDOWN_FEATURE_HOP)
    list(APPEND command_arguments --feature-hop "${PAR_BEATDOWN_FEATURE_HOP}")
endif()

execute_process(
    COMMAND "${PAR_BEATDOWN_TOOL}" ${command_arguments}
    WORKING_DIRECTORY "${PAR_BEATDOWN_SOURCE_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "par-beatdown failed: ${result}\nstdout:\n${stdout}\nstderr:\n${stderr}")
endif()

file(READ "${PAR_BEATDOWN_EXPECTED}" expected)
file(READ "${PAR_BEATDOWN_OUTPUT}" actual)
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

function(assert_json_number_greater field minimum_value)
    json_get_value(actual_value "${field}" ${ARGN})
    if(NOT actual_value GREATER minimum_value)
        fail_field_check("${field}"
            "expected greater than ${minimum_value}, actual ${actual_value}")
    endif()
endfunction()

function(assert_json_array_not_empty field)
    json_array_length(actual_length "${field}" ${ARGN})
    if(NOT actual_length GREATER 0)
        fail_field_check("${field}" "expected non-empty array")
    endif()
endfunction()

function(has_include output include_name)
    set(found FALSE)
    if(DEFINED PAR_BEATDOWN_INCLUDE)
        list(FIND PAR_BEATDOWN_INCLUDE "${include_name}" include_index)
        if(NOT include_index EQUAL -1)
            set(found TRUE)
        endif()
    endif()
    set(${output} ${found} PARENT_SCOPE)
endfunction()

assert_json_equal("schema" "par-beatdown.tracker-timeline" schema)
assert_json_number_equal("version" 1 version)
assert_json_equal("generator.name" "par-beatdown" generator name)
assert_json_equal("generator.backend" "tracker-file" generator backend)
assert_json_equal("source.format" "xm" source format)
assert_json_equal("source.title" "" source title)

if(DEFINED PAR_BEATDOWN_FPS)
    assert_json_number_equal("render.fps" "${PAR_BEATDOWN_FPS}" render fps)
else()
    assert_json_number_equal("render.fps" 30 render fps)
endif()
if(DEFINED PAR_BEATDOWN_OFFSET)
    assert_json_number_equal("render.offset_seconds" "${PAR_BEATDOWN_OFFSET}"
        render offset_seconds)
else()
    assert_json_number_equal("render.offset_seconds" 0 render offset_seconds)
endif()
if(DEFINED PAR_BEATDOWN_FEATURE_HOP)
    assert_json_number_equal("render.feature_hop_seconds"
        "${PAR_BEATDOWN_FEATURE_HOP}" render feature_hop_seconds)
endif()

has_include(has_module module)
if(has_module)
    assert_json_number_greater("module.channel_count" 0 module channel_count)
    assert_json_number_greater("module.order_count" 0 module order_count)
    assert_json_number_greater("module.pattern_count" 0 module pattern_count)
endif()

has_include(has_timeline timeline)
if(has_timeline)
    assert_json_array_not_empty("events" events)
    assert_json_equal("events[0].kind" "order" events 0 kind)
    if(DEFINED PAR_BEATDOWN_FPS)
        assert_json_number_equal("events[3].frame" 2 events 3 frame)
    endif()
endif()

has_include(has_events events)
if(has_events)
    assert_json_array_not_empty("events" events)
    assert_json_equal("events[0].kind" "note" events 0 kind)
endif()

has_include(has_features features)
if(has_features)
    assert_json_array_not_empty("features" features)
    assert_json_number_greater("features[0].rms" 0 features 0 rms)
    assert_json_array_not_empty("features[0].channel_vu_mono"
        features 0 channel_vu_mono)
endif()

if(NOT expected STREQUAL actual)
    message(FATAL_ERROR
        "tracker timeline output differs from gold JSON\n"
        "expected: ${PAR_BEATDOWN_EXPECTED}\n"
        "actual: ${PAR_BEATDOWN_OUTPUT}")
endif()
