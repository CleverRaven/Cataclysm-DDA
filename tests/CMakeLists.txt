cmake_minimum_required(VERSION 3.1.4)

if (BUILD_TESTING)
    file(GLOB CATACLYSM_DDA_TEST_SOURCES
            ${CMAKE_SOURCE_DIR}/tests/*.cpp)

    # Enabling benchmarks
    add_definitions(-DCATCH_CONFIG_ENABLE_BENCHMARKING)

    # Need to build the test .mo file for the tests to pass.
    # Unfortunately we currently need to put this in the source dir, not the
    # build dir
    set(TEST_LANG ${CMAKE_SOURCE_DIR}/data/mods/TEST_DATA/lang)
    set(TEST_MO ${TEST_LANG}/mo/ru/LC_MESSAGES/TEST_DATA.mo)
    if(MSVC)
        find_program(GETTEXT_MSGFMT_EXECUTABLE
            msgfmt.exe
            HINTS
                  C:\\msys64\\usr\\bin
                  C:\\Prorogram\ Files\\Git\\mingw64\\bin
            REQUIRED
        )
    else()
		set(GETTEXT_MSGFMT_EXECUTABLE msgfmt)
    endif()
    add_custom_command(
        OUTPUT ${TEST_MO}
        MAIN_DEPENDENCY ${TEST_LANG}/po/ru.po
        COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -f -o ${TEST_MO} ${TEST_LANG}/po/ru.po)

    add_custom_target(test_mo DEPENDS ${TEST_MO})

    if (TILES)
        add_executable(cata_test-tiles ${CATACLYSM_DDA_TEST_SOURCES})
        add_dependencies(cata_test-tiles test_mo)
        target_link_libraries(cata_test-tiles cataclysm-tiles-common)
        add_test(NAME test-tiles
                COMMAND sh -c
                "$<TARGET_FILE:cata_test-tiles> --rng-seed time"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif ()

    if (CURSES)
        add_executable(cata_test ${CATACLYSM_DDA_TEST_SOURCES})
        add_dependencies(cata_test test_mo)
        target_link_libraries(cata_test cataclysm-common)
        add_test(NAME test
                COMMAND sh -c
                "$<TARGET_FILE:cata_test> --rng-seed time"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif ()
endif ()
