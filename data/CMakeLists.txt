cmake_minimum_required(VERSION 3.1.4)

set(CATACLYSM_DATA_DIRS
        ${CMAKE_SOURCE_DIR}/data/font
        ${CMAKE_SOURCE_DIR}/data/json
        ${CMAKE_SOURCE_DIR}/data/mods
        ${CMAKE_SOURCE_DIR}/data/names
        ${CMAKE_SOURCE_DIR}/data/raw
        ${CMAKE_SOURCE_DIR}/data/motd
        ${CMAKE_SOURCE_DIR}/data/credits
        ${CMAKE_SOURCE_DIR}/data/title
        ${CMAKE_SOURCE_DIR}/data/core
        ${CMAKE_SOURCE_DIR}/data/help)

if (SOUND)
    set(CATACLYSM_DATA_DIRS
            ${CATACLYSM_DATA_DIRS}
            ${CMAKE_SOURCE_DIR}/data/sound)
endif ()

set(CATACLYSM_DATA_FILES
        ${CMAKE_SOURCE_DIR}/data/changelog.txt
        ${CMAKE_SOURCE_DIR}/data/cataicon.ico
        ${CMAKE_SOURCE_DIR}/data/fontdata.json)

if (RELEASE)
    install(DIRECTORY ${CATACLYSM_DATA_DIRS} DESTINATION ${DATA_PREFIX})
    install(FILES ${CATACLYSM_DATA_FILES} DESTINATION ${DATA_PREFIX})
    install(FILES
        ${CMAKE_SOURCE_DIR}/README.md
        ${CMAKE_SOURCE_DIR}/LICENSE.txt
        ${CMAKE_SOURCE_DIR}/LICENSE-OFL-Terminus-Font.txt
        DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc DESTINATION ${CMAKE_INSTALL_PREFIX})
    if (TILES)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/gfx DESTINATION ${CMAKE_INSTALL_PREFIX})
    endif()
endif ()
