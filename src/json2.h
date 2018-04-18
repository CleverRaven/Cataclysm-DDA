#ifndef JSON2_H
#define JSON2_H

#include "json.h"

/* JsonSerDes
 * ==========
 */
class JsonSerDes : public JsonSerializer, public JsonDeserializer
{
    public:
        virtual ~JsonSerDes() = default;
        JsonSerDes() = default;
        JsonSerDes( JsonSerDes && ) = default;
        JsonSerDes( const JsonSerDes & ) = default;
        JsonSerDes &operator=( JsonSerDes && ) = default;
        JsonSerDes &operator=( const JsonSerDes & ) = default;
};

/* JsonSerDesAdapter
 * ===============
 */
template<typename T>
class JsonSerDesAdapter: public JsonSerDes
{
    protected:
        T &data;
    public:
        JsonSerDesAdapter( T &data ) : data( data ) { };
    public:
        virtual void serialize( JsonOut &json ) const override {
            data.serialize( json );
        };
        virtual void deserialize( JsonIn &jsin ) override {
            data.deserialize( jsin );
        };
};

#endif
