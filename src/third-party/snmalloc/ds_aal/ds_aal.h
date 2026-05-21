/**
 * Data structures used by snmalloc that only depend on the AAL (Architecture
 * Abstraction Layer) and not on the platform. These structures can be used
 * to implement the Pal.
 */
#pragma once
#include "../aal/aal.h"
#include "flaglock.h"
#include "prevent_fork.h"
#include "seqset.h"
#include "singleton.h"