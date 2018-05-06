#pragma once
#ifndef UISTATE_H
#define UISTATE_H

#include <typeinfo>
#include <list>

#include "enums.h"
#include "omdata.h"
#include "pimpl.h"

#include <map>
#include <vector>
#include <string>

class ammunition_type;
using ammotype = string_id<ammunition_type>;

class item;
class JsonIn;
class JsonOut;
class JsonSerDes;

class advanced_inv_uistatedata;
class construction_uistatedata;
class crafting_uistatedata;
class editmap_uistatedata;
class game_uistatedata;
class iexamine_uistatedata;
class overmap_uistatedata;
class player_uistatedata;
class wish_uistatedata;

/*
  centralized depot for trivial ui data such as sorting, string_input_popup history, etc.
  To use this, see the ****notes**** below
*/
class uistatedata
{
    protected:
        std::vector<std::unique_ptr<JsonSerDes>> modules;
    public:
        uistatedata();
    public:
        pimpl<advanced_inv_uistatedata> advanced_inv;
        pimpl<construction_uistatedata> construction;
        pimpl<crafting_uistatedata> crafting;
        pimpl<editmap_uistatedata> editmap;
        pimpl<game_uistatedata> game;
        pimpl<iexamine_uistatedata> iexamine;
        pimpl<overmap_uistatedata> overmap;
        pimpl<player_uistatedata> player;
        pimpl<wish_uistatedata> wish;
        /**** this will set a default value on startup, however to save, see below ****/
    public:
        /**** declare your variable here. It can be anything, really *****/
        bool debug_ranged;
        int last_inv_start = -2;
        int last_inv_sel = -2;

        // internal stuff
        bool _testing_save = true; // internal: whine on json errors. set false if no complaints in 2 weeks.
        bool _really_testing_save = false; // internal: spammy
    public:
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};
extern uistatedata uistate;

#endif
