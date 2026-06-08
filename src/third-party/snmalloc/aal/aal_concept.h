#pragma once

#ifdef __cpp_concepts
#  include "../ds_core/ds_core.h"
#  include "aal_consts.h"

#  include <stdint.h>

namespace snmalloc
{
  /**
   * AALs must advertise the bit vector of supported features, their name,
   * machine word size, and an upper bound on the address space size
   */
  template<typename AAL>
  concept IsAAL_static_members =
    requires() {
      typename stl::integral_constant<uint64_t, AAL::aal_features>;
      typename stl::integral_constant<int, AAL::aal_name>;
      typename stl::integral_constant<size_t, AAL::bits>;
      typename stl::integral_constant<size_t, AAL::address_bits>;
    };

  /**
   * AALs provide a prefetch operation.
   */
  template<typename AAL>
  concept IsAAL_prefetch = requires(void* ptr) {
                             {
                               AAL::prefetch(ptr)
                               } noexcept -> ConceptSame<void>;
                           };

  /**
   * AALs provide a notion of high-precision timing.
   */
  template<typename AAL>
  concept IsAAL_tick = requires() {
                         {
                           AAL::tick()
                           } noexcept -> ConceptSame<uint64_t>;
                       };

  template<typename AAL>
  concept IsAAL_capptr_methods =
    requires(capptr::Chunk<void> auth, capptr::AllocFull<void> ret, size_t sz) {
      /**
       * Produce a pointer with reduced authority from a more privilged pointer.
       * The resulting pointer will have base at auth's address and length of
       * exactly sz.  auth+sz must not exceed auth's limit.
       */
      {
        AAL::template capptr_bound<void, capptr::bounds::Chunk>(auth, sz)
        } noexcept -> ConceptSame<capptr::Chunk<void>>;

      /**
       * "Amplify" by copying the address of one pointer into one of higher
       * privilege.  The resulting pointer differs from auth only in address.
       */
      {
        AAL::capptr_rebound(auth, ret)
        } noexcept -> ConceptSame<capptr::Chunk<void>>;

      /**
       * Round up an allocation size to a size this architecture can represent.
       * While there may also, in general, be alignment requirements for
       * representability, in snmalloc so far we have not had reason to consider
       * these explicitly: when we use our...
       *
       * - sizeclass machinery (for user-facing data), we assume that all
       *   sizeclasses describe architecturally representable aligned-and-sized
       *   regions
       *
       * - Range machinery (for internal meta-data), we always choose NAPOT
       *   regions big enough for the requested size (returning space above the
       *   allocation within such regions for use as smaller NAPOT regions).
       *
       * That is, capptr_size_round is not needed on the user-facing fast paths,
       * merely internally for bootstrap and metadata management.
       */
      {
        AAL::capptr_size_round(sz)
        } noexcept -> ConceptSame<size_t>;
    };

  template<typename AAL>
  concept IsAAL = IsAAL_static_members<AAL> && IsAAL_prefetch<AAL> &&
    IsAAL_tick<AAL> && IsAAL_capptr_methods<AAL>;

} // namespace snmalloc
#endif
