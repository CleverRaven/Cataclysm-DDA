MESSAGE(STATUS "Process LANGUAGES variable --\n")

FIND_PACKAGE(Gettext)

IF (NOT GETTEXT_FOUND)
    MESSAGE(FATAL_ERROR
            "Gettext not found. Install gettext package or disable
             localization with: -DLOCALIZE=OFF \nSee INSTALL file for details and more info\n"
    )
ENDIF (NOT GETTEXT_FOUND)

foreach (LANG ${LANGUAGES})
    MESSAGE(STATUS "Add translation for ${LANG}: ${LANG}.po")
endforeach ()
MESSAGE("\n")

# Extract json strings

add_custom_target (
    extract_string
    COMMAND python lang/extract_json_strings.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Generate cataclysm-dda.pot
add_custom_target (
    translations
    COMMAND xgettext --default-domain="cataclysm-dda"
                     --sort-by-file
                     --add-comments="~"
                     --output="${CMAKE_SOURCE_DIR}/lang/po/cataclysm-dda.pot"
                     --keyword="_"
                     --keyword="pgettext:1c,2"
                     --keyword="ngettext:1,2"
                     --from-code="UTF-8"
                     ${CMAKE_SOURCE_DIR}/src/*.cpp
                     ${CMAKE_SOURCE_DIR}/src/*.h
                     ${CMAKE_SOURCE_DIR}/lang/json/*.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS extract_string
)

add_custom_target (
    translations_prepare
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Auto-Compile translation on release builds only
IF (RELEASE)
    add_custom_target (
      translations_compile
      ALL
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      DEPENDS translations_prepare
    )
ELSE (RELEASE)
    add_custom_target (
      translations_compile
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      DEPENDS translations_prepare
    )
ENDIF (RELEASE)

foreach (LANG ${LANGUAGES})
    add_custom_command (
      TARGET translations_prepare
      PRE_BUILD
      COMMAND ${CMAKE_COMMAND}
              -E make_directory ${CMAKE_SOURCE_DIR}/lang/mo/${LANG}/LC_MESSAGES
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    add_custom_command (
      TARGET translations_compile
      PRE_BUILD
      COMMAND 
      ${GETTEXT_MSGFMT_EXECUTABLE} -f ${CMAKE_SOURCE_DIR}/lang/po/${LANG}.po
             -o ${CMAKE_SOURCE_DIR}/lang/mo/${LANG}/LC_MESSAGES/cataclysm-dda.mo
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    IF(RELEASE)
      IF (${CMAKE_SYSTEM_NAME} MATCHES Windows)
        #install(DIRECTORY ${CMAKE_SOURCE_DIR}/lang/mo/${LANG} DESTINATION ${DATA_PREFIX})
      ELSE (${CMAKE_SYSTEM_NAME} MATCHES Windows)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/lang/mo/${LANG} DESTINATION ${LOCALE_DIR})
      ENDIF (${CMAKE_SYSTEM_NAME} MATCHES Windows)
    ENDIF(RELEASE)
endforeach()

