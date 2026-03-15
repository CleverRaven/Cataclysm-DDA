#pragma once

#include "../ds_core/ds_core.h"

namespace snmalloc
{
  /**
   * Simple pagemap that for each GRANULARITY_BITS of the address range
   * stores a T.
   */
  template<size_t GRANULARITY_BITS, typename T, typename PAL, bool has_bounds>
  class FlatPagemap
  {
  public:
    static constexpr size_t SHIFT = GRANULARITY_BITS;
    static constexpr size_t GRANULARITY = bits::one_at_bit(GRANULARITY_BITS);

  private:
    /**
     * Before init is called will contain a single entry
     * that is the default value.  This is needed so that
     * various calls do not have to check for nullptr.
     *   free(nullptr)
     * and
     *   malloc_usable_size(nullptr)
     * do not require an allocation to have ocurred before
     * they are called.
     */
    inline static const T default_value{};

    /**
     * The representation of the page map.
     *
     * Initially a single element to ensure nullptr operations
     * work.
     */
    T* body{const_cast<T*>(&default_value)};

    /**
     * The representation of the pagemap, but nullptr if it has not been
     * initialised.  Used to combine init checking and lookup.
     */
    T* body_opt{nullptr};

    /**
     * If `has_bounds` is set, then these should contain the
     * bounds of the heap that is being managed by this pagemap.
     */
    address_t base{0};
    size_t size{0};

  public:
    using EntryType = T;

    /**
     * Ensure this range of pagemap is accessible
     */
    bool register_range(address_t p, size_t length)
    {
      SNMALLOC_ASSERT(is_initialised());

      if constexpr (has_bounds)
      {
        if ((p - base >= size) || (p + length - base > size))
        {
          PAL::error("Internal error: Pagemap registering out of range.");
        }
        p = p - base;
      }

      // Calculate range in pagemap that is associated to this space.
      auto first = &body[p >> SHIFT];
      auto last = &body[(p + length + bits::one_at_bit(SHIFT) - 1) >> SHIFT];

      // Commit OS pages associated to the range.
      auto page_start = pointer_align_down<OS_PAGE_SIZE, char>(first);
      auto page_end = pointer_align_up<OS_PAGE_SIZE, char>(last);
      size_t using_size = pointer_diff(page_start, page_end);
      auto result = PAL::template notify_using<NoZero>(page_start, using_size);
      if (!result)
      {
        // If notify_using fails, fails this call.
        return false;
      }
      if constexpr (pal_supports<CoreDump, PAL>)
      {
        PAL::notify_do_dump(page_start, using_size);
      }

      return true;
    }

    constexpr FlatPagemap() = default;

    /**
     * For pagemaps that cover an entire fixed address space, return the size
     * that they must be.  This allows the caller to allocate the correct
     * amount of memory to be passed to `init`.  This is not available for
     * fixed-range pagemaps, whose size depends on dynamic configuration.
     */
    template<bool has_bounds_ = has_bounds>
    static constexpr stl::enable_if_t<!has_bounds_, size_t> required_size()
    {
      static_assert(
        has_bounds_ == has_bounds, "Don't set SFINAE template parameter!");
      constexpr size_t COVERED_BITS = PAL::address_bits - GRANULARITY_BITS;
      constexpr size_t ENTRIES = bits::one_at_bit(COVERED_BITS);
      return ENTRIES * sizeof(T);
    }

    /**
     * Initialise with pre-allocated memory.
     *
     * This is currently disabled for bounded pagemaps but may be reenabled if
     * `required_size` is enabled for the has-bounds case.
     */
    template<bool has_bounds_ = has_bounds>
    stl::enable_if_t<!has_bounds_> init(T* address)
    {
      SNMALLOC_ASSERT(!is_initialised());

      static_assert(
        has_bounds_ == has_bounds, "Don't set SFINAE template parameter!");
      body = address;
      body_opt = address;
    }

    /**
     * Initialise the pagemap with bounds.
     *
     * Returns usable range after pagemap has been allocated
     */
    template<bool has_bounds_ = has_bounds>
    stl::enable_if_t<has_bounds_, stl::Pair<void*, size_t>>
    init(void* b, size_t s)
    {
      SNMALLOC_ASSERT(!is_initialised());

      static_assert(
        has_bounds_ == has_bounds, "Don't set SFINAE template parameter!");
#ifdef SNMALLOC_TRACING
      message<1024>("Pagemap.init {} ({})", b, s);
#endif
      SNMALLOC_ASSERT(s != 0);
      // TODO take account of pagemap size in the calculation of how big it
      // needs to be.  The following code creates a pagemap that covers the
      // pagemap as well as the left over. This is not ideal, and we should
      // really calculate the division with
      //
      //  GRANULARITY + sizeof(T)
      //
      // There are awkward corner cases for the alignment of the start and
      // the end that are hard to calculate. So this is not currently done.

      // Calculate range in pagemap that is associated to this space.
      // Over calculate to cover any unaligned parts at either end.
      base = bits::align_down(address_cast(b), GRANULARITY);
      auto end = bits::align_up(address_cast(b) + s, GRANULARITY);
      size = end - base;

      // Setup the pagemap.
      body = static_cast<T*>(b);
      body_opt = body;

      // Calculate size of pagemap.
      auto pagemap_size = (size >> SHIFT) * sizeof(T);

      // Advance by size of pagemap.
      // TODO CHERI capability bound here!
      auto heap_base = pointer_offset(b, pagemap_size);

      // The following assert prevents the corner case where the pagemap
      // occupies the entire address space, and this
      //     s - pagemap_size
      // can underflow.
      static_assert(
        sizeof(T) < (1 << SHIFT),
        "Pagemap entry too large relative to granularity");

      if (pagemap_size > s)
      {
        // The pagemap is larger than the available space.
        error("Pagemap is larger than the available space.");
      }

      return {heap_base, s - pagemap_size};
    }

    /**
     * Initialise the pagemap without bounds.
     */
    template<bool randomize_position, bool has_bounds_ = has_bounds>
    stl::enable_if_t<!has_bounds_> init()
    {
      SNMALLOC_ASSERT(!is_initialised());

      static_assert(
        has_bounds_ == has_bounds, "Don't set SFINAE template parameter!");
      static constexpr size_t REQUIRED_SIZE = required_size();

      // Allocate a power of two extra to allow the placement of the
      // pagemap be difficult to guess if randomize_position set.
      size_t additional_size =
#ifdef SNMALLOC_THREAD_SANITIZER_ENABLED
        // When running with TSAN we failed to allocate the very large range
        // randomly
        randomize_position ? bits::next_pow2(REQUIRED_SIZE) : 0;
#else
        randomize_position ? bits::next_pow2(REQUIRED_SIZE) * 4 : 0;
#endif
      size_t request_size = REQUIRED_SIZE + additional_size;

      auto new_body_untyped = PAL::reserve(request_size);

      if constexpr (pal_supports<CoreDump, PAL>)
      {
        // Pagemap should not be in core dump except where it is non-zero.
        PAL::notify_do_not_dump(new_body_untyped, request_size);
      }

      if (new_body_untyped == nullptr)
      {
        PAL::error("Failed to initialise snmalloc.");
      }

      T* new_body;

      if constexpr (randomize_position)
      {
        // Begin pagemap at random offset within the additionally allocated
        // space.
        static_assert(bits::is_pow2(sizeof(T)), "Next line assumes this.");
        size_t offset = get_entropy64<PAL>() & (additional_size - sizeof(T));
        new_body = pointer_offset<T>(new_body_untyped, offset);

        if constexpr (pal_supports<LazyCommit, PAL>)
        {
          void* start_page = pointer_align_down<OS_PAGE_SIZE>(new_body);
          void* end_page = pointer_align_up<OS_PAGE_SIZE>(
            pointer_offset(new_body, REQUIRED_SIZE));
          // Only commit readonly memory for this range, if the platform
          // supports lazy commit.  Otherwise, this would be a lot of memory to
          // have mapped.
          auto result = PAL::notify_using_readonly(
            start_page, pointer_diff(start_page, end_page));
          if (!result)
          {
            PAL::error("Failed to initialise snmalloc.");
          }
        }
      }
      else
      {
        if constexpr (pal_supports<LazyCommit, PAL>)
        {
          auto result =
            PAL::notify_using_readonly(new_body_untyped, REQUIRED_SIZE);
          if (!result)
          {
            PAL::error("Failed to initialise snmalloc.");
          }
        }
        new_body = static_cast<T*>(new_body_untyped);
      }
      // Ensure bottom page is committed
      // ASSUME: new memory is zeroed.
      auto result = PAL::template notify_using<NoZero>(
        pointer_align_down<OS_PAGE_SIZE>(new_body), OS_PAGE_SIZE);
      if (!result)
      {
        PAL::error("Failed to initialise snmalloc.");
      }

      // Set up zero page
      new_body[0] = body[0];

      body = new_body;
      body_opt = new_body;
    }

    template<bool has_bounds_ = has_bounds>
    stl::enable_if_t<has_bounds_, stl::Pair<address_t, size_t>> get_bounds()
    {
      SNMALLOC_ASSERT(is_initialised());

      static_assert(
        has_bounds_ == has_bounds, "Don't set SFINAE template parameter!");

      return {base, size};
    }

    /**
     * Get the number of entries.
     */
    [[nodiscard]] constexpr size_t num_entries() const
    {
      SNMALLOC_ASSERT(is_initialised());

      if constexpr (has_bounds)
      {
        return size >> GRANULARITY_BITS;
      }
      else
      {
        return bits::one_at_bit(PAL::address_bits - GRANULARITY_BITS);
      }
    }

    /**
     *
     * Get a non-constant reference to the slot of this pagemap corresponding
     * to a particular address.
     *
     * If the location has not been used before, then
     * `potentially_out_of_range` should be set to true.  This will ensure
     * there is memory backing the returned reference.
     */
    template<bool potentially_out_of_range>
    T& get_mut(address_t p)
    {
      if constexpr (potentially_out_of_range)
      {
        if (SNMALLOC_UNLIKELY(body_opt == nullptr))
          return const_cast<T&>(default_value);
      }

      SNMALLOC_ASSERT(is_initialised() || p == 0);

      if constexpr (has_bounds)
      {
        if (p - base > size)
        {
          if constexpr (potentially_out_of_range)
          {
            return const_cast<T&>(default_value);
          }
          else
          {
            // Out of range null should
            // still return the default value.
            if (p == 0)
              return const_cast<T&>(default_value);
            PAL::error("Internal error: Pagemap read access out of range.");
          }
        }
        p = p - base;
      }

      //  If this is potentially_out_of_range, then the pages will not have
      //  been mapped. With Lazy commit they will at least be mapped read-only
      //  Note that: this means external pointer on Windows will be slow.
      if constexpr (potentially_out_of_range && !pal_supports<LazyCommit, PAL>)
      {
        register_range(p, 1);
      }

      if constexpr (potentially_out_of_range)
        return body_opt[p >> SHIFT];
      else
        return body[p >> SHIFT];
    }

    /**
     * Get a constant reference to the slot of this pagemap corresponding to a
     * particular address.
     *
     * If the location has not been used before, then
     * `potentially_out_of_range` should be set to true.  This will ensure
     * there is memory backing any reads through the returned reference.
     */
    template<bool potentially_out_of_range>
    const T& get(address_t p)
    {
      return get_mut<potentially_out_of_range>(p);
    }

    /**
     * Check if the pagemap has been initialised.
     */
    [[nodiscard]] bool is_initialised() const
    {
      return body_opt != nullptr;
    }

    /**
     * Return the starting address corresponding to a given entry within the
     * Pagemap. Also checks that the reference actually points to a valid
     * entry.
     */
    [[nodiscard]] address_t get_address(const T& t) const
    {
      SNMALLOC_ASSERT(is_initialised());
      address_t entry_offset = address_cast(&t) - address_cast(body);
      address_t entry_index = entry_offset / sizeof(T);
      SNMALLOC_ASSERT(
        entry_offset % sizeof(T) == 0 && entry_index < num_entries());
      return base + (entry_index << GRANULARITY_BITS);
    }

    void set(address_t p, const T& t)
    {
      SNMALLOC_ASSERT(is_initialised());
#ifdef SNMALLOC_TRACING
      message<1024>("Pagemap.Set {}", p);
#endif
      if constexpr (has_bounds)
      {
        if (p - base > size)
        {
          PAL::error("Internal error: Pagemap write access out of range.");
        }
        p = p - base;
      }

      body[p >> SHIFT] = t;
    }
  };
} // namespace snmalloc
