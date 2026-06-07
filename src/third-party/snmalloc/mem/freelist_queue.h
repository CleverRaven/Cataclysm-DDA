#pragma once

#include "../ds/ds.h"
#include "freelist.h"
#include "snmalloc/stl/atomic.h"

namespace snmalloc
{
  /**
   * A FreeListMPSCQ is a chain of freed objects exposed as a MPSC append-only
   * atomic queue that uses one xchg per append.
   *
   * The internal pointers are considered QueuePtr-s to support deployment
   * scenarios in which the MPSCQ itself is exposed to the client.  This is
   * excessively paranoid in the common case that these metadata are as "hard"
   * for the client to reach as the Pagemap, which we trust to store not just
   * Tame CapPtr<>s but raw C++ pointers.
   *
   * Where necessary, methods expose two domesticator callbacks at the
   * interface and are careful to use one for the front and back values and the
   * other for pointers read from the queue itself.  That's not ideal, but it
   * lets the client condition its behavior appropriately and prevents us from
   * accidentally following either of these pointers in generic code.
   * Specifically,
   *
   *   * `domesticate_head` is used for the MPSCQ pointers used to reach into
   *     the chain of objects
   *
   *   * `domesticate_queue` is used to traverse links in that chain (and in
   *     fact, we traverse only the first).
   *
   * In the case that the MPSCQ is not easily accessible to the client,
   * `domesticate_head` can just be a type coersion, and `domesticate_queue`
   * should perform actual validation.  If the MPSCQ is exposed to the
   * allocator client, both Domesticators should perform validation.
   */
  template<FreeListKey& Key, address_t Key_tweak = NO_KEY_TWEAK>
  struct alignas(REMOTE_MIN_ALIGN) FreeListMPSCQ
  {
    // Store the message queue on a separate cacheline. It is mutable data that
    // is read by other threads.
    alignas(CACHELINE_SIZE) freelist::AtomicQueuePtr back{nullptr};
    // Store the two ends on different cache lines as access by different
    // threads.
    alignas(CACHELINE_SIZE) freelist::AtomicQueuePtr front{nullptr};

    constexpr FreeListMPSCQ() = default;

    void invariant()
    {
      SNMALLOC_ASSERT(pointer_align_up(this, REMOTE_MIN_ALIGN) == this);
    }

    void init()
    {
      back.store(nullptr);
      front.store(nullptr);
      invariant();
    }

    freelist::QueuePtr destroy()
    {
      if (back.load(stl::memory_order_relaxed) == nullptr)
        return nullptr;

      freelist::QueuePtr fnt = front.load();
      back.store(nullptr, stl::memory_order_relaxed);
      front.store(nullptr, stl::memory_order_relaxed);
      return fnt;
    }

    template<typename Domesticator_queue, typename Cb>
    void destroy_and_iterate(Domesticator_queue domesticate, Cb cb)
    {
      auto p = domesticate(destroy());

      while (p != nullptr)
      {
        auto n = p->atomic_read_next(Key, Key_tweak, domesticate);
        cb(p);
        p = n;
      }
    }

    inline bool can_dequeue()
    {
      return front.load(stl::memory_order_relaxed) !=
        back.load(stl::memory_order_relaxed);
    }

    /**
     * Pushes a list of messages to the queue. Each message from first to
     * last should be linked together through their next pointers.
     *
     * The Domesticator here is used only on pointers read from the head.  See
     * the commentary on the class.
     */
    template<typename Domesticator_head>
    void enqueue(
      freelist::HeadPtr first,
      freelist::HeadPtr last,
      Domesticator_head domesticate_head)
    {
      invariant();
      freelist::Object::atomic_store_null(last, Key, Key_tweak);

      // // The following non-linearisable effect is normally benign,
      // // but could lead to a remote list become completely detached
      // // during a fork in a multi-threaded process. This would lead
      // // to a memory leak, which is probably the least of your problems
      // // if you forked in during a deallocation.  We can prevent this
      // // with the following code, but it is not currently enabled as it
      // // has negative performance impact.
      // // An alternative would be to reset the queue on the child postfork
      // // handler to ensure that the queue has not been blackholed.
      // PreventFork pf;
      // snmalloc::UNUSED(pf);

      // Exchange needs to be acq_rel.
      // *  It needs to be a release, so nullptr in next is visible.
      // *  Needs to be acquire, so linking into the list does not race with
      //    the other threads nullptr init of the next field.
      freelist::QueuePtr prev =
        back.exchange(capptr_rewild(last), stl::memory_order_acq_rel);

      if (SNMALLOC_LIKELY(prev != nullptr))
      {
        freelist::Object::atomic_store_next(
          domesticate_head(prev), first, Key, Key_tweak);
        return;
      }

      front.store(capptr_rewild(first));
    }

    /**
     * Destructively iterate the queue.  Each queue element is removed and fed
     * to the callback in turn.  The callback may return false to stop iteration
     * early (but must have processed the element it was given!).
     *
     * Takes a domestication callback for each of "pointers read from head" and
     * "pointers read from queue".  See the commentary on the class.
     */
    template<
      typename Domesticator_head,
      typename Domesticator_queue,
      typename Cb>
    void dequeue(
      Domesticator_head domesticate_head,
      Domesticator_queue domesticate_queue,
      Cb cb)
    {
      invariant();

      freelist::HeadPtr curr = domesticate_head(front.load());
      if (curr == nullptr)
      {
        // First entry is still in progress of being added.
        // Nothing to do.
        return;
      }

      // Use back to bound, so we don't handle new entries.
      auto b = back.load(stl::memory_order_relaxed);

      while (address_cast(curr) != address_cast(b))
      {
        freelist::HeadPtr next =
          curr->atomic_read_next(Key, Key_tweak, domesticate_queue);
        // We have observed a non-linearisable effect of the queue.
        // Just go back to allocating normally.
        if (SNMALLOC_UNLIKELY(next == nullptr))
          break;
        // We want this element next, so start it loading.
        Aal::prefetch(next.unsafe_ptr());
        if (SNMALLOC_UNLIKELY(!cb(curr)))
        {
          /*
           * We've domesticate_queue-d next so that we can read through it, but
           * we're storing it back into client-accessible memory in
           * !QueueHeadsAreTame builds, so go ahead and consider it Wild again.
           * On QueueHeadsAreTame builds, the subsequent domesticate_head call
           * above will also be a type-level sleight of hand, but we can still
           * justify it by the domesticate_queue that happened in this
           * dequeue().
           */
          front = capptr_rewild(next);
          invariant();
          return;
        }

        curr = next;
      }

      /*
       * Here, we've hit the end of the queue: next is nullptr and curr has not
       * been handed to the callback.  The same considerations about Wildness
       * above hold here.
       */
      front = capptr_rewild(curr);
      invariant();
    }
  };
} // namespace snmalloc
