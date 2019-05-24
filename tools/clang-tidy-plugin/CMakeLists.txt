add_library(
    CataAnalyzerPlugin MODULE
    CataTidyModule.cpp NoLongCheck.cpp)
MESSAGE("Building clang-tidy plugin with include paths: ${LLVM_INCLUDE_DIRS}")
if ("${LLVM_INCLUDE_DIRS}" STREQUAL "")
else()
    target_include_directories(
        CataAnalyzerPlugin SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
endif()
if(MSVC)
else()
    target_compile_options(
        CataAnalyzerPlugin PRIVATE -fno-exceptions -fno-rtti)
endif()
