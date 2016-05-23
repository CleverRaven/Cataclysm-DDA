#ifndef uid_H
#define uid_H

#include "json.h"

template <unsigned char N>
class uid;

enum {
    UID_ITEM = 0
};

using item_uid = uid<UID_ITEM>;

template <unsigned char N>
class uid_factory : public JsonSerializer, public JsonDeserializer
{
        friend class uid<N>;

    public:
        uid_factory() {
            instance = this;
        }

        void serialize( JsonOut &js ) const {
            js.write( val );
        }

        void deserialize( JsonIn &js ) {
            js.read( val );
        }

        uid<N> assign() {
            return uid<N>( this, next_id() );
        }

    private:
        unsigned long long val;

        unsigned long long next_id() {
            // set high-order byte to version
            return ++val |= static_cast<unsigned long long>( N ) << 55;
        }

        static uid_factory<N> *instance;
};

template <unsigned char N>
class uid : public JsonSerializer, public JsonDeserializer
{
        friend class uid_factory<N>;
        friend class item_location; /** needs clone() */

    public:
        uid() = default;

        uid( uid && ) = default;
        uid &operator=( uid && ) = default;

        uid( const uid & ) {
            val = factory ? factory->next_id() : 0;
        }

        uid &operator=( const uid & ) {
            val = factory ? factory->next_id() : 0;
            return *this;
        }

        bool operator==( const uid &rhs ) const {
            return val == rhs.val;
        }
        bool operator!=( const uid &rhs ) const {
            return val != rhs.val;
        }
        bool operator< ( const uid &rhs ) const {
            return val < rhs.val;
        }

        bool valid() const {
            return factory != nullptr && val != 0;
        }

        void serialize( JsonOut &js ) const {
            js.write( val );
        }

        void deserialize( JsonIn &js ) {
            factory = uid_factory<N>::instance;
            js.read( val );
        }

    private:
        uid_factory<N> *factory = nullptr;
        unsigned long long val = 0;

        uid( uid_factory<N> *factory, unsigned long long val )
            : factory( factory ), val( val ) {}

        uid clone() const {
            return uid<N>( factory, val );
        }
};

#endif
