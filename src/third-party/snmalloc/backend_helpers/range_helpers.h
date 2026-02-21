#pragma once

#include "../ds_core/ds_core.h"

namespace snmalloc
{
  template<size_t MIN_BITS, SNMALLOC_CONCEPT(capptr::IsBound) B, typename F>
  void range_to_pow_2_blocks(CapPtr<void, B> base, size_t length, F f)
  {
    auto end = pointer_offset(base, length);
    base = pointer_align_up(base, bits::one_at_bit(MIN_BITS));
    end = pointer_align_down(end, bits::one_at_bit(MIN_BITS));
    length = pointer_diff(base, end);

    bool first = true;

    // Find the minimum set of maximally aligned blocks in this range.
    // Each block's alignment and size are equal.
    while (length >= bits::one_at_bit(MIN_BITS))
    {
      size_t base_align_bits = bits::ctz(address_cast(base));
      size_t length_align_bits = (bits::BITS - 1) - bits::clz(length);
      size_t align_bits = bits::min(base_align_bits, length_align_bits);
      size_t align = bits::one_at_bit(align_bits);

      /*
       * Now that we have found a maximally-aligned block, we can set bounds
       * and be certain that we won't hit representation imprecision.
       */
      f(base, align, first);
      first = false;

      base = pointer_offset(base, align);
      length -= align;
    }
  }

  /**
   * Forward definition to allow multiple template specialisations.
   *
   * This struct is used to recursively compose ranges.
   */
  template<typename... Args>
  struct PipeImpl;

  /**
   * Base case of one range that needs nothing.
   */
  template<typename Only>
  struct PipeImpl<Only>
  {
    using result = Only;
  };

  /**
   * Recursive case of applying a base range as an argument to the
   * next, and then using that as the new base range.
   */
  template<typename First, typename Fun, typename... Rest>
  struct PipeImpl<First, Fun, Rest...>
  {
  public:
    using result =
      typename PipeImpl<typename Fun::template Type<First>, Rest...>::result;
  };

  /**
   * Nice type so the caller doesn't need to call result explicitly.
   */
  template<typename... Args>
  using Pipe = typename PipeImpl<Args...>::result;

  /**
   * Helper class for allowing a range to be navigated to find an
   * ancestor of a specific type. The parent is an instance field.
   */
  template<typename Parent>
  class ContainsParent
  {
  protected:
    Parent parent{};

  public:
    /**
     * Returns the outermost Ancestor with the correct type.
     *
     * Fails to compile if no such ancestor exists.
     */
    template<typename Anc>
    Anc* ancestor()
    {
      if constexpr (stl::is_same_v<Anc, Parent>)
      {
        return &parent;
      }
      else
      {
        return parent.template ancestor<Anc>();
      }
    }
  };

  /**
   * Helper class for allowing a range to be navigated to find an
   * ancestor of a specific type. The parent is a static field.
   */
  template<typename Parent>
  class StaticParent
  {
  protected:
    SNMALLOC_REQUIRE_CONSTINIT inline static Parent parent{};

  public:
    /**
     * Returns the outermost Ancestor with the correct type.
     *
     * Fails to compile if no such ancestor exists.
     */
    template<typename Anc>
    Anc* ancestor()
    {
      if constexpr (stl::is_same_v<Anc, Parent>)
      {
        return &parent;
      }
      else
      {
        return parent.template ancestor<Anc>();
      }
    }
  };

  /**
   * Helper class for allowing a range to be navigated to find an
   * ancestor of a specific type. The parent is a pointer to a range;
   * this allows the parent to be shared.
   */
  template<typename Parent>
  class RefParent
  {
  protected:
    Parent* parent{};

  public:
    /**
     * Returns the outermost Ancestor with the correct type.
     *
     * Fails to compile if no such ancestor exists.
     */
    template<typename Anc>
    Anc* ancestor()
    {
      if constexpr (stl::is_same_v<Anc, Parent>)
      {
        return parent;
      }
      else
      {
        return parent->template ancestor<Anc>();
      }
    }
  };
} // namespace snmalloc
