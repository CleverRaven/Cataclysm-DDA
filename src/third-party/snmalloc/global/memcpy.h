#pragma once
#include "bounds_checks.h"

namespace snmalloc
{
  /**
   * Copy a single element of a specified size.  Uses a compiler builtin that
   * expands to a single load and store.
   */
  template<size_t Size>
  SNMALLOC_FAST_PATH_INLINE void copy_one(void* dst, const void* src)
  {
#if __has_builtin(__builtin_memcpy_inline)
    __builtin_memcpy_inline(dst, src, Size);
#else
    // Define a structure of size `Size` that has alignment 1 and a default
    // copy-assignment operator.  We can then copy the data as this type.  The
    // compiler knows the exact width and so will generate the correct wide
    // instruction for us (clang 10 and gcc 12 both generate movups for the
    // 16-byte version of this when targeting SSE.
    struct Block
    {
      char data[Size];
    };

    auto* d = static_cast<Block*>(dst);
    auto* s = static_cast<const Block*>(src);
    *d = *s;
#endif
  }

  /**
   * Copy a block using the specified size.  This copies as many complete
   * chunks of size `Size` as are possible from `len`.
   */
  template<size_t Size, size_t PrefetchOffset = 0>
  SNMALLOC_FAST_PATH_INLINE void
  block_copy(void* dst, const void* src, size_t len)
  {
    for (size_t i = 0; (i + Size) <= len; i += Size)
    {
      copy_one<Size>(pointer_offset(dst, i), pointer_offset(src, i));
    }
  }

  /**
   * Perform an overlapping copy of the end.  This will copy one (potentially
   * unaligned) `T` from the end of the source to the end of the destination.
   * This may overlap other bits of the copy.
   */
  template<size_t Size>
  SNMALLOC_FAST_PATH_INLINE void
  copy_end(void* dst, const void* src, size_t len)
  {
    copy_one<Size>(
      pointer_offset(dst, len - Size), pointer_offset(src, len - Size));
  }

  /**
   * Predicate indicating whether the source and destination are sufficiently
   * aligned to be copied as aligned chunks of `Size` bytes.
   */
  template<size_t Size>
  SNMALLOC_FAST_PATH_INLINE bool is_aligned_memcpy(void* dst, const void* src)
  {
    return (pointer_align_down<Size>(const_cast<void*>(src)) == src) &&
      (pointer_align_down<Size>(dst) == dst);
  }

  /**
   * Copy a small size (`Size` bytes) as a sequence of power-of-two-sized loads
   * and stores of decreasing size.  `Word` is the largest size to attempt for a
   * single copy.
   */
  template<size_t Size, size_t Word>
  SNMALLOC_FAST_PATH_INLINE void small_copy(void* dst, const void* src)
  {
    static_assert(bits::is_pow2(Word), "Word size must be a power of two!");
    if constexpr (Size != 0)
    {
      if constexpr (Size >= Word)
      {
        copy_one<Word>(dst, src);
        small_copy<Size - Word, Word>(
          pointer_offset(dst, Word), pointer_offset(src, Word));
      }
      else
      {
        small_copy<Size, Word / 2>(dst, src);
      }
    }
    else
    {
      UNUSED(src);
      UNUSED(dst);
    }
  }

  /**
   * Generate small copies for all sizes up to `Size`, using `WordSize` as the
   * largest size to copy in a single operation.
   */
  template<size_t Size, size_t WordSize = Size>
  SNMALLOC_FAST_PATH_INLINE void
  small_copies(void* dst, const void* src, size_t len)
  {
    if (len == Size)
    {
      small_copy<Size, WordSize>(dst, src);
    }
    if constexpr (Size > 0)
    {
      small_copies<Size - 1, WordSize>(dst, src, len);
    }
  }

  /**
   * If the source and destination are the same displacement away from being
   * aligned on a `BlockSize` boundary, do a small copy to ensure alignment and
   * update `src`, `dst`, and `len` to reflect the remainder that needs
   * copying.
   *
   * Note that this, like memcpy, requires that the source and destination do
   * not overlap.  It unconditionally copies `BlockSize` bytes, so a subsequent
   * copy may not do the right thing.
   */
  template<size_t BlockSize, size_t WordSize>
  SNMALLOC_FAST_PATH_INLINE void
  unaligned_start(void*& dst, const void*& src, size_t& len)
  {
    constexpr size_t block_mask = BlockSize - 1;
    size_t src_addr = static_cast<size_t>(reinterpret_cast<uintptr_t>(src));
    size_t dst_addr = static_cast<size_t>(reinterpret_cast<uintptr_t>(dst));
    size_t src_offset = src_addr & block_mask;
    if ((src_offset > 0) && (src_offset == (dst_addr & block_mask)))
    {
      size_t disp = BlockSize - src_offset;
      small_copies<BlockSize, WordSize>(dst, src, disp);
      src = pointer_offset(src, disp);
      dst = pointer_offset(dst, disp);
      len -= disp;
    }
  }

  /**
   * Default architecture definition.  Provides sane defaults.
   */
  struct GenericArch
  {
    /**
     * The largest register size that we can use for loads and stores.  These
     * types are expected to work for overlapping copies: we can always load
     * them into a register and store them.  Note that this is at the C abstract
     * machine level: the compiler may spill temporaries to the stack, just not
     * to the source or destination object.
     */
    SNMALLOC_UNUSED_FUNCTION
    static constexpr size_t LargestRegisterSize =
      bits::max(sizeof(uint64_t), sizeof(void*));

    /**
     * Hook for architecture-specific optimisations.
     */
    static void* copy(void* dst, const void* src, size_t len)
    {
      auto orig_dst = dst;
      // If this is a small size, use a jump table for small sizes.
      if (len <= LargestRegisterSize)
      {
        small_copies<LargestRegisterSize>(dst, src, len);
      }
      // Otherwise do a simple bulk copy loop.
      else
      {
        block_copy<LargestRegisterSize>(dst, src, len);
        copy_end<LargestRegisterSize>(dst, src, len);
      }
      return orig_dst;
    }
  };

  /**
   * StrictProvenance architectures are prickly about their pointers.  In
   * particular, they may not permit misaligned loads and stores of
   * pointer-sized data, even if they can have non-pointers in their
   * pointer registers.  On the other hand, pointers might be hiding anywhere
   * they are architecturally permitted!
   */
  struct GenericStrictProvenance
  {
    static_assert(bits::is_pow2(sizeof(void*)));
    /*
     * It's not entirely clear what we would do if this were not the case.
     * Best not think too hard about it now.
     */
    static_assert(
      alignof(void*) == sizeof(void*)); // NOLINT(misc-redundant-expression)

    static constexpr size_t LargestRegisterSize = 16;

    static void* copy(void* dst, const void* src, size_t len)
    {
      auto orig_dst = dst;
      /*
       * As a function of misalignment relative to pointers, how big do we need
       * to be such that the span could contain an aligned pointer?  We'd need
       * to be big enough to contain the pointer and would need an additional
       * however many bytes it would take to get us up to alignment.  That is,
       * (sizeof(void*) - src_misalign) except in the case that src_misalign is
       * 0, when the answer is 0, which we can get with some bit-twiddling.
       *
       * Below that threshold, just use a jump table to move bytes around.
       */
      if (
        len < sizeof(void*) +
          (static_cast<size_t>(-static_cast<ptrdiff_t>(address_cast(src))) &
           (alignof(void*) - 1)))
      {
        small_copies<2 * sizeof(void*) - 1, LargestRegisterSize>(dst, src, len);
      }
      /*
       * Equally-misaligned segments could be holding pointers internally,
       * assuming they're sufficiently large.  In this case, perform unaligned
       * operations at the top and bottom of the range.  This check also
       * suffices to include the case where both segments are
       * alignof(void*)-aligned.
       */
      else if (
        address_misalignment<alignof(void*)>(address_cast(src)) ==
        address_misalignment<alignof(void*)>(address_cast(dst)))
      {
        /*
         * Find the buffers' ends.  Do this before the unaligned_start so that
         * there are fewer dependencies in the instruction stream; it would be
         * functionally equivalent to do so below.
         */
        auto dep = pointer_offset(dst, len);
        auto sep = pointer_offset(src, len);

        /*
         * Come up to alignof(void*)-alignment using a jump table.  This
         * operation will move no pointers, since it serves to get us up to
         * alignof(void*).  Recall that unaligned_start takes its arguments by
         * reference, so they will be aligned hereafter.
         */
        unaligned_start<alignof(void*), sizeof(long)>(dst, src, len);

        /*
         * Move aligned pointer *pairs* for as long as we can (possibly none).
         * This generates load-pair/store-pair operations where we have them,
         * and should be benign where we don't, looking like just a bit of loop
         * unrolling with two loads and stores.
         */
        {
          struct Ptr2
          {
            void* p[2];
          };

          if (sizeof(Ptr2) <= len)
          {
            auto dp = static_cast<Ptr2*>(dst);
            auto sp = static_cast<const Ptr2*>(src);
            for (size_t i = 0; i <= len - sizeof(Ptr2); i += sizeof(Ptr2))
            {
              *dp++ = *sp++;
            }
          }
        }

        /*
         * After that copy loop, there can be at most one pointer-aligned and
         * -sized region left.  If there is one, copy it.
         */
        len = len & (2 * sizeof(void*) - 1);
        if (sizeof(void*) <= len)
        {
          ptrdiff_t o = -static_cast<ptrdiff_t>(sizeof(void*));
          auto dp =
            pointer_align_down<alignof(void*)>(pointer_offset_signed(dep, o));
          auto sp =
            pointer_align_down<alignof(void*)>(pointer_offset_signed(sep, o));
          *static_cast<void**>(dp) = *static_cast<void* const*>(sp);
        }

        /*
         * There are up to sizeof(void*)-1 bytes left at the end, aligned at
         * alignof(void*).  Figure out where and how many...
         */
        len = len & (sizeof(void*) - 1);
        dst = pointer_align_down<alignof(void*)>(dep);
        src = pointer_align_down<alignof(void*)>(sep);
        /*
         * ... and use a jump table at the end, too.  If we did the copy_end
         * overlapping store backwards trick, we'd risk damaging the capability
         * in the cell behind us.
         */
        small_copies<sizeof(void*), sizeof(long)>(dst, src, len);
      }
      /*
       * Otherwise, we cannot use pointer-width operations because one of
       * the load or store is going to be misaligned and so will trap.
       * So, same dance, but with integer registers only.
       */
      else
      {
        block_copy<LargestRegisterSize>(dst, src, len);
        copy_end<LargestRegisterSize>(dst, src, len);
      }
      return orig_dst;
    }
  };

#if defined(__x86_64__) || defined(_M_X64)
  /**
   * x86-64 architecture.  Prefers SSE registers for small and medium copies
   * and uses `rep movsb` for large ones.
   */
  struct X86_64Arch
  {
    /**
     * The largest register size that we can use for loads and stores.  These
     * types are expected to work for overlapping copies: we can always load
     * them into a register and store them.  Note that this is at the C abstract
     * machine level: the compiler may spill temporaries to the stack, just not
     * to the source or destination object.
     *
     * We set this to 16 unconditionally for now because using AVX registers
     * imposes stronger alignment requirements that seem to not be a net win.
     */
    static constexpr size_t LargestRegisterSize = 16;

    /**
     * Platform-specific copy hook.  For large copies, use `rep movsb`.
     */
    static inline void* copy(void* dst, const void* src, size_t len)
    {
      auto orig_dst = dst;
      // If this is a small size, use a jump table for small sizes, like on the
      // generic architecture case above.
      if (len <= LargestRegisterSize)
      {
        small_copies<LargestRegisterSize>(dst, src, len);
      }

      // The Intel optimisation manual recommends doing this for sizes >256
      // bytes on modern systems and for all sizes on very modern systems.
      // Testing shows that this is somewhat overly optimistic.
      else if (SNMALLOC_UNLIKELY(len >= 512))
      {
        // Align to cache-line boundaries if possible.
        unaligned_start<64, LargestRegisterSize>(dst, src, len);
        // Bulk copy.  This is aggressively optimised on modern x86 cores.
#  ifdef __GNUC__
        asm volatile("rep movsb"
                     : "+S"(src), "+D"(dst), "+c"(len)
                     :
                     : "memory");
#  elif defined(_MSC_VER)
        __movsb(
          static_cast<unsigned char*>(dst),
          static_cast<const unsigned char*>(src),
          len);
#  else
#    error No inline assembly or rep movsb intrinsic for this compiler.
#  endif
      }

      // Otherwise do a simple bulk copy loop.
      else
      {
        block_copy<LargestRegisterSize>(dst, src, len);
        copy_end<LargestRegisterSize>(dst, src, len);
      }
      return orig_dst;
    }
  };
#endif

#if defined(__powerpc64__)
  struct PPC64Arch
  {
    /**
     * Modern POWER machines have vector registers
     */
    static constexpr size_t LargestRegisterSize = 16;

    /**
     * For large copies (128 bytes or above), use a copy loop that moves up to
     * 128 bytes at once with pre-loop alignment up to 64 bytes.
     */
    static void* copy(void* dst, const void* src, size_t len)
    {
      auto orig_dst = dst;
      if (len < LargestRegisterSize)
      {
        block_copy<1>(dst, src, len);
      }
      else if (SNMALLOC_UNLIKELY(len >= 128))
      {
        // Eight vector operations per loop
        static constexpr size_t block_size = 128;

        // Cache-line align first
        unaligned_start<64, LargestRegisterSize>(dst, src, len);
        block_copy<block_size>(dst, src, len);
        copy_end<block_size>(dst, src, len);
      }
      else
      {
        block_copy<LargestRegisterSize>(dst, src, len);
        copy_end<LargestRegisterSize>(dst, src, len);
      }
      return orig_dst;
    }
  };
#endif

  using DefaultArch =
#ifdef __x86_64__
    X86_64Arch
#elif defined(__powerpc64__)
    PPC64Arch
#else
    stl::conditional_t<
      aal_supports<StrictProvenance>,
      GenericStrictProvenance,
      GenericArch>
#endif
    ;

  /**
   * Snmalloc checked memcpy.  The `Arch` parameter must provide:
   *
   *  - A `size_t` value `LargestRegisterSize`, describing the largest size to
   *    use for single copies.
   *  - A `copy` function that takes (optionally, references to) the arguments
   *    of `memcpy` and returns `true` if it performs a copy, `false`
   *    otherwise.  This can be used to special-case some or all sizes for a
   *    particular architecture.
   */
  template<
    bool Checked,
    bool ReadsChecked = CheckReads,
    typename Arch = DefaultArch>
  SNMALLOC_FAST_PATH_INLINE void* memcpy(void* dst, const void* src, size_t len)
  {
    return check_bound<(Checked && ReadsChecked)>(
      src, len, "memcpy with source out of bounds of heap allocation", [&]() {
        return check_bound<Checked>(
          dst,
          len,
          "memcpy with destination out of bounds of heap allocation",
          [&]() { return Arch::copy(dst, src, len); });
      });
  }
} // namespace snmalloc
