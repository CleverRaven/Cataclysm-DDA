#pragma once

#include "../aal/aal.h"
#include "../ds_aal/ds_aal.h"
#include "pal_timer_default.h"

#ifdef _WIN32
#  ifndef _MSC_VER
#    include <errno.h>
#  endif
#  include <cstdio>
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  pragma comment(lib, "bcrypt.lib")
#  include <bcrypt.h>
// VirtualAlloc2 is exposed in RS5 headers.
#  ifdef NTDDI_WIN10_RS5
#    if (NTDDI_VERSION >= NTDDI_WIN10_RS5) && \
      (WINVER >= _WIN32_WINNT_WIN10) && !defined(USE_SYSTEMATIC_TESTING)
#      define PLATFORM_HAS_VIRTUALALLOC2
#      define PLATFORM_HAS_WAITONADDRESS
#    endif
#  endif

/**
 * In order to properly cleanup our pagemap reservation,
 * we need to make sure we do it is done after all other
 * allocations have been freed.
 *
 * One way to guarantee that the reservations get released
 * at the absolute end of the program is to force them to
 * be initialized first. Statics and globals get destroyed
 * in FILO order of when they were initialized. The pragma
 * init_seg makes sure the statics and globals in this
 * file are handled first, and thus will be the last to
 * be destroyed when the program exits or the DLL is
 * unloaded.
 */
#  pragma warning(disable : 4075)
#  pragma init_seg(".CRT$XCB")

namespace snmalloc
{
  class PALWindows : public PalTimerDefaultImpl<PALWindows>
  {
    /**
     * A flag indicating that we have tried to register for low-memory
     * notifications.
     */
    static inline stl::Atomic<bool> registered_for_notifications;
    static inline HANDLE lowMemoryObject;

    /**
     * List of callbacks for low-memory notification
     */
    static inline PalNotifier low_memory_callbacks;

    /**
     * Callback, used when the system delivers a low-memory notification.  This
     * calls all the handlers registered with the PAL.
     */
    static void CALLBACK low_memory(_In_ PVOID, _In_ BOOLEAN)
    {
      low_memory_callbacks.notify_all();
    }

    // A list of reserved ranges, used to handle lazy commit on readonly pages.
    // We currently only need one, so haven't implemented a backup if the
    // initial 16 is insufficient.
    inline static stl::Array<stl::Pair<address_t, size_t>, 16> reserved_ranges;

    // Lock for the reserved ranges.
    inline static FlagWord reserved_ranges_lock{};

    // Exception handler for handling lazy commit on readonly pages.
    static LONG NTAPI
    HandleReadonlyLazyCommit(struct _EXCEPTION_POINTERS* ExceptionInfo)
    {
      // Check this is an AV
      if (
        ExceptionInfo->ExceptionRecord->ExceptionCode !=
        EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;

      // Check this is a read access
      if (ExceptionInfo->ExceptionRecord->ExceptionInformation[0] != 0)
        return EXCEPTION_CONTINUE_SEARCH;

      // Get faulting address from exception info.
      snmalloc::address_t faulting_address =
        ExceptionInfo->ExceptionRecord->ExceptionInformation[1];

      bool found = false;
      {
        FlagLock lock(reserved_ranges_lock);
        // Check if the address is in a reserved range.
        for (auto& r : reserved_ranges)
        {
          if ((faulting_address - r.first) < r.second)
          {
            found = true;
            break;
          }
        }
      }

      if (!found)
        return EXCEPTION_CONTINUE_SEARCH;

      // Commit the page as readonly
      auto pagebase = snmalloc::bits::align_down(faulting_address, page_size);
      VirtualAlloc((void*)pagebase, page_size, MEM_COMMIT, PAGE_READONLY);

      // Resume execution
      return EXCEPTION_CONTINUE_EXECUTION;
    }

    static void initialise_for_singleton(size_t*) noexcept
    {
      // Keep a handle for the exception handler, so we can remove it later
      // when needed.
      static PVOID g_Handler{};
      // Destructor for removing exception handler.
      static OnDestruct tidy([]() {
        if (g_Handler)
        {
          RemoveVectoredExceptionHandler(g_Handler);
          g_Handler = NULL; // Prevent dangling pointer
        }
      });
      // Add exception handler for lazy commit.
      if (!g_Handler)
      {
        g_Handler = AddVectoredExceptionHandler(1, HandleReadonlyLazyCommit);
      }
    }

    // Ensure the exception handler is registered.
    static void initialise_readonly_av() noexcept
    {
      static Singleton<size_t, &initialise_for_singleton> init;
      init.get();
    }

  public:
    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.  This PAL supports low-memory notifications.
     */
    static constexpr uint64_t pal_features = LowMemoryNotification | Entropy |
      Time | LazyCommit
#  if defined(PLATFORM_HAS_VIRTUALALLOC2) && !defined(USE_SYSTEMATIC_TESTING)
      | AlignedAllocation
#  endif
#  if defined(PLATFORM_HAS_WAITONADDRESS)
      | WaitOnAddress
#  endif
      ;

    static SNMALLOC_CONSTINIT_STATIC size_t minimum_alloc_size = 0x10000;

    static constexpr size_t page_size = 0x1000;

    /**
     * Windows always inherits its underlying architecture's full address range.
     */
    static constexpr size_t address_bits = Aal::address_bits;

    /**
     * Check whether the low memory state is still in effect.  This is an
     * expensive operation and should not be on any fast paths.
     */
    static bool expensive_low_memory_check()
    {
      BOOL result;
      QueryMemoryResourceNotification(lowMemoryObject, &result);
      return result;
    }

    /**
     * Register callback object for low-memory notifications.
     * Client is responsible for allocation, and ensuring the object is live
     * for the duration of the program.
     */
    static void
    register_for_low_memory_callback(PalNotificationObject* callback)
    {
      // No error handling here - if this doesn't work, then we will just
      // consume more memory.  There's nothing sensible that we could do in
      // error handling.  We also leak both the low memory notification object
      // handle and the wait object handle.  We'll need them until the program
      // exits, so there's little point doing anything else.
      //
      // We only try to register once.  If this fails, give up.  Even if we
      // create multiple PAL objects, we don't want to get more than one
      // callback.
      if (!registered_for_notifications.exchange(true))
      {
        lowMemoryObject =
          CreateMemoryResourceNotification(LowMemoryResourceNotification);
        HANDLE waitObject;
        RegisterWaitForSingleObject(
          &waitObject,
          lowMemoryObject,
          low_memory,
          nullptr,
          INFINITE,
          WT_EXECUTEDEFAULT);
      }

      low_memory_callbacks.register_notification(callback);
    }

    static void message(const char* const str)
    {
      fputs(str, stderr);
      fputc('\n', stderr);
      fflush(stderr);
    }

    [[noreturn]] static void error(const char* const str)
    {
      message(str);
      abort();
    }

    /// Notify platform that we will not be using these pages
    static void notify_not_using(void* p, size_t size) noexcept
    {
      SNMALLOC_ASSERT(is_aligned_block<page_size>(p, size));

      BOOL ok = VirtualFree(p, size, MEM_DECOMMIT);

      if (!ok)
        error("VirtualFree failed");
    }

    /// Notify platform that we will be using these pages
    template<ZeroMem zero_mem>
    static bool notify_using(void* p, size_t size) noexcept
    {
      SNMALLOC_ASSERT(
        is_aligned_block<page_size>(p, size) || (zero_mem == NoZero));

      void* r = VirtualAlloc(p, size, MEM_COMMIT, PAGE_READWRITE);

      return r != nullptr;
    }

    static bool notify_using_readonly(void* p, size_t size) noexcept
    {
      initialise_readonly_av();

      {
        FlagLock lock(reserved_ranges_lock);
        for (auto& r : reserved_ranges)
        {
          if (r.first == 0)
          {
            r.first = (address_t)p;
            r.second = size;
            return true;
          }
        }
      }

      error("Implementation error: Too many lazy commit regions!");
    }

    /// OS specific function for zeroing memory
    template<bool page_aligned = false>
    static void zero(void* p, size_t size) noexcept
    {
      if (page_aligned || is_aligned_block<page_size>(p, size))
      {
        SNMALLOC_ASSERT(is_aligned_block<page_size>(p, size));
        notify_not_using(p, size);
        notify_using<YesZero>(p, size);
      }
      else
        ::memset(p, 0, size);
    }

#  ifdef PLATFORM_HAS_VIRTUALALLOC2
    template<bool state_using>
    static void* reserve_aligned(size_t size) noexcept;
#  endif

    static void* reserve(size_t size) noexcept;

    /**
     * Source of Entropy
     */
    static uint64_t get_entropy64()
    {
      uint64_t result;
      if (
        BCryptGenRandom(
          nullptr,
          reinterpret_cast<PUCHAR>(&result),
          sizeof(result),
          BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
        error("Failed to get entropy.");
      return result;
    }

    static uint64_t performance_counter_frequency()
    {
      static stl::Atomic<uint64_t> freq_cache = 0;
      SNMALLOC_UNINITIALISED LARGE_INTEGER buf;

      auto freq = freq_cache.load(stl::memory_order_relaxed);
      if (SNMALLOC_UNLIKELY(freq == 0))
      {
        // On systems that run Windows XP or later, the function will always
        // succeed and will thus never return zero.
        ::QueryPerformanceFrequency(&buf);
        freq = static_cast<uint64_t>(buf.QuadPart);
        freq_cache.store(freq, stl::memory_order_relaxed);
      }

      return freq;
    }

    static uint64_t internal_time_in_ms()
    {
      constexpr uint64_t ms_per_second = 1000;
      SNMALLOC_UNINITIALISED LARGE_INTEGER buf;
      auto freq = performance_counter_frequency();
      ::QueryPerformanceCounter(&buf);
      return (static_cast<uint64_t>(buf.QuadPart) * ms_per_second) / freq;
    }

    static uint64_t tick()
    {
      if constexpr (
        (Aal::aal_features & NoCpuCycleCounters) != NoCpuCycleCounters)
      {
        return Aal::tick();
      }
      else
      {
        constexpr uint64_t ns_per_second = 1'000'000'000;
        SNMALLOC_UNINITIALISED LARGE_INTEGER buf;
        auto freq = performance_counter_frequency();
        ::QueryPerformanceCounter(&buf);
        return (static_cast<uint64_t>(buf.QuadPart) * ns_per_second) / freq;
      }
    }

#  ifdef PLATFORM_HAS_WAITONADDRESS
    using WaitingWord = char;

    template<class T>
    static void wait_on_address(stl::Atomic<T>& addr, T expected)
    {
      while (addr.load(stl::memory_order_relaxed) == expected)
      {
        ::WaitOnAddress(&addr, &expected, sizeof(T), INFINITE);
      }
    }

    template<class T>
    static void notify_one_on_address(stl::Atomic<T>& addr)
    {
      ::WakeByAddressSingle(&addr);
    }

    template<class T>
    static void notify_all_on_address(stl::Atomic<T>& addr)
    {
      ::WakeByAddressAll(&addr);
    }
#  endif
  };

  /**
   * This VirtualVector class is an implementation of
   * a vector, but does not use new/malloc or any other
   * STL containers. We cannot use these after
   * init_seg(".CRT$XCB") usage, because the CRT
   * might not be fully initialized yet. The segments
   * compiler, lib, user cannot be used because of
   * CRT internals like std::locale facets
   */
  class VirtualVector
  {
    void** data = nullptr;
    size_t size = 0;
    size_t committed_elements = 0;
    size_t reserved_elements = 0;

    static constexpr size_t MinCommit =
      snmalloc::PALWindows::page_size / sizeof(void*);
    static constexpr size_t MinReserve =
      16 * snmalloc::PALWindows::page_size / sizeof(void*);

    // Lock for the reserved ranges.
    inline static snmalloc::FlagWord push_back_lock{};

  public:
    VirtualVector(
      size_t reserve_elems = MinReserve, size_t initial_commit = MinCommit)
    {
      reserve_and_commit(reserve_elems, initial_commit);
    }

    ~VirtualVector()
    {
      if (data)
      {
        for (size_t i = size; i > 0; i--)
        {
          size_t index = i - 1;
          if (data[index] == nullptr)
            continue;

          BOOL ok = VirtualFree(data[index], 0, MEM_RELEASE);

          data[index] = nullptr;

          if (!ok)
          {
            snmalloc::PALWindows::error("VirtualFree failed");
          }
        }

        BOOL ok = VirtualFree(data, 0, MEM_RELEASE);

        data = nullptr;

        if (!ok)
        {
          snmalloc::PALWindows::error("VirtualFree failed");
        }
      }
    }

    void push_back(void* value)
    {
      snmalloc::FlagLock lock(push_back_lock);
      ensure_capacity();
      data[size++] = value;
    }

  private:
    // Simple max function (avoiding <algorithm>)
    static size_t max_size_t(size_t a, size_t b)
    {
      return a > b ? a : b;
    }

    void ensure_capacity()
    {
      if (size >= committed_elements)
      {
        size_t grow = max_size_t(MinCommit, committed_elements / 2);
        commit_more(committed_elements + grow);
      }

      if (size >= reserved_elements)
      {
        grow_reserved();
      }
    }

    void reserve_and_commit(size_t reserve_elems, size_t commit_elems)
    {
      size_t reserve_bytes = reserve_elems * sizeof(void*);
      void** new_block = (void**)VirtualAlloc(
        nullptr, reserve_bytes, MEM_RESERVE, PAGE_READWRITE);
      if (!new_block)
        snmalloc::PALWindows::error("VirtualAlloc failed");

      size_t commit_bytes = commit_elems * sizeof(void*);
      if (!VirtualAlloc(new_block, commit_bytes, MEM_COMMIT, PAGE_READWRITE))
      {
        VirtualFree(new_block, 0, MEM_RELEASE);
        snmalloc::PALWindows::error("VirtualAlloc failed");
      }

      data = new_block;
      reserved_elements = reserve_elems;
      committed_elements = commit_elems;
    }

    void commit_more(size_t new_commit_elements)
    {
      if (new_commit_elements > reserved_elements)
      {
        grow_reserved();
        return;
      }

      size_t old_bytes = committed_elements * sizeof(void*);
      size_t new_bytes = new_commit_elements * sizeof(void*);
      size_t commit_bytes = new_bytes - old_bytes;

      if (commit_bytes > 0)
      {
        void* result = VirtualAlloc(
          (char*)data + old_bytes, commit_bytes, MEM_COMMIT, PAGE_READWRITE);
        if (!result)
          error("VirtualAlloc failed");

        committed_elements = new_commit_elements;
      }
    }

    void grow_reserved()
    {
      size_t new_reserved =
        reserved_elements == 0 ? MinReserve : reserved_elements * 2;
      size_t new_commit = max_size_t(committed_elements, size + MinCommit);

      void** new_block = (void**)VirtualAlloc(
        nullptr, new_reserved * sizeof(void*), MEM_RESERVE, PAGE_READWRITE);
      if (!new_block)
        error("VirtualAlloc failed");

      if (!VirtualAlloc(
            new_block, new_commit * sizeof(void*), MEM_COMMIT, PAGE_READWRITE))
      {
        VirtualFree(new_block, 0, MEM_RELEASE);
        error("VirtualAlloc failed");
      }

      // Copy existing values
      for (size_t i = 0; i < size; ++i)
      {
        new_block[i] = data[i];
      }

      VirtualFree(data, 0, MEM_RELEASE);
      data = new_block;
      reserved_elements = new_reserved;
      committed_elements = new_commit;
    }

  public:
    void*& operator[](size_t index)
    {
      return data[index];
    }

    const void* operator[](size_t index) const
    {
      return data[index];
    }

    size_t get_size() const
    {
      return size;
    }

    size_t get_capacity() const
    {
      return committed_elements;
    }

    void** begin()
    {
      return data;
    }

    void** end()
    {
      return data + size;
    }

    const void* const* begin() const
    {
      return data;
    }

    const void* const* end() const
    {
      return data + size;
    }
  };

  /**
   * This will be destroyed last of all of the
   * statics and globals due to init_seg
   */
  static inline VirtualVector reservations;

#  ifdef PLATFORM_HAS_VIRTUALALLOC2
  template<bool state_using>
  void* PALWindows::reserve_aligned(size_t size) noexcept
  {
    SNMALLOC_ASSERT(bits::is_pow2(size));
    SNMALLOC_ASSERT(size >= minimum_alloc_size);

    DWORD flags = MEM_RESERVE;

    if (state_using)
      flags |= MEM_COMMIT;

    // If we're on Windows 10 or newer, we can use the VirtualAlloc2
    // function.  The FromApp variant is useable by UWP applications and
    // cannot allocate executable memory.
    MEM_ADDRESS_REQUIREMENTS addressReqs = {NULL, NULL, size};

    MEM_EXTENDED_PARAMETER param = {
      {MemExtendedParameterAddressRequirements, 0}, {0}};
    // Separate assignment as MSVC doesn't support .Pointer in the
    // initialisation list.
    param.Pointer = &addressReqs;

    void* ret = VirtualAlloc2FromApp(
      nullptr, nullptr, size, flags, PAGE_READWRITE, &param, 1);

    reservations.push_back(ret);

    return ret;
  }
#  endif

  void* PALWindows::reserve(size_t size) noexcept
  {
    void* ret = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);

    reservations.push_back(ret);

    return ret;
  }

}
#endif
