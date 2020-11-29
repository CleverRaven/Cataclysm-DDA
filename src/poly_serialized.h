#pragma once
#ifndef CATA_SRC_POLY_SERIALIZED_PTR_H
#define CATA_SRC_POLY_SERIALIZED_PTR_H

#include <memory>
#include "json.h"

namespace cata
{

/**
 * Copyable unique_ptr that writes and reads objects of derived types.
 * Base type must implement get_type and static create.
 */
template <class T>
class poly_serialized : public std::unique_ptr<T>
{
    public:
        poly_serialized() = default;
        poly_serialized( poly_serialized && ) = default;
        poly_serialized( T *value ) : std::unique_ptr<T>( value ) {}
        poly_serialized( const poly_serialized<T> &other ) :
            std::unique_ptr<T>( other ? other.get()->clone() : nullptr ) {}
        poly_serialized &operator=( poly_serialized<T> other ) {
            std::unique_ptr<T>::operator=( std::move( other ) );
            return *this;
        }

        void serialize( JsonOut &jsout ) const {
            jsout.start_array();
            if( this->get() ) {
                jsout.write( this->get()->get_type() );
                jsout.start_object();
                this->get()->serialize( jsout );
                jsout.end_object();
            } else {
                jsout.write_null();
            }
            jsout.end_array();
        }

        void deserialize( JsonIn &jsin ) {
            jsin.start_array();
            if( jsin.test_null() ) {
                this->reset();
            } else {
                std::decay_t < decltype( this->get()->get_type() ) > read_type;
                jsin.read( read_type );
                this->reset( T::create( read_type ) );
                this->get()->deserialize( jsin );
            }
            jsin.end_array();
        }
};

template <class T, class... Args>
poly_serialized<T> make_poly_serialized( Args &&...args )
{
    return poly_serialized<T>( new T( std::forward<Args>( args )... ) );
}

} // namespace cata

#endif // CATA_SRC_POLY_SERIALIZED_PTR_H
