#pragma once

#include "mitigations.h"

namespace snmalloc
{
  /*
   * Many of these tests come with an "or is null" branch that they'd need to
   * add if we did them up front.  Instead, defer them until we're past the
   * point where we know, from the pagemap, or by explicitly testing, that the
   * pointer under test is not nullptr.
   */
  SNMALLOC_FAST_PATH_INLINE void dealloc_cheri_checks(void* p)
  {
#if defined(__CHERI_PURE_CAPABILITY__)
    /*
     * Enforce the use of an unsealed capability.
     *
     * TODO In CHERI+MTE, this, is part of the CAmoCDecVersion instruction;
     * elide this test in that world.
     */
    snmalloc_check_client(
      mitigations(cheri_checks),
      !__builtin_cheri_sealed_get(p),
      "Sealed capability in deallocation");

    /*
     * Enforce permissions on the returned pointer.  These pointers end up in
     * free queues and will be cycled out to clients again, so try to catch
     * erroneous behavior now, rather than later.
     *
     * TODO In the CHERI+MTE case, we must reconstruct the pointer for the
     * free queues as part of the discovery of the start of the object (so
     * that it has the correct version), and the CAmoCDecVersion call imposes
     * its own requirements on the permissions (to ensure that it's at least
     * not zero).  They are somewhat more lax than we might wish, so this test
     * may remain, guarded by SNMALLOC_CHECK_CLIENT, but no explicit
     * permissions checks are required in the non-SNMALLOC_CHECK_CLIENT case
     * to defend ourselves or other clients against a misbehaving client.
     */
    static const size_t reqperm = CHERI_PERM_LOAD | CHERI_PERM_STORE |
      CHERI_PERM_LOAD_CAP | CHERI_PERM_STORE_CAP;
    snmalloc_check_client(
      mitigations(cheri_checks),
      (__builtin_cheri_perms_get(p) & reqperm) == reqperm,
      "Insufficient permissions on capability in deallocation");

    /*
     * We check for a valid tag here, rather than in domestication, because
     * domestication might be answering a slightly different question, about
     * the plausibility of addresses rather than of exact pointers.
     *
     * TODO Further, in the CHERI+MTE case, the tag check will be implicit in
     * a future CAmoCDecVersion instruction, and there should be no harm in
     * the lookups we perform along the way to get there.  In that world,
     * elide this test.
     */
    snmalloc_check_client(
      mitigations(cheri_checks),
      __builtin_cheri_tag_get(p),
      "Untagged capability in deallocation");

    /*
     * Verify that the capability is not zero-length, ruling out the other
     * edge case around monotonicity.
     */
    snmalloc_check_client(
      mitigations(cheri_checks),
      __builtin_cheri_length_get(p) > 0,
      "Zero-length capability in deallocation");

    /*
     * At present we check for the pointer also being the start of an
     * allocation closer to dealloc; for small objects, that happens in
     * dealloc_local_object, either below or *on the far end of message
     * receipt*.  For large objects, it happens below by directly rounding to
     * power of two rather than using the is_start_of_object helper.
     * (XXX This does mean that we might end up threading our remote queue
     * state somewhere slightly unexpected rather than at the head of an
     * object.  That is perhaps fine for now?)
     */

    /*
     * TODO
     *
     * We could enforce other policies here, including that the length exactly
     * match the sizeclass.  At present, we bound caps we give for allocations
     * to the underlying sizeclass, so even malloc(0) will have a non-zero
     * length.  Monotonicity would then imply that the pointer must be the
     * head of an object (modulo, perhaps, temporal aliasing if we somehow
     * introduced phase shifts in heap layout like some allocators do).
     *
     * If we switched to bounding with upwards-rounded representable bounds
     * (c.f., CRRL) rather than underlying object size, then we should,
     * instead, in general require plausibility of p_raw by checking that its
     * length is nonzero and the snmalloc size class associated with its
     * length is the one for the slab in question... except for the added
     * challenge of malloc(0).  Since 0 rounds up to 0, we might end up
     * constructing zero-length caps to hand out, which we would then reject
     * upon receipt.  Instead, as part of introducing CRRL bounds, we should
     * introduce a sizeclass for slabs holding zero-size objects.  All told,
     * we would want to check that
     *
     *   size_to_sizeclass(length) == entry.get_sizeclass()
     *
     * I believe a relaxed CRRL test of
     *
     *   length > 0 || (length == sizeclass_to_size(entry.get_sizeclass()))
     *
     * would also suffice and may be slightly less expensive than the test
     * above, at the cost of not catching as many misbehaving clients.
     *
     * In either case, having bounded by CRRL bounds, we would need to be
     * *reconstructing* the capabilities headed to our free lists to be given
     * out to clients again; there are many more CRRL classes than snmalloc
     * sizeclasses (this is the same reason that we can always get away with
     * CSetBoundsExact in capptr_bound).  Switching to CRRL bounds, if that's
     * ever a thing we want to do, will be easier after we've done the
     * plumbing for CHERI+MTE.
     */

    /*
     * TODO: Unsurprisingly, the CHERI+MTE case once again has something to
     * say here.  In that world, again, we are certain to be reconstructing
     * the capability for the free queue anyway, and so exactly what we wish
     * to enforce, length-wise, of the provided capability, is somewhat more
     * flexible.  Using the provided capability bounds when recoloring memory
     * could be a natural way to enforce that it covers the entire object, at
     * the cost of a more elaborate recovery story (as we risk aborting with a
     * partially recolored object).  On non-SNMALLOC_CHECK_CLIENT builds, it
     * likely makes sense to just enforce that length > 0 (*not* enforced by
     * the CAmoCDecVersion instruction) and say that any authority-bearing
     * interior pointer suffices to free the object.  I believe that to be an
     * acceptable security posture for the allocator and between clients;
     * misbehavior is confined to the misbehaving client.
     */
#else
    UNUSED(p);
#endif
  }
} // namespace snmalloc