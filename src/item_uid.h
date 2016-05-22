#ifndef ITEM_UID_H
#define ITEM_UID_H

#include <limits>

#include "json.h"

class item_uid : public JsonSerializer, public JsonDeserializer
{
        friend class game;

    public:
        item_uid() = default;

        item_uid( item_uid && ) = default;
        item_uid &operator=( item_uid && ) = default;

        item_uid( const item_uid & );
        item_uid &operator=( const item_uid & );

        bool operator==( const item_uid &rhs ) const {
            return val == rhs.val;
        }
        bool operator!=( const item_uid &rhs ) const {
            return val != rhs.val;
        }
        bool operator< ( const item_uid &rhs ) const {
            return val <  rhs.val;
        }

        void serialize( JsonOut &js ) const;
        void deserialize( JsonIn &js );

    private:
        item_uid( long long val ) : val( val ) {}
        long long val = std::numeric_limits<long long>::min();
};

#endif
