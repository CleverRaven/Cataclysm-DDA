#pragma once

#include "../aal/aal.h"
#include "../ds_core/ds_core.h"
#include "snmalloc/stl/type_traits.h"
#include "snmalloc/stl/utility.h"

#include <stdint.h>

namespace snmalloc
{
  /**
   * Simple sequential set of T.
   *
   * Implemented as a doubly linked cyclic list.
   * Linked using the T::node field.
   *
   * Can be used in either Fifo or Lifo mode, which is
   * specified by template parameter to `pop`.
   */
  template<typename T>
  class SeqSet
  {
  public:
    /**
     * The doubly linked Node.
     */
    class Node
    {
      Node* next;
      Node* prev;

      friend class SeqSet;

      constexpr Node(Node* next, Node* prev) : next(next), prev(prev) {}

    public:
      /// Default constructor, creates an invalid node.
      constexpr Node() : Node(nullptr, nullptr) {}

      void invariant()
      {
        SNMALLOC_ASSERT(next != nullptr);
        SNMALLOC_ASSERT(prev != nullptr);
        SNMALLOC_ASSERT(next->prev == this);
        SNMALLOC_ASSERT(prev->next == this);
      }

      void remove()
      {
        invariant();
        next->invariant();
        prev->invariant();
        next->prev = prev;
        prev->next = next;
        next->invariant();
        prev->invariant();
      }
    };

  private:
    // Cyclic doubly linked list (initially empty)
    Node head{&head, &head};

    /**
     * Returns the containing object.
     */
    T* containing(Node* n)
    {
      // We could use -static_cast<ptrdiff_t>(offsetof(T, node)) here but CHERI
      // compiler complains. So we restrict to first entries only.

      static_assert(offsetof(T, node) == 0);

      return pointer_offset<T>(n, 0);
    }

    /**
     * Gets the doubly linked node for the object.
     */
    Node* get_node(T* t)
    {
#ifdef __CHERI_PURE_CAPABILITY__
      return &__builtin_no_change_bounds(t->node);
#else
      return &(t->node);
#endif
    }

  public:
    /**
     * Empty queue
     */
    constexpr SeqSet() = default;

    /**
     * Check for empty
     */
    SNMALLOC_FAST_PATH bool is_empty()
    {
      static_assert(
        stl::is_same_v<Node, decltype(stl::declval<T>().node)>,
        "T->node must be Node for T");
      head.invariant();
      return head.next == &head;
    }

    /**
     * Remove an element from the queue
     *
     * Assumes queue is non-empty
     */
    SNMALLOC_FAST_PATH T* pop_front()
    {
      head.invariant();
      SNMALLOC_ASSERT(!this->is_empty());
      auto node = head.next;
      node->remove();
      auto result = containing(node);
      head.invariant();
      return result;
    }

    /**
     * Remove an element from the queue
     *
     * Assumes queue is non-empty
     */
    SNMALLOC_FAST_PATH T* pop_back()
    {
      head.invariant();
      SNMALLOC_ASSERT(!this->is_empty());
      auto node = head.prev;
      node->remove();
      auto result = containing(node);
      head.invariant();
      return result;
    }

    template<bool is_fifo>
    SNMALLOC_FAST_PATH T* pop()
    {
      head.invariant();
      if constexpr (is_fifo)
        return pop_front();
      else
        return pop_back();
    }

    /**
     * Applies `f` to all the elements in the set.
     *
     * `f` is allowed to remove the element from the set.
     */
    template<typename Fn>
    SNMALLOC_FAST_PATH void iterate(Fn&& f)
    {
      auto curr = head.next;
      curr->invariant();

      while (curr != &head)
      {
        // Read next first, as f may remove curr.
        auto next = curr->next;
        f(containing(curr));
        curr = next;
      }
    }

    /**
     * Add an element to the queue.
     */
    SNMALLOC_FAST_PATH void insert(T* item)
    {
      auto n = get_node(item);

      n->next = head.next;
      head.next->prev = n;

      n->prev = &head;
      head.next = n;

      n->invariant();
      head.invariant();
    }

    /**
     * Peek at next element in the set.
     */
    SNMALLOC_FAST_PATH const T* peek()
    {
      return containing(head.next);
    }
  };
} // namespace snmalloc
