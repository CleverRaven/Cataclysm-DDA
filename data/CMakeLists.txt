set(CATACLYSM_DATA_DIRS
        font
        json
        mods
        names
        raw
        motd
        credits
        title
        core
        screenshots
        xdg
        help)

if (SOUND)
    list(APPEND CATACLYSM_DATA_DIRS sound)
endif ()

if (USE_SDL3)
    list(APPEND CATACLYSM_DATA_DIRS shaders)
endif ()

set(CATACLYSM_DATA_FILES
        changelog.txt
        cataicon.ico
        fontdata.json)

if (RELEASE)
    install(DIRECTORY ${CATACLYSM_DATA_DIRS} TYPE DATA)
    install(FILES ${CATACLYSM_DATA_FILES} TYPE DATA)
endif ()
