#ifndef uuid_h
#define uuid_h

#include <cstdint>
#include <cstring>

#include "json.h"

/**
 * An RFC4122 v4 UUID that uniquely identifies instances of a class within a world.
 * Unlike a C++ pointer it can be serialized to JSON.
 */
class uuid : public JsonSerializer, public JsonDeserializer
{
        friend class item_location; /** needs clone() */

    public:
        uuid();

        uuid( const uuid & );
        uuid &operator=( const uuid & );

        uuid( uuid && ) = default;
        uuid &operator=( uuid && ) = default;

        bool operator==( const uuid &rhs ) const {
            return memcmp( data, rhs.data, sizeof( data ) ) == 0;
        }
        bool operator!=( const uuid &rhs ) const {
            return memcmp( data, rhs.data, sizeof( data ) ) != 0;
        }
        bool operator< ( const uuid &rhs ) const {
            return memcmp( data, rhs.data, sizeof( data ) ) < 0;
        }

        /** output as 36 character RFC4122 v4 string */
        operator std::string() const;

        void serialize( JsonOut &js ) const;
        void deserialize( JsonIn &js );

        /** call before constructing any instances to seed rng */
        static void init();

    private:
        uint32_t data[4];

        uuid clone() const {
            uuid res;
            memcpy( res.data, this->data, sizeof( res.data ) );
            return res;
        }
};

#endif