#pragma once
#ifndef COMPILER_DEFINES_H
#define COMPILER_DEFINES_H

#ifndef __has_cpp_attribute       // Optional of course.
#define __has_cpp_attribute(x) 0  // Compatibility with non-clang compilers.
#endif

#if __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]];
#elseif __has_cpp_attribute(clang::fallthrough)
#define FALLTHROUGH [[clang::fallthrough]];
#else
#ifdef _MSC_VER
#define FALLTHROUGH [[fallthrough]];
#else
#define FALLTHROUGH
#endif
#endif

#endif
