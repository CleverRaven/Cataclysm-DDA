#[=======================================================================[

VsDevCmd.cmake
--------------

Run VsDevCmd.bat and extracts environment variables it changes.
Inject them into the global scope and _MSVC_DEVENV for
CMakeUserPreset.json at a later step.

#]=======================================================================]

if("$ENV{VSCMD_VER}" STREQUAL "")
    # This cmake process is running under VsDevCmd.bat or VS IDE GUI
    # Avoid to run VsDevCmd.bat twice

    if ("$ENV{DevEnvDir}" STREQUAL "")
        # Use Community Edition when not specified
        set(ENV{DevEnvDir} "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/")
    endif()

    # Run VsDevCmd.bat and set all environment variables it changes
    set(DevEnvDir $ENV{DevEnvDir})
    cmake_path(APPEND DevEnvDir ../Tools/VsDevCmd.bat OUTPUT_VARIABLE VSDEVCMD_BAT)
    cmake_path(NATIVE_PATH VSDEVCMD_BAT VSDEVCMD_BAT NORMALIZE)
    cmake_path(NATIVE_PATH DevEnvDir DevEnvDir NORMALIZE)
    set(ENV{DevEnvDir} ${DevEnvDir})
    set(ENV{VSDEVCMD_BAT} \"${VSDEVCMD_BAT}\")
    # Use short DOS path names because of spaces in path names
    # See https://gitlab.kitware.com/cmake/cmake/-/issues/16321
    execute_process(COMMAND cmd /c for %A in (%VSDEVCMD_BAT%) do @echo %~sA
        OUTPUT_STRIP_TRAILING_WHITESPACE
        OUTPUT_VARIABLE VSDEVCMD_BAT)
    # Run it
    execute_process(COMMAND cmd /c ${VSDEVCMD_BAT} -no_logo -arch=amd64 && set
        OUTPUT_STRIP_TRAILING_WHITESPACE
        OUTPUT_VARIABLE _ENV)
    # This list the environment variables we need.
    # It is essentially a revised result of:
    # comm -1 -3 <(sort before.txt) <(sort after.txt) |egrep -o '^[^=]+='"
    set(_replace 
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
    string(REGEX REPLACE ";" "\\\\;" _ENV "${_ENV}")
    string(REGEX MATCHALL "[^\n]+\n" _ENV "${_ENV}")
    foreach(_env IN LISTS _ENV)
        string(REGEX MATCH ^[^=]+    _key   "${_env}")
        string(REGEX MATCH =[^\n]+\n _value "${_env}")
        # We may get some spurious output. Skip the line
        if(NOT _value MATCHES ^=)
            continue()
        endif()
        string(SUBSTRING "${_value}" 1 -1 _value) # Removes the = at begin
        string(STRIP "${_value}" _value) # Remove the \r
        list(FIND _replace ${_key}= _idx)
        if(-1 EQUAL _idx)
            continue()
        endif()
        list(REMOVE_AT _replace ${_idx})
        set(ENV{${_key}} "${_value}")
        string(REPLACE \\ \\\\ _value "${_value}")
        set(_json_entry "\"${_key}\": \"${_value}\"")
        if("${_MSVC_DEVENV}" STREQUAL "")
            string(APPEND _MSVC_DEVENV "${_json_entry}")
            continue()
        endif()
        string(APPEND _MSVC_DEVENV ",\n${_json_entry}")
    endforeach() # _ENV
endif() # VSCMD_VER

