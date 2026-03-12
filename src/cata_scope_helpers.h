#pragma once
#ifndef CATA_SRC_CATA_SCOPE_HELPERS_H
#define CATA_SRC_CATA_SCOPE_HELPERS_H

#include <functional>
#include <type_traits>

class on_out_of_scope
{
    private:
        std::function<void()> func;
    public:
        explicit on_out_of_scope( const std::function<void()> &func ) : func( func ) {}

        on_out_of_scope( const on_out_of_scope & ) = delete;
        on_out_of_scope( on_out_of_scope && ) = delete;
        on_out_of_scope &operator=( const on_out_of_scope & ) = delete;
        on_out_of_scope &operator=( on_out_of_scope && ) = delete;

        ~on_out_of_scope() {
            if( func ) {
                func();
            }
        }

        void cancel() {
            func = nullptr;
        }
};

template<typename T>
class restore_on_out_of_scope
{
    private:
        T &t;
        T orig_t;
        on_out_of_scope impl;
    public:
        // *INDENT-OFF*
        explicit restore_on_out_of_scope(T& t_in) : t(t_in), orig_t(t_in),
            impl([this]() { t = std::move(orig_t); }) {
        }

        // Ideally this would be deleted, but it is needed to support unique_ptr<U>
        explicit restore_on_out_of_scope(T&& t_in) : t(t_in), orig_t(std::move(t_in)),
            impl([this]() { t = std::move(orig_t); }) {
        }

        // Don't allow restoring any variable that is not a T, because that means the caller
        // would potentially induce a conversion which results in a temporary T.
        template<typename U, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, std::decay_t<U>>>>
        restore_on_out_of_scope(U&&) = delete;
        // *INDENT-ON*

        void cancel() {
            impl.cancel();
        }

        restore_on_out_of_scope( const restore_on_out_of_scope<T> & ) = delete;
        restore_on_out_of_scope( restore_on_out_of_scope<T> && ) = delete;
        restore_on_out_of_scope &operator=( const restore_on_out_of_scope<T> & ) = delete;
        restore_on_out_of_scope &operator=( restore_on_out_of_scope<T> && ) = delete;
};

#endif // CATA_SRC_CATA_SCOPE_HELPERS_H
