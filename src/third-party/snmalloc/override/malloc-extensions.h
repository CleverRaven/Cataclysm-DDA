#pragma once

/**
 * Malloc extensions
 *
 * This file contains additional non-standard API surface for snmalloc.
 * The API is subject to changes, but will be clearly noted in release
 * notes.
 */

/**
 * Structure for returning memory used by snmalloc.
 *
 * The statistics are very coarse grained as they only track
 * usage at the superslab/chunk level. Meta-data and object
 * data is not tracked independantly.
 */
struct malloc_info_v1
{
  /**
   * Current memory usage of the allocator. Extremely coarse
   * grained for efficient calculation.
   */
  size_t current_memory_usage;

  /**
   * High-water mark of current_memory_usage.
   */
  size_t peak_memory_usage;
};

/**
 * Populates a malloc_info_v1 structure for the latest values
 * from snmalloc.
 */
void get_malloc_info_v1(malloc_info_v1* stats);
