# Cataclysm data files
cmake_minimum_required(VERSION 3.1.4)

SET(CATACLYSM_DATA_DIRS
    ${CMAKE_SOURCE_DIR}/data/font
    ${CMAKE_SOURCE_DIR}/data/json
    ${CMAKE_SOURCE_DIR}/data/mods
    ${CMAKE_SOURCE_DIR}/data/names
    ${CMAKE_SOURCE_DIR}/data/raw
    ${CMAKE_SOURCE_DIR}/data/motd
    ${CMAKE_SOURCE_DIR}/data/credits
    ${CMAKE_SOURCE_DIR}/data/title
)

IF(SOUND)
    SET(CATACLYSM_DATA_DIRS
      ${CATACLYSM_DATA_DIRS}
      ${CMAKE_SOURCE_DIR}/data/sound
    )
ENDIF(SOUND)

IF(TILES)
    SET(CATACLYSM_DATA_DIRS
      ${CATACLYSM_DATA_DIRS}
      ${CMAKE_SOURCE_DIR}/gfx
    )
ENDIF(TILES)

SET(CATACLYSM_DATA_FILES
    ${CMAKE_SOURCE_DIR}/data/changelog.txt
    ${CMAKE_SOURCE_DIR}/data/cataicon.ico
    ${CMAKE_SOURCE_DIR}/data/fontdata.json
    ${CMAKE_SOURCE_DIR}/LICENSE.txt
)

IF(RELEASE)
    install(DIRECTORY ${CATACLYSM_DATA_DIRS} DESTINATION ${DATA_PREFIX})
    install(FILES ${CATACLYSM_DATA_FILES} DESTINATION ${DATA_PREFIX})
ENDIF(RELEASE)
