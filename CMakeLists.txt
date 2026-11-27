cmake_minimum_required(VERSION 3.24)

# On a Mac the program is one universal binary for macOS 12 and later. Both
# must be set before project(); a command-line value wins.
if(CMAKE_HOST_APPLE AND NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Oldest macOS the program runs on")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Architectures of the macOS build")
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" CHR_RELEASE_VERSION)
string(STRIP "${CHR_RELEASE_VERSION}" CHR_RELEASE_VERSION)
project(CrescentHawksRevenge VERSION ${CHR_RELEASE_VERSION} LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
# The importer (src/Import) and its JSON support are C++20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHR_BUILD_SDL "Build the SDL3 replacement for the DOS platform" ON)
option(CHR_FETCH_SDL "Fetch the pinned SDL3 release when it is not installed" ON)
option(CHR_PRESERVATION_INSTRUMENTATION
    "Compile reconstruction-only trace points inside Original procedures" OFF)
include(CTest)

function(chr_strict target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /Gy)
        target_link_options(${target} PRIVATE /OPT:REF)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    # Preservation tests use the standard assert macro as their witness
    # mechanism. Keep those checks active in Release builds as well.
    if("${target}" MATCHES "^test_")
        if(MSVC)
            target_compile_options(${target} PRIVATE /UNDEBUG)
        else()
            target_compile_options(${target} PRIVATE -UNDEBUG)
        endif()
    endif()
endfunction()

include(cmake/OriginalAuditedSources.cmake)
# The story (revenge/story.h): its texts' places and the importer that fills them.
set(CHR_STORY_SOURCES
	src/Support/story_text.c
	src/Support/story_text_slots.c
	src/Support/Json.cpp
	src/Import/Story.cpp
	src/Import/StoryHost.cpp)
# The rules, layout and starting state (revenge/rules.h): our numbers, read
# from data/rules, data/ui and data/start at start.
set(CHR_RULES_SOURCES
	src/Rules/rule_table.c
	src/Rules/Rules.cpp
	src/Rules/RulesHost.cpp)
# A test whose code reads the rules' tables loads them from the source tree's
# data/ (tests/rules_for_tests.h).
function(chr_uses_rules target)
	target_compile_definitions(${target} PRIVATE
		CHR_DATA_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/data")
endfunction()
add_library(chr_original_runtime STATIC ${CHR_ORIGINAL_SOURCES})
target_sources(chr_original_runtime PRIVATE
	src/Support/debug.c
	src/Support/media_settings.c
    src/Support/far_memory.c
    src/Support/host_dispatch.c
    src/Support/digital_bridge.c
    src/Support/far_heap_bridge.c
    src/Support/midi_data.c
    src/Support/program_break_bridge.c
    src/Support/representation.c
    src/Support/video_bridge.c
    src/Support/presentation_callback_bridge.c
	src/Support/tactical_panel_data.c
	${CHR_STORY_SOURCES}
	${CHR_RULES_SOURCES})
target_include_directories(chr_original_runtime
    PUBLIC include
    PRIVATE src/Original src/SDL)
target_compile_definitions(chr_original_runtime PRIVATE REVENGE_NO_DOS_MAIN)
if(CHR_PRESERVATION_INSTRUMENTATION)
    target_compile_definitions(chr_original_runtime PRIVATE
        REVENGE_ENABLE_PRESERVATION_INSTRUMENTATION)
endif()
chr_strict(chr_original_runtime)

# Small preservation-family targets provide focused compiler verification in
# addition to the complete audited Original runtime inventory.
add_library(chr_original_dos STATIC
    ${CHR_3A6A_DATA_SOURCES}
    src/Original/IO/REVENGE_38C3_DOS_ERROR.c
    src/Original/IO/REVENGE_3913_DOS_MEMORY.c
    src/Original/IO/REVENGE_3915_FILE_CREATE.c
    src/Original/IO/REVENGE_392F_FILE_OPEN.c
    src/Original/IO/REVENGE_3934_DEVICE_IO.c
    src/Original/IO/REVENGE_3936_CRT_FILE_CLOSE.c
    src/Original/IO/REVENGE_3939_DOS_FILE_CLOSE.c
    src/Original/IO/REVENGE_3948_DOS_FILE_READ.c
    src/Original/IO/REVENGE_3961_DOS_FILE_WRITE.c
    src/Original/IO/REVENGE_3965_FILE_SEEK.c
    src/Original/IO/REVENGE_3978_FILE_DELETE.c
    src/Original/IO/REVENGE_3989_FILE_ATTRIBUTES.c)
target_include_directories(chr_original_dos
    PUBLIC include
    PRIVATE src/Original src/SDL)
chr_strict(chr_original_dos)

add_library(chr_original_audio STATIC
    src/Original/Audio/REVENGE_3621_MIDI_SEQUENCE.c
    src/Support/midi_data.c)
target_include_directories(chr_original_audio
    PUBLIC include
    PRIVATE src/Original src/SDL)
if(CHR_PRESERVATION_INSTRUMENTATION)
    target_compile_definitions(chr_original_audio PRIVATE
        REVENGE_ENABLE_PRESERVATION_INSTRUMENTATION)
endif()
chr_strict(chr_original_audio)

if(BUILD_TESTING)
	find_package(Python3 COMPONENTS Interpreter)
	if(Python3_Interpreter_FOUND)
		add_test(NAME preservation.ledger_integrity
			COMMAND ${Python3_EXECUTABLE} -I
				${CMAKE_CURRENT_SOURCE_DIR}/tools/check_ledgers.py
				--root ${CMAKE_CURRENT_SOURCE_DIR})

		# The game's text stays out of the tree, and the import tables match
		# the sources and the player's copy. Both need a copy of the game named
		# by REVENGE_GAME and skip without one; the second checks the editions
		# list first, which needs none.
		add_test(NAME source.text_audit
			COMMAND ${Python3_EXECUTABLE} -I
				${CMAKE_CURRENT_SOURCE_DIR}/tools/text_audit.py
				--root ${CMAKE_CURRENT_SOURCE_DIR})
		add_test(NAME source.import_table_current
			COMMAND ${Python3_EXECUTABLE} -I
				${CMAKE_CURRENT_SOURCE_DIR}/tools/import/make_import_table.py --check)
		set_tests_properties(source.text_audit source.import_table_current
			PROPERTIES SKIP_RETURN_CODE 77)
	endif()

    add_executable(test_midi_data
        tests/test_midi_data.c
        src/Support/midi_data.c)
    target_include_directories(test_midi_data PRIVATE include)
    chr_strict(test_midi_data)
    add_test(NAME audio.original_segment_data_access COMMAND test_midi_data)

    add_executable(test_dos_redirects
        tests/test_dos_redirects.c
        tests/fake_backend.c)
    target_include_directories(test_dos_redirects PRIVATE src/SDL tests)
    target_link_libraries(test_dos_redirects PRIVATE chr_original_runtime)
    chr_strict(test_dos_redirects)
    add_test(NAME platform.original_dos_redirects COMMAND test_dos_redirects)

    add_executable(test_keyboard_translation
        tests/test_keyboard_translation.c
        tests/fake_backend.c)
    target_include_directories(test_keyboard_translation PRIVATE src/SDL tests)
    target_link_libraries(test_keyboard_translation PRIVATE chr_original_runtime)
    chr_strict(test_keyboard_translation)
    add_test(NAME input.original_keyboard_translation COMMAND test_keyboard_translation)

    add_executable(test_random
        tests/test_random.c
        tests/fake_backend.c)
	target_include_directories(test_random PRIVATE tests)
    target_link_libraries(test_random PRIVATE chr_original_runtime)
    chr_strict(test_random)
    add_test(NAME runtime.original_random_generator COMMAND test_random)

	add_executable(test_daylight_saving
		tests/test_daylight_saving.c
		tests/fake_backend.c)
	target_include_directories(test_daylight_saving PRIVATE src/Original tests)
	target_link_libraries(test_daylight_saving PRIVATE chr_original_runtime)
	chr_strict(test_daylight_saving)
	add_test(NAME runtime.original_daylight_saving COMMAND test_daylight_saving)

	add_executable(test_battletech_mechanics
		tests/test_battletech_mechanics.c
		tests/fake_backend.c)
	target_include_directories(test_battletech_mechanics PRIVATE
		src/Original src/SDL tests)
	target_link_libraries(test_battletech_mechanics PRIVATE
		chr_original_runtime)
	chr_strict(test_battletech_mechanics)
	add_test(NAME battletech.original_mechanics
		COMMAND test_battletech_mechanics)

	add_executable(test_battletech_combat_flow
		tests/test_battletech_combat_flow.c
		tests/fake_backend.c)
	target_include_directories(test_battletech_combat_flow PRIVATE
		src/Original src/SDL tests)
	target_link_libraries(test_battletech_combat_flow PRIVATE
		chr_original_runtime)
	chr_strict(test_battletech_combat_flow)
	add_test(NAME battletech.original_combat_flow
		COMMAND test_battletech_combat_flow)

	add_executable(test_game_clock
		tests/test_game_clock.c
		tests/fake_backend.c)
	target_include_directories(test_game_clock PRIVATE
		src/Original tests)
	target_link_libraries(test_game_clock PRIVATE chr_original_runtime)
	chr_strict(test_game_clock)
	add_test(NAME runtime.original_game_clock COMMAND test_game_clock)

	add_executable(test_exit_handlers tests/test_exit_handlers.c)
	target_link_libraries(test_exit_handlers PRIVATE chr_original_runtime)
	chr_strict(test_exit_handlers)
	add_test(NAME runtime.original_exit_handler_order
		COMMAND test_exit_handlers)

    add_executable(test_video_redirects
        tests/test_video_redirects.c
        tests/fake_backend.c)
    target_include_directories(test_video_redirects PRIVATE src/SDL tests)
    target_link_libraries(test_video_redirects PRIVATE chr_original_runtime)
    chr_strict(test_video_redirects)
    add_test(NAME graphics.original_video_redirects COMMAND test_video_redirects)

    add_executable(test_tactical_tile_raster
        tests/test_tactical_tile_raster.c
        tests/fake_backend.c)
    target_include_directories(test_tactical_tile_raster PRIVATE
        src/SDL tests)
    target_link_libraries(test_tactical_tile_raster PRIVATE
        chr_original_runtime)
    chr_strict(test_tactical_tile_raster)
    add_test(NAME graphics.original_tactical_tile_raster
        COMMAND test_tactical_tile_raster)

    add_executable(test_encoded_sprites
        tests/test_encoded_sprites.c
        tests/fake_backend.c)
    target_include_directories(test_encoded_sprites PRIVATE
        src/SDL tests)
    target_link_libraries(test_encoded_sprites PRIVATE chr_original_runtime)
    chr_strict(test_encoded_sprites)
    add_test(NAME graphics.original_encoded_sprites
        COMMAND test_encoded_sprites)

    add_executable(test_clipped_sprite_dispatch
        tests/test_clipped_sprite_dispatch.c
        tests/fake_backend.c)
    target_include_directories(test_clipped_sprite_dispatch PRIVATE
        src/SDL tests)
    target_link_libraries(test_clipped_sprite_dispatch PRIVATE
        chr_original_runtime)
    chr_strict(test_clipped_sprite_dispatch)
    add_test(NAME graphics.original_clipped_sprite_dispatch
        COMMAND test_clipped_sprite_dispatch)

    add_executable(test_presentation_leaves
        tests/test_presentation_leaves.c
        tests/fake_backend.c)
    target_include_directories(test_presentation_leaves PRIVATE
        src/Original src/SDL tests)
    target_link_libraries(test_presentation_leaves PRIVATE
        chr_original_runtime)
    chr_strict(test_presentation_leaves)
    add_test(NAME graphics.original_presentation_leaves
        COMMAND test_presentation_leaves)

    add_executable(test_video_segment
        tests/test_video_segment.c
        tests/fake_backend.c)
    target_include_directories(test_video_segment PRIVATE src/Original tests)
    target_link_libraries(test_video_segment PRIVATE chr_original_runtime)
    chr_strict(test_video_segment)
    add_test(NAME graphics.video_segment COMMAND test_video_segment)

    add_executable(test_vga_display_backing
        tests/test_vga_display_backing.c
        tests/fake_backend.c)
    target_include_directories(test_vga_display_backing PRIVATE
        src/Original tests)
    target_link_libraries(test_vga_display_backing PRIVATE
        chr_original_runtime)
    chr_strict(test_vga_display_backing)
    add_test(NAME graphics.original_vga_display_backing
        COMMAND test_vga_display_backing)

    add_executable(test_cursor_backing_rebuild
        tests/test_cursor_backing_rebuild.c)
    target_include_directories(test_cursor_backing_rebuild PRIVATE
        src/Original)
    target_link_libraries(test_cursor_backing_rebuild PRIVATE
        chr_original_runtime)
    chr_strict(test_cursor_backing_rebuild)
    add_test(NAME graphics.original_cursor_backing_rebuild
        COMMAND test_cursor_backing_rebuild)

    add_executable(test_graphics_resource_dispatch
        tests/test_graphics_resource_dispatch.c)
    target_include_directories(test_graphics_resource_dispatch PRIVATE
        src/Original)
    target_link_libraries(test_graphics_resource_dispatch PRIVATE
        chr_original_runtime)
    chr_strict(test_graphics_resource_dispatch)
    add_test(NAME graphics.original_resource_dispatch
        COMMAND test_graphics_resource_dispatch)

    add_executable(test_font_raster
        tests/test_font_raster.c
        tests/fake_backend.c)
    target_include_directories(test_font_raster PRIVATE src/SDL tests)
    target_link_libraries(test_font_raster PRIVATE chr_original_runtime)
    chr_strict(test_font_raster)
    add_test(NAME graphics.original_font_raster COMMAND test_font_raster)

    add_executable(test_mouse_cursor_control
        tests/test_mouse_cursor_control.c
        tests/fake_backend.c)
    target_include_directories(test_mouse_cursor_control PRIVATE
        src/Original src/SDL tests)
    target_link_libraries(test_mouse_cursor_control PRIVATE chr_original_runtime)
    chr_strict(test_mouse_cursor_control)
    add_test(NAME input.original_mouse_cursor_control
        COMMAND test_mouse_cursor_control)

    add_executable(test_timer_edges
        tests/test_timer_edges.c
        tests/fake_backend.c
        src/Original/Audio/REVENGE_38AE_PIT_CALIBRATION.c)
    target_include_directories(test_timer_edges PRIVATE
        src/Original src/SDL tests)
    target_link_libraries(test_timer_edges PRIVATE chr_original_runtime)
    chr_strict(test_timer_edges)
    add_test(NAME platform.original_timer_edges COMMAND test_timer_edges)

    add_executable(test_video_tile_capture tests/test_video_tile_capture.c)
    target_include_directories(test_video_tile_capture PRIVATE src/Original)
    target_link_libraries(test_video_tile_capture PRIVATE chr_original_runtime)
    chr_strict(test_video_tile_capture)
    add_test(NAME graphics.original_video_tile_capture
        COMMAND test_video_tile_capture)

    add_executable(test_packed_icon_resource
        tests/test_packed_icon_resource.c)
    target_include_directories(test_packed_icon_resource PRIVATE src/Original)
    target_link_libraries(test_packed_icon_resource PRIVATE chr_original_runtime)
    chr_strict(test_packed_icon_resource)
    add_test(NAME assets.original_packed_icon_resource
        COMMAND test_packed_icon_resource)

    add_executable(test_packed_icon_page tests/test_packed_icon_page.c)
    target_include_directories(test_packed_icon_page PRIVATE src/Original)
    target_link_libraries(test_packed_icon_page PRIVATE chr_original_runtime)
    chr_strict(test_packed_icon_page)
    add_test(NAME graphics.original_packed_icon_page
        COMMAND test_packed_icon_page)

    add_executable(test_critical_error_handler
        tests/test_critical_error_handler.c
        tests/fake_backend.c)
    target_include_directories(test_critical_error_handler PRIVATE
        src/Original src/SDL tests)
    target_link_libraries(test_critical_error_handler PRIVATE
        chr_original_runtime)
    chr_strict(test_critical_error_handler)
    add_test(NAME platform.original_critical_error_handler
        COMMAND test_critical_error_handler)

    add_executable(test_video_tile_strip tests/test_video_tile_strip.c)
    target_include_directories(test_video_tile_strip PRIVATE src/Original)
    target_link_libraries(test_video_tile_strip PRIVATE chr_original_runtime)
    chr_strict(test_video_tile_strip)
    add_test(NAME graphics.original_video_tile_strip
        COMMAND test_video_tile_strip)

    add_executable(test_decoded_icon_page tests/test_decoded_icon_page.c)
    target_include_directories(test_decoded_icon_page PRIVATE src/Original)
    target_link_libraries(test_decoded_icon_page PRIVATE chr_original_runtime)
    chr_strict(test_decoded_icon_page)
    add_test(NAME graphics.original_decoded_icon_page
        COMMAND test_decoded_icon_page)

    add_executable(test_single_ui_region_selection
        tests/test_single_ui_region_selection.c)
    target_include_directories(test_single_ui_region_selection PRIVATE
        src/Original)
    target_link_libraries(test_single_ui_region_selection PRIVATE
        chr_original_runtime)
    chr_strict(test_single_ui_region_selection)
    add_test(NAME input.original_single_ui_region_selection
        COMMAND test_single_ui_region_selection)

    add_executable(test_ui_region_poller tests/test_ui_region_poller.c)
    target_include_directories(test_ui_region_poller PRIVATE src/Original)
    target_link_libraries(test_ui_region_poller PRIVATE chr_original_runtime)
    chr_strict(test_ui_region_poller)
    add_test(NAME input.original_ui_region_poller
        COMMAND test_ui_region_poller)

    add_executable(test_navigation_normalization
        tests/test_navigation_normalization.c)
    target_include_directories(test_navigation_normalization PRIVATE
        src/Original)
    target_link_libraries(test_navigation_normalization PRIVATE
        chr_original_runtime)
    chr_strict(test_navigation_normalization)
    add_test(NAME input.original_navigation_normalization
        COMMAND test_navigation_normalization)

    add_executable(test_random_range tests/test_random_range.c)
    target_include_directories(test_random_range PRIVATE src/Original)
    target_link_libraries(test_random_range PRIVATE chr_original_runtime)
    chr_strict(test_random_range)
    add_test(NAME runtime.original_random_range COMMAND test_random_range)

    add_executable(test_output_leaves tests/test_output_leaves.c)
    target_include_directories(test_output_leaves PRIVATE src/Original)
    target_link_libraries(test_output_leaves PRIVATE chr_original_runtime)
    chr_strict(test_output_leaves)
    add_test(NAME platform.original_output_leaves COMMAND test_output_leaves)

    add_executable(test_text_window_state tests/test_text_window_state.c)
    target_include_directories(test_text_window_state PRIVATE src/Original)
    target_link_libraries(test_text_window_state PRIVATE chr_original_runtime)
    chr_strict(test_text_window_state)
    add_test(NAME output.original_text_window_state
        COMMAND test_text_window_state)

    add_executable(test_render_text_window tests/test_render_text_window.c)
    target_include_directories(test_render_text_window PRIVATE src/Original)
    target_link_libraries(test_render_text_window PRIVATE chr_original_runtime)
    chr_strict(test_render_text_window)
    add_test(NAME output.original_render_text_window
        COMMAND test_render_text_window)

    add_executable(test_active_render_target
        tests/test_active_render_target.c)
    target_include_directories(test_active_render_target PRIVATE src/Original)
    target_link_libraries(test_active_render_target PRIVATE
        chr_original_runtime)
    chr_strict(test_active_render_target)
    add_test(NAME graphics.original_active_render_target
        COMMAND test_active_render_target)

    add_executable(test_fill_video_rectangle
        tests/test_fill_video_rectangle.c)
    target_include_directories(test_fill_video_rectangle PRIVATE src/Original)
    target_link_libraries(test_fill_video_rectangle PRIVATE
        chr_original_runtime)
    chr_strict(test_fill_video_rectangle)
    add_test(NAME graphics.original_fill_video_rectangle
        COMMAND test_fill_video_rectangle)

    add_executable(test_resource_drive tests/test_resource_drive.c)
    target_include_directories(test_resource_drive PRIVATE src/Original)
    target_link_libraries(test_resource_drive PRIVATE chr_original_runtime)
    chr_strict(test_resource_drive)
    add_test(NAME platform.original_resource_drive COMMAND test_resource_drive)

    add_executable(test_string_length tests/test_string_length.c)
    target_include_directories(test_string_length PRIVATE src/Original)
    target_link_libraries(test_string_length PRIVATE chr_original_runtime)
    chr_strict(test_string_length)
    add_test(NAME utility.original_string_length COMMAND test_string_length)

    add_executable(test_far_string_compare tests/test_far_string_compare.c)
    target_include_directories(test_far_string_compare PRIVATE src/Original)
    target_link_libraries(test_far_string_compare PRIVATE chr_original_runtime)
    chr_strict(test_far_string_compare)
    add_test(NAME utility.original_far_string_compare
        COMMAND test_far_string_compare)

	add_executable(test_integer_format tests/test_integer_format.c)
	target_include_directories(test_integer_format PRIVATE src/Original)
	target_link_libraries(test_integer_format PRIVATE chr_original_runtime)
	chr_strict(test_integer_format)
	add_test(NAME utility.original_integer_format COMMAND test_integer_format)

	add_executable(test_far_string_tokenize tests/test_far_string_tokenize.c)
	target_include_directories(test_far_string_tokenize PRIVATE src/Original)
	target_link_libraries(test_far_string_tokenize PRIVATE chr_original_runtime)
	chr_strict(test_far_string_tokenize)
	add_test(NAME utility.original_far_string_tokenize
		COMMAND test_far_string_tokenize)

    add_executable(test_far_memory_primitives
        tests/test_far_memory_primitives.c
        tests/fake_backend.c)
    target_include_directories(test_far_memory_primitives PRIVATE
        src/Original tests)
    target_link_libraries(test_far_memory_primitives PRIVATE
        chr_original_runtime)
    chr_strict(test_far_memory_primitives)
    add_test(NAME utility.original_far_memory_primitives
        COMMAND test_far_memory_primitives)

    add_executable(test_display_backing_encoding
        tests/test_display_backing_encoding.c
        tests/fake_backend.c)
    target_include_directories(test_display_backing_encoding PRIVATE
        src/Original src/SDL tests)
    target_link_libraries(test_display_backing_encoding PRIVATE
        chr_original_runtime)
    chr_strict(test_display_backing_encoding)
    add_test(NAME graphics.original_display_backing_encoding
        COMMAND test_display_backing_encoding)

    add_executable(test_far_pointer_bridge
        tests/test_far_pointer_bridge.c
        src/Support/far_memory.c)
    target_include_directories(test_far_pointer_bridge PRIVATE include)
    chr_strict(test_far_pointer_bridge)
    add_test(NAME support.far_pointer_bridge COMMAND test_far_pointer_bridge)

    add_executable(test_program_break
        tests/test_program_break.c
        src/Original/Runtime/REVENGE_38FE_PROGRAM_BREAK.c
        ${CHR_3A6A_DATA_SOURCES}
        src/Support/far_memory.c
        src/Support/program_break_bridge.c)
    target_include_directories(test_program_break PRIVATE
        include src/Original)
    chr_strict(test_program_break)
    add_test(NAME runtime.original_program_break COMMAND test_program_break)

    add_executable(test_far_heap
        tests/test_far_heap.c
        src/Original/Runtime/REVENGE_0D56_FAR_MEMORY.c
        src/Original/Runtime/REVENGE_38CD_FAR_HEAP_UNLINK.c
        src/Original/Runtime/REVENGE_398D_FAR_HEAP.c
        ${CHR_3A6A_DATA_SOURCES}
        src/Support/far_memory.c
        src/Support/far_heap_bridge.c)
    target_include_directories(test_far_heap PRIVATE include src/Original)
    chr_strict(test_far_heap)
    add_test(NAME runtime.original_far_heap COMMAND test_far_heap)

    add_executable(test_formatted_text
        tests/test_formatted_text.c
        src/Original/Text/REVENGE_35CE_FORMATTED_TEXT.c
        src/Original/Utility/REVENGE_397A_STRING_CONCAT.c
        src/Original/Utility/REVENGE_3983_STRING_COPY.c
        ${CHR_3A6A_DATA_SOURCES}
        ${CHR_STORY_SOURCES}
        src/Support/debug.c)
    target_include_directories(test_formatted_text PRIVATE include src/Original tests)
    chr_strict(test_formatted_text)
    add_test(NAME output.original_formatted_text COMMAND test_formatted_text)
    set_tests_properties(output.original_formatted_text PROPERTIES SKIP_RETURN_CODE 77)

    add_executable(test_original_editions
        tests/test_original_editions.c
        ${CHR_3A6A_DATA_SOURCES})
    target_include_directories(test_original_editions PRIVATE include src/Original)
    chr_strict(test_original_editions)
    add_test(NAME import.original_editions COMMAND test_original_editions)

    add_executable(test_file_translation
        tests/test_file_translation.c
        src/Original/IO/REVENGE_393C_TEXT_FILE_READ.c
        src/Original/IO/REVENGE_394A_TEXT_FILE_WRITE.c
        src/Original/IO/REVENGE_39C0_FILE_LENGTH.c
        ${CHR_3A6A_DATA_SOURCES}
        src/Support/far_memory.c)
    target_include_directories(test_file_translation PRIVATE include src/Original)
    chr_strict(test_file_translation)
    add_test(NAME platform.original_file_translation COMMAND test_file_translation)

    add_executable(test_resource_stream
        tests/test_resource_stream.c
        src/Original/Resources/REVENGE_3390_RESOURCE_STREAM.c
        src/Original/Utility/REVENGE_3A04_STRING_UPPERCASE.c
        ${CHR_3A6A_DATA_SOURCES}
        src/Support/debug.c
        src/Support/far_memory.c
        src/Support/representation.c)
    target_include_directories(test_resource_stream PRIVATE
        include src/Original)
    chr_strict(test_resource_stream)
    add_test(NAME assets.original_resource_stream COMMAND test_resource_stream)

    add_executable(test_font_resource tests/test_font_resource.c)
    target_include_directories(test_font_resource PRIVATE src/Original)
    target_link_libraries(test_font_resource PRIVATE chr_original_runtime)
    chr_strict(test_font_resource)
    add_test(NAME output.original_font_resource COMMAND test_font_resource)

    add_executable(test_rectangle_outline tests/test_rectangle_outline.c)
    target_include_directories(test_rectangle_outline PRIVATE src/Original)
    target_link_libraries(test_rectangle_outline PRIVATE chr_original_runtime)
    chr_strict(test_rectangle_outline)
    add_test(NAME graphics.original_rectangle_outline
        COMMAND test_rectangle_outline)

    add_executable(test_clipped_video_region
        tests/test_clipped_video_region.c)
    target_include_directories(test_clipped_video_region PRIVATE src/Original)
    target_link_libraries(test_clipped_video_region PRIVATE
        chr_original_runtime)
    chr_strict(test_clipped_video_region)
    add_test(NAME graphics.original_clipped_video_region
        COMMAND test_clipped_video_region)

    add_executable(test_asset_decoders
        tests/test_asset_decoders.c
        tests/fake_backend.c)
    target_include_directories(test_asset_decoders PRIVATE src/SDL tests)
    target_link_libraries(test_asset_decoders PRIVATE chr_original_runtime)
    chr_strict(test_asset_decoders)
    add_test(NAME assets.original_codec_transcriptions COMMAND test_asset_decoders)

    add_executable(test_pc_speaker
        tests/test_pc_speaker.c
        tests/fake_backend.c)
    target_include_directories(test_pc_speaker PRIVATE src/SDL tests)
    target_link_libraries(test_pc_speaker PRIVATE chr_original_runtime)
    chr_strict(test_pc_speaker)
    add_test(NAME audio.pc_speaker_conversion COMMAND test_pc_speaker)

    add_executable(test_midi_sequence
        tests/test_midi_sequence.c
        tests/fake_backend.c)
    target_include_directories(test_midi_sequence PRIVATE src/SDL tests)
    target_link_libraries(test_midi_sequence PRIVATE
        chr_original_runtime
        chr_original_audio)
    chr_strict(test_midi_sequence)
    add_test(NAME audio.midi_sequence_parser COMMAND test_midi_sequence)

    add_executable(test_digital_descriptors
        tests/test_digital_descriptors.c
        tests/fake_backend.c)
    target_include_directories(test_digital_descriptors PRIVATE
        src/Original src/SDL tests)
    target_link_libraries(test_digital_descriptors PRIVATE chr_original_runtime)
    chr_strict(test_digital_descriptors)
    add_test(NAME audio.original_digital_descriptors
        COMMAND test_digital_descriptors)

    add_executable(test_weapon_effects
        tests/test_weapon_effects.c
        tests/audio_preservation_shims.c
        tests/fake_backend.c)
    target_include_directories(test_weapon_effects PRIVATE src/SDL tests)
    target_link_libraries(test_weapon_effects PRIVATE chr_original_runtime)
    chr_strict(test_weapon_effects)
    add_test(NAME audio.original_weapon_effect_dispatch
        COMMAND test_weapon_effects)
    set_tests_properties(audio.original_weapon_effect_dispatch PROPERTIES SKIP_RETURN_CODE 77)

    add_executable(test_status_row_highlight
        tests/test_status_row_highlight.c
        tests/fake_backend.c)
    target_include_directories(test_status_row_highlight PRIVATE src/SDL tests)
    target_link_libraries(test_status_row_highlight PRIVATE chr_original_runtime)
    chr_strict(test_status_row_highlight)
    add_test(NAME tactical.original_status_row_highlight
        COMMAND test_status_row_highlight)

    add_executable(test_tactical_cursor_retreat
        tests/test_tactical_cursor_retreat.c
        tests/fake_backend.c)
    target_include_directories(test_tactical_cursor_retreat PRIVATE src/SDL tests)
    target_link_libraries(test_tactical_cursor_retreat PRIVATE chr_original_runtime)
    chr_strict(test_tactical_cursor_retreat)
    add_test(NAME tactical.original_cursor_retreat
        COMMAND test_tactical_cursor_retreat)

    add_executable(test_line_of_sight_map_width
        tests/test_line_of_sight_map_width.c
        tests/fake_backend.c)
    target_include_directories(test_line_of_sight_map_width PRIVATE src/SDL tests)
    target_link_libraries(test_line_of_sight_map_width PRIVATE chr_original_runtime)
    chr_strict(test_line_of_sight_map_width)
    add_test(NAME tactical.original_line_of_sight_map_width
        COMMAND test_line_of_sight_map_width)

    add_executable(test_scenario_clock_subtick
        tests/test_scenario_clock_subtick.c
        tests/fake_backend.c)
    target_include_directories(test_scenario_clock_subtick PRIVATE src/SDL tests)
    target_link_libraries(test_scenario_clock_subtick PRIVATE chr_original_runtime)
    chr_strict(test_scenario_clock_subtick)
    add_test(NAME tactical.original_scenario_clock_subtick
        COMMAND test_scenario_clock_subtick)

    # The rules: data/rules, ui and start rebuild the original's tables (rebuild), the
    # player's copy holds them (exe), and a wrong file is a fault naming it.
    add_executable(test_rules tests/test_rules.cpp tests/fake_backend.c)
    target_include_directories(test_rules PRIVATE src/SDL tests)
    target_link_libraries(test_rules PRIVATE chr_original_runtime)
    chr_strict(test_rules)
    add_test(NAME rules.rebuild COMMAND test_rules from_data)
    add_test(NAME rules.exe COMMAND test_rules in_copy)
    add_test(NAME rules.range COMMAND test_rules too_large)
    foreach(mode stray absent length shared location direction engagement overfull unit_type class_lists experience month record frame pilot_count section speed slot_name nofile)
        add_test(NAME rules.${mode} COMMAND test_rules ${mode})
    endforeach()
    set_tests_properties(rules.rebuild rules.exe rules.range rules.stray rules.absent
        rules.length rules.shared rules.location rules.direction rules.engagement rules.overfull rules.unit_type rules.class_lists rules.experience rules.month rules.record rules.frame rules.pilot_count rules.section rules.speed rules.slot_name rules.nofile
        PROPERTIES SKIP_RETURN_CODE 77)
    set_tests_properties(rules.stray PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: weapons.json: weapon_table.small_laser.rate_of_fire is no key of the rules here")
    set_tests_properties(rules.absent PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: weapons.json: weapon_table.ppc.heat is missing")
    set_tests_properties(rules.range PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: weapons.json: weapon_table.ppc.heat is not a whole number from 0 to 255")
    set_tests_properties(rules.length PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: weapons.json: ammunition_capacity_multipliers is not an array of 14 numbers")
    set_tests_properties(rules.shared PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: weapons.json: weapon_table.lrm_5.improved_missile_damage_bonus is one byte with the high byte of combat.json's hit_table_by_direction.15., which is 0")
    set_tests_properties(rules.location PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: combat.json: hit_location_tables.0..0. is not an armour location the code names")
    set_tests_properties(rules.direction PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: combat.json: hit_table_by_direction.0. is not a whole number from 0 to 3")
    set_tests_properties(rules.engagement PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: ai.json: engagement_by_comparison_mask.0. is not an engagement mode the code names")
    set_tests_properties(rules.overfull PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: ai.json: weapon_priorities.15. doesn't fit: the bank holds 176 entries in all")
    set_tests_properties(rules.unit_type PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: units.json: fixed_unit_types.0. is not a whole number from 0 to 88")
    set_tests_properties(rules.class_lists PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: units.json: unit_class_types.4. doesn't fit: the lists hold 68 bytes in all")
    set_tests_properties(rules.experience PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: pilots.json: pilots.0..experience is not a whole number from 0 to 3")
    set_tests_properties(rules.month PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: campaign.json: mission_dates.0..month is not a whole number from 0 to 11")
    set_tests_properties(rules.record PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: lance_editor.json: layout.unit_editor_sections.0..left is missing")
    set_tests_properties(rules.frame PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: screens.json: text_window_frame doesn't fit: the frame holds 12 tiles and ends in all")
    set_tests_properties(rules.pilot_count PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: new_campaign.json: initial.random_pilot_count is not a whole number from 0 to 15")
    set_tests_properties(rules.section PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: lance_editor.json .start.: section_enabled.1. is not a whole number from 0 to 1")
    set_tests_properties(rules.speed PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: game.json: tactical_game_speed is not a whole number from 0 to 7")
    set_tests_properties(rules.slot_name PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: save_directory.json: slots.2..name is not a name of up to 20 characters")
    set_tests_properties(rules.nofile PROPERTIES PASS_REGULAR_EXPRESSION
        "fault: rules: combat.json can't be read from")

    add_executable(test_initial_data_values
        tests/test_initial_data_values.c
        tests/fake_backend.c)
    target_include_directories(test_initial_data_values PRIVATE src/SDL tests)
    target_link_libraries(test_initial_data_values PRIVATE chr_original_runtime)
    chr_strict(test_initial_data_values)
    add_test(NAME import.original_initial_data_values
        COMMAND test_initial_data_values)
    set_tests_properties(import.original_initial_data_values PROPERTIES SKIP_RETURN_CODE 77)

    add_executable(test_index_guard
        tests/test_index_guard.c
        src/Support/debug.c)
    target_include_directories(test_index_guard PRIVATE include)
    chr_strict(test_index_guard)
    add_test(NAME support.index_guard COMMAND test_index_guard)
    add_test(NAME support.index_guard_past COMMAND test_index_guard past)
    add_test(NAME support.index_guard_negative COMMAND test_index_guard negative)
    set_tests_properties(support.index_guard_past PROPERTIES
        PASS_REGULAR_EXPRESSION "fault: GuardedTable\\[5\\] is past its 5 entries")
    set_tests_properties(support.index_guard_negative PROPERTIES
        PASS_REGULAR_EXPRESSION "fault: GuardedTable\\[[0-9]+\\] is past its 5 entries")

    add_executable(test_random_pilot_pool
        tests/test_random_pilot_pool.c
        tests/fake_backend.c)
    target_include_directories(test_random_pilot_pool PRIVATE src/SDL src/Original tests)
    target_link_libraries(test_random_pilot_pool PRIVATE chr_original_runtime)
    chr_strict(test_random_pilot_pool)
    add_test(NAME import.random_pilot_pool
        COMMAND test_random_pilot_pool)
    set_tests_properties(import.random_pilot_pool PROPERTIES SKIP_RETURN_CODE 77)

    add_executable(test_unit_durability_totals
        tests/test_unit_durability_totals.c
        tests/fake_backend.c)
    target_include_directories(test_unit_durability_totals PRIVATE src/SDL tests)
    target_link_libraries(test_unit_durability_totals PRIVATE chr_original_runtime)
    chr_strict(test_unit_durability_totals)
    add_test(NAME units.original_durability_totals
        COMMAND test_unit_durability_totals)

    add_executable(test_omni_unit_list
        tests/test_omni_unit_list.c
        tests/fake_backend.c)
    target_include_directories(test_omni_unit_list PRIVATE src/Original src/SDL tests)
    target_link_libraries(test_omni_unit_list PRIVATE chr_original_runtime)
    chr_strict(test_omni_unit_list)
    add_test(NAME units.original_omni_unit_list COMMAND test_omni_unit_list)
    set_tests_properties(units.original_omni_unit_list PROPERTIES SKIP_RETURN_CODE 77)

    add_executable(test_scenario_sound
        tests/test_scenario_sound.c
        tests/audio_preservation_shims.c
        tests/fake_backend.c)
    target_include_directories(test_scenario_sound PRIVATE src/SDL tests)
    target_link_libraries(test_scenario_sound PRIVATE chr_original_runtime)
    chr_strict(test_scenario_sound)
    add_test(NAME audio.original_scenario_narration
        COMMAND test_scenario_sound)
    set_tests_properties(audio.original_scenario_narration PROPERTIES SKIP_RETURN_CODE 77)

endif()

if(CHR_BUILD_SDL)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)

    if(CHR_FETCH_SDL)
        include(FetchContent)
        FetchContent_Declare(SDL3
            URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
            URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68)
        set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
        # XTest is only for synthetic input, and many Linux systems lack its
        # headers. A value given on the command line wins.
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            set(SDL_X11_XTEST OFF CACHE BOOL "Build SDL's X11 XTest support")
        endif()
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL3)
    else()
        find_package(SDL3 3.2 CONFIG REQUIRED)
    endif()

    if(NOT TARGET SDL3::SDL3-static)
        message(FATAL_ERROR
            "A static SDL3 library was not found. Set SDL3_DIR to a static SDL3 installation "
            "or configure with -DCHR_FETCH_SDL=ON.")
    endif()

    set(CHR_SDL_TARGET SDL3::SDL3-static)

    add_library(chr_sdl_backend STATIC
        src/SDL/audio.c
        src/SDL/host_config.c
        src/SDL/io.c
        src/SDL/game_paths.c
        src/SDL/settings.c
        src/SDL/harness.c
        src/SDL/platform.c
        src/SDL/system.c
        src/SDL/map_dump.c)
    target_include_directories(chr_sdl_backend PUBLIC src/SDL include)
    # The object dump names the game's data, some of it declared beside the
    # preserved source.
    set_source_files_properties(src/SDL/map_dump.c PROPERTIES
        INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src/Original")
    target_link_libraries(chr_sdl_backend PUBLIC ${CHR_SDL_TARGET})
    chr_strict(chr_sdl_backend)
    # The backend names the runtime's data objects (map_dump.c), and the
    # runtime calls the backend, so a program links the two as one group:
    # GNU ld reads each archive once, in order, and would leave the objects
    # only map_dump.c names undefined. Apple's and MSVC's linkers search every
    # archive as needed, and CMake has no group for them.
    if(CMAKE_LINK_GROUP_USING_RESCAN_SUPPORTED OR CMAKE_C_LINK_GROUP_USING_RESCAN_SUPPORTED
       OR CMAKE_CXX_LINK_GROUP_USING_RESCAN_SUPPORTED)
        set(CHR_RUNTIME_WITH_BACKEND
            "$<LINK_GROUP:RESCAN,chr_original_runtime,chr_sdl_backend>")
    else()
        set(CHR_RUNTIME_WITH_BACKEND chr_original_runtime chr_sdl_backend)
    endif()

    # The diagnostic still consumes the retired host-side CPS adapter.
    add_executable(RevengeDiagnostic EXCLUDE_FROM_ALL src/SDL/diagnostic.c)
    target_link_libraries(RevengeDiagnostic PRIVATE
        ${CHR_RUNTIME_WITH_BACKEND})
    chr_strict(RevengeDiagnostic)

    # The preserved Original source owns main() and all game flow. SDL only
    # satisfies the DOS/BIOS/hardware boundary calls made by that source.
    # The campaign loop is admitted and this target links end to end. It is a
    # normal build product now that the preservation inventory is complete.
    add_executable(Revenge src/SDL/entry.c)
    target_compile_definitions(Revenge PRIVATE
        CHR_VERSION="${PROJECT_VERSION}")
    target_link_libraries(Revenge PRIVATE
        ${CHR_RUNTIME_WITH_BACKEND})
    chr_strict(Revenge)
    add_custom_command(TARGET Revenge POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/config/REVENGE.CFG
            $<TARGET_FILE_DIR:Revenge>/REVENGE.CFG
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data
            $<TARGET_FILE_DIR:Revenge>/data)

    # The golden traces (tools/trace/README.md): every script against its
    # golden, and the battle's script twice, byte for byte. Both run the game
    # REVENGE_GAME names, headless, and skip without one.
    if(BUILD_TESTING AND Python3_Interpreter_FOUND)
        add_test(NAME trace.goldens
            COMMAND ${Python3_EXECUTABLE} -I
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/trace/check.py
                --exe $<TARGET_FILE:Revenge>)
        add_test(NAME trace.repeat
            COMMAND ${Python3_EXECUTABLE} -I
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/trace/repeat.py
                --exe $<TARGET_FILE:Revenge>
                --script ${CMAKE_CURRENT_SOURCE_DIR}/tools/trace/scripts/tactical.keys)
        # Every script again with the program file out of the game folder and
        # the story already imported: play reads nothing from the copy.
        add_test(NAME trace.exe_free
            COMMAND ${Python3_EXECUTABLE} -I
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/trace/exe_free.py
                --exe $<TARGET_FILE:Revenge>)
        set_tests_properties(trace.goldens trace.repeat trace.exe_free PROPERTIES SKIP_RETURN_CODE 77)
        # Bytes a run must hold, by the map's places, against the image.
        add_test(NAME map.dump_checks
            COMMAND ${Python3_EXECUTABLE} -I
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/map/check_dump.py
                --exe $<TARGET_FILE:Revenge>)
        set_tests_properties(map.dump_checks PROPERTIES SKIP_RETURN_CODE 77)
        # Every data object has a line in the map of the original's memory.
        add_test(NAME map.data_map
            COMMAND ${Python3_EXECUTABLE} -I
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/map/check_map.py)
    endif()

    add_custom_target(revenge-package
        DEPENDS Revenge
        COMMAND ${CMAKE_COMMAND} -E rm -rf
            ${CMAKE_BINARY_DIR}/package
        COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_BINARY_DIR}/package
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Revenge>
            ${CMAKE_BINARY_DIR}/package/Revenge.exe
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/config/REVENGE.CFG
            ${CMAKE_BINARY_DIR}/package/REVENGE.CFG
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data
            ${CMAKE_BINARY_DIR}/package/data
        COMMENT "Assembling the Revenge runtime: the program, REVENGE.CFG and data/")

    if(BUILD_TESTING)
        add_executable(test_sdl_dos_errors tests/test_sdl_dos_errors.c)
        target_link_libraries(test_sdl_dos_errors PRIVATE
            ${CHR_RUNTIME_WITH_BACKEND})
        chr_strict(test_sdl_dos_errors)
        add_test(NAME platform.sdl_dos_error_state
            COMMAND test_sdl_dos_errors)

        add_executable(test_sdl_dos_io tests/test_sdl_dos_io.c)
        target_link_libraries(test_sdl_dos_io PRIVATE
            ${CHR_RUNTIME_WITH_BACKEND})
        chr_strict(test_sdl_dos_io)
        add_test(NAME platform.sdl_dos_file_semantics COMMAND test_sdl_dos_io)
        # Tests that write files keep them in the build folder, never in the
        # player's own save folder.
        set_tests_properties(platform.sdl_dos_file_semantics PROPERTIES
            ENVIRONMENT "REVENGE_SAVE_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/test-saves/")

        add_executable(test_sdl_game_paths tests/test_sdl_game_paths.c)
        target_link_libraries(test_sdl_game_paths PRIVATE
            ${CHR_RUNTIME_WITH_BACKEND})
        chr_strict(test_sdl_game_paths)
        add_test(NAME platform.sdl_game_paths COMMAND test_sdl_game_paths)
        # Play opens no program file: asking for one stops the game.
        add_test(NAME platform.sdl_no_program_file
            COMMAND test_sdl_game_paths program_file)
        set_tests_properties(platform.sdl_no_program_file PROPERTIES
            PASS_REGULAR_EXPRESSION
            "fault: play opens no program file, and the game asked for a:.REVENGE.exe")

        # The player's settings file, made once from the install's MEDIA.CFG.
        add_executable(test_settings tests/test_settings.c)
        target_include_directories(test_settings PRIVATE src/Original src/SDL tests)
        target_link_libraries(test_settings PRIVATE
            ${CHR_RUNTIME_WITH_BACKEND})
        chr_strict(test_settings)
        # The story: the texts the import took, the story file, and its faults.
        add_executable(test_story tests/test_story.cpp)
        target_include_directories(test_story PRIVATE src/Import tests)
        target_link_libraries(test_story PRIVATE ${CHR_RUNTIME_WITH_BACKEND})
        chr_strict(test_story)
        foreach(mode match state refuse twice stray absent long media_length)
            add_test(NAME import.story_${mode} COMMAND test_story ${mode})
            set_tests_properties(import.story_${mode} PROPERTIES SKIP_RETURN_CODE 77)
        endforeach()
        set_tests_properties(import.story_twice PROPERTIES PASS_REGULAR_EXPRESSION
            "fault: the story: .*text.json: the id .title.0. twice|skipped")
        set_tests_properties(import.story_stray PROPERTIES PASS_REGULAR_EXPRESSION
            "fault: .*text.json: the id .title.99. is no text of the game's|skipped")
        set_tests_properties(import.story_absent PROPERTIES PASS_REGULAR_EXPRESSION
            "fault: .*text.json has no text .title.0.|skipped")
        set_tests_properties(import.story_long PROPERTIES PASS_REGULAR_EXPRESSION
            "fault: the story text title.0 is [0-9]+ bytes, and its place holds|skipped")
        set_tests_properties(import.story_media_length PROPERTIES PASS_REGULAR_EXPRESSION
            "fault: the story: .*digital_sound.bin is 1580 bytes, and the game's digital_sound is 1581|skipped")

        # A file the game can't open is a fault naming it, not a disk prompt.
        add_executable(test_tracked_file_fault tests/test_tracked_file_fault.c)
        target_include_directories(test_tracked_file_fault PRIVATE src/Original tests)
        target_link_libraries(test_tracked_file_fault PRIVATE
            ${CHR_RUNTIME_WITH_BACKEND})
        chr_strict(test_tracked_file_fault)
        add_test(NAME resources.tracked_file_read COMMAND test_tracked_file_fault read)
        add_test(NAME resources.tracked_file_make COMMAND test_tracked_file_fault make)
        set_tests_properties(resources.tracked_file_read PROPERTIES SKIP_RETURN_CODE 77
            PASS_REGULAR_EXPRESSION "fault: can't open ABSENT.DAT|skipped")
        set_tests_properties(resources.tracked_file_make PROPERTIES SKIP_RETURN_CODE 77
            PASS_REGULAR_EXPRESSION "fault: can't create ABSENT.DAT|skipped")

        foreach(mode first none own bad parse)
            add_test(NAME platform.settings_${mode} COMMAND test_settings ${mode})
            set_tests_properties(platform.settings_${mode} PROPERTIES SKIP_RETURN_CODE 77)
        endforeach()
        set_tests_properties(platform.settings_bad PROPERTIES PASS_REGULAR_EXPRESSION
            "fault: .*settings.txt:3: music can.t be .loud.|skipped")

        # The objects the source places in the data segment, as the program
        # starts them, against the player's copy's load image.
        add_executable(test_initial_data_image tests/test_initial_data_image.c)
        target_include_directories(test_initial_data_image PRIVATE src/Original tests)
        target_link_libraries(test_initial_data_image PRIVATE
            ${CHR_RUNTIME_WITH_BACKEND})
        chr_strict(test_initial_data_image)
        add_test(NAME import.original_initial_data_image COMMAND test_initial_data_image)
        set_tests_properties(import.original_initial_data_image PROPERTIES SKIP_RETURN_CODE 77)

        # A save the original wrote (REVENGE_ORIGINAL_SAVE): its selection
        # cards through the lance editor's card preparation.
        add_executable(test_lance_editor_cards tests/test_lance_editor_cards.c)
        target_include_directories(test_lance_editor_cards PRIVATE src/Original tests)
        target_link_libraries(test_lance_editor_cards PRIVATE
            ${CHR_RUNTIME_WITH_BACKEND})
        chr_strict(test_lance_editor_cards)
        add_test(NAME units.lance_editor_cards COMMAND test_lance_editor_cards)
        add_test(NAME units.lance_editor_cards_stale COMMAND test_lance_editor_cards stale)
        set_tests_properties(units.lance_editor_cards PROPERTIES SKIP_RETURN_CODE 77)
        set_tests_properties(units.lance_editor_cards_stale PROPERTIES SKIP_RETURN_CODE 77
            PASS_REGULAR_EXPRESSION "fault: a selection card's sprite pointer 7929:0008|skipped")

        add_executable(test_sdl_renderer_fallback tests/test_sdl_renderer_fallback.c)
        target_link_libraries(test_sdl_renderer_fallback PRIVATE
            ${CHR_RUNTIME_WITH_BACKEND})
        chr_strict(test_sdl_renderer_fallback)
        add_test(NAME platform.sdl_renderer_fallback COMMAND test_sdl_renderer_fallback)
    endif()
endif()

# Every test linking the runtime gets the source tree's data/
# (tests/rules_for_tests.h): one that takes the copy takes the rules too.
get_property(chr_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(chr_target IN LISTS chr_targets)
	if(chr_target MATCHES "^test_")
		get_target_property(chr_links ${chr_target} LINK_LIBRARIES)
		if(chr_links MATCHES "chr_original_runtime")
			chr_uses_rules(${chr_target})
		endif()
	endif()
endforeach()
