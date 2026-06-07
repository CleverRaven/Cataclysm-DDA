#pragma once

namespace snmalloc
{
  // 0 intermediate bits results in power of 2 small allocs. 1 intermediate
  // bit gives additional sizeclasses at the midpoint between each power of 2.
  // 2 intermediate bits gives 3 intermediate sizeclasses, etc.
  static constexpr size_t INTERMEDIATE_BITS =
#ifdef USE_INTERMEDIATE_BITS
    USE_INTERMEDIATE_BITS
#else
    2
#endif
    ;

  // The remaining values are derived, not configurable.
  static constexpr size_t POINTER_BITS =
    bits::next_pow2_bits_const(sizeof(uintptr_t));

  // Used to isolate values on cache lines to prevent false sharing.
  static constexpr size_t CACHELINE_SIZE = 64;

  /// The "machine epsilon" for the small sizeclass machinery.
  static constexpr size_t MIN_ALLOC_STEP_SIZE =
#if defined(SNMALLOC_MIN_ALLOC_STEP_SIZE)
    SNMALLOC_MIN_ALLOC_STEP_SIZE;
#else
    2 * sizeof(void*);
#endif

  /// Derived from MIN_ALLOC_STEP_SIZE
  static constexpr size_t MIN_ALLOC_STEP_BITS =
    bits::ctz_const(MIN_ALLOC_STEP_SIZE);
  static_assert(bits::is_pow2(MIN_ALLOC_STEP_SIZE));

  /**
   * Minimum allocation size is space for two pointers.  If the small sizeclass
   * machinery permits smaller values (that is, if MIN_ALLOC_STEP_SIZE is
   * smaller than MIN_ALLOC_SIZE), which may be useful if MIN_ALLOC_SIZE must
   * be large or not a power of two, those smaller size classes will be unused.
   */
  static constexpr size_t MIN_ALLOC_SIZE =
#if defined(SNMALLOC_MIN_ALLOC_SIZE)
    SNMALLOC_MIN_ALLOC_SIZE;
#else
    2 * sizeof(void*);
#endif

  // Minimum slab size.
#if defined(SNMALLOC_QEMU_WORKAROUND) && defined(SNMALLOC_VA_BITS_64)
  /*
   * QEMU user-mode, up through and including v7.2.0-rc4, the latest tag at the
   * time of this writing, does not use a tree of any sort to store its opinion
   * of the address space, allocating an amount of memory linear in the size of
   * any created map, not the number of pages actually used.  This is
   * exacerbated in and after qemu v6 (or, more specifically, d9c58585), which
   * grew the proportionality constant.
   *
   * In any case, for our CI jobs, then, use a larger minimum chunk size (that
   * is, pagemap granularity) than by default to reduce the size of the
   * pagemap.  We can't raise this *too* much, lest we hit constexpr step
   * limits in the sizeclasstable magic!  17 bits seems to be the sweet spot
   * and means that any of our tests can run in a little under 2 GiB of RSS
   * even on QEMU versions after v6.
   */
  static constexpr size_t MIN_CHUNK_BITS = static_cast<size_t>(17);
#else
  static constexpr size_t MIN_CHUNK_BITS = static_cast<size_t>(14);
#endif
  static constexpr size_t MIN_CHUNK_SIZE = bits::one_at_bit(MIN_CHUNK_BITS);

  // Minimum number of objects on a slab
  static constexpr size_t MIN_OBJECT_COUNT =
    mitigations(random_larger_thresholds) ? 13 : 4;

  // Maximum size of an object that uses sizeclasses.
#if defined(SNMALLOC_QEMU_WORKAROUND) && defined(SNMALLOC_VA_BITS_64)
  /*
   * As a consequence of our significantly larger minimum chunk size, we need
   * to raise the threshold for what constitutes a large object (which must
   * be a multiple of the minimum chunk size).  Extend the space of small
   * objects up enough to match yet preserve the notion that there exist small
   * objects larger than MIN_CHUNK_SIZE.
   */
  static constexpr size_t MAX_SMALL_SIZECLASS_BITS = 19;
#else
  static constexpr size_t MAX_SMALL_SIZECLASS_BITS = 16;
#endif
  static constexpr size_t MAX_SMALL_SIZECLASS_SIZE =
    bits::one_at_bit(MAX_SMALL_SIZECLASS_BITS);

  static_assert(
    MAX_SMALL_SIZECLASS_SIZE >= MIN_CHUNK_SIZE,
    "Large sizes need to be representable by as a multiple of MIN_CHUNK_SIZE");

  /**
   * The number of bits needed to count the number of objects within a slab.
   *
   * Most likely, this is achieved by the smallest sizeclass, which will have
   * many more than MIN_OBJECT_COUNT objects in its slab.  But, just in case,
   * it's defined here and checked when we compute the sizeclass table, since
   * computing this number is potentially nontrivial.
   */
#if defined(SNMALLOC_QEMU_WORKAROUND) && defined(SNMALLOC_VA_BITS_64)
  static constexpr size_t MAX_CAPACITY_BITS = 13;
#else
  static constexpr size_t MAX_CAPACITY_BITS = 11;
#endif

  /**
   * The maximum distance between the start of two objects in the same slab.
   */
  static constexpr size_t MAX_SLAB_SPAN_SIZE =
    (MIN_OBJECT_COUNT - 1) * MAX_SMALL_SIZECLASS_SIZE;
  static constexpr size_t MAX_SLAB_SPAN_BITS =
    bits::next_pow2_bits_const(MAX_SLAB_SPAN_SIZE);

  // Number of slots for remote deallocation.
  static constexpr size_t REMOTE_SLOT_BITS = 8;
  static constexpr size_t REMOTE_SLOTS = 1 << REMOTE_SLOT_BITS;
  static constexpr size_t REMOTE_MASK = REMOTE_SLOTS - 1;

#if defined(SNMALLOC_DEALLOC_BATCH_RING_ASSOC)
  static constexpr size_t DEALLOC_BATCH_RING_ASSOC =
    SNMALLOC_DEALLOC_BATCH_RING_ASSOC;
#else
#  if defined(__has_cpp_attribute)
#    if ( \
      __has_cpp_attribute(msvc::no_unique_address) && \
      (__cplusplus >= 201803L || _MSVC_LANG >= 201803L)) || \
      __has_cpp_attribute(no_unique_address)
  // For C++20 or later, we do have [[no_unique_address]] and so can also do
  // batching if we aren't turning on the backward-pointer mitigations
  static constexpr size_t DEALLOC_BATCH_MIN_ALLOC_WORDS =
    mitigations(freelist_backward_edge) ? 4 : 2;
#    else
  // For C++17, we don't have [[no_unique_address]] and so we always end up
  // needing all four pointers' worth of space (because BatchedRemoteMessage has
  // two freelist::Object::T<> links within, each of which will have two fields
  // and will be padded to two pointers).
  static constexpr size_t DEALLOC_BATCH_MIN_ALLOC_WORDS = 4;
#    endif
#  else
  // If we don't even have the feature test macro, we're C++17 or earlier.
  static constexpr size_t DEALLOC_BATCH_MIN_ALLOC_WORDS = 4;
#  endif

  static constexpr size_t DEALLOC_BATCH_RING_ASSOC =
    (MIN_ALLOC_SIZE >= (DEALLOC_BATCH_MIN_ALLOC_WORDS * sizeof(void*))) ? 2 : 0;
#endif

#if defined(SNMALLOC_DEALLOC_BATCH_RING_SET_BITS)
  static constexpr size_t DEALLOC_BATCH_RING_SET_BITS =
    SNMALLOC_DEALLOC_BATCH_RING_SET_BITS;
#else
  static constexpr size_t DEALLOC_BATCH_RING_SET_BITS = 3;
#endif

  static constexpr size_t DEALLOC_BATCH_RINGS =
    DEALLOC_BATCH_RING_ASSOC * bits::one_at_bit(DEALLOC_BATCH_RING_SET_BITS);

  static_assert(
    INTERMEDIATE_BITS < MIN_ALLOC_STEP_BITS,
    "INTERMEDIATE_BITS must be less than MIN_ALLOC_BITS");
  static_assert(
    MIN_ALLOC_SIZE >= (sizeof(void*) * 2),
    "MIN_ALLOC_SIZE must be sufficient for two pointers");
  static_assert(
    1 << (INTERMEDIATE_BITS + MIN_ALLOC_STEP_BITS) >=
      bits::next_pow2_const(MIN_ALLOC_SIZE),
    "Entire sizeclass exponent is below MIN_ALLOC_SIZE; adjust STEP_SIZE");
  static_assert(
    MIN_ALLOC_SIZE >= MIN_ALLOC_STEP_SIZE,
    "Minimum alloc sizes below minimum step size; raise MIN_ALLOC_SIZE");

  // Return remote small allocs when the local cache reaches this size.
  static constexpr int64_t REMOTE_CACHE =
#ifdef USE_REMOTE_CACHE
    USE_REMOTE_CACHE
#else
    MIN_CHUNK_SIZE
#endif
    ;

  // Stop processing remote batch when we reach this amount of deallocations in
  // bytes
  static constexpr int64_t REMOTE_BATCH_LIMIT =
#ifdef SNMALLOC_REMOTE_BATCH_PROCESS_SIZE
    SNMALLOC_REMOTE_BATCH_PROCESS_SIZE
#else
    1 * 1024 * 1024
#endif
    ;

  // Used to configure when the backend should use thread local buddies.
  // This only basically is used to disable some buddy allocators on small
  // fixed heap scenarios like OpenEnclave.
  static constexpr size_t MIN_HEAP_SIZE_FOR_THREAD_LOCAL_BUDDY =
    bits::one_at_bit(27);
} // namespace snmalloc
