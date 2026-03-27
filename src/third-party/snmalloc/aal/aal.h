/**
 * The snmalloc architecture abstraction layer.  This defines
 * CPU-architecture-specific functionality.
 *
 * Files in this directory may depend on `ds_core` and each other, but nothing
 * else in snmalloc.
 */
#pragma once
#include "../ds_core/ds_core.h"
#include "aal_concept.h"
#include "aal_consts.h"
#include "snmalloc/stl/utility.h"

#include <stdint.h>

#if ( \
  defined(__i386__) || defined(_M_IX86) || defined(_X86_) || \
  defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || \
  defined(_M_AMD64)) && \
  !defined(_M_ARM64EC)
#  if defined(SNMALLOC_SGX)
#    define PLATFORM_IS_X86_SGX
#    define SNMALLOC_NO_AAL_BUILTINS
#  else
#    define PLATFORM_IS_X86
#  endif
#endif

#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64) || \
  defined(_M_ARM64EC)
#  define PLATFORM_IS_ARM
#endif

#if defined(__powerpc__) || defined(__powerpc64__)
#  define PLATFORM_IS_POWERPC
#endif

#if defined(__sparc__)
#  define PLATFORM_IS_SPARC
#endif

#if defined(__riscv)
#  define PLATFORM_IS_RISCV
#endif

namespace snmalloc
{
  /**
   * Architecture Abstraction Layer. Includes default implementations of some
   * functions using compiler builtins.  Falls back to the definitions in the
   * platform's AAL if the builtin does not exist.
   */
  template<class Arch>
  struct AAL_Generic : Arch
  {
    /*
     * Provide a default specification of address_t as uintptr_t for Arch-es
     * that support IntegerPointers.  Those Arch-es without IntegerPointers
     * must explicitly give their address_t.
     *
     * This somewhat obtuse way of spelling the defaulting is necessary so
     * that all arguments to stl::conditional_t are valid, even if they
     * wouldn't be valid in context.  One might rather wish to say
     *
     *   stl::conditional_t<..., uintptr_t, Arch::address_t>
     *
     * but that requires that Arch::address_t always be given, precisely
     * the thing we're trying to avoid with the conditional.
     */

    struct default_address_t
    {
      using address_t = uintptr_t;
    };

    using address_t = typename stl::conditional_t<
      (Arch::aal_features & IntegerPointers) != 0,
      default_address_t,
      Arch>::address_t;

  private:
    /**
     * SFINAE template and default case.  T will be Arch and the second template
     * argument defaults at the call site (below).
     */
    template<typename T, typename = int>
    struct default_bits_t
    {
      static constexpr size_t value = sizeof(size_t) * 8;
    };

    /**
     * SFINAE override case.  T will be Arch, and the computation in the second
     * position yields the type int iff T::bits exists and is a substituion
     * failure otherwise.  That is, if T::bits exists, this specialization
     * shadows the default; otherwise, this specialization has no effect.
     */
    template<typename T>
    struct default_bits_t<T, decltype(((void)T::bits, 0))>
    {
      static constexpr size_t value = T::bits;
    };

  public:
    /**
     * Architectural word width as overridden by the underlying Arch-itecture or
     * defaulted as per above.
     */
    static constexpr size_t bits = default_bits_t<Arch>::value;

  private:
    /**
     * Architectures have a default opinion of their address space size, but
     * this is mediated by the platform (e.g., the kernel may cleave the address
     * space in twain or my use only some of the radix points available to
     * hardware paging mechanisms).
     *
     * This is more SFINAE-based type-level trickery; see default_bits_t, above,
     * for more details.
     */
    template<typename T, typename = int>
    struct default_address_bits_t
    {
      static constexpr size_t value = (bits == 64) ? 48 : 32;
    };

    /**
     * Yet more SFINAE; see default_bits_t for more discussion.  Here, the
     * computation in the second parameter yields the type int iff
     * T::address_bits exists.
     */
    template<typename T>
    struct default_address_bits_t<T, decltype(((void)T::address_bits, 0))>
    {
      static constexpr size_t value = T::address_bits;
    };

  public:
    static constexpr size_t address_bits = default_address_bits_t<Arch>::value;

    /**
     * Prefetch a specific address.
     *
     * If the compiler provides a portable prefetch builtin, use it directly,
     * otherwise delegate to the architecture-specific layer.  This allows new
     * architectures to avoid needing to implement a custom `prefetch` method
     * if they are used only with a compiler that provides the builtin.
     */
    static inline void prefetch(void* ptr) noexcept
    {
#if __has_builtin(__builtin_prefetch) && !defined(SNMALLOC_NO_AAL_BUILTINS)
      __builtin_prefetch(ptr, 1, 3);
#else
      Arch::prefetch(ptr);
#endif
    }

    /**
     * Return an architecture-specific cycle counter.
     *
     * If the architecture reports that CPU cycle counters are unavailable,
     * use any architecture-specific implementation that exists, otherwise
     * fall back to zero. When counters are available, prefer a compiler
     * builtin and then the architecture-specific implementation.
     */
    static inline uint64_t tick() noexcept
    {
      if constexpr (
        (Arch::aal_features & NoCpuCycleCounters) == NoCpuCycleCounters)
      {
        return 0;
      }
      else
      {
#if __has_builtin(__builtin_readcyclecounter) && !defined(__APPLE__) && \
  !defined(SNMALLOC_NO_AAL_BUILTINS)
        return __builtin_readcyclecounter();
#else
        return Arch::tick();
#endif
      }
    }
  };

  template<class Arch>
  class AAL_NoStrictProvenance : public Arch
  {
    static_assert(
      (Arch::aal_features & StrictProvenance) == 0,
      "AAL_NoStrictProvenance requires what it says on the tin");

  public:
    /**
     * For architectures which do not enforce StrictProvenance, we can just
     * perform an underhanded bit of type-casting.
     */
    template<
      typename T,
      SNMALLOC_CONCEPT(capptr::IsBound) BOut,
      SNMALLOC_CONCEPT(capptr::IsBound) BIn,
      typename U = T>
    static SNMALLOC_FAST_PATH CapPtr<T, BOut>
    capptr_bound(CapPtr<U, BIn> a, size_t size) noexcept
    {
      static_assert(
        capptr::is_spatial_refinement<BIn, BOut>(),
        "capptr_bound must preserve non-spatial CapPtr dimensions");

      UNUSED(size);
      return CapPtr<T, BOut>::unsafe_from(
        a.template as_static<T>().unsafe_ptr());
    }

    template<
      typename T,
      SNMALLOC_CONCEPT(capptr::IsBound) BOut,
      SNMALLOC_CONCEPT(capptr::IsBound) BIn,
      typename U = T>
    static SNMALLOC_FAST_PATH CapPtr<T, BOut>
    capptr_rebound(CapPtr<T, BOut> a, CapPtr<U, BIn> b) noexcept
    {
      UNUSED(a);
      return CapPtr<T, BOut>::unsafe_from(
        b.template as_static<T>().unsafe_ptr());
    }

    static SNMALLOC_FAST_PATH size_t capptr_size_round(size_t sz) noexcept
    {
      return sz;
    }
  };
} // namespace snmalloc

#if defined(PLATFORM_IS_X86)
#  include "aal_x86.h"
#elif defined(PLATFORM_IS_X86_SGX)
#  include "aal_x86_sgx.h"
#elif defined(PLATFORM_IS_ARM)
#  include "aal_arm.h"
#elif defined(PLATFORM_IS_POWERPC)
#  include "aal_powerpc.h"
#elif defined(PLATFORM_IS_SPARC)
#  include "aal_sparc.h"
#elif defined(PLATFORM_IS_RISCV)
#  include "aal_riscv.h"
#endif

#if defined(__CHERI_PURE_CAPABILITY__)
#  include "aal_cheri.h"
#endif

namespace snmalloc
{
#if defined(__CHERI_PURE_CAPABILITY__)
  using Aal = AAL_Generic<AAL_CHERI<AAL_Arch>>;
#else
  using Aal = AAL_Generic<AAL_NoStrictProvenance<AAL_Arch>>;
#endif

  template<AalFeatures F, SNMALLOC_CONCEPT(IsAAL) AAL = Aal>
  constexpr bool aal_supports = (AAL::aal_features & F) == F;

  /*
   * The backend's leading-order response to StrictProvenance is entirely
   * within its data structures and not actually anything to do with the
   * architecture.  Rather than test aal_supports<StrictProvenance> or
   * defined(__CHERI_PURE_CAPABILITY__) or such therein, using this
   * backend_strict_provenance flag makes it easy to test a lot of machinery
   * on non-StrictProvenance architectures.
   */
  static constexpr bool backend_strict_provenance =
    aal_supports<StrictProvenance>;
} // namespace snmalloc

#ifdef __POINTER_WIDTH__
#  if ((__POINTER_WIDTH__ == 64) && !defined(SNMALLOC_VA_BITS_64)) || \
    ((__POINTER_WIDTH__ == 32) && !defined(SNMALLOC_VA_BITS_32))
#    error Compiler and PAL define inconsistent bit widths
#  endif
#endif

#if defined(SNMALLOC_VA_BITS_32) && defined(SNMALLOC_VA_BITS_64)
#  error Only one of SNMALLOC_VA_BITS_64 and SNMALLOC_VA_BITS_32 may be defined!
#endif

#ifdef SNMALLOC_VA_BITS_32
static_assert(sizeof(size_t) == 4);
#elif defined(SNMALLOC_VA_BITS_64)
static_assert(sizeof(size_t) == 8);
#endif

// Included after the AAL has been defined, depends on the AAL's notion of an
// address
#include "address.h"
