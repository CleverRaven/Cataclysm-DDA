#pragma once
/**
 * This file encapsulates the in disused object free lists
 * that are used per slab of small objects. The implementation
 * can be configured to introduce randomness to the reallocation,
 * and also provide signing to detect free list corruption.
 *
 * # Corruption
 *
 * The corruption detection works as follows
 *
 *   free Object
 *   -----------------------------
 *   | next | prev_encoded | ... |
 *   -----------------------------
 * A free object contains a pointer to next object in the free list, and
 * a prev pointer, but the prev pointer is really a signature with the
 * following property
 *
 *  If n = c->next && n != 0, then n->prev_encoded = f(c,n).
 *
 * If f just returns the first parameter, then this degenerates to a doubly
 * linked list.  (Note that doing the degenerate case can be useful for
 * debugging snmalloc bugs.) By making it a function of both pointers, it
 * makes it harder for an adversary to mutate prev_encoded to a valid value.
 *
 * This provides protection against the free-list being corrupted by memory
 * safety issues.
 *
 * # Randomness
 *
 * The randomness is introduced by building two free lists simulatenously,
 * and randomly deciding which list to add an element to.
 */

#include "../ds/ds.h"
#include "entropy.h"
#include "snmalloc/stl/new.h"

#include <stdint.h>

namespace snmalloc
{
  class BatchedRemoteMessage;

  static constexpr address_t NO_KEY_TWEAK = 0;

  /**
   * This function is used to sign back pointers in the free list.
   */
  inline static address_t signed_prev(
    address_t curr, address_t next, const FreeListKey& key, address_t tweak)
  {
    auto c = curr;
    auto n = next;
    return (c + key.key1) * (n + (key.key2 ^ tweak));
  }

  namespace freelist
  {
    template<
      bool RANDOM,
      bool TRACK_LENGTH = RANDOM,
      SNMALLOC_CONCEPT(capptr::IsBound) BView = capptr::bounds::Alloc,
      SNMALLOC_CONCEPT(capptr::IsBound) BQueue = capptr::bounds::AllocWild>
    class Builder;

    class Object
    {
    public:
      /**
       * Shared key for slab free lists (but tweaked by metadata address).
       *
       * XXX Maybe this belongs somewhere else
       */
      inline static FreeListKey key_root{0xdeadbeef, 0xbeefdead, 0xdeadbeef};

      template<
        SNMALLOC_CONCEPT(capptr::IsBound) BQueue = capptr::bounds::AllocWild>
      class T;

      /**
       * This "inductive step" type -- a queue-annotated pointer to a free
       * Object containing a queue-annotated pointer -- shows up all over the
       * place.  Give it a shorter name (Object::BQueuePtr<BQueue>) for
       * convenience.
       */
      template<SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      using BQueuePtr = CapPtr<Object::T<BQueue>, BQueue>;

      /**
       * As with BQueuePtr, but atomic.
       */
      template<SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      using BAtomicQueuePtr = AtomicCapPtr<Object::T<BQueue>, BQueue>;

      /**
       * This is the "base case" of that induction.  While we can't get rid of
       * the two different type parameters (in general), we can at least get rid
       * of a bit of the clutter.  "freelist::Object::HeadPtr<BView, BQueue>"
       * looks a little nicer than "CapPtr<freelist::Object::T<BQueue>, BView>".
       */
      template<
        SNMALLOC_CONCEPT(capptr::IsBound) BView,
        SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      using BHeadPtr = CapPtr<Object::T<BQueue>, BView>;

      /**
       * As with BHeadPtr, but atomic.
       */
      template<
        SNMALLOC_CONCEPT(capptr::IsBound) BView,
        SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      using BAtomicHeadPtr = AtomicCapPtr<Object::T<BQueue>, BView>;

      /**
       * Free objects within each slab point directly to the next.
       * There is an optional second field that is effectively a
       * back pointer in a doubly linked list, however, it is encoded
       * to prevent corruption.
       *
       * This is an inner class to avoid the need to specify BQueue when calling
       * static methods.
       *
       * Raw C++ pointers to this type are *assumed to be domesticated*.  In
       * some cases we still explicitly annotate domesticated free Object*-s as
       * CapPtr<>, but more often CapPtr<Object::T<A>,B> will have B = A.
       *
       * TODO: Consider putting prev_encoded at the end of the object, would
       * require size to be threaded through, but would provide more OOB
       * detection.
       */
      template<SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      class T
      {
        template<
          bool,
          bool,
          SNMALLOC_CONCEPT(capptr::IsBound),
          SNMALLOC_CONCEPT(capptr::IsBound)>
        friend class Builder;

        friend class Object;

        friend class ::snmalloc::BatchedRemoteMessage;

        class Empty
        {
        public:
          void check_prev(address_t) {}

          void set_prev(address_t) {}
        };

        class Prev
        {
          address_t prev_encoded;

        public:
          /**
           * Check the signature of this free Object
           */
          void check_prev(address_t signed_prev)
          {
            snmalloc_check_client(
              mitigations(freelist_backward_edge),
              signed_prev == prev_encoded,
              "Heap corruption - free list corrupted!");
            UNUSED(signed_prev);
          }

          void set_prev(address_t signed_prev)
          {
            prev_encoded = signed_prev;
          }
        };

        union
        {
          BQueuePtr<BQueue> next_object;
          // TODO: Should really use C++20 atomic_ref rather than a union.
          BAtomicQueuePtr<BQueue> atomic_next_object;
        };

        SNMALLOC_NO_UNIQUE_ADDRESS
        stl::conditional_t<mitigations(freelist_backward_edge), Prev, Empty>
          prev{};

      public:
        constexpr T() : next_object(){};

        template<
          SNMALLOC_CONCEPT(capptr::IsBound) BView = typename BQueue::
            template with_wildness<capptr::dimension::Wildness::Tame>,
          typename Domesticator>
        BHeadPtr<BView, BQueue> atomic_read_next(
          const FreeListKey& key, address_t key_tweak, Domesticator domesticate)
        {
          auto n_wild = Object::decode_next(
            address_cast(&this->next_object),
            this->atomic_next_object.load(stl::memory_order_acquire),
            key,
            key_tweak);
          auto n_tame = domesticate(n_wild);
          if constexpr (mitigations(freelist_backward_edge))
          {
            if (n_tame != nullptr)
            {
              n_tame->prev.check_prev(signed_prev(
                address_cast(this), address_cast(n_tame), key, key_tweak));
            }
          }
          else
          {
            UNUSED(key_tweak);
          }
          Aal::prefetch(n_tame.unsafe_ptr());
          return n_tame;
        }

        /**
         * Read the next pointer
         */
        template<
          SNMALLOC_CONCEPT(capptr::IsBound) BView = typename BQueue::
            template with_wildness<capptr::dimension::Wildness::Tame>,
          typename Domesticator>
        BHeadPtr<BView, BQueue> read_next(
          const FreeListKey& key, address_t key_tweak, Domesticator domesticate)
        {
          return domesticate(Object::decode_next(
            address_cast(&this->next_object),
            this->next_object,
            key,
            key_tweak));
        }

        /**
         * Check the signature of this free Object
         */
        void check_prev(address_t signed_prev)
        {
          prev.check_prev(signed_prev);
        }

        /**
         * Clean up this object when removing it from the list.
         */
        void cleanup()
        {
          if constexpr (mitigations(clear_meta))
          {
            this->next_object = nullptr;
            if constexpr (mitigations(freelist_backward_edge))
            {
              this->prev.set_prev(0);
            }
          }
        }
      };

      // Note the inverted template argument order, since BView is inferable.
      template<
        SNMALLOC_CONCEPT(capptr::IsBound) BQueue,
        SNMALLOC_CONCEPT(capptr::IsBound) BView>
      static BHeadPtr<BView, BQueue> make(CapPtr<void, BView> p)
      {
        return CapPtr<Object::T<BQueue>, BView>::unsafe_from(
          new (p.unsafe_ptr(), placement_token) Object::T());
      }

      /**
       * A container-of operation to convert &f->next_object to f
       */
      template<SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      static Object::T<BQueue>*
      from_next_ptr(CapPtr<Object::T<BQueue>, BQueue>* ptr)
      {
        static_assert(offsetof(Object::T<BQueue>, next_object) == 0);
        return reinterpret_cast<Object::T<BQueue>*>(ptr);
      }

      /**
       * Involutive encryption with raw pointers
       */
      template<SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      inline static Object::T<BQueue>* code_next(
        address_t curr,
        Object::T<BQueue>* next,
        const FreeListKey& key,
        address_t key_tweak)
      {
        // Note we can consider other encoding schemes here.
        //   * XORing curr and next.  This doesn't require any key material
        //   * XORing (curr * key). This makes it harder to guess the underlying
        //     key, as each location effectively has its own key.
        // Curr is not used in the current encoding scheme.
        UNUSED(curr);

        if constexpr (
          mitigations(freelist_forward_edge) && !aal_supports<StrictProvenance>)
        {
          return unsafe_from_uintptr<Object::T<BQueue>>(
            unsafe_to_uintptr<Object::T<BQueue>>(next) ^ key.key_next ^
            key_tweak);
        }
        else
        {
          UNUSED(key);
          UNUSED(key_tweak);
          return next;
        }
      }

      /**
       * Encode next.  We perform two convenient little bits of type-level
       * sleight of hand here:
       *
       *  1) We convert the provided HeadPtr to a QueuePtr, forgetting BView in
       *  the result; all the callers write the result through a pointer to a
       *  QueuePtr, though, strictly, the result itself is no less domesticated
       *  than the input (even if it is obfuscated).
       *
       *  2) Speaking of obfuscation, we continue to use a CapPtr<> type even
       *  though the result is likely not safe to dereference, being an
       *  obfuscated bundle of bits (on non-CHERI architectures, anyway). That's
       *  additional motivation to consider the result BQueue-bounded, as that
       *  is likely (but not necessarily) Wild.
       */
      template<
        SNMALLOC_CONCEPT(capptr::IsBound) BView,
        SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      inline static BQueuePtr<BQueue> encode_next(
        address_t curr,
        BHeadPtr<BView, BQueue> next,
        const FreeListKey& key,
        address_t key_tweak)
      {
        return BQueuePtr<BQueue>::unsafe_from(
          code_next(curr, next.unsafe_ptr(), key, key_tweak));
      }

      /**
       * Decode next.  While traversing a queue, BView and BQueue here will
       * often be equal (i.e., AllocUserWild) rather than dichotomous. However,
       * we do occasionally decode an actual head pointer, so be polymorphic
       * here.
       *
       * TODO: We'd like, in some sense, to more tightly couple or integrate
       * this into to the domestication process.  We could introduce an
       * additional state in the capptr_bounds::wild taxonomy (e.g, Obfuscated)
       * so that the Domesticator-s below have to call through this function to
       * get the Wild pointer they can then make Tame.  It's not yet entirely
       * clear what that would look like and whether/how the encode_next side of
       * things should be exposed.  For the moment, obfuscation is left
       * encapsulated within Object and we do not capture any of it statically.
       */
      template<
        SNMALLOC_CONCEPT(capptr::IsBound) BView,
        SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      inline static BHeadPtr<BView, BQueue> decode_next(
        address_t curr,
        BHeadPtr<BView, BQueue> next,
        const FreeListKey& key,
        address_t key_tweak)
      {
        return BHeadPtr<BView, BQueue>::unsafe_from(
          code_next(curr, next.unsafe_ptr(), key, key_tweak));
      }

      template<
        SNMALLOC_CONCEPT(capptr::IsBound) BView,
        SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      static void assert_view_queue_bounds()
      {
        static_assert(
          BView::wildness == capptr::dimension::Wildness::Tame,
          "Free Object View must be domesticated, justifying raw pointers");

        static_assert(
          stl::is_same_v<
            typename BQueue::template with_wildness<
              capptr::dimension::Wildness::Tame>,
            BView>,
          "Free Object Queue bounds must match View bounds (but may be Wild)");
      }

      template<
        SNMALLOC_CONCEPT(capptr::IsBound) BView,
        SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      static void store_nextish(
        BQueuePtr<BQueue>* curr,
        BHeadPtr<BView, BQueue> next,
        const FreeListKey& key,
        address_t key_tweak,
        BHeadPtr<BView, BQueue> next_value)
      {
        assert_view_queue_bounds<BView, BQueue>();

        if constexpr (mitigations(freelist_backward_edge))
        {
          next->prev.set_prev(signed_prev(
            address_cast(curr), address_cast(next), key, key_tweak));
        }
        else
        {
          UNUSED(next);
          UNUSED(key);
          UNUSED(key_tweak);
        }

        *curr = encode_next(address_cast(curr), next_value, key, key_tweak);
      }

      /**
       * Assign next_object and update its prev_encoded if
       * SNMALLOC_CHECK_CLIENT. Static so that it can be used on reference to a
       * free Object.
       *
       * Returns a pointer to the next_object field of the next parameter as an
       * optimization for repeated snoc operations (in which
       * next->next_object is nullptr).
       */
      template<
        SNMALLOC_CONCEPT(capptr::IsBound) BView,
        SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      static BQueuePtr<BQueue>* store_next(
        BQueuePtr<BQueue>* curr,
        BHeadPtr<BView, BQueue> next,
        const FreeListKey& key,
        address_t key_tweak)
      {
        store_nextish(curr, next, key, key_tweak, next);
        return &(next->next_object);
      }

      template<SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      static void store_null(
        BQueuePtr<BQueue>* curr, const FreeListKey& key, address_t key_tweak)
      {
        *curr = encode_next(
          address_cast(curr), BQueuePtr<BQueue>(nullptr), key, key_tweak);
      }

      /**
       * Assign next_object and update its prev_encoded if SNMALLOC_CHECK_CLIENT
       *
       * Uses the atomic view of next, so can be used in the message queues.
       */
      template<
        SNMALLOC_CONCEPT(capptr::IsBound) BView,
        SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      static void atomic_store_next(
        BHeadPtr<BView, BQueue> curr,
        BHeadPtr<BView, BQueue> next,
        const FreeListKey& key,
        address_t key_tweak)
      {
        static_assert(BView::wildness == capptr::dimension::Wildness::Tame);

        if constexpr (mitigations(freelist_backward_edge))
        {
          next->prev.set_prev(signed_prev(
            address_cast(curr), address_cast(next), key, key_tweak));
        }
        else
        {
          UNUSED(key);
          UNUSED(key_tweak);
        }

        // Signature needs to be visible before item is linked in
        // so requires release semantics.
        curr->atomic_next_object.store(
          encode_next(address_cast(&curr->next_object), next, key, key_tweak),
          stl::memory_order_release);
      }

      template<
        SNMALLOC_CONCEPT(capptr::IsBound) BView,
        SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
      static void atomic_store_null(
        BHeadPtr<BView, BQueue> curr,
        const FreeListKey& key,
        address_t key_tweak)
      {
        static_assert(BView::wildness == capptr::dimension::Wildness::Tame);

        curr->atomic_next_object.store(
          encode_next(
            address_cast(&curr->next_object),
            BQueuePtr<BQueue>(nullptr),
            key,
            key_tweak),
          stl::memory_order_relaxed);
      }
    };

    static_assert(
      sizeof(Object) <= MIN_ALLOC_SIZE,
      "Needs to be able to fit in smallest allocation.");

    /**
     * External code almost always uses Alloc and AllocWild for its free lists.
     * Give them a convenient alias.
     */
    using HeadPtr =
      Object::BHeadPtr<capptr::bounds::Alloc, capptr::bounds::AllocWild>;

    /**
     * Like HeadPtr, but atomic
     */
    using AtomicHeadPtr =
      Object::BAtomicHeadPtr<capptr::bounds::Alloc, capptr::bounds::AllocWild>;

    /**
     * External code's inductive cases almost always use AllocWild.
     */
    using QueuePtr = Object::BQueuePtr<capptr::bounds::AllocWild>;

    /**
     * Like QueuePtr, but atomic
     */
    using AtomicQueuePtr = Object::BAtomicQueuePtr<capptr::bounds::AllocWild>;

    class Prev
    {
      address_t prev{0};

    protected:
      constexpr Prev(address_t prev) : prev(prev) {}

      constexpr Prev() = default;

      address_t replace(address_t next)
      {
        auto p = prev;
        prev = next;
        return p;
      }
    };

    class NoPrev
    {
    protected:
      constexpr NoPrev(address_t){};
      constexpr NoPrev() = default;

      address_t replace(address_t t)
      {
        // This should never be called.
        SNMALLOC_CHECK(false);
        return t;
      }
    };

    using IterBase =
      stl::conditional_t<mitigations(freelist_backward_edge), Prev, NoPrev>;

    /**
     * Used to iterate a free list in object space.
     *
     * Checks signing of pointers
     */
    template<
      SNMALLOC_CONCEPT(capptr::IsBound) BView = capptr::bounds::Alloc,
      SNMALLOC_CONCEPT(capptr::IsBound) BQueue = capptr::bounds::AllocWild>
    class Iter : IterBase
    {
      Object::BHeadPtr<BView, BQueue> curr{nullptr};

      struct KeyTweak
      {
        address_t key_tweak = 0;

        SNMALLOC_FAST_PATH address_t get()
        {
          return key_tweak;
        }

        void set(address_t kt)
        {
          key_tweak = kt;
        }

        constexpr KeyTweak() = default;
      };

      struct NoKeyTweak
      {
        SNMALLOC_FAST_PATH address_t get()
        {
          return 0;
        }

        void set(address_t) {}
      };

      SNMALLOC_NO_UNIQUE_ADDRESS
      stl::conditional_t<
        mitigations(freelist_forward_edge) ||
          mitigations(freelist_backward_edge),
        KeyTweak,
        NoKeyTweak>
        key_tweak;

    public:
      constexpr Iter(
        Object::BHeadPtr<BView, BQueue> head,
        address_t prev_value,
        address_t kt)
      : IterBase(prev_value), curr(head)
      {
        UNUSED(prev_value);
        key_tweak.set(kt);
      }

      constexpr Iter() = default;

      /**
       * Checks if there are any more values to iterate.
       */
      bool empty()
      {
        return curr == nullptr;
      }

      /**
       * Returns current head without affecting the iterator.
       */
      Object::BHeadPtr<BView, BQueue> peek()
      {
        return curr;
      }

      /**
       * Moves the iterator on, and returns the current value.
       */
      template<typename Domesticator>
      Object::BHeadPtr<BView, BQueue>
      take(const FreeListKey& key, Domesticator domesticate)
      {
        auto c = curr;
        auto next = curr->read_next(key, key_tweak.get(), domesticate);

        Aal::prefetch(next.unsafe_ptr());
        curr = next;

        if constexpr (mitigations(freelist_backward_edge))
        {
          auto p = replace(signed_prev(
            address_cast(c), address_cast(next), key, key_tweak.get()));
          c->check_prev(p);
        }
        else
          UNUSED(key);

        c->cleanup();
        return c;
      }
    };

    /**
     * Used to build a free list in object space.
     *
     * Adds signing of pointers in the SNMALLOC_CHECK_CLIENT mode
     *
     * If RANDOM is enabled, the builder uses two queues, and
     * "randomly" decides to add to one of the two queues.  This
     * means that we will maintain a randomisation of the order
     * between allocations.
     *
     * The fields are paired up to give better codegen as then they are offset
     * by a power of 2, and the bit extract from the interleaving seed can
     * be shifted to calculate the relevant offset to index the fields.
     *
     * If RANDOM is set to false, then the code does not perform any
     * randomisation.
     */
    template<
      bool RANDOM,
      bool TRACK_LENGTH,
      SNMALLOC_CONCEPT(capptr::IsBound) BView,
      SNMALLOC_CONCEPT(capptr::IsBound) BQueue>
    class Builder
    {
      static_assert(!RANDOM || TRACK_LENGTH);

      static constexpr size_t LENGTH = RANDOM ? 2 : 1;

      /*
       * We use native pointers below so that we don't run afoul of strict
       * aliasing rules.  head is a Object::HeadPtr<BView, BQueue> -- that is, a
       * known-domesticated pointer to a queue of wild pointers -- and it's
       * usually the case that end is a Object::BQueuePtr<BQueue>* -- that is, a
       * known-domesticated pointer to a wild pointer to a queue of wild
       * pointers.  However, in order to do branchless inserts, we set end =
       * &head, which breaks strict aliasing rules with the types as given.
       * Fortunately, these are private members and so we can use native
       * pointers and just expose a more strongly typed interface.
       */

      // Pointer to the first element.
      stl::Array<void*, LENGTH> head{nullptr};
      // Pointer to the reference to the last element.
      // In the empty case end[i] == &head[i]
      // This enables branch free enqueuing.
      stl::Array<void**, LENGTH> end{nullptr};

      [[nodiscard]] Object::BQueuePtr<BQueue>* cast_end(uint32_t ix) const
      {
        return reinterpret_cast<Object::BQueuePtr<BQueue>*>(end[ix]);
      }

      void set_end(uint32_t ix, Object::BQueuePtr<BQueue>* p)
      {
        end[ix] = reinterpret_cast<void**>(p);
      }

      [[nodiscard]] Object::BHeadPtr<BView, BQueue> cast_head(uint32_t ix) const
      {
        return Object::BHeadPtr<BView, BQueue>::unsafe_from(
          static_cast<Object::T<BQueue>*>(head[ix]));
      }

      SNMALLOC_NO_UNIQUE_ADDRESS
      stl::Array<uint16_t, RANDOM ? 2 : (TRACK_LENGTH ? 1 : 0)> length{};

    public:
      constexpr Builder() = default;

      /**
       * Checks if the builder contains any elements.
       */
      bool empty()
      {
        for (size_t i = 0; i < LENGTH; i++)
        {
          if (end[i] != &head[i])
          {
            return false;
          }
        }
        return true;
      }

      /**
       * Adds an element to the builder
       */
      void add(
        Object::BHeadPtr<BView, BQueue> n,
        const FreeListKey& key,
        address_t key_tweak,
        LocalEntropy& entropy)
      {
        uint32_t index;
        if constexpr (RANDOM)
          index = entropy.next_bit();
        else
          index = 0;

        set_end(index, Object::store_next(cast_end(index), n, key, key_tweak));
        if constexpr (TRACK_LENGTH)
        {
          length[index]++;
        }
      }

      /**
       * Adds an element to the builder, if we are guaranteed that
       * RANDOM is false.  This is useful in certain construction
       * cases that do not need to introduce randomness, such as
       * during the initialisation construction of a free list, which
       * uses its own algorithm, or during building remote deallocation
       * lists, which will be randomised at the other end.
       */
      template<bool RANDOM_ = RANDOM>
      stl::enable_if_t<!RANDOM_> add(
        Object::BHeadPtr<BView, BQueue> n,
        const FreeListKey& key,
        address_t key_tweak)
      {
        static_assert(RANDOM_ == RANDOM, "Don't set template parameter");
        set_end(0, Object::store_next(cast_end(0), n, key, key_tweak));
        if constexpr (TRACK_LENGTH)
        {
          length[0]++;
        }
      }

      /**
       * Makes a terminator to a free list.
       */
      SNMALLOC_FAST_PATH void terminate_list(
        uint32_t index, const FreeListKey& key, address_t key_tweak)
      {
        Object::store_null(cast_end(index), key, key_tweak);
      }

      /**
       * Read head removing potential encoding
       *
       * Although, head does not require meta-data protection
       * as it is not stored in an object allocation. For uniformity
       * it is treated like the next_object field in a free Object
       * and is thus subject to encoding if the next_object pointers
       * encoded.
       */
      [[nodiscard]] Object::BHeadPtr<BView, BQueue> read_head(
        uint32_t index, const FreeListKey& key, address_t key_tweak) const
      {
        return Object::decode_next(
          address_cast(&head[index]), cast_head(index), key, key_tweak);
      }

      address_t get_fake_signed_prev(
        uint32_t index, const FreeListKey& key, address_t key_tweak)
      {
        return signed_prev(
          address_cast(&head[index]),
          address_cast(read_head(index, key, key_tweak)),
          key,
          key_tweak);
      }

      /**
       * Close a free list, and set the iterator parameter
       * to iterate it.
       *
       * In the RANDOM case, it may return only part of the freelist.
       *
       * The return value is how many entries are still contained in the
       * builder.
       */
      SNMALLOC_FAST_PATH uint16_t close(
        Iter<BView, BQueue>& fl, const FreeListKey& key, address_t key_tweak)
      {
        uint32_t i;
        if constexpr (RANDOM)
        {
          SNMALLOC_ASSERT(end[1] != &head[0]);
          SNMALLOC_ASSERT(end[0] != &head[1]);

          // Select longest list.
          i = length[0] > length[1] ? 0 : 1;
        }
        else
        {
          i = 0;
        }

        terminate_list(i, key, key_tweak);

        fl = {
          read_head(i, key, key_tweak),
          get_fake_signed_prev(i, key, key_tweak),
          key_tweak};

        end[i] = &head[i];

        if constexpr (RANDOM)
        {
          length[i] = 0;
          return length[1 - i];
        }
        else
        {
          return 0;
        }
      }

      /**
       * Set the builder to a not building state.
       */
      constexpr void
      init(address_t slab, const FreeListKey& key, address_t key_tweak)
      {
        for (size_t i = 0; i < LENGTH; i++)
        {
          end[i] = &head[i];
          if constexpr (TRACK_LENGTH)
          {
            length[i] = 0;
          }

          // Head is not live when a building is initialised.
          // We use this slot to store a pointer into the slab for the
          // allocations. This then establishes the invariant that head is
          // always (a possibly encoded) pointer into the slab, and thus
          // the Freelist builder always knows which block it is referring too.
          head[i] = Object::code_next(
            address_cast(&head[i]),
            useless_ptr_from_addr<Object::T<BQueue>>(slab),
            key,
            key_tweak);
        }
      }

      template<bool RANDOM_ = RANDOM>
      stl::enable_if_t<!RANDOM_, size_t> extract_segment_length()
      {
        static_assert(RANDOM_ == RANDOM, "Don't set SFINAE parameter!");
        return length[0];
      }

      template<bool RANDOM_ = RANDOM>
      stl::enable_if_t<
        !RANDOM_,
        stl::Pair<
          Object::BHeadPtr<BView, BQueue>,
          Object::BHeadPtr<BView, BQueue>>>
      extract_segment(const FreeListKey& key, address_t key_tweak)
      {
        static_assert(RANDOM_ == RANDOM, "Don't set SFINAE parameter!");
        SNMALLOC_ASSERT(!empty());

        auto first = read_head(0, key, key_tweak);
        // end[0] is pointing to the first field in the object,
        // this is doing a CONTAINING_RECORD like cast to get back
        // to the actual object.  This isn't true if the builder is
        // empty, but you are not allowed to call this in the empty case.
        auto last = Object::BHeadPtr<BView, BQueue>::unsafe_from(
          Object::from_next_ptr(cast_end(0)));
        init(address_cast(head[0]), key, key_tweak);
        return {first, last};
      }

      /**
       * Put back an extracted segment from a builder using the same key.
       *
       * The caller must tell us how many elements are involved.
       */
      void append_segment(
        Object::BHeadPtr<BView, BQueue> first,
        Object::BHeadPtr<BView, BQueue> last,
        uint16_t size,
        const FreeListKey& key,
        address_t key_tweak,
        LocalEntropy& entropy)
      {
        uint32_t index;
        if constexpr (RANDOM)
          index = entropy.next_bit();
        else
          index = 0;

        if constexpr (TRACK_LENGTH)
          length[index] += size;
        else
          UNUSED(size);

        Object::store_next(cast_end(index), first, key, key_tweak);
        set_end(index, &(last->next_object));
      }

      template<typename Domesticator>
      SNMALLOC_FAST_PATH void validate(
        const FreeListKey& key, address_t key_tweak, Domesticator domesticate)
      {
        if constexpr (mitigations(freelist_teardown_validate))
        {
          for (uint32_t i = 0; i < LENGTH; i++)
          {
            if (&head[i] == end[i])
            {
              SNMALLOC_CHECK(!TRACK_LENGTH || (length[i] == 0));
              continue;
            }

            size_t count = 1;
            auto curr = read_head(i, key, key_tweak);
            auto prev = get_fake_signed_prev(i, key, key_tweak);
            while (true)
            {
              curr->check_prev(prev);
              if (address_cast(&(curr->next_object)) == address_cast(end[i]))
                break;
              count++;
              auto next = curr->read_next(key, key_tweak, domesticate);
              prev = signed_prev(
                address_cast(curr), address_cast(next), key, key_tweak);
              curr = next;
            }
            SNMALLOC_CHECK(!TRACK_LENGTH || (count == length[i]));
          }
        }
        else
        {
          UNUSED(key);
          UNUSED(key_tweak);
          UNUSED(domesticate);
        }
      }
    };
  } // namespace freelist
} // namespace snmalloc
