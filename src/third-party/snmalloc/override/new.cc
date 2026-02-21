#include "snmalloc/snmalloc.h"

#include <new>

#ifdef _WIN32
#  ifdef __clang__
#    define EXCEPTSPEC noexcept
#  else
#    define EXCEPTSPEC
#  endif
#else
#  ifdef _GLIBCXX_USE_NOEXCEPT
#    define EXCEPTSPEC _GLIBCXX_USE_NOEXCEPT
#  elif defined(_NOEXCEPT)
#    define EXCEPTSPEC _NOEXCEPT
#  else
#    define EXCEPTSPEC
#  endif
#endif

// only define these if we are not using the vendored STL
#ifndef SNMALLOC_USE_SELF_VENDORED_STL

namespace snmalloc
{
  namespace handler
  {
    class Base
    {
    public:
      static void*
      success(void* p, size_t size, bool secondary_allocator = false)
      {
        UNUSED(secondary_allocator, size);
        SNMALLOC_ASSERT(p != nullptr);

        SNMALLOC_ASSERT(
          secondary_allocator ||
          is_start_of_object(size_to_sizeclass_full(size), address_cast(p)));

        return p;
      }
    };

    class Throw : public Base
    {
    public:
      static void* failure(size_t size)
      {
        // Throw std::bad_alloc on failure.
        auto new_handler = std::get_new_handler();
        if (new_handler != nullptr)
        {
          // Call the new handler, which may throw an exception.
          new_handler();
          // Retry now new_handler has been called.
          // I dislike the unbounded retrying here, but that seems to be what
          // other implementations do.
          return alloc<Throw>(size);
        }

        // Throw std::bad_alloc on failure.
        throw std::bad_alloc();
      }
    };

    class NoThrow : public Base
    {
    public:
      static void* failure(size_t size) noexcept
      {
        auto new_handler = std::get_new_handler();
        if (new_handler != nullptr)
        {
          try
          {
            // Call the new handler, which may throw an exception.
            new_handler();
          }
          catch (...)
          {
            // If the new handler throws, we just return nullptr.
            return nullptr;
          }
          // Retry now new_handler has been called.
          return alloc<NoThrow>(size);
        }
        // If we are here, then the allocation failed.
        // Set errno to ENOMEM, as per the C standard.
        errno = ENOMEM;

        // Return nullptr on failure.
        return nullptr;
      }
    };
  }
} // namespace snmalloc

void* operator new(size_t size)
{
  return snmalloc::alloc<snmalloc::handler::Throw>(size);
}

void* operator new[](size_t size)
{
  return snmalloc::alloc<snmalloc::handler::Throw>(size);
}

void* operator new(size_t size, const std::nothrow_t&) noexcept
{
  return snmalloc::alloc<snmalloc::handler::NoThrow>(size);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept
{
  return snmalloc::alloc<snmalloc::handler::NoThrow>(size);
}

void operator delete(void* p) EXCEPTSPEC
{
  snmalloc::libc::free(p);
}

void operator delete(void* p, size_t size) EXCEPTSPEC
{
  snmalloc::libc::free_sized(p, size);
}

void operator delete(void* p, const std::nothrow_t&) noexcept
{
  snmalloc::libc::free(p);
}

void operator delete[](void* p) EXCEPTSPEC
{
  snmalloc::libc::free(p);
}

void operator delete[](void* p, size_t size) EXCEPTSPEC
{
  snmalloc::libc::free_sized(p, size);
}

void operator delete[](void* p, const std::nothrow_t&) noexcept
{
  snmalloc::libc::free(p);
}

void* operator new(size_t size, std::align_val_t val)
{
  size = snmalloc::aligned_size(size_t(val), size);
  return snmalloc::alloc<snmalloc::handler::Throw>(size);
}

void* operator new[](size_t size, std::align_val_t val)
{
  size = snmalloc::aligned_size(size_t(val), size);
  return snmalloc::alloc<snmalloc::handler::Throw>(size);
}

void* operator new(
  size_t size, std::align_val_t val, const std::nothrow_t&) noexcept
{
  size = snmalloc::aligned_size(size_t(val), size);
  return snmalloc::alloc<snmalloc::handler::NoThrow>(size);
}

void* operator new[](
  size_t size, std::align_val_t val, const std::nothrow_t&) noexcept
{
  size = snmalloc::aligned_size(size_t(val), size);
  return snmalloc::alloc<snmalloc::handler::NoThrow>(size);
}

void operator delete(void* p, std::align_val_t) EXCEPTSPEC
{
  snmalloc::libc::free(p);
}

void operator delete[](void* p, std::align_val_t) EXCEPTSPEC
{
  snmalloc::libc::free(p);
}

void operator delete(void* p, size_t size, std::align_val_t val) EXCEPTSPEC
{
  size = snmalloc::aligned_size(size_t(val), size);
  snmalloc::libc::free_sized(p, size);
}

void operator delete[](void* p, size_t size, std::align_val_t val) EXCEPTSPEC
{
  size = snmalloc::aligned_size(size_t(val), size);
  snmalloc::libc::free_sized(p, size);
}
#endif
