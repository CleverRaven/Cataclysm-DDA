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

if (TILES)
    set(CATACLYSM_DATA_DIRS
            ${CATACLYSM_DATA_DIRS}
            ${CMAKE_SOURCE_DIR}/gfx)
endif ()

set(CATACLYSM_DATA_FILES
        ${CMAKE_SOURCE_DIR}/data/changelog.txt
        ${CMAKE_SOURCE_DIR}/data/cataicon.ico
        ${CMAKE_SOURCE_DIR}/data/fontdata.json
        ${CMAKE_SOURCE_DIR}/LICENSE.txt)

if (RELEASE)
    install(DIRECTORY ${CATACLYSM_DATA_DIRS} DESTINATION ${DATA_PREFIX})
    install(FILES ${CATACLYSM_DATA_FILES} DESTINATION ${DATA_PREFIX})
endif ()
