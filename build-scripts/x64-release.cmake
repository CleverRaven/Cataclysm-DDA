if (NOT $ENV{VCPKG_INSTALLATION_ROOT} STREQUAL "")
    set(ENV{VCPKG_ROOT} $ENV{VCPKG_INSTALLATION_ROOT})
endif()
if ("$ENV{VCPKG_ROOT}" STREQUAL "" AND WIN32)
    set(ENV{VCPKG_ROOT} C:/vcpkg)
endif()

if("$ENV{VSCMD_VER}" STREQUAL "")
    # This cmake process is running under VsDevCmd.bat or VS IDE GUI
    # Avoid to run VsDevCmd.bat twice

    if (NOT "$ENV{DevEnvDir}")
        # Use Community Edition when not specified
        set(ENV{DevEnvDir} "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\")
    endif()
    # Ninja is provided by Microsoft but not in the Path
    set(CMAKE_MAKE_PROGRAM $ENV{DevEnvDir}CommonExtensions\\Microsoft\\CMake\\Ninja\\ninja.exe CACHE PATH "")

    # Run VsDevCmd.bat and set all environment variables it changes
    set(VSDEVCMD_BAT "$ENV{DevEnvDir}\\..\\Tools\\VsDevCmd.bat")
    cmake_path(NATIVE_PATH VSDEVCMD_BAT VSDEVCMD_BAT)
    execute_process(COMMAND cmd /c ${VSDEVCMD_BAT} -no_logo -arch=amd64 && set
        OUTPUT_STRIP_TRAILING_WHITESPACE
        OUTPUT_VARIABLE _ENV)
    string(REGEX REPLACE ";" "\\\\;" _ENV "${_ENV}")
    string(REGEX MATCHALL "[^\n]+\n" _ENV "${_ENV}")
    foreach(_env IN LISTS _ENV)
        string(REGEX MATCH ^[^=]+    _key   "${_env}")
        string(REGEX MATCH =[^\n]+\n _value "${_env}")
        message(XX${_value}XX)
        string(SUBSTRING "${_value}" 1 -1 _value) # Remove = at begin
        string(STRIP "${_value}" _value) # Remove \r
        # This list is essentially a revised result of :comm -1 -3 <(sort before.txt) <(sort after.txt) |egrep -o '^[^=]+='"
        foreach(_replace 
            ExtensionSdkDir=
            Framework40Version=
            FrameworkDIR64=
            FrameworkDir=
            FrameworkVersion64=
            FrameworkVersion=
            INCLUDE=
            LIB=
            LIBPATH=
            NETFXSDKDir=
            Path=
            UCRTVersion=
            UniversalCRTSdkDir=
            VCIDEInstallDir=
            VCINSTALLDIR=
            VCToolsInstallDir=
            VCToolsRedistDir=
            VCToolsVersion=
            VS170COMNTOOLS=
            VSCMD_ARG_HOST_ARCH=
            VSCMD_ARG_TGT_ARCH=
            VSCMD_ARG_app_plat=
            VSCMD_VER=
            VSINSTALLDIR=
            VisualStudioVersion=
            WindowsLibPath=
            WindowsSDKLibVersion=
            WindowsSDKVersion=
            WindowsSDK_ExecutablePath_x64=
            WindowsSDK_ExecutablePath_x86=
            WindowsSdkBinPath=
            WindowsSdkDir=
            WindowsSdkVerBinPath=
        )
            if("${_key}=" STREQUAL "${_replace}")
                set(ENV{${_key}} "${_value}")
                string(REPLACE \\ \\\\ _value "${_value}")
                set(_json_entry "\"${_key}\": \"${_value}\"")
                if("${_MSVC_DEVENV}" STREQUAL "")
                    string(APPEND _MSVC_DEVENV "${_json_entry}")
                else()
                    string(APPEND _MSVC_DEVENV ",\n${_json_entry}")
                endif()
            endif()
        endforeach()
    endforeach()
endif() # VSCMD_VER
configure_file(
    ${CMAKE_SOURCE_DIR}/build-scripts/CMakeUserPresets.json.in
    ${CMAKE_SOURCE_DIR}/CMakeUserPresets.json
    @ONLY
)
