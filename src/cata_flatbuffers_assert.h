#ifndef CATA_SRC_CATA_FLATBUFFERS_ASSERT_H
#define CATA_SRC_CATA_FLATBUFFERS_ASSERT_H

// A substitute for assert to be used in flatbuffers / flexbuffers so that parsing errors
// raise a catchable error like currently happens with JsonObject/JsonIn

#ifdef FLATBUFFERS_ASSERT
#undef FLATBUFFERS_ASSERT
#endif

#include "cata_assert.h"

#define FLATBUFFERS_ASSERT(expression) cata_assert(expression)

#endif
