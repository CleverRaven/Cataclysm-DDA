set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_BUILD_TYPE release)

# mpg123 is LGPL-2.1: static-linking into a closed binary triggers full
# LGPL obligations. Force dynamic so the DLL stays swappable; SDL_mixer
# links it via the import lib (IAT), shipped beside cataclysm-tiles.exe.
if(PORT MATCHES "^mpg123$")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
