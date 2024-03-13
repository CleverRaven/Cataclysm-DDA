cmake_minimum_required(VERSION 3.20)
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
        U8PathCheck.cpp
        UnitOverflowCheck.cpp
        UnsequencedCallsCheck.cpp
        UnusedStaticsCheck.cpp
        UseLocalizedSortingCheck.cpp
        UseMdarrayCheck.cpp
        UseNamedPointConstantsCheck.cpp
        UsePointApisCheck.cpp
        UsePointArithmeticCheck.cpp
        UseStringViewCheck.cpp
        UTF8ToLowerUpperCheck.cpp
        Utils.cpp
        XYCheck.cpp)

if (CATA_CLANG_TIDY_EXECUTABLE)
    set(CataAnalyzerName CataAnalyzer)
    add_executable(${CataAnalyzerName} ${CataAnalyzerSrc})
    target_link_libraries(${CataAnalyzerName} PRIVATE clangTidyMain)
    add_definitions(-DCATA_CLANG_TIDY_EXECUTABLE)
else ()
    set(CataAnalyzerName CataAnalyzerPlugin)
    add_library(${CataAnalyzerName} MODULE ${CataAnalyzerSrc})
    if ("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
        target_link_options(${CataAnalyzerName} PRIVATE -undefined dynamic_lookup)
    endif ()
endif ()

target_include_directories(${CataAnalyzerName} SYSTEM PRIVATE
        ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})

if (NOT "${CATA_CLANG_TIDY_INCLUDE_DIR}" STREQUAL "")
    target_include_directories(${CataAnalyzerName} SYSTEM PRIVATE ${CATA_CLANG_TIDY_INCLUDE_DIR})
endif ()

separate_arguments(LLVM_DEFINITIONS_SEP UNIX_COMMAND "${LLVM_DEFINITIONS}")
target_compile_definitions(${CataAnalyzerName} PRIVATE ${LLVM_DEFINITIONS_SEP})

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

