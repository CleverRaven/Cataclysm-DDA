cmake_minimum_required(VERSION 3.20)

if (BUILD_TESTING)
    file(GLOB CATACLYSM_DDA_TEST_SOURCES
            ${CMAKE_SOURCE_DIR}/tests/*.cpp)

    # Enabling benchmarks
    add_definitions(-DCATCH_CONFIG_ENABLE_BENCHMARKING)

    if(LOCALIZE)
        # Need to build the test .mo file for the tests to pass.
        # Unfortunately we currently need to put this in the source dir, not the
        # build dir
        set(TEST_LANG ${CMAKE_SOURCE_DIR}/data/mods/TEST_DATA/lang)
        set(TEST_MO ${TEST_LANG}/mo/ru/LC_MESSAGES/TEST_DATA.mo)
        add_custom_command(
            OUTPUT ${TEST_MO}
            MAIN_DEPENDENCY ${TEST_LANG}/po/ru.po
            COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -f -o ${TEST_MO} ${TEST_LANG}/po/ru.po)

        add_custom_target(test_mo DEPENDS ${TEST_MO})
    endif()

    if (TILES)
        add_executable(cata_test-tiles ${CATACLYSM_DDA_TEST_SOURCES})
        if (LOCALIZE)
            add_dependencies(cata_test-tiles test_mo)
        endif()
        target_link_libraries(cata_test-tiles PRIVATE cataclysm-tiles-common)
	target_compile_definitions(cata_test-tiles PUBLIC SDL_MAIN_HANDLED)
        add_test(NAME cata.tiles.default
                COMMAND cata_test-tiles --rng-seed time
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
        # ctest -C RelWithDebInfo -j 2 -V -L ^gha$
        list(APPEND _n 1 2)
        list(APPEND _args "[slow] ~starting_items" "~[slow] ~[.],starting_items")
        foreach(n args IN ZIP_LISTS _n _args)
            add_test(NAME cata.tiles.gha-${n}
                    COMMAND cata_test-tiles --min-duration 20 --use-colour yes --rng-seed time
                    --order lex --user-dir=test_user_dir_${n} ${args}
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
            set_tests_properties(cata.tiles.gha-${n} PROPERTIES LABELS "gha")
        endforeach()
        catch_discover_tests(cata_test-tiles
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif ()

    if (CURSES)
        add_executable(cata_test ${CATACLYSM_DDA_TEST_SOURCES})
        if (LOCALIZE)
            add_dependencies(cata_test test_mo)
        endif()
        target_link_libraries(cata_test PRIVATE cataclysm-common)
        add_test(NAME test
                COMMAND cata_test --rng-seed time
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif ()
endif ()
