#pragma once

#include "freelist_queue.h"
#include "snmalloc/stl/new.h"

namespace snmalloc
{
  class RemoteMessageAssertions;

  /**
   * Entries on a remote message queue.  Logically, this is a pair of freelist
   * linkages, together with some metadata:
   *
   * - a cyclic list ("ring") of free objects (atypically for rings, there is
   *   no sentinel node here: the message itself is a free object),
   *
   * - the length of that ring
   *
   * - the linkage for the message queue itself
   *
   * In practice, there is a fair bit more going on here: the ring of free
   * objects is not entirely encoded as a freelist.  While traversing the
   * successor pointers in objects on the ring will eventually lead back to
   * this RemoteMessage object, the successor pointer from this object is
   * encoded as a relative displacement.  This is guaranteed to be physically
   * smaller than a full pointer (because slabs are smaller than the whole
   * address space).  This gives us enough room to pack in the length of the
   * ring, without needing to grow the structure.
   */
  class BatchedRemoteMessage
  {
    friend class BatchedRemoteMessageAssertions;

    freelist::Object::T<> free_ring;
    freelist::Object::T<> message_link;

    static_assert(
      sizeof(free_ring.next_object) >= sizeof(void*),
      "BatchedRemoteMessage bitpacking needs sizeof(void*) in next_object");

  public:
    static auto emplace_in_alloc(capptr::Alloc<void> alloc)
    {
      return CapPtr<BatchedRemoteMessage, capptr::bounds::Alloc>::unsafe_from(
        new (alloc.unsafe_ptr(), placement_token) BatchedRemoteMessage());
    }

    static auto mk_from_freelist_builder(
      freelist::Builder<false, true>& flb,
      const FreeListKey& key,
      address_t key_tweak)
    {
      size_t size = flb.extract_segment_length();

      SNMALLOC_ASSERT(size < bits::one_at_bit(MAX_CAPACITY_BITS));

      auto [first, last] = flb.extract_segment(key, key_tweak);

      /*
       * Preserve the last node's backpointer and change its type.  Because we
       * use placement new to build our RemoteMessage atop the memory of a
       * freelist::Object::T<> (to avoid UB) and the constructor may nullify
       * the `prev` field, put it right back.  Ideally the compiler is smart
       * enough to see that this is a no-op.
       */
      auto last_prev = last->prev;
      auto self =
        CapPtr<BatchedRemoteMessage, capptr::bounds::Alloc>::unsafe_from(
          new (last.unsafe_ptr(), placement_token) BatchedRemoteMessage());
      self->free_ring.prev = last_prev;

      // XXX On CHERI, we could do a fair bit better if we had a primitive for
      // extracting and discarding the offset.  That probably beats the dance
      // done below, but it should work as it stands.

      auto n = freelist::HeadPtr::unsafe_from(
        unsafe_from_uintptr<freelist::Object::T<>>(
          (static_cast<uintptr_t>(pointer_diff_signed(self, first))
           << MAX_CAPACITY_BITS) +
          size));

      // Close the ring, storing our bit-packed value in the next field.
      freelist::Object::store_nextish(
        &self->free_ring.next_object, first, key, key_tweak, n);

      return self;
    }

    static freelist::HeadPtr
    to_message_link(capptr::Alloc<BatchedRemoteMessage> m)
    {
      return pointer_offset(m, offsetof(BatchedRemoteMessage, message_link))
        .as_reinterpret<freelist::Object::T<>>();
    }

    static capptr::Alloc<BatchedRemoteMessage>
    from_message_link(freelist::HeadPtr chainPtr)
    {
      return pointer_offset_signed(
               chainPtr,
               -static_cast<ptrdiff_t>(
                 offsetof(BatchedRemoteMessage, message_link)))
        .as_reinterpret<BatchedRemoteMessage>();
    }

    template<SNMALLOC_CONCEPT(IsConfigLazy) Config, typename Domesticator_queue>
    SNMALLOC_FAST_PATH static stl::Pair<freelist::HeadPtr, uint16_t>
    open_free_ring(
      capptr::Alloc<BatchedRemoteMessage> m,
      size_t objsize,
      const FreeListKey& key,
      address_t key_tweak,
      Domesticator_queue domesticate)
    {
      uintptr_t encoded =
        m->free_ring.read_next(key, key_tweak, domesticate).unsafe_uintptr();

      uint16_t decoded_size =
        static_cast<uint16_t>(encoded) & bits::mask_bits(MAX_CAPACITY_BITS);
      static_assert(sizeof(decoded_size) * 8 > MAX_CAPACITY_BITS);

      /*
       * Derive an out-of-bounds pointer to the next allocation, then use the
       * authmap to reconstruct an in-bounds version, which we then immediately
       * bound and rewild and then domesticate (how strange).
       *
       * XXX See above re: doing better on CHERI.
       */
      auto next = domesticate(
        capptr_rewild(
          Config::Backend::capptr_rederive_alloc(
            pointer_offset_signed(
              m, static_cast<ptrdiff_t>(encoded) >> MAX_CAPACITY_BITS),
            objsize))
          .template as_static<freelist::Object::T<>>());

      if constexpr (mitigations(freelist_backward_edge))
      {
        next->check_prev(
          signed_prev(address_cast(m), address_cast(next), key, key_tweak));
      }
      else
      {
        UNUSED(key);
        UNUSED(key_tweak);
      }

      return {next.template as_static<freelist::Object::T<>>(), decoded_size};
    }

    template<SNMALLOC_CONCEPT(IsConfigLazy) Config, typename Domesticator_queue>
    static uint16_t ring_size(
      capptr::Alloc<BatchedRemoteMessage> m,
      const FreeListKey& key,
      address_t key_tweak,
      Domesticator_queue domesticate)
    {
      uintptr_t encoded =
        m->free_ring.read_next(key, key_tweak, domesticate).unsafe_uintptr();

      uint16_t decoded_size =
        static_cast<uint16_t>(encoded) & bits::mask_bits(MAX_CAPACITY_BITS);
      static_assert(sizeof(decoded_size) * 8 > MAX_CAPACITY_BITS);

      if constexpr (mitigations(freelist_backward_edge))
      {
        /*
         * Like above, but we don't strictly need to rebound the pointer,
         * since it's only used internally.  Still, doesn't hurt to bound
         * to the free list linkage.
         */
        auto next = domesticate(
          capptr_rewild(
            Config::Backend::capptr_rederive_alloc(
              pointer_offset_signed(
                m, static_cast<ptrdiff_t>(encoded) >> MAX_CAPACITY_BITS),
              sizeof(freelist::Object::T<>)))
            .template as_static<freelist::Object::T<>>());

        next->check_prev(
          signed_prev(address_cast(m), address_cast(next), key, key_tweak));
      }
      else
      {
        UNUSED(key);
        UNUSED(key_tweak);
        UNUSED(domesticate);
      }

      return decoded_size;
    }
  };

  class BatchedRemoteMessageAssertions
  {
    static_assert(
      (DEALLOC_BATCH_RINGS == 0) ||
      (sizeof(BatchedRemoteMessage) <= MIN_ALLOC_SIZE));
    static_assert(offsetof(BatchedRemoteMessage, free_ring) == 0);

    static_assert(
      (DEALLOC_BATCH_RINGS == 0) ||
        (MAX_SLAB_SPAN_BITS + MAX_CAPACITY_BITS < 8 * sizeof(void*)),
      "Ring bit-stuffing trick can't reach far enough to enclose a slab");
  };

  /** The type of a remote message when we are not batching messages onto
   * rings.
   *
   * Relative to BatchRemoteMessage, this type is smaller, as it contains only
   * a single linkage, to the next message.  (And possibly a backref, if
   * mitigations(freelist_backward_edge) is enabled.)
   */
  class SingletonRemoteMessage
  {
    friend class SingletonRemoteMessageAssertions;

    freelist::Object::T<> message_link;

  public:
    static auto emplace_in_alloc(capptr::Alloc<void> alloc)
    {
      return CapPtr<SingletonRemoteMessage, capptr::bounds::Alloc>::unsafe_from(
        new (alloc.unsafe_ptr(), placement_token) SingletonRemoteMessage());
    }

    static freelist::HeadPtr
    to_message_link(capptr::Alloc<SingletonRemoteMessage> m)
    {
      return pointer_offset(m, offsetof(SingletonRemoteMessage, message_link))
        .as_reinterpret<freelist::Object::T<>>();
    }

    static capptr::Alloc<SingletonRemoteMessage>
    from_message_link(freelist::HeadPtr chainPtr)
    {
      return pointer_offset_signed(
               chainPtr,
               -static_cast<ptrdiff_t>(
                 offsetof(SingletonRemoteMessage, message_link)))
        .as_reinterpret<SingletonRemoteMessage>();
    }

    template<SNMALLOC_CONCEPT(IsConfigLazy) Config, typename Domesticator_queue>
    SNMALLOC_FAST_PATH static stl::Pair<freelist::HeadPtr, uint16_t>
    open_free_ring(
      capptr::Alloc<SingletonRemoteMessage> m,
      size_t,
      const FreeListKey&,
      address_t,
      Domesticator_queue)
    {
      return {
        m.as_reinterpret<freelist::Object::T<>>(), static_cast<uint16_t>(1)};
    }

    template<SNMALLOC_CONCEPT(IsConfigLazy) Config, typename Domesticator_queue>
    static uint16_t ring_size(
      capptr::Alloc<SingletonRemoteMessage>,
      const FreeListKey&,
      address_t,
      Domesticator_queue)
    {
      return 1;
    }
  };

  class SingletonRemoteMessageAssertions
  {
    static_assert(sizeof(SingletonRemoteMessage) <= MIN_ALLOC_SIZE);
    static_assert(
      sizeof(SingletonRemoteMessage) == sizeof(freelist::Object::T<>));
    static_assert(offsetof(SingletonRemoteMessage, message_link) == 0);
  };

  using RemoteMessage = stl::conditional_t<
    (DEALLOC_BATCH_RINGS > 0),
    BatchedRemoteMessage,
    SingletonRemoteMessage>;

  static_assert(sizeof(RemoteMessage) <= MIN_ALLOC_SIZE);

  /**
   * A RemoteAllocator is the message queue of freed objects.  It builds on the
   * FreeListMPSCQ but encapsulates knowledge that the objects are actually
   * RemoteMessage-s and not just any freelist::object::T<>s.
   *
   * RemoteAllocator-s may be exposed to client tampering.  As a result,
   * pointer domestication may be necessary.  See the documentation for
   * FreeListMPSCQ for details.
   */
  struct RemoteAllocator
  {
    /**
     * Global key for all remote lists.
     *
     * Note that we use a single key for all remote free lists and queues.
     * This is so that we do not have to recode next pointers when sending
     * segments, and look up specific keys based on destination.  This is
     * potentially more performant, but could make it easier to guess the key.
     */
    inline static FreeListKey key_global{0xdeadbeef, 0xbeefdead, 0xdeadbeef};

    FreeListMPSCQ<key_global> list;

    using alloc_id_t = address_t;

    constexpr RemoteAllocator() = default;

    void invariant()
    {
      list.invariant();
    }

    void init()
    {
      list.init();
    }

    template<typename Domesticator_queue, typename Cb>
    void destroy_and_iterate(Domesticator_queue domesticate, Cb cb)
    {
      auto cbwrap = [cb](freelist::HeadPtr p) SNMALLOC_FAST_PATH_LAMBDA {
        cb(RemoteMessage::from_message_link(p));
      };

      return list.destroy_and_iterate(domesticate, cbwrap);
    }

    inline bool can_dequeue()
    {
      return list.can_dequeue();
    }

    /**
     * Pushes a list of messages to the queue. Each message from first to
     * last should be linked together through their next pointers.
     *
     * The Domesticator here is used only on pointers read from the head.  See
     * the commentary on the class.
     */
    template<typename Domesticator_head>
    void enqueue(
      capptr::Alloc<RemoteMessage> first,
      capptr::Alloc<RemoteMessage> last,
      Domesticator_head domesticate_head)
    {
      list.enqueue(
        RemoteMessage::to_message_link(first),
        RemoteMessage::to_message_link(last),
        domesticate_head);
    }

    /**
     * Destructively iterate the queue.  Each queue element is removed and fed
     * to the callback in turn.  The callback may return false to stop iteration
     * early (but must have processed the element it was given!).
     *
     * Takes a domestication callback for each of "pointers read from head" and
     * "pointers read from queue".  See the commentary on the class.
     */
    template<
      typename Domesticator_head,
      typename Domesticator_queue,
      typename Cb>
    void dequeue(
      Domesticator_head domesticate_head,
      Domesticator_queue domesticate_queue,
      Cb cb)
    {
      auto cbwrap = [cb](freelist::HeadPtr p) SNMALLOC_FAST_PATH_LAMBDA {
        return cb(RemoteMessage::from_message_link(p));
      };
      list.dequeue(domesticate_head, domesticate_queue, cbwrap);
    }

    alloc_id_t trunc_id()
    {
      return address_cast(this);
    }
  };
} // namespace snmalloc
