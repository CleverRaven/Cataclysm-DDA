#pragma once

#include "../ds/ds.h"
#include "check_init.h"
#include "freelist.h"
#include "metadata.h"
#include "pool.h"
#include "remotecache.h"
#include "sizeclasstable.h"
#include "snmalloc/stl/new.h"
#include "ticker.h"

#if defined(_MSC_VER)
#  define ALLOCATOR __declspec(allocator) __declspec(restrict)
#elif __has_attribute(malloc)
#  define ALLOCATOR __attribute__((malloc))
#else
#  define ALLOCATOR
#endif

namespace snmalloc
{
  /**
   * @brief This is the default continuation class used for handling successful,
   * and failing allocations.
   *
   * @tparam zero_mem  Specifies whether the allocated memory should be zeroed.
   *
   * The failure case sets errno to ENOMEM, as per the C standard.
   */
  template<ZeroMem zero_mem = ZeroMem::NoZero>
  class DefaultConts
  {
  public:
    static void* success(void* p, size_t size, bool secondary_allocator = false)
    {
      UNUSED(secondary_allocator);
      SNMALLOC_ASSERT(p != nullptr);

      SNMALLOC_ASSERT(
        secondary_allocator ||
        is_start_of_object(size_to_sizeclass_full(size), address_cast(p)));

      if (zero_mem == ZeroMem::YesZero)
      {
        // Zero the memory if requested.
        return ::memset(p, 0, round_size(size));
      }

      return p;
    }

    static void* failure(size_t size) noexcept
    {
      UNUSED(size);
      // If we are here, then the allocation failed.
      // Set errno to ENOMEM, as per the C standard.
      errno = ENOMEM;
      // Return nullptr on failure.
      return nullptr;
    }
  };

  using Zero = DefaultConts<ZeroMem::YesZero>;
  using Uninit = DefaultConts<ZeroMem::NoZero>;

  template<typename Conts>
  inline static SNMALLOC_FAST_PATH void*
  finish_alloc(freelist::HeadPtr p, size_t size)
  {
    return Conts::success(capptr_reveal(p.as_void()), size, false);
  }

  struct FastFreeLists
  {
    // Free list per small size class.  These are used for
    // allocation on the fast path. This part of the code is inspired by
    // mimalloc.
    freelist::Iter<> small_fast_free_lists[NUM_SMALL_SIZECLASSES] = {};
  };

  /**
   * The core, stateful, part of a memory allocator.
   *
   * The template parameter provides all of the global configuration for this
   * instantiation of snmalloc.  This includes three options that apply to this
   * class:
   *
   * - `AllocIsPoolAllocated` defines whether this `Allocator`
   *   configuration should support pool allocation.  This defaults to true but
   *   a configuration that allocates allocators eagerly may opt out.
   * - `AllocOwnsLocalState` defines whether the `Allocator` owns the
   *   associated `LocalState` object.  If this is true (the default) then
   *   `Allocator` embeds the LocalState object.  If this is set to false
   *   then a `LocalState` object must be provided to the constructor.  This
   *   allows external code to provide explicit configuration of the address
   *   range managed by this object.
   * - `IsQueueInline` (defaults to true) defines whether the message queue
   *   (`RemoteAllocator`) for this class is inline or provided externally.  If
   *   provided externally, then it must be set explicitly with
   *   `init_message_queue`.
   */
  template<SNMALLOC_CONCEPT(IsConfigLazy) Config_>
  class Allocator : public FastFreeLists,
                    public stl::conditional_t<
                      Config_::Options.AllocIsPoolAllocated,
                      Pooled<Allocator<Config_>>,
                      Empty>
  {
  public:
    using Config = Config_;

  private:
    /**
     * Define local names for specialised versions of various types that are
     * specialised for the back-end that we are using.
     * @{
     */
    using BackendSlabMetadata = typename Config::Backend::SlabMetadata;
    using PagemapEntry = typename Config::PagemapEntry;

    /// }@

    /**
     * Remote deallocations for other threads
     */
    RemoteDeallocCache<Config> remote_dealloc_cache;

    /**
     * Per size class list of active slabs for this allocator.
     */
    struct SlabMetadataCache
    {
      SeqSet<BackendSlabMetadata> available{};

      uint16_t unused = 0;
      uint16_t length = 0;
    } alloc_classes[NUM_SMALL_SIZECLASSES]{};

    /**
     * The set of all slabs and large allocations
     * from this allocator that are full or almost full.
     */
    SeqSet<BackendSlabMetadata> laden{};

    /**
     * Local entropy source and current version of keys for
     * this thread
     */
    LocalEntropy entropy;

    /**
     * Message queue for allocations being returned to this
     * allocator
     */
    stl::conditional_t<
      Config::Options.IsQueueInline,
      RemoteAllocator,
      RemoteAllocator*>
      remote_alloc;

    /**
     * The type used local state.  This is defined by the back end.
     */
    using LocalState = typename Config::LocalState;

    /**
     * A local area of address space managed by this allocator.
     * Used to reduce calls on the global address space.  This is inline if the
     * core allocator owns the local state or indirect if it is owned
     * externally.
     */
    stl::conditional_t<
      Config::Options.AllocOwnsLocalState,
      LocalState,
      LocalState*>
      backend_state;

    /**
     * Ticker to query the clock regularly at a lower cost.
     */
    Ticker<typename Config::Pal> ticker;

    /**
     * The message queue needs to be accessible from other threads
     *
     * In the cross trust domain version this is the minimum amount
     * of allocator state that must be accessible to other threads.
     */
    auto* public_state()
    {
      if constexpr (Config::Options.IsQueueInline)
      {
        return &remote_alloc;
      }
      else
      {
        return remote_alloc;
      }
    }

    /**
     * Return a pointer to the backend state.
     */
    LocalState* backend_state_ptr()
    {
      if constexpr (Config::Options.AllocOwnsLocalState)
      {
        return &backend_state;
      }
      else
      {
        return backend_state;
      }
    }

    /**
     * Return this allocator's "truncated" ID, an integer useful as a hash
     * value of this allocator.
     *
     * Specifically, this is the address of this allocator's message queue
     * with the least significant bits missing, masked by SIZECLASS_MASK.
     * This will be unique for Allocs with inline queues; Allocs with
     * out-of-line queues must ensure that no two queues' addresses collide
     * under this masking.
     */
    size_t get_trunc_id()
    {
      return public_state()->trunc_id();
    }

    /**
     * Abstracts access to the message queue to handle different
     * layout configurations of the allocator.
     */
    auto& message_queue()
    {
      return *public_state();
    }

    /**
     * Check if this allocator has messages to deallocate blocks from another
     * thread
     */
    SNMALLOC_FAST_PATH bool has_messages()
    {
      return message_queue().can_dequeue();
    }

    /**
     * Initialiser, shared code between the constructors for different
     * configurations.
     */
    void init()
    {
#ifdef SNMALLOC_TRACING
      message<1024>("Making an allocator.");
#endif

      // Entropy must be first, so that all data-structures can use the key
      // it generates.
      // This must occur before any freelists are constructed.
      entropy.init<typename Config::Pal>();

      // Ignoring stats for now.
      //      stats().start();

      // Initialise the remote cache
      remote_dealloc_cache.init();
    }

    /**
     * Initialiser, shared code between the constructors for different
     * configurations.
     *
     * spare is the amount of space directly after the allocator that is
     * reserved as meta-data, but is not required by this Allocator.
     */
    void init(Range<capptr::bounds::Alloc>& spare)
    {
      init();

      if (spare.length != 0)
      {
        /*
         * Seed this frontend's private metadata allocation cache with any
         * excess space from the metadata allocation holding the frontend
         * Allocator object itself.  This alleviates thundering herd
         * contention on the backend during startup: each slab opened now
         * makes one trip to the backend, for the slab itself, rather than
         * two, for the slab and its metadata.
         */
        Config::Backend::dealloc_meta_data(
          get_backend_local_state(), spare.base, spare.length);
      }
    }

    friend class ThreadAlloc;
    constexpr Allocator(bool){};

  public:
    /**
     * Constructor for the case that the core allocator owns the local state.
     * SFINAE disabled if the allocator does not own the local state.
     *
     * spare is the amount of space directly after the allocator that is
     * reserved as meta-data, but is not required by this Allocator.
     */
    template<
      typename Config__ = Config,
      typename = stl::enable_if_t<Config__::Options.AllocOwnsLocalState>>
    Allocator(Range<capptr::bounds::Alloc>& spare)
    {
      init(spare);
    }

    /**
     * Constructor for the case that the core allocator does not owns the local
     * state. SFINAE disabled if the allocator does own the local state.
     *
     * spare is the amount of space directly after the allocator that is
     * reserved as meta-data, but is not required by this Allocator.
     */
    template<
      typename Config__ = Config,
      typename = stl::enable_if_t<!Config__::Options.AllocOwnsLocalState>>
    Allocator(
      Range<capptr::bounds::Alloc>& spare, LocalState* backend = nullptr)
    : backend_state(backend)
    {
      init(spare);
    }

    /**
     * Constructor for the case that the core allocator does not owns the local
     * state. SFINAE disabled if the allocator does own the local state.
     *
     * This constructor is used when the allocator is not pool allocated.
     */
    template<
      typename Config__ = Config,
      typename = stl::enable_if_t<Config__::Options.AllocOwnsLocalState>>
    Allocator()
    {
      init();
    }

    /**
     * If the message queue is not inline, provide it.  This will then
     * configure the message queue for use.
     */
    template<bool InlineQueue = Config::Options.IsQueueInline>
    stl::enable_if_t<!InlineQueue> init_message_queue(RemoteAllocator* q)
    {
      remote_alloc = q;
    }

    /**
     * Post deallocations onto other threads.
     *
     * Returns true if it actually performed a post,
     * and false otherwise.
     */
    SNMALLOC_FAST_PATH bool post()
    {
      // stats().remote_post();  // TODO queue not in line!
      bool sent_something =
        remote_dealloc_cache.template post<sizeof(Allocator)>(
          backend_state_ptr(), public_state()->trunc_id());

      return sent_something;
    }

    /**
     * Accessor for the local state.  This hides whether the local state is
     * stored inline or provided externally from the rest of the code.
     */
    SNMALLOC_FAST_PATH
    LocalState& get_backend_local_state()
    {
      if constexpr (Config::Options.AllocOwnsLocalState)
      {
        return backend_state;
      }
      else
      {
        SNMALLOC_ASSERT(backend_state);
        return *backend_state;
      }
    }

    /**
     * Handles the messages in the queue if there are any,
     * and then calls the action.
     *
     * This is designed to provide a fast path common case that simply
     * checks the queue and calls the action if it is empty. If there
     * are messages, then it goes to the slow path, handle_message_queue_slow.
     *
     * This is heavily templated to cause inlining, and passing of arguments on
     * the stack as often closing over the arguments would cause less good
     * codegen.
     */
    template<bool noexc, typename Action, typename... Args>
    SNMALLOC_FAST_PATH decltype(auto)
    handle_message_queue(Action action, Args... args) noexcept(noexc)
    {
      // Inline the empty check, but not necessarily the full queue handling.
      if (SNMALLOC_LIKELY(!has_messages()))
      {
        return action(args...);
      }

      return handle_message_queue_slow<noexc>(action, args...);
    }

    /**
     * Process remote frees into this allocator.
     */
    template<bool noexc, typename Action, typename... Args>
    SNMALLOC_SLOW_PATH decltype(auto)
    handle_message_queue_slow(Action action, Args... args) noexcept(noexc)
    {
      bool need_post = false;
      size_t bytes_freed = 0;
      auto local_state = backend_state_ptr();
      auto domesticate = [local_state](freelist::QueuePtr p)
                           SNMALLOC_FAST_PATH_LAMBDA {
                             return capptr_domesticate<Config>(local_state, p);
                           };
      auto cb = [this, domesticate, &need_post, &bytes_freed](
                  capptr::Alloc<RemoteMessage> msg) SNMALLOC_FAST_PATH_LAMBDA {
        auto& entry =
          Config::Backend::get_metaentry(snmalloc::address_cast(msg));
        handle_dealloc_remote(entry, msg, need_post, domesticate, bytes_freed);
        return bytes_freed < REMOTE_BATCH_LIMIT;
      };

#ifdef SNMALLOC_TRACING
      message<1024>("Handling remote queue before proceeding...");
#endif

      if constexpr (Config::Options.QueueHeadsAreTame)
      {
        /*
         * The front of the queue has already been validated; just change the
         * annotating type.
         */
        auto domesticate_first =
          [](freelist::QueuePtr p) SNMALLOC_FAST_PATH_LAMBDA {
            return freelist::HeadPtr::unsafe_from(p.unsafe_ptr());
          };
        message_queue().dequeue(domesticate_first, domesticate, cb);
      }
      else
      {
        message_queue().dequeue(domesticate, domesticate, cb);
      }

      if (need_post)
      {
        post();
      }

      return action(args...);
    }

    /**
     * Dealloc a message either by putting for a forward, or
     * deallocating locally.
     *
     * need_post will be set to true, if capacity is exceeded.
     */
    template<typename Domesticator_queue>
    void handle_dealloc_remote(
      const PagemapEntry& entry,
      capptr::Alloc<RemoteMessage> msg,
      bool& need_post,
      Domesticator_queue domesticate,
      size_t& bytes_returned)
    {
      // TODO this needs to not double count stats
      // TODO this needs to not double revoke if using MTE
      // TODO thread capabilities?

      if (SNMALLOC_LIKELY(entry.get_remote() == public_state()))
      {
        auto meta = entry.get_slab_metadata();

        auto unreturned = dealloc_local_objects_fast(
          msg, entry, meta, entropy, domesticate, bytes_returned);

        /*
         * dealloc_local_objects_fast has updated the free list but not updated
         * the slab metadata; it falls to us to do so.  It is UNLIKELY that we
         * will need to take further steps, but we might.
         */
        if (SNMALLOC_UNLIKELY(unreturned.template step<true>()))
        {
          dealloc_local_object_slow(msg.as_void(), entry, meta);

          while (SNMALLOC_UNLIKELY(unreturned.template step<false>()))
          {
            dealloc_local_object_meta(entry, meta);
          }
        }

        return;
      }

      auto nelem = RemoteMessage::template ring_size<Config>(
        msg,
        freelist::Object::key_root,
        entry.get_slab_metadata()->as_key_tweak(),
        domesticate);
      if (!need_post && !remote_dealloc_cache.reserve_space(entry, nelem))
      {
        need_post = true;
      }
      remote_dealloc_cache.template forward<sizeof(Allocator)>(
        entry.get_remote()->trunc_id(), msg);
    }

    template<typename Domesticator>
    SNMALLOC_FAST_PATH static auto dealloc_local_objects_fast(
      capptr::Alloc<RemoteMessage> msg,
      const PagemapEntry& entry,
      BackendSlabMetadata* meta,
      LocalEntropy& entropy,
      Domesticator domesticate,
      size_t& bytes_freed)
    {
      SNMALLOC_ASSERT(!meta->is_unused());

      snmalloc_check_client(
        mitigations(sanity_checks),
        is_start_of_object(entry.get_sizeclass(), address_cast(msg)),
        "Not deallocating start of an object");

      size_t objsize = sizeclass_full_to_size(entry.get_sizeclass());

      auto [curr, length] = RemoteMessage::template open_free_ring<Config>(
        msg,
        objsize,
        freelist::Object::key_root,
        meta->as_key_tweak(),
        domesticate);

      bytes_freed += objsize * length;

      // Update the head and the next pointer in the free list.
      meta->free_queue.append_segment(
        curr,
        msg.template as_reinterpret<freelist::Object::T<>>(),
        length,
        freelist::Object::key_root,
        meta->as_key_tweak(),
        entropy);

      return meta->return_objects(length);
    }

    /*************************************************************************
     * Allocation Code Overview
     *
     * The code is structured to allow the compiler to inline and turn most
     * paths into tail calls.
     *
     * The overall shape is:
     *
     *  - alloc(size_t)
     *    - small_alloc(size_t)
     *      - gets allocation from a fast free list and is done.
     *      - if no fast free list,
     *         - check for message queue
     *         - small_refill(size_t)
     *           - If another free list is available, use it.
     *           - Otherwise,
     *             - Check if the allocator has been initialised, if it hasn't
     *               restart with a fresh allocator.
     *             - Otherwise, call small_refill_slow(size_t) which allocates
     *               a new slab and fills it with a free list using
     *               alloc_new_list.
     *    - static alloc_not_small(size_t, Allocator*)
     *      - If size is zero, call small_alloc(1) - this deals with the
     *        annoying case of zero off the fast path.
     *      - Otherwise,
     *        - Check for initialisation, and
     *        - call backend to allocate a large allocation.
     *************************************************************************/

    /**
     * Allocates a block of memory of the given size.
     * @param size The size of the block to allocate.
     * @return A pointer to the allocated block, or nullptr if the allocation
     *        failed.
     * @tparam Conts : Carries two function success and failure, that are used
     * to either apply additional operations in the case of success, such as
     * zeroing, or to handle the failure case, such as set_new_handler, or
     * throwing an exception or setting errno to ENOMEM.
     * @tparam CheckInit is a class that can handle lazy initiasation if
     * required. It is defaulted to do nothing.
     */
    template<typename Conts = Uninit, typename CheckInit = CheckInitNoOp>
    SNMALLOC_FAST_PATH ALLOCATOR void*
    alloc(size_t size) noexcept(noexcept(Conts::failure(0)))
    {
      // Perform the - 1 on size, so that zero wraps around and ends up on
      // slow path.
      if (SNMALLOC_LIKELY(
            (size - 1) <= (sizeclass_to_size(NUM_SMALL_SIZECLASSES - 1) - 1)))
      {
        // Small allocations are more likely. Improve
        // branch prediction by placing this case first.
        return small_alloc<Conts, CheckInit>(size);
      }

      return alloc_not_small<Conts, CheckInit>(size, this);
    }

    /**
     * Fast allocation for small objects.
     */
    template<typename Conts, typename CheckInit>
    SNMALLOC_FAST_PATH void*
    small_alloc(size_t size) noexcept(noexcept(Conts::failure(0)))
    {
      auto domesticate =
        [this](freelist::QueuePtr p) SNMALLOC_FAST_PATH_LAMBDA {
          return capptr_domesticate<Config>(backend_state_ptr(), p);
        };

      auto& key = freelist::Object::key_root;
      smallsizeclass_t sizeclass = size_to_sizeclass(size);
      auto* fl = &small_fast_free_lists[sizeclass];
      if (SNMALLOC_LIKELY(!fl->empty()))
      {
        auto p = fl->take(key, domesticate);
        return finish_alloc<Conts>(p, size);
      }

      return handle_message_queue<noexcept(Conts::failure(0))>(
        [](
          Allocator* alloc,
          smallsizeclass_t sizeclass,
          freelist::Iter<>* fl,
          size_t size) SNMALLOC_FAST_PATH_LAMBDA {
          return alloc->small_refill<Conts, CheckInit>(sizeclass, *fl, size);
        },
        this,
        sizeclass,
        fl,
        size);
    }

    /**
     * Allocation that are larger that will result in an allocation directly
     * from the backend. This additionally checks for 0 size allocations and
     * handles checking for messages and initialisation.
     *
     * Note this is a static method to switch the argument order to improve
     * code quality as the calling code already has size in the first argument
     * register.
     */
    template<typename Conts, typename CheckInit>
    static SNMALLOC_SLOW_PATH void* alloc_not_small(
      size_t size, Allocator* self) noexcept(noexcept(Conts::failure(0)))
    {
      if (size == 0)
      {
        // Deal with alloc zero of with a small object here.
        // Alternative semantics giving nullptr is also allowed by the
        // standard.
        return self->small_alloc<Conts, CheckInit>(1);
      }

      return self->template handle_message_queue<noexcept(Conts::failure(0))>(
        [](Allocator* self, size_t size) SNMALLOC_FAST_PATH_LAMBDA {
          return CheckInit::check_init(
            [self, size]() SNMALLOC_FAST_PATH_LAMBDA {
              if (size > bits::one_at_bit(bits::BITS - 1))
              {
                // Cannot allocate something that is more that half the size of
                // the address space
                return Conts::failure(size);
              }

              // Check if secondary allocator wants to offer the memory
              void* result = Config::SecondaryAllocator::allocate(
                [size]() -> stl::Pair<size_t, size_t> {
                  return {size, natural_alignment(size)};
                });
              if (result != nullptr)
              {
                return Conts::success(result, size, true);
              }

              // Grab slab of correct size
              // Set remote as large allocator remote.
              auto [chunk, meta] = Config::Backend::alloc_chunk(
                self->get_backend_local_state(),
                large_size_to_chunk_size(size),
                PagemapEntry::encode(
                  self->public_state(), size_to_sizeclass_full(size)),
                size_to_sizeclass_full(size));

#ifdef SNMALLOC_TRACING
              message<1024>(
                "size {} pow2size {}", size, bits::next_pow2_bits(size));
#endif

              SNMALLOC_ASSERT(
                (chunk.unsafe_ptr() == nullptr) == (meta == nullptr));

              // set up meta data so sizeclass is correct, and hence alloc size,
              // and external pointer. Initialise meta data for a successful
              // large allocation.
              if (meta != nullptr)
              {
                meta->initialise_large(
                  address_cast(chunk), freelist::Object::key_root);
                self->laden.insert(meta);

                // Make capptr system happy we are allowed to pass this to
                // `success`.
                auto p = capptr_reveal(
                  capptr_chunk_is_alloc(capptr_to_user_address_control(chunk)));
                return Conts::success(p, size);
              }

              return Conts::failure(size);
            },
            [](Allocator* a, size_t size) SNMALLOC_FAST_PATH_LAMBDA {
              return alloc_not_small<Conts, CheckInitNoOp>(size, a);
            },
            size);
        },
        self,
        size);
    }

    template<typename Conts, typename CheckInit>
    SNMALLOC_FAST_PATH void* small_refill(
      smallsizeclass_t sizeclass,
      freelist::Iter<>& fast_free_list,
      size_t size) noexcept(noexcept(Conts::failure(0)))
    {
      void* result = Config::SecondaryAllocator::allocate(
        [size]() -> stl::Pair<size_t, size_t> {
          return {size, natural_alignment(size)};
        });

      if (result != nullptr)
      {
        result = Conts::success(result, size, true);

        // We need to check for initialisation here in the case where
        // this is the first allocation in the system, so snmalloc has
        // not initialised the pagemap.  If this allocation is subsequently
        // deallocated, before snmalloc is initialised, then it will fail
        // to access the pagemap.
        return CheckInit::check_init(
          [result]() { return result; },
          [](Allocator*, void* result) { return result; },
          result);
      }

      // Look to see if we can grab a free list.
      auto& sl = alloc_classes[sizeclass].available;
      if (SNMALLOC_LIKELY(alloc_classes[sizeclass].length > 0))
      {
        if constexpr (mitigations(random_extra_slab))
        {
          // Occassionally don't use the last list.
          if (SNMALLOC_UNLIKELY(alloc_classes[sizeclass].length == 1))
          {
            if (entropy.next_bit() == 0)
              return small_refill_slow<Conts, CheckInit>(
                sizeclass, fast_free_list, size);
          }
        }

        // Mitigations use LIFO to increase time to reuse.
        auto meta = sl.template pop<!mitigations(reuse_LIFO)>();
        // Drop length of sl, and empty count if it was empty.
        alloc_classes[sizeclass].length--;
        if (meta->needed() == 0)
          alloc_classes[sizeclass].unused--;

        auto domesticate =
          [this](freelist::QueuePtr p) SNMALLOC_FAST_PATH_LAMBDA {
            return capptr_domesticate<Config>(backend_state_ptr(), p);
          };
        auto [p, still_active] = BackendSlabMetadata::alloc_free_list(
          domesticate, meta, fast_free_list, entropy, sizeclass);

        if (still_active)
        {
          alloc_classes[sizeclass].length++;
          sl.insert(meta);
        }
        else
        {
          laden.insert(meta);
        }

        auto r = finish_alloc<Conts>(p, size);
        return ticker.check_tick(r);
      }
      return small_refill_slow<Conts, CheckInit>(
        sizeclass, fast_free_list, size);
    }

    template<typename Conts, typename CheckInit>
    SNMALLOC_SLOW_PATH void* small_refill_slow(
      smallsizeclass_t sizeclass,
      freelist::Iter<>& fast_free_list,
      size_t size) noexcept(noexcept(Conts::failure(0)))
    {
      return CheckInit::check_init(
        [this, size, sizeclass, &fast_free_list]() SNMALLOC_FAST_PATH_LAMBDA {
          size_t rsize = sizeclass_to_size(sizeclass);

          // No existing free list get a new slab.
          size_t slab_size = sizeclass_to_slab_size(sizeclass);

#ifdef SNMALLOC_TRACING
          message<1024>(
            "small_refill_slow rsize={} slab size={}", rsize, slab_size);
#endif

          auto [slab, meta] = Config::Backend::alloc_chunk(
            get_backend_local_state(),
            slab_size,
            PagemapEntry::encode(
              public_state(), sizeclass_t::from_small_class(sizeclass)),
            sizeclass_t::from_small_class(sizeclass));

          if (slab == nullptr)
          {
            return Conts::failure(sizeclass_to_size(sizeclass));
          }

          // Set meta slab to empty.
          meta->initialise(
            sizeclass, address_cast(slab), freelist::Object::key_root);

          // Build a free list for the slab
          alloc_new_list(slab, meta, rsize, slab_size, entropy);

          auto domesticate =
            [this](freelist::QueuePtr p) SNMALLOC_FAST_PATH_LAMBDA {
              return capptr_domesticate<Config>(backend_state_ptr(), p);
            };
          auto [p, still_active] = BackendSlabMetadata::alloc_free_list(
            domesticate, meta, fast_free_list, entropy, sizeclass);

          if (still_active)
          {
            alloc_classes[sizeclass].length++;
            alloc_classes[sizeclass].available.insert(meta);
          }
          else
          {
            laden.insert(meta);
          }

          auto r = finish_alloc<Conts>(p, size);
          return ticker.check_tick(r);
        },
        [](Allocator* a, size_t size) SNMALLOC_FAST_PATH_LAMBDA {
          return a->small_alloc<Conts, CheckInitNoOp>(size);
        },
        size);
    }

    static SNMALLOC_FAST_PATH void alloc_new_list(
      capptr::Chunk<void>& bumpptr,
      BackendSlabMetadata* meta,
      size_t rsize,
      size_t slab_size,
      LocalEntropy& entropy)
    {
      auto slab_end = pointer_offset(bumpptr, slab_size + 1 - rsize);

      auto key_tweak = meta->as_key_tweak();

      auto& b = meta->free_queue;

      if constexpr (mitigations(random_initial))
      {
        // Structure to represent the temporary list elements
        struct PreAllocObject
        {
          capptr::AllocFull<PreAllocObject> next;
        };

        // The following code implements Sattolo's algorithm for generating
        // random cyclic permutations.  This implementation is in the opposite
        // direction, so that the original space does not need initialising.
        // This is described as outside-in without citation on Wikipedia,
        // appears to be Folklore algorithm.

        // Note the wide bounds on curr relative to each of the ->next fields;
        // curr is not persisted once the list is built.
        capptr::Chunk<PreAllocObject> curr =
          pointer_offset(bumpptr, 0).template as_static<PreAllocObject>();
        curr->next =
          Aal::capptr_bound<PreAllocObject, capptr::bounds::AllocFull>(
            curr, rsize);

        uint16_t count = 1;
        for (curr =
               pointer_offset(curr, rsize).template as_static<PreAllocObject>();
             curr.as_void() < slab_end;
             curr =
               pointer_offset(curr, rsize).template as_static<PreAllocObject>())
        {
          size_t insert_index = entropy.sample(count);
          curr->next = stl::exchange(
            pointer_offset(bumpptr, insert_index * rsize)
              .template as_static<PreAllocObject>()
              ->next,
            Aal::capptr_bound<PreAllocObject, capptr::bounds::AllocFull>(
              curr, rsize));
          count++;
        }

        // Pick entry into space, and then build linked list by traversing cycle
        // to the start.  Use ->next to jump from Chunk to Alloc.
        auto start_index = entropy.sample(count);
        auto start_ptr = pointer_offset(bumpptr, start_index * rsize)
                           .template as_static<PreAllocObject>()
                           ->next;
        auto curr_ptr = start_ptr;
        do
        {
          auto next_ptr = curr_ptr->next;
          b.add(
            // Here begins our treatment of the heap as containing Wild pointers
            freelist::Object::make<capptr::bounds::AllocWild>(
              capptr_to_user_address_control(curr_ptr.as_void())),
            freelist::Object::key_root,
            key_tweak,
            entropy);
          curr_ptr = next_ptr;
        } while (curr_ptr != start_ptr);
      }
      else
      {
        auto p = bumpptr;
        do
        {
          b.add(
            // Here begins our treatment of the heap as containing Wild pointers
            freelist::Object::make<capptr::bounds::AllocWild>(
              capptr_to_user_address_control(
                Aal::capptr_bound<void, capptr::bounds::AllocFull>(
                  p.as_void(), rsize))),
            freelist::Object::key_root,
            key_tweak,
            entropy);
          p = pointer_offset(p, rsize);
        } while (p < slab_end);
      }
      // This code consumes everything up to slab_end.
      bumpptr = slab_end;
    }

    /***************************************************************************
     *   Deallocation code
     *
     * The main deallocation code is in dealloc.  This, like alloc, takes a
     * template to initialise the state of the allocator.  It is possible for
     * the first operation on a thread to be a deallocation.
     *
     * The algorithm is
     * - dealloc(void*)
     *  - If object, originated from this allocator, then
     *    - dealloc_local_object(void*) which returns the object to the free
     *      list.
     *      - the free list length is updated, which can trigger a slow path for
     *        * large objects (dealloc_local_object_slow)
     *        * completely full list
     *        * previously empty list - (in checked builds we move the notion of
     *          empty to increase randomness)
     *        The later two are handled by dealloc_local_object_meta.
     *        In the completely full list case, then we heuristically determine
     *        if we should return slabs to the backend using dealloc_local_slabs
     *  - Otherwise, the object originated from another allocator, so we call
     *    - dealloc_remote(void*)
     *      - If there is sufficient budget to put in the remote cache, we do
     *        that and return.
     *      - Otherwise, we need to create some space in the remote cache using
     *        dealloc_remote_slow. This initially checks if we actually have a
     *        proper allocator.
     *        - If not, we acquire an allocator, and completely restart the
     *          deallocation process (we may acquire the originating allocator).
     *        - If we do have an allocator, we post the remote cache and this
     *          deallocation to the other allocators.
     ***************************************************************************/
    template<typename CheckInit = CheckInitNoOp>
    SNMALLOC_FAST_PATH void dealloc(void* p_raw) noexcept
    {
#ifdef __CHERI_PURE_CAPABILITY__
      /*
       * On CHERI platforms, snap the provided pointer to its base, ignoring
       * any client-provided offset, which may have taken the pointer out of
       * bounds and so appear to designate a different object.  The base is
       * is guaranteed by monotonicity either...
       *  * to be within the bounds originally returned by alloc(), or
       *  * one past the end (in which case, the capability length must be 0).
       *
       * Setting the offset does not trap on untagged capabilities, so the tag
       * might be clear after this, as well.
       *
       * For a well-behaved client, this is a no-op: the base is already at
       * the start of the allocation and so the offset is zero.
       */
      p_raw = __builtin_cheri_offset_set(p_raw, 0);
#endif
      capptr::AllocWild<void> p_wild =
        capptr_from_client(const_cast<void*>(p_raw));
      auto p_tame = capptr_domesticate<Config>(backend_state_ptr(), p_wild);
      const PagemapEntry& entry =
        Config::Backend::get_metaentry(address_cast(p_tame));

      /*
       * p_tame may be nullptr, even if p_raw/p_wild are not, in the case
       * where domestication fails.  We exclusively use p_tame below so that
       * such failures become no ops; in the nullptr path, which should be
       * well off the fast path, we could be slightly more aggressive and test
       * that p_raw is also nullptr and Pal::error() if not. (TODO)
       *
       * We do not rely on the bounds-checking ability of domestication here,
       * and just check the address (and, on other architectures, perhaps
       * well-formedness) of this pointer.  The remainder of the logic will
       * deal with the object's extent.
       */
      if (SNMALLOC_LIKELY(public_state() == entry.get_remote()))
      {
        dealloc_cheri_checks(p_tame.unsafe_ptr());
        dealloc_local_object(p_tame, entry);
        return;
      }

      dealloc_remote<CheckInit>(entry, p_tame);
    }

    SNMALLOC_FAST_PATH void dealloc_local_object(
      CapPtr<void, capptr::bounds::Alloc> p,
      const typename Config::PagemapEntry& entry) noexcept
    {
      auto meta = entry.get_slab_metadata();

      SNMALLOC_ASSERT(!meta->is_unused());

      snmalloc_check_client(
        mitigations(sanity_checks),
        is_start_of_object(entry.get_sizeclass(), address_cast(p)),
        "Not deallocating start of an object");

      auto cp = p.as_static<freelist::Object::T<>>();

      // Update the head and the next pointer in the free list.
      meta->free_queue.add(
        cp, freelist::Object::key_root, meta->as_key_tweak(), entropy);

      if (SNMALLOC_LIKELY(!meta->return_object()))
        return;

      dealloc_local_object_slow(p, entry, meta);
    }

    /**
     * Slow path for deallocating an object locally.
     * This is either waking up a slab that was not actively being used
     * by this thread, or handling the final deallocation onto a slab,
     * so it can be reused by other threads.
     *
     * Live large objects look like slabs that need attention when they become
     * free; that attention is also given here.
     */
    SNMALLOC_SLOW_PATH void dealloc_local_object_slow(
      capptr::Alloc<void> p,
      const PagemapEntry& entry,
      BackendSlabMetadata* meta) noexcept
    {
      // TODO: Handle message queue on this path?

      if (meta->is_large())
      {
        // Handle large deallocation here.

        // XXX: because large objects have unique metadata associated with them,
        // the ring size here is one.  We should probably assert that.

        size_t entry_sizeclass = entry.get_sizeclass().as_large();
        size_t size = bits::one_at_bit(entry_sizeclass);

#ifdef SNMALLOC_TRACING
        message<1024>("Large deallocation: {}", size);
#else
        UNUSED(size);
#endif

        // Remove from set of fully used slabs.
        meta->node.remove();

        Config::Backend::dealloc_chunk(
          get_backend_local_state(), *meta, p, size, entry.get_sizeclass());

        return;
      }

      // Not a large object; update slab metadata
      dealloc_local_object_meta(entry, meta);
    }

    /**
     * Very slow path for object deallocation.
     *
     * The object has already been returned to the slab, so all that is left to
     * do is update its metadata and, if that pushes us into having too many
     * unused slabs in this size class, return some.
     *
     * Also while here, check the time.
     */
    SNMALLOC_SLOW_PATH void dealloc_local_object_meta(
      const PagemapEntry& entry, BackendSlabMetadata* meta)
    {
      smallsizeclass_t sizeclass = entry.get_sizeclass().as_small();

      if (meta->is_sleeping())
      {
        // Slab has been woken up add this to the list of slabs with free space.

        //  Wake slab up.
        meta->set_not_sleeping(sizeclass);

        // Remove from set of fully used slabs.
        meta->node.remove();

        alloc_classes[sizeclass].available.insert(meta);
        alloc_classes[sizeclass].length++;

#ifdef SNMALLOC_TRACING
        message<1024>("Slab is woken up");
#endif

        ticker.check_tick();
        return;
      }

      alloc_classes[sizeclass].unused++;

      // If we have several slabs, and it isn't too expensive as a proportion
      // return to the global pool.
      if (
        (alloc_classes[sizeclass].unused > 2) &&
        (alloc_classes[sizeclass].unused >
         (alloc_classes[sizeclass].length >> 2)))
      {
        dealloc_local_slabs(sizeclass);
      }
      ticker.check_tick();
    }

    template<bool check_slabs = false>
    SNMALLOC_SLOW_PATH void dealloc_local_slabs(smallsizeclass_t sizeclass)
    {
      if constexpr (!check_slabs)
        if (alloc_classes[sizeclass].unused == 0)
          return;

      // Return unused slabs of sizeclass_t back to global allocator
      alloc_classes[sizeclass].available.iterate([this, sizeclass](auto* meta) {
        auto domesticate =
          [this](freelist::QueuePtr p) SNMALLOC_FAST_PATH_LAMBDA {
            auto res = capptr_domesticate<Config>(backend_state_ptr(), p);
#ifdef SNMALLOC_TRACING
            if (res.unsafe_ptr() != p.unsafe_ptr())
              printf(
                "Domesticated %p to %p!\n", p.unsafe_ptr(), res.unsafe_ptr());
#endif
            return res;
          };

        if (meta->needed() != 0)
        {
          if (check_slabs)
          {
            meta->free_queue.validate(
              freelist::Object::key_root, meta->as_key_tweak(), domesticate);
          }
          return;
        }

        alloc_classes[sizeclass].length--;
        alloc_classes[sizeclass].unused--;

        // Remove from the list.  This must be done before dealloc chunk
        // as that may corrupt the node.
        meta->node.remove();

        // TODO delay the clear to the next user of the slab, or teardown so
        // don't touch the cache lines at this point in snmalloc_check_client.
        auto start = clear_slab(meta, sizeclass);

        Config::Backend::dealloc_chunk(
          get_backend_local_state(),
          *meta,
          start,
          sizeclass_to_slab_size(sizeclass),
          sizeclass_t::from_small_class(sizeclass));
      });
    }

    capptr::Alloc<void>
    clear_slab(BackendSlabMetadata* meta, smallsizeclass_t sizeclass)
    {
      auto key_tweak = meta->as_key_tweak();
      freelist::Iter<> fl;
      auto more =
        meta->free_queue.close(fl, freelist::Object::key_root, key_tweak);
      UNUSED(more);
      auto local_state = backend_state_ptr();
      auto domesticate = [local_state](freelist::QueuePtr p)
                           SNMALLOC_FAST_PATH_LAMBDA {
                             return capptr_domesticate<Config>(local_state, p);
                           };
      capptr::Alloc<void> p =
        fl.take(freelist::Object::key_root, domesticate).as_void();
      SNMALLOC_ASSERT(is_start_of_object(
        sizeclass_t::from_small_class(sizeclass), address_cast(p)));

      // If clear_meta is requested, we should also walk the free list to clear
      // it.
      // TODO: we could optimise the clear_meta case to not walk the free list
      // and instead just clear the whole slab, but that requires amplification.
      if constexpr (
        mitigations(freelist_teardown_validate) || mitigations(clear_meta))
      {
        // Check free list is well-formed on platforms with
        // integers as pointers.
        size_t count = 1; // Already taken one above.
        while (!fl.empty())
        {
          fl.take(freelist::Object::key_root, domesticate);
          count++;
        }
        // Check the list contains all the elements
        SNMALLOC_CHECK(
          (count + more) ==
          snmalloc::sizeclass_to_slab_object_count(sizeclass));

        if (more > 0)
        {
          auto no_more =
            meta->free_queue.close(fl, freelist::Object::key_root, key_tweak);
          SNMALLOC_ASSERT(no_more == 0);
          UNUSED(no_more);

          while (!fl.empty())
          {
            fl.take(freelist::Object::key_root, domesticate);
            count++;
          }
        }
        SNMALLOC_CHECK(
          count == snmalloc::sizeclass_to_slab_object_count(sizeclass));
      }
      auto start_of_slab = pointer_align_down<void>(
        p, snmalloc::sizeclass_to_slab_size(sizeclass));

#ifdef SNMALLOC_TRACING
      message<1024>(
        "Slab {} is unused, Object sizeclass {}",
        start_of_slab.unsafe_ptr(),
        sizeclass);
#endif
      return start_of_slab;
    }

    template<typename CheckInit>
    SNMALLOC_FAST_PATH void dealloc_remote(
      const PagemapEntry& entry, capptr::Alloc<void> p_tame) noexcept
    {
      if (SNMALLOC_LIKELY(entry.is_owned()))
      {
        dealloc_cheri_checks(p_tame.unsafe_ptr());

        // Detect double free of large allocations here.
        snmalloc_check_client(
          mitigations(sanity_checks),
          !entry.is_backend_owned(),
          "Memory corruption detected");

        // Check if we have space for the remote deallocation
        if (SNMALLOC_LIKELY(remote_dealloc_cache.reserve_space(entry)))
        {
          remote_dealloc_cache.template dealloc<sizeof(Allocator)>(
            entry.get_slab_metadata(), p_tame, &entropy);
#ifdef SNMALLOC_TRACING
          message<1024>(
            "Remote dealloc fast {} ({}, {})",
            address_cast(p_tame),
            sizeclass_full_to_size(entry.get_sizeclass()),
            address_cast(entry.get_slab_metadata()));
#endif
          return;
        }

        dealloc_remote_slow<CheckInit>(entry, p_tame);
        return;
      }

      if (SNMALLOC_LIKELY(p_tame == nullptr))
      {
#ifdef SNMALLOC_TRACING
        message<1024>("nullptr deallocation");
#endif
        return;
      }

      dealloc_cheri_checks(p_tame.unsafe_ptr());
      Config::SecondaryAllocator::deallocate(p_tame.unsafe_ptr());
    }

    /**
     * Slow path for deallocation we do not have space for this remote
     * deallocation. This could be because,
     *   - we actually don't have space for this remote deallocation,
     *     and need to send them on; or
     *   - the allocator was not already initialised.
     * In the second case we need to recheck if this is a remote deallocation,
     * as we might acquire the originating allocator.
     */
    template<typename CheckInit>
    SNMALLOC_SLOW_PATH void dealloc_remote_slow(
      const PagemapEntry& entry, capptr::Alloc<void> p) noexcept
    {
      CheckInit::check_init(
        [this, &entry, p]() SNMALLOC_FAST_PATH_LAMBDA {
#ifdef SNMALLOC_TRACING
          message<1024>(
            "Remote dealloc post {} ({}, {})",
            p.unsafe_ptr(),
            sizeclass_full_to_size(entry.get_sizeclass()),
            address_cast(entry.get_slab_metadata()));
#endif
          remote_dealloc_cache.template dealloc<sizeof(Allocator)>(
            entry.get_slab_metadata(), p, &entropy);

          post();
        },
        [](Allocator* a, void* p) SNMALLOC_FAST_PATH_LAMBDA {
          // Recheck what kind of dealloc we should do in case the allocator
          // we get from lazy_init is the originating allocator.
          a->dealloc(p); // TODO don't double count statistics
        },
        p.unsafe_ptr());
    }

    /**
     * Flush the cached state and delayed deallocations
     *
     * Returns true if messages are sent to other threads.
     */
    bool flush(bool destroy_queue = false)
    {
      auto local_state = backend_state_ptr();
      auto domesticate = [local_state](freelist::QueuePtr p)
                           SNMALLOC_FAST_PATH_LAMBDA {
                             return capptr_domesticate<Config>(local_state, p);
                           };

      size_t bytes_flushed = 0; // Not currently used.

      if (destroy_queue)
      {
        auto cb =
          [this, domesticate, &bytes_flushed](capptr::Alloc<RemoteMessage> m) {
            bool need_post = true; // Always going to post, so ignore.
            const PagemapEntry& entry =
              Config::Backend::get_metaentry(snmalloc::address_cast(m));
            handle_dealloc_remote(
              entry, m, need_post, domesticate, bytes_flushed);
          };

        message_queue().destroy_and_iterate(domesticate, cb);
      }
      else
      {
        // Process incoming message queue
        // Loop as normally only processes a batch
        while (has_messages())
          handle_message_queue<true>([]() {});
      }

      auto& key = freelist::Object::key_root;

      for (size_t i = 0; i < NUM_SMALL_SIZECLASSES; i++)
      {
        if (small_fast_free_lists[i].empty())
          continue;

        // All elements should have the same entry.
        const auto& entry = Config::Backend::get_metaentry(
          address_cast(small_fast_free_lists[i].peek()));
        do
        {
          auto p = small_fast_free_lists[i].take(key, domesticate);
          SNMALLOC_ASSERT(is_start_of_object(
            sizeclass_t::from_small_class(i), address_cast(p)));
          dealloc_local_object(p.as_void(), entry);
        } while (!small_fast_free_lists[i].empty());
      }

      auto posted = remote_dealloc_cache.template post<sizeof(Allocator)>(
        local_state, get_trunc_id());

      // We may now have unused slabs, return to the global allocator.
      for (smallsizeclass_t sizeclass = 0; sizeclass < NUM_SMALL_SIZECLASSES;
           sizeclass++)
      {
        dealloc_local_slabs<mitigations(freelist_teardown_validate)>(sizeclass);
      }

      if constexpr (mitigations(freelist_teardown_validate))
      {
        laden.iterate(
          [domesticate](BackendSlabMetadata* meta) SNMALLOC_FAST_PATH_LAMBDA {
            if (!meta->is_large())
            {
              meta->free_queue.validate(
                freelist::Object::key_root, meta->as_key_tweak(), domesticate);
            }
          });
      }
      // Set the remote_dealloc_cache to immediately slow path.
      remote_dealloc_cache.capacity = 0;

      return posted;
    }

    /**
     * If result parameter is non-null, then false is assigned into the
     * the location pointed to by result if this allocator is non-empty.
     *
     * If result pointer is null, then this code raises a Pal::error on the
     * particular check that fails, if any do fail.
     *
     * Do not run this while other thread could be deallocating as the
     * message queue invariant is temporarily broken.
     */
    bool debug_is_empty(bool* result)
    {
#ifdef SNMALLOC_TRACING
      message<1024>("debug_is_empty");
#endif

      auto error = [&result](auto slab_metadata) {
        auto slab_interior =
          slab_metadata->get_slab_interior(freelist::Object::key_root);
        const PagemapEntry& entry =
          Config::Backend::get_metaentry(slab_interior);
        SNMALLOC_ASSERT(slab_metadata == entry.get_slab_metadata());
        auto size_class = entry.get_sizeclass();
        auto slab_size = sizeclass_full_to_slab_size(size_class);
        auto slab_start = bits::align_down(slab_interior, slab_size);

        if (result != nullptr)
          *result = false;
        else
          report_fatal_error(
            "debug_is_empty: found non-empty allocator: size={} on "
            "slab_start {} meta {} entry {}",
            sizeclass_full_to_size(size_class),
            slab_start,
            address_cast(slab_metadata),
            address_cast(&entry));
      };

      auto test = [&error](auto& queue) {
        queue.iterate([&error](auto slab_metadata) {
          if (slab_metadata->needed() != 0)
          {
            error(slab_metadata);
          }
        });
      };

      bool sent_something = flush(true);

      for (auto& alloc_class : alloc_classes)
      {
        test(alloc_class.available);
      }

      if (!laden.is_empty())
      {
        error(laden.peek());
      }

#ifdef SNMALLOC_TRACING
      message<1024>("debug_is_empty - done");
#endif
      return sent_something;
    }
  };

  template<typename Config>
  class ConstructAllocator
  {
    using CA = Allocator<Config>;

    /**
     * SFINAE helper.  Matched only if `T` implements `ensure_init`.  Calls it
     * if it exists.
     */
    template<typename T>
    static SNMALLOC_FAST_PATH auto call_ensure_init(T*, int)
      -> decltype(T::ensure_init())
    {
      T::ensure_init();
    }

    /**
     * SFINAE helper.  Matched only if `T` does not implement `ensure_init`.
     * Does nothing if called.
     */
    template<typename T>
    static SNMALLOC_FAST_PATH auto call_ensure_init(T*, long)
    {}

    /**
     * Call `Config::ensure_init()` if it is implemented, do
     * nothing otherwise.
     */
    SNMALLOC_FAST_PATH
    static void ensure_config_init()
    {
      call_ensure_init<Config>(nullptr, 0);
    }

  public:
    static capptr::Alloc<CA> make()
    {
      ensure_config_init();

      size_t size = sizeof(CA);
      size_t round_sizeof = Aal::capptr_size_round(size);
      size_t request_size = bits::next_pow2(round_sizeof);
      size_t spare = request_size - round_sizeof;

      auto raw =
        Config::Backend::template alloc_meta_data<CA>(nullptr, request_size);

      if (raw == nullptr)
      {
        Config::Pal::error("Failed to initialise thread local allocator.");
      }

      capptr::Alloc<void> spare_start = pointer_offset(raw, round_sizeof);
      Range<capptr::bounds::Alloc> r{spare_start, spare};

      auto p = capptr::Alloc<CA>::unsafe_from(
        new (raw.unsafe_ptr(), placement_token) CA(r));

      // Remove excess from the bounds.
      p = Aal::capptr_bound<CA, capptr::bounds::Alloc>(p, round_sizeof);
      return p;
    }
  };

  /**
   * Use this alias to access the pool of allocators throughout snmalloc.
   */
  template<typename Config>
  using AllocPool =
    Pool<Allocator<Config>, ConstructAllocator<Config>, Config::pool>;
} // namespace snmalloc
