// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com


#ifndef CHAISCRIPT_THREADING_HPP_
#define CHAISCRIPT_THREADING_HPP_


#include <unordered_map>

#ifndef CHAISCRIPT_NO_THREADS
#include <thread>
#include <mutex>
#else
#ifndef CHAISCRIPT_NO_THREADS_WARNING
#pragma message ("ChaiScript is compiling without thread safety.")
#endif
#endif

#include "chaiscript_defines.hpp"

/// \file
///
/// This file contains code necessary for thread support in ChaiScript.
/// If the compiler definition CHAISCRIPT_NO_THREADS is defined then thread safety
/// is disabled in ChaiScript. This has the result that some code is faster, because mutex locks are not required.
/// It also has the side effect that the chaiscript::ChaiScript object may not be accessed from more than
/// one thread simultaneously.

namespace chaiscript
{
  namespace detail
  {
    /// If threading is enabled, then this namespace contains std thread classes.
    /// If threading is not enabled, then stubbed in wrappers that do nothing are provided.
    /// This allows us to avoid \#ifdef code in the sections that need thread safety.
    namespace threading
    {

#ifndef CHAISCRIPT_NO_THREADS

      template<typename T>
        using unique_lock = std::unique_lock<T>;

      template<typename T>
        using shared_lock = std::unique_lock<T>;

      template<typename T>
        using lock_guard = std::lock_guard<T>;


      using shared_mutex = std::mutex;

      using std::mutex;

      using std::recursive_mutex;

      /// Typesafe thread specific storage. If threading is enabled, this class uses a mutex protected map. If
      /// threading is not enabled, the class always returns the same data, regardless of which thread it is called from.
      template<typename T>
        class Thread_Storage
        {
          public:
            Thread_Storage() = default;
            Thread_Storage(const Thread_Storage &) = delete;
            Thread_Storage(Thread_Storage &&) = delete;
            Thread_Storage &operator=(const Thread_Storage &) = delete;
            Thread_Storage &operator=(Thread_Storage &&) = delete;

            ~Thread_Storage()
            {
              t().erase(this);
            }

            inline const T *operator->() const
            {
              return &(t()[this]);
            }

            inline const T &operator*() const
            {
              return t()[this];
            }

            inline T *operator->()
            {
              return &(t()[this]);
            }

            inline T &operator*()
            {
              return t()[this];
            }


            void *m_key;

          private:
            static std::unordered_map<const void*, T> &t()
            {
              thread_local std::unordered_map<const void *, T> my_t;
              return my_t;
            }
        };

#else // threading disabled
      template<typename T>
      class unique_lock 
      {
        public:
          explicit unique_lock(T &) {}
          void lock() {}
          void unlock() {}
      };

      template<typename T>
      class shared_lock 
      {
        public:
          explicit shared_lock(T &) {}
          void lock() {}
          void unlock() {}
      };

      template<typename T>
      class lock_guard 
      {
        public:
          explicit lock_guard(T &) {}
      };

      class shared_mutex { };

      class recursive_mutex {};


      template<typename T>
        class Thread_Storage
        {
          public:
            explicit Thread_Storage()
            {
            }

            inline T *operator->() const
            {
              return &obj;
            }

            inline T &operator*() const
            {
              return obj;
            }

          private:
            mutable T obj;
        };

#endif
    }
  }
}



#endif

