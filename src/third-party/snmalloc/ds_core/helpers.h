#pragma once

#include "bits.h"
#include "snmalloc/ds_core/defines.h"
#include "snmalloc/stl/array.h"
#include "snmalloc/stl/type_traits.h"
#include "snmalloc/stl/utility.h"

#include <stddef.h>

namespace snmalloc
{
  /**
   * Wrapper for wrapping values.
   *
   * Wraps on read. This allows code to trust the value is in range, even when
   * there is a memory corruption.
   */
  template<size_t length, typename T>
  class Mod
  {
    static_assert(bits::is_pow2(length), "Must be a power of two.");

  private:
    T value = 0;

  public:
    operator T()
    {
      return static_cast<T>(value & (length - 1));
    }

    Mod& operator=(const T v)
    {
      value = v;
      return *this;
    }
  };

#ifdef SNMALLOC_CHECK_CLIENT // TODO is this used/helpful?
  template<size_t length, typename T>
  class ModArray
  {
    /**
     * Align the elements, so that access is cheaper.
     */
    struct alignas(bits::next_pow2_const(sizeof(T))) TWrap
    {
      T v;
    };

    static constexpr size_t rlength = bits::next_pow2_const(length);
    stl::Array<TWrap, rlength> array;

  public:
    constexpr const T& operator[](const size_t i) const
    {
      return array[i & (rlength - 1)].v;
    }

    constexpr T& operator[](const size_t i)
    {
      return array[i & (rlength - 1)].v;
    }
  };
#else
  template<size_t length, typename T>
  using ModArray = stl::Array<T, length>;
#endif

  /**
   * Helper class to execute a specified function on destruction.
   */
  template<typename F>
  class OnDestruct
  {
    F f;

  public:
    OnDestruct(F f) : f(f) {}

    ~OnDestruct()
    {
      f();
    }
  };

  /**
   * Helper class for building fatal errors.  Used by `report_fatal_error` to
   * build an on-stack buffer containing the formatted string.
   */
  template<size_t BufferSize>
  class MessageBuilder
  {
    /**
     * The buffer that is used to store the formatted output.
     */
    stl::Array<char, BufferSize> buffer;

    /**
     * Space in the buffer, excluding a trailing null terminator.
     */
    static constexpr size_t SafeLength = BufferSize - 1;

    /**
     * The insert position within `buffer`.
     */
    size_t insert = 0;

    /**
     * Append a single character into the buffer.  This is the single primitive
     * operation permitted on the buffer and performs bounds checks to ensure
     * that there is space for the character and for a null terminator.
     */
    void append_char(char c)
    {
      if (insert < SafeLength)
      {
        buffer[insert++] = c;
      }
    }

    /**
     * Append a string to the buffer.
     */
    template<size_t N>
    void append(const char (&s)[N])
    {
      for (size_t i = 0; i < N - 1; i++)
      {
        append_char(s[i]);
      }
    }

    /*
     * TODO: This is not quite the right thing we want to check, but it
     * suffices on all currently-supported platforms and CHERI.  We'd rather
     * compare UINTPTR_WIDTH and ULLONG_WIDTH, I think, but those don't
     * exist until C's FP Ext 1 TS (merged into C2x).
     */
#ifdef __CHERI_PURE_CAPABILITY__
    /**
     * Append an intptr_t to the buffer as a hex string
     */
    void append(intptr_t v)
    {
      append(reinterpret_cast<void*>(v));
    }

    /**
     * Append a uintptr_t to the buffer as a hex string
     */
    void append(uintptr_t v)
    {
      append(reinterpret_cast<void*>(v));
    }
#endif

    /**
     * Append a nullptr
     */
    void append(decltype(nullptr))
    {
      append("(nullptr)");
    }

    /**
     * Append a raw pointer to the buffer as a hex string.
     */
    void append(void* ptr)
    {
      if (ptr == nullptr)
      {
        append(nullptr);
        return;
      }
      append(static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(ptr)));
      // TODO: CHERI bits.
    }

    /**
     * Append a literal pointer.
     */
    void append(const char* ptr)
    {
      if (ptr == nullptr)
      {
        append(nullptr);
        return;
      }

      while (char data = *ptr++)
      {
        append_char(data);
      }
    }

    /**
     * Append a signed integer to the buffer, as a decimal string.
     */
    void append(long long s)
    {
      if (s < 0)
      {
        append_char('-');
        s = 0 - s;
      }
      stl::Array<char, 20> buf{{0}};
      const char digits[] = "0123456789";
      for (long i = static_cast<long>(buf.size() - 1); i >= 0; i--)
      {
        buf[static_cast<size_t>(i)] = digits[s % 10];
        s /= 10;
      }
      bool skipZero = true;
      for (auto c : buf)
      {
        if (skipZero && (c == '0'))
        {
          continue;
        }
        skipZero = false;
        append_char(c);
      }
      if (skipZero)
      {
        append_char('0');
      }
    }

    /**
     * Append a size to the buffer, as a hex string.
     */
    void append(unsigned long long s)
    {
      append_char('0');
      append_char('x');
      stl::Array<char, 16> buf{{0}};
      const char hexdigits[] = "0123456789abcdef";
      // Length of string including null terminator
      static_assert(sizeof(hexdigits) == 0x11);
      for (long i = static_cast<long>(buf.size() - 1); i >= 0; i--)
      {
        buf[static_cast<size_t>(i)] = hexdigits[s & 0xf];
        s >>= 4;
      }
      bool skipZero = true;
      for (auto c : buf)
      {
        if (skipZero && (c == '0'))
        {
          continue;
        }
        skipZero = false;
        append_char(c);
      }
      if (skipZero)
      {
        append_char('0');
      }
    }

    /**
     * Overload to force `long` to be promoted to `long long`.
     */
    void append(long x)
    {
      append(static_cast<long long>(x));
    }

    /**
     * Overload to force `unsigned long` to be promoted to `unsigned long long`.
     */
    void append(unsigned long x)
    {
      append(static_cast<unsigned long long>(x));
    }

    /**
     * Overload to force `int` to be promoted to `long long`.
     */
    void append(int x)
    {
      append(static_cast<long long>(x));
    }

    /**
     * Overload to force `unsigned int` to be promoted to `unsigned long long`.
     */
    void append(unsigned int x)
    {
      append(static_cast<unsigned long long>(x));
    }

    /**
     * Append with format string and arguments. Compiler
     * is able to optimize the recursion into loops.
     */
    template<typename Head, typename... Tail>
    SNMALLOC_FAST_PATH_INLINE void
    append(const char* fmt, Head&& head, Tail&&... tail)
    {
      for (;;)
      {
        if (fmt[0] == '\0')
        {
          error("Internal error: format string missing `{}`!");
        }

        if (fmt[0] == '{' && fmt[1] == '}')
        {
          append(stl::forward<Head>(head));
          return append(fmt + 2, stl::forward<Tail>(tail)...);
        }

        append_char(*fmt);
        fmt++;
      }
    }

  public:
    /**
     * Constructor.  Takes a format string and the arguments to output.
     */
    template<typename... Args>
    SNMALLOC_FAST_PATH MessageBuilder(const char* fmt, Args&&... args)
    {
      buffer[SafeLength] = 0;
      append(fmt, stl::forward<Args>(args)...);
      append_char('\0');
    }

    /**
     * Return the error buffer.
     */
    const char* get_message()
    {
      return buffer.data();
    }
  };

  /**
   * Convenience type that has no fields / methods.
   */
  struct Empty
  {};

} // namespace snmalloc
