// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// and Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_ANY_HPP_
#define CHAISCRIPT_ANY_HPP_

#include <utility>

namespace chaiscript {
  namespace detail {
    namespace exception
    {
      /// \brief Thrown in the event that an Any cannot be cast to the desired type
      ///
      /// It is used internally during function dispatch.
      ///
      /// \sa chaiscript::detail::Any
      class bad_any_cast : public std::bad_cast
      {
        public:
          bad_any_cast() = default;

          bad_any_cast(const bad_any_cast &) = default;

          ~bad_any_cast() noexcept override = default;

          /// \brief Description of what error occurred
          const char * what() const noexcept override
          {
            return m_what.c_str();
          }

        private:
          std::string m_what = "bad any cast";
      };
    }
  

    class Any {
      private:
        struct Data
        {
          explicit Data(const std::type_info &t_type) 
            : m_type(t_type)
          {
          }

          Data &operator=(const Data &) = delete;

          virtual ~Data() = default;

          virtual void *data() = 0;

          const std::type_info &type() const
          {
            return m_type;
          }

          virtual std::unique_ptr<Data> clone() const = 0;
          const std::type_info &m_type;
        };

        template<typename T>
          struct Data_Impl : Data
          {
            explicit Data_Impl(T t_type)
              : Data(typeid(T)),
                m_data(std::move(t_type))
            {
            }

            void *data() override
            {
              return &m_data;
            }

            std::unique_ptr<Data> clone() const override
            {
              return std::unique_ptr<Data>(new Data_Impl<T>(m_data));
            }

            Data_Impl &operator=(const Data_Impl&) = delete;

            T m_data;
          };

        std::unique_ptr<Data> m_data;

      public:
        // construct/copy/destruct
        Any() = default;
        Any(Any &&) = default;
        Any &operator=(Any &&t_any) = default;

        Any(const Any &t_any) 
        { 
          if (!t_any.empty())
          {
            m_data = t_any.m_data->clone(); 
          } else {
            m_data.reset();
          }
        }


        template<typename ValueType,
          typename = typename std::enable_if<!std::is_same<Any, typename std::decay<ValueType>::type>::value>::type>
        explicit Any(ValueType &&t_value)
          : m_data(std::unique_ptr<Data>(new Data_Impl<typename std::decay<ValueType>::type>(std::forward<ValueType>(t_value))))
        {
        }


        Any & operator=(const Any &t_any)
        {
          Any copy(t_any);
          swap(copy);
          return *this; 
        }

        template<typename ToType>
          ToType &cast() const
          {
            if (m_data && typeid(ToType) == m_data->type())
            {
              return *static_cast<ToType *>(m_data->data());
            } else {
              throw chaiscript::detail::exception::bad_any_cast();
            }
          }


        // modifiers
        Any & swap(Any &t_other)
        {
          std::swap(t_other.m_data, m_data);
          return *this;
        }

        // queries
        bool empty() const
        {
          return !bool(m_data);
        }

        const std::type_info & type() const
        {
          if (m_data) {
            return m_data->type();
          } else {
            return typeid(void);
          }
        }
    };

  }
}

#endif


