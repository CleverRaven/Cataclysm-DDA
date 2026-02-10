#pragma once

#include "../ds_core/ds_core.h"
#include "aba.h"
#include "allocconfig.h"

namespace snmalloc
{
  template<class T, Construction c = RequiresInit>
  class MPMCStack
  {
    using ABAT = ABA<T, c>;

  private:
    alignas(CACHELINE_SIZE) ABAT stack;

#ifdef SNMALLOC_THREAD_SANITIZER_ENABLED
    __attribute__((no_sanitize("thread"))) static T*
    racy_read(stl::Atomic<T*>& ptr)
    {
      // reinterpret_cast is required as TSAN still instruments
      // stl::Atomic operations, even if you disable TSAN on
      // the function.
      return *reinterpret_cast<T**>(&ptr);
    }
#else
    static T* racy_read(stl::Atomic<T*>& ptr)
    {
      return ptr.load(stl::memory_order_relaxed);
    }
#endif

  public:
    constexpr MPMCStack() = default;

    void push(T* item)
    {
      static_assert(
        stl::is_same_v<decltype(T::next), stl::Atomic<T*>>,
        "T->next must be an stl::Atomic<T*>");

      return push(item, item);
    }

    void push(T* first, T* last)
    {
      // Pushes an item on the stack.
      auto cmp = stack.read();

      do
      {
        auto top = cmp.ptr();
        last->next.store(top, stl::memory_order_release);
      } while (!cmp.store_conditional(first));
    }

    T* pop()
    {
      // Returns the next item. If the returned value is decommitted, it is
      // possible for the read of top->next to segfault.
      auto cmp = stack.read();
      T* top;
      T* next;

      do
      {
        top = cmp.ptr();

        if (top == nullptr)
          break;

        // The following read can race with non-atomic accesses
        // this is undefined behaviour. There is no way to use
        // CAS sensibly that conforms to the standard with optimistic
        // concurrency.
        next = racy_read(top->next);
      } while (!cmp.store_conditional(next));

      return top;
    }

    T* pop_all()
    {
      // Returns all items as a linked list, leaving an empty stack.
      auto cmp = stack.read();
      T* top;

      do
      {
        top = cmp.ptr();

        if (top == nullptr)
          break;
      } while (!cmp.store_conditional(nullptr));

      return top;
    }
  };
} // namespace snmalloc
