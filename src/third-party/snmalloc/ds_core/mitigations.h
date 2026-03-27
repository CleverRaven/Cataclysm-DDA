#pragma once
#include "defines.h"

#include <stddef.h>

namespace snmalloc
{
  template<typename... Args>
  inline SNMALLOC_FAST_PATH void
  check_client_error(const char* const str, Args... args)
  {
    //[[clang::musttail]]
    return snmalloc::report_fatal_error(str, args...);
  }

  template<typename... Args>
  inline SNMALLOC_FAST_PATH void
  check_client_impl(bool test, const char* const str, Args... args)
  {
    if (SNMALLOC_UNLIKELY(!test))
    {
      if constexpr (!Debug)
      {
        UNUSED(str, args...);
        SNMALLOC_FAST_FAIL();
      }
      else
      {
        check_client_error(str, args...);
      }
    }
  }

#ifdef SNMALLOC_CHECK_CLIENT
  static constexpr bool CHECK_CLIENT = true;
#else
  static constexpr bool CHECK_CLIENT = false;
#endif

  namespace mitigation
  {
    class type
    {
      size_t mask;

    public:
      constexpr type(size_t f) : mask(f){};
      constexpr type(const type& t) = default;

      constexpr type operator+(const type b) const
      {
        return {mask | b.mask};
      }

      constexpr type operator-(const type b) const
      {
        return {mask & ~(b.mask)};
      }

      constexpr bool operator()(const type a) const
      {
        return (mask & a.mask) != 0;
      }
    };
  } // namespace mitigation

  /**
   * Randomize the location of the pagemap within a larger address space
   * allocation.  The other pages in that allocation may fault if accessed, on
   * platforms that can efficiently express such configurations.
   *
   * This guards against adversarial attempts to access the pagemap.
   *
   * This is unnecessary on StrictProvenance architectures.
   */
  constexpr mitigation::type random_pagemap{1 << 0};
  /**
   * Ensure that every slab (especially slabs used for larger "small" size
   * classes) has a larger minimum number of objects and that a larger
   * percentage of objects in a slab must be free to awaken the slab.
   *
   * This should frustrate use-after-reallocation attacks by delaying reuse.
   * When combined with random_preserve, below, it additionally ensures that at
   * least some shuffling of free objects is possible, and, hence, that there
   * is at least some unpredictability of reuse.
   *
   * TODO: should this be split? mjp: Would require changing some thresholds.
   * The min waking count needs to be ensure we have enough objects on a slab,
   * hence is related to the min count on a slab.  Currently we without this, we
   * have min count of slab of 16, and a min waking count with this enabled
   * of 32. So we would leak memory.
   */
  constexpr mitigation::type random_larger_thresholds{1 << 1};
  /**
   *
   * Obfuscate forward-edge pointers in intra-slab free lists.
   *
   * This helps prevent a UAF write from re-pointing the free list arbitrarily,
   * as the de-obfuscation of a corrupted pointer will generate a wild address.
   *
   * This is not available on StrictProvenance architectures.
   */
  constexpr mitigation::type freelist_forward_edge{1 << 2};
  /**
   * Store obfuscated backward-edge addresses in intra-slab free lists.
   *
   * Ordinarily, these lists are singly-linked.  Storing backward-edges allows
   * the allocator to verify the well-formedness of the links and, importantly,
   * the acyclicity of the list itself.  These backward-edges are also
   * obfuscated in an attempt to frustrate an attacker armed with UAF
   * attempting to construct a new well-formed list.
   *
   * Because the backward-edges are not traversed, this is available on
   * StrictProvenance architectures, unlike freelist_forward_edge.
   *
   * This is required to detect double frees as it will break the doubly linked
   * nature of the free list.
   */
  constexpr mitigation::type freelist_backward_edge{1 << 3};
  /**
   * When de-purposing a slab (releasing its address space for reuse at a
   * different size class or allocation), walk the free list and validate the
   * domestication of all nodes along it.
   *
   * If freelist_forward_edge is also enabled, this will probe the
   * domestication status of the de-obfuscated pointers before traversal.
   * Each of domestication and traversal may probabilistically catch UAF
   * corruption of the free list.
   *
   * If freelist_backward_edge is also enabled, this will verify the integrity
   * of the free list links.
   *
   * This gives the allocator "one last chance" to catch UAF corruption of a
   * slab's free list before the slab is de-purposed.
   *
   * This is required to comprehensively detect double free.
   */
  constexpr mitigation::type freelist_teardown_validate{1 << 4};
  /**
   * When initializing a slab, shuffle its free list.
   *
   * This guards against attackers relying on object-adjacency or address-reuse
   * properties of the allocation stream.
   */
  constexpr mitigation::type random_initial{1 << 5};
  /**
   * When a slab is operating, randomly assign freed objects to one of two
   * intra-slab free lists.  When selecting a slab's free list for allocations,
   * select the longer of the two.
   *
   * This guards against attackers relying on object-adjacency or address-reuse
   * properties of the allocation stream.
   */
  constexpr mitigation::type random_preserve{1 << 6};
  /**
   * Randomly introduce another slab for a given size-class, rather than use
   * the last available to an allocator.
   *
   * This guards against attackers relying on address-reuse, especially in the
   * pathological case of a size-class having only one slab with free entries.
   */
  constexpr mitigation::type random_extra_slab{1 << 7};
  /**
   * Use a LIFO queue, rather than a stack, of slabs with free elements.
   *
   * This generally increases the time between address reuse.
   */
  constexpr mitigation::type reuse_LIFO{1 << 8};
  /**
   * This performs a variety of inexpensive "sanity" tests throughout the
   * allocator:
   *
   * - Requests to free objects must
   *   - not be interior pointers
   *   - be of allocated address space
   * - Requests to free objects which also specify the size must specify a size
   *   that agrees with the current allocation.
   *
   * This guards gainst various forms of client misbehavior.
   *
   * TODO: Should this be split? mjp: It could, but let's not do this until
   * we have performance numbers to see what this costs.
   */
  constexpr mitigation::type sanity_checks{1 << 9};
  /**
   * On CHERI, perform a series of well-formedness tests on capabilities given
   * when requesting to free an object.
   */
  constexpr mitigation::type cheri_checks{1 << 10};
  /**
   * Erase intra-slab free list metadata before completing an allocation.
   *
   * This mitigates information disclosure.
   */
  constexpr mitigation::type clear_meta{1 << 11};
  /**
   * Protect meta data blocks by allocating separate from chunks for
   * user allocations. This involves leaving gaps in address space.
   * This is less efficient, so should only be applied for the checked
   * build.
   */
  constexpr mitigation::type metadata_protection{1 << 12};
  /**
   * If this mitigation is enabled, then Pal implementations should provide
   * exceptions/segfaults if accesses do not obey the
   *  - using
   *  - using_readonly
   *  - not_using
   * model.
   */
  static constexpr mitigation::type pal_enforce_access{1 << 13};

  constexpr mitigation::type full_checks = random_pagemap +
    random_larger_thresholds + freelist_forward_edge + freelist_backward_edge +
    freelist_teardown_validate + random_initial + random_preserve +
    metadata_protection + random_extra_slab + reuse_LIFO + sanity_checks +
    clear_meta + pal_enforce_access;

  constexpr mitigation::type no_checks{0};

  using namespace mitigation;
  constexpr mitigation::type mitigations =
#ifdef SNMALLOC_CHECK_CLIENT_MITIGATIONS
    no_checks + SNMALLOC_CHECK_CLIENT_MITIGATIONS;
#elif defined(OPEN_ENCLAVE)
    /**
     * On Open Enclave the address space is limited, so we disable
     * metadata-protection feature.
     */
    CHECK_CLIENT ? full_checks - metadata_protection - random_pagemap :
                   no_checks;
#elif defined(__NetBSD__)
    /**
     * pal_enforce_access was failing on NetBSD, so we disable it.
     */
    CHECK_CLIENT ? full_checks - pal_enforce_access : no_checks;
#elif defined(__CHERI_PURE_CAPABILITY__)
    CHECK_CLIENT ?
    /**
     * freelist_forward_edge should not be used on CHERI as we cannot encode
     * pointers as the tag will be destroyed.
     *
     * TODO: There is a known bug in CheriBSD that means round-tripping through
     * PROT_NONE sheds capability load and store permissions (while restoring
     * data read/write, for added excitement).  For the moment, just force this
     * down on CHERI.
     */
    full_checks + cheri_checks + clear_meta - freelist_forward_edge -
      pal_enforce_access :
     /**
      * clear_meta is important on CHERI to avoid leaking capabilities.
      */
     sanity_checks + cheri_checks + clear_meta;
#else
    CHECK_CLIENT ? full_checks : no_checks;
#endif
} // namespace snmalloc

#define snmalloc_check_client(mitigation, test, str, ...) \
  if constexpr (mitigation) \
  { \
    snmalloc::check_client_impl(test, str, ##__VA_ARGS__); \
  }
