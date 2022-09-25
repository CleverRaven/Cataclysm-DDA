#pragma once
#ifndef CATA_SRC_CATA_SCOPE_HELPERS_H
#define CATA_SRC_CATA_SCOPE_HELPERS_H

#include <functional>

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

        explicit restore_on_out_of_scope(T&& t_in) : t(t_in), orig_t(std::move(t_in)),
            impl([this]() { t = std::move(orig_t); }) {
        }
        // *INDENT-ON*

        restore_on_out_of_scope( const restore_on_out_of_scope<T> & ) = delete;
        restore_on_out_of_scope( restore_on_out_of_scope<T> && ) = delete;
        restore_on_out_of_scope &operator=( const restore_on_out_of_scope<T> & ) = delete;
        restore_on_out_of_scope &operator=( restore_on_out_of_scope<T> && ) = delete;
};

#endif // CATA_SRC_CATA_SCOPE_HELPERS_H
