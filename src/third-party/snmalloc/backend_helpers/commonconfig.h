#pragma once

#include "../mem/mem.h"

namespace snmalloc
{
  /**
   * Options for a specific snmalloc configuration.  Every globals object must
   * have one `constexpr` instance of this class called `Options`.  This should
   * be constructed to explicitly override any of the defaults.  A
   * configuration that does not need to override anything would simply declare
   * this as a field of the global object:
   *
   * ```c++
   * static constexpr snmalloc::Flags Options{};
   * ```
   *
   * A global configuration that wished to use out-of-line message queues but
   * accept the defaults for everything else would instead do this:
   *
   * ```c++
   *     static constexpr snmalloc::Flags Options{.IsQueueInline = false};
   * ```
   *
   * To maintain backwards source compatibility in future versions, any new
   * option added here should have its default set to be whatever snmalloc was
   * doing before the new option was added.
   */
  struct Flags
  {
    /**
     * Should allocators have inline message queues?  If this is true then
     * the `Allocator` is responsible for allocating the
     * `RemoteAllocator` that contains its message queue.  If this is false
     * then the `RemoteAllocator` must be separately allocated and provided
     * to the `Allocator` before it is used.
     */
    bool IsQueueInline = true;

    /**
     * Does the `Allocator` own a `Backend::LocalState` object?  If this is
     * true then the `Allocator` is responsible for allocating and
     * deallocating a local state object, otherwise the surrounding code is
     * responsible for creating it.
     */
    bool AllocOwnsLocalState = true;

    /**
     * Are `Allocator` allocated by the pool allocator?  If not then the
     * code embedding this snmalloc configuration is responsible for allocating
     * `Allocator` instances.
     */
    bool AllocIsPoolAllocated = true;

    /**
     * Are the front and back pointers to the message queue in a RemoteAllocator
     * considered to be capptr_bounds::Wildness::Tame (as opposed to Wild)?
     * That is, is it presumed that clients or other potentialadversaries cannot
     * access the front and back pointers themselves, even if they can access
     * the queue nodes themselves (which are always considered Wild)?
     */
    bool QueueHeadsAreTame = true;

    /**
     * Does the backend provide a capptr_domesticate function to sanity check
     * pointers? If so it will be called when untrusted pointers are consumed
     * (on dealloc and in freelists) otherwise a no-op version is provided.
     */
    bool HasDomesticate = false;
  };

  struct NoClientMetaDataProvider
  {
    using StorageType = Empty;
    using DataRef = Empty&;

    static size_t required_count(size_t)
    {
      return 1;
    }

    static DataRef get(StorageType* base, size_t)
    {
      return *base;
    }
  };

  template<typename T>
  struct ArrayClientMetaDataProvider
  {
    using StorageType = T;
    using DataRef = T&;

    static size_t required_count(size_t max_count)
    {
      return max_count;
    }

    static DataRef get(StorageType* base, size_t index)
    {
      return base[index];
    }
  };

  /**
   * Class containing definitions that are likely to be used by all except for
   * the most unusual back-end implementations.  This can be subclassed as a
   * convenience for back-end implementers, but is not required.
   */
  class CommonConfig
  {
  public:
    /**
     * Special remote that should never be used as a real remote.
     * This is used to initialise allocators that should always hit the
     * remote path for deallocation. Hence moving a branch off the critical
     * path.
     */
    SNMALLOC_REQUIRE_CONSTINIT
    inline static RemoteAllocator unused_remote;
  };

  template<typename PAL>
  static constexpr size_t MinBaseSizeBits()
  {
    if constexpr (pal_supports<AlignedAllocation, PAL>)
    {
      return bits::next_pow2_bits_const(PAL::minimum_alloc_size);
    }
    else
    {
      return MIN_CHUNK_BITS;
    }
  }
} // namespace snmalloc

#include "../mem/remotecache.h"
