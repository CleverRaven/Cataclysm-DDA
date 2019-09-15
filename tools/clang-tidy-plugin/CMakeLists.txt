include(ExternalProject)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

add_library(
    CataAnalyzerPlugin MODULE
    CataTidyModule.cpp
    NoStaticGettextCheck.cpp
    NoLongCheck.cpp
    PointInitializationCheck.cpp
    SimplifyPointConstructorsCheck.cpp
    UseNamedPointConstantsCheck.cpp
    UsePointApisCheck.cpp
    UsePointArithmeticCheck.cpp
    Utils.cpp
    XYCheck.cpp
    )

target_include_directories(
    CataAnalyzerPlugin SYSTEM PRIVATE
    ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})

if ("${CATA_CLANG_TIDY_INCLUDE_DIR}" STREQUAL "")
    SET(ctps_releases
        https://github.com/jbytheway/clang-tidy-plugin-support/releases/download)
    SET(ctps_version llvm-8.0.1-r12)
    SET(ctps_src
        ${CMAKE_CURRENT_BINARY_DIR}/clang-tidy-plugin-support)

    ExternalProject_Add(
        clang-tidy-plugin-support
        URL ${ctps_releases}/${ctps_version}/clang-tidy-plugin-support-${ctps_version}.tar.xz
        URL_HASH SHA256=00ffab0df11250f394830735514c62ae787bd2eb6eb9d5e97471206d270c54e2
        SOURCE_DIR ${ctps_src}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        TEST_COMMAND ""
    )

    add_dependencies(CataAnalyzerPlugin clang-tidy-plugin-support)
    target_include_directories(
        CataAnalyzerPlugin SYSTEM PRIVATE ${ctps_src}/include)
else()
    target_include_directories(
        CataAnalyzerPlugin SYSTEM PRIVATE ${CATA_CLANG_TIDY_INCLUDE_DIR})
endif()

target_compile_definitions(
    CataAnalyzerPlugin PRIVATE ${LLVM_DEFINITIONS})

# We need to turn off exceptions and RTTI to match the LLVM build.
# I feel there ought to be a way to extract these flags from the
# LLVMConfig.cmake as we have done for e.g. LLVM_INCLUDE_DIRS above, but I
# haven't found one.
if(MSVC)
else()
    target_compile_options(
        CataAnalyzerPlugin PRIVATE -fno-exceptions -fno-rtti)
endif()

configure_file(test/lit.site.cfg.in test/lit.site.cfg @ONLY)
configure_file(test/.clang-tidy test/.clang-tidy COPYONLY)
