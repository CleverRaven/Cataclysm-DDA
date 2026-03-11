#pragma once

#ifndef SNMALLOC_PLATFORM_HAS_GETENTROPY
#  include <random>
#endif

namespace snmalloc
{
  template<typename PAL>
  stl::enable_if_t<pal_supports<Entropy, PAL>, uint64_t> get_entropy64()
  {
    return PAL::get_entropy64();
  }

  template<typename PAL>
  stl::enable_if_t<!pal_supports<Entropy, PAL>, uint64_t> get_entropy64()
  {
#ifdef SNMALLOC_PLATFORM_HAS_GETENTROPY
    return DefaultPal::get_entropy64();
#else
    std::random_device rd;
    uint64_t a = rd();
    return (a << 32) ^ rd();
#endif
  }
} // namespace snmalloc
