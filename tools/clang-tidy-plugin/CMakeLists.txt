cmake_minimum_required(VERSION 3.1.4)
include(ExternalProject)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

set(CataAnalyzerSrc
        AlmostNeverAutoCheck.cpp
        AssertCheck.cpp
        AvoidAlternativeTokensCheck.cpp
        CataTidyModule.cpp
        CombineLocalsIntoPointCheck.cpp
        DeterminismCheck.cpp
        HeaderGuardCheck.cpp
        JsonTranslationInputCheck.cpp
        LargeInlineFunctionCheck.cpp
        LargeStackObjectCheck.cpp
        NoLongCheck.cpp
        NoStaticTranslationCheck.cpp
        OtMatchCheck.cpp
        PointInitializationCheck.cpp
        RedundantParenthesesCheck.cpp
        SerializeCheck.cpp
        SimplifyPointConstructorsCheck.cpp
        StaticDeclarationsCheck.cpp
        StaticInitializationOrderCheck.cpp
        StaticIntIdConstantsCheck.cpp
        StaticStringIdConstantsCheck.cpp
        StringLiteralIterator.cpp
        TestFilenameCheck.cpp
        TestsMustRestoreGlobalStateCheck.cpp
        TextStyleCheck.cpp
        TranslateStringLiteralCheck.cpp
        TranslationsInDebugMessagesCheck.cpp
        TranslatorCommentsCheck.cpp
        UnsequencedCallsCheck.cpp
        UnusedStaticsCheck.cpp
        UseLocalizedSortingCheck.cpp
        UseNamedPointConstantsCheck.cpp
        UsePointApisCheck.cpp
        UsePointArithmeticCheck.cpp
        UTF8ToLowerUpperCheck.cpp
        Utils.cpp
        XYCheck.cpp)

if (CATA_CLANG_TIDY_EXECUTABLE)
    set(CataAnalyzerName CataAnalyzer)
    add_executable(${CataAnalyzerName} ${CataAnalyzerSrc})
    target_link_libraries(${CataAnalyzerName} clangTidyMain)
    add_definitions(-DCATA_CLANG_TIDY_EXECUTABLE)
else ()
    set(CataAnalyzerName CataAnalyzerPlugin)
    add_library(${CataAnalyzerName} MODULE ${CataAnalyzerSrc})
endif ()

target_include_directories(${CataAnalyzerName} SYSTEM PRIVATE
        ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})

if ("${CATA_CLANG_TIDY_INCLUDE_DIR}" STREQUAL "")
    set(CTPS_RELEASES https://github.com/jbytheway/clang-tidy-plugin-support/releases/download)
    set(CTPS_VERSION llvm-12.0.0-r3)
    set(CTPS_SRC ${CMAKE_CURRENT_BINARY_DIR}/clang-tidy-plugin-support)

    ExternalProject_Add(clang-tidy-plugin-support
            URL ${CTPS_RELEASES}/${CTPS_VERSION}/clang-tidy-plugin-support-${CTPS_VERSION}.tar.xz
            URL_HASH SHA256=c84aaf35b4d5bb9130b1cc309a92146ae5c62fd69db30a6254725beac9d60990
            SOURCE_DIR ${CTPS_SRC}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
            TEST_COMMAND "")

    add_dependencies(${CataAnalyzerName} clang-tidy-plugin-support)
    target_include_directories(${CataAnalyzerName} SYSTEM PRIVATE ${CTPS_SRC}/include)
else ()
    target_include_directories(${CataAnalyzerName} SYSTEM PRIVATE ${CATA_CLANG_TIDY_INCLUDE_DIR})
endif ()

target_compile_definitions(${CataAnalyzerName} PRIVATE ${LLVM_DEFINITIONS})

# We need to turn off exceptions and RTTI to match the LLVM build.
# I feel there ought to be a way to extract these flags from the
# LLVMConfig.cmake as we have done for e.g. LLVM_INCLUDE_DIRS above, but I
# haven't found one.
if (MSVC)
else ()
    target_compile_options(${CataAnalyzerName} PRIVATE -fno-exceptions -fno-rtti)
endif ()

configure_file(test/lit.site.cfg.in test/lit.site.cfg @ONLY)
configure_file(test/.clang-tidy test/.clang-tidy COPYONLY)

