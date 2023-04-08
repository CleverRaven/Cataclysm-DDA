#pragma once
#ifndef CATA_TEST_DATA_H
#define CATA_TEST_DATA_H

#include <map>
#include <set>
#include <vector>

#include "type_id.h"

class JsonObject;

struct efficiency_data {
    std::vector<int> forward;
    std::vector<int> reverse;

    void deserialize( const JsonObject &jo );
};

class test_data
{
    public:
        // todo: remove when all known bad items got fixed
        static std::set<itype_id> known_bad;
        static std::map<vproto_id, std::vector<double>> drag_data;
        static std::map<vproto_id, efficiency_data> eff_data;
        static std::map<itype_id, double> expected_dps;

        static void load( const JsonObject &jo );
};

#endif // CATA_TEST_DATA_H
