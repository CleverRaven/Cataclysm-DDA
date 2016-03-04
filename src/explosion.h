#ifndef EXPLOSION_H
#define EXPLOSION_H

using itype_id = std::string;

class JsonObject;

struct shrapnel_data {
    int count           = 0;
    int mass            = 10;
    // Percentage
    int recovery        = 0;
    itype_id drop       = "null";
};

struct explosion_data {
    float power             = -1.0f;
    float distance_factor   = 0.8f;
    bool fire               = false;
    shrapnel_data shrapnel;
};

shrapnel_data load_shrapnel_data( JsonObject &jo );
explosion_data load_explosion_data( JsonObject &jo );

#endif
