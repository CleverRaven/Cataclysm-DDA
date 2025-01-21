#pragma once
#ifndef CATA_SRC_COMPILER_ATTRIBUTES_H
#define CATA_SRC_COMPILER_ATTRIBUTES_H

#ifdef __clang__
#define CLANG_REINITIALIZES [[clang::reinitializes]]
#else
#define CLANG_REINITIALIZES
#endif

#endif // CATA_SRC_COMPILER_ATTRIBUTES_H
