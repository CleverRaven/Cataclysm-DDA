add_compile_options(
        /MP # cl.exe build with multiple processes
        /utf-8 # set source and execution character sets to UTF-8
        /bigobj # increase # of sections in object files
        /permissive- # to allow alternative operators ("and", "or", "not")
        /sdl- # disable additional security checks
        /GS- # disable buffer security checks
        /wd4068 # unknown pragma
        /wd4146 # negate unsigned
        /wd4819 # codepage?
        /wd6237 # short-circuit eval
        /wd6319 # a, b: unused a
        /wd26444 # unnamed objects
        /wd26451 # overflow
        /wd26495 # uninitialized member
)
add_compile_definitions(
    _SCL_SECURE_NO_WARNINGS
    _CTR_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
    LOCALIZE
    USE_VCPKG
)
add_link_options(
    /LTCG:OFF
    /MANIFEST:NO
)
