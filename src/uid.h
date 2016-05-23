#ifndef uid_H
#define uid_H

#include <cstdint>

#include "json.h"

class uid;

class uid_factory : public JsonSerializer, public JsonDeserializer
{
        friend class uid;

    public:
        void serialize( JsonOut &js ) const;
        void deserialize( JsonIn &js );

        uid assign();

    private:
        unsigned long long val;

        unsigned long long next_id();
};

class uid : public JsonSerializer, public JsonDeserializer
{
        friend class uid_factory;
        friend class item_location; /** needs clone() */

    public:
        uid() = default;

        uid( uid && ) = default;
        uid &operator=( uid && ) = default;

        uid( const uid & );
        uid &operator=( const uid & );

        bool operator==( const uid &rhs ) const {
            return val == rhs.val;
        }
        bool operator!=( const uid &rhs ) const {
            return val != rhs.val;
        }
        bool operator< ( const uid &rhs ) const {
            return val < rhs.val;
        }

        bool valid() const { return factory && val; }

        void serialize( JsonOut &js ) const;
        void deserialize( JsonIn &js );

    private:
        uid_factory *factory = nullptr;
        unsigned long long val = 0;

        uid( uid_factory *factory, unsigned long long val )
            : factory( factory ), val( val ) {}

        uid clone() const;
};

#endif
