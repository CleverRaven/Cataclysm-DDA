message(STATUS "Process LANGUAGES variable --")

if(NOT GETTEXT_MSGFMT_EXECUTABLE)
    message(FATAL_ERROR
            "Gettext not found. Install gettext package or disable \
             localization with \"-DLOCALIZE=OFF\". \
             See doc/COMPILING/COMPILING-CMAKE.md for details and more info.")
endif()

foreach (LANG ${LANGUAGES})
    message(VERBOSE "Add translation for ${LANG}: ${LANG}.po")
endforeach ()

cmake_path(GET GETTEXT_MSGFMT_EXECUTABLE PARENT_PATH _path)
find_program(XGETTEXT_MSGFMT_EXECUTABLE
    xgettext
    HINTS
        ${_path}
)
if(NOT ${XGETTEXT_MSGFMT_EXECUTABLE})
    set(XGETTEXT_MSGFMT_EXECUTABLE xgettext)
endif()

# Generate cataclysm-dda.pot
add_custom_target(
        translations
        COMMAND ${CMAKE_SOURCE_DIR}/lang/update_pot.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

foreach (LANG ${LANGUAGES})
    set(_PO ${CMAKE_SOURCE_DIR}/lang/po/${LANG}.po)
    set(_MO ${CMAKE_SOURCE_DIR}/lang/mo/${LANG}/LC_MESSAGES/cataclysm-dda.mo)
    add_custom_command(
            OUTPUT ${_MO}
            COMMAND ${CMAKE_COMMAND} -E
                make_directory ${CMAKE_SOURCE_DIR}/lang/mo/${LANG}/LC_MESSAGES
            COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -f ${_PO} -o ${_MO}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${_PO}
    )
    list(APPEND _MO_FILES ${_MO})
    if (RELEASE)
        if(USE_PREFIX_DATA_DIR)
            install(DIRECTORY ${CMAKE_SOURCE_DIR}/lang/mo/${LANG}
                TYPE LOCALE)
        else()
            install(DIRECTORY ${CMAKE_SOURCE_DIR}/lang/mo/${LANG}
                DESTINATION lang/mo)
        endif()
    endif ()
endforeach()

# Auto-Compile translation on release builds only
if (RELEASE)
    add_custom_target(locale ALL DEPENDS ${_MO_FILES})
else()
    add_custom_target(locale DEPENDS ${_MO_FILES})
endif()
