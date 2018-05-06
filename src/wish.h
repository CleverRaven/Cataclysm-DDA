#pragma once
#ifndef WISH_H
#define WISH_H

class JsonIn;
class JsonOut;

class wish_uistatedata
{
    public:
        int wishitem_selected = 0;
        int wishmutate_selected = 0;
        int wishmonster_selected = 0;
    public:
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};

#endif
