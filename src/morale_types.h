#pragma once
#ifndef CATA_SRC_MORALE_TYPES_H
#define CATA_SRC_MORALE_TYPES_H

#include <string>

#include "translations.h"
#include "type_id.h"

class JsonObject;
struct itype;

class morale_type_data
{
    private:
        bool permanent = false;
        // May contain '%s' format string
        translation text;
    public:
        morale_type id;
        bool was_loaded = false;

        /** Describes this morale type, with item type to replace wildcard with. */
        std::string describe( const itype *it = nullptr ) const;
        bool is_permanent() const {
            return permanent;
        }

        void load( const JsonObject &jo, const std::string &src );
        void check() const;

        static void load_type( const JsonObject &jo, const std::string &src );
        static void check_all();
        static void reset();

        static const morale_type &convert_legacy( int lmt );
};

// Legacy crap - get rid of it when possible
extern const morale_type MORALE_NULL;
extern const morale_type MORALE_FOOD_GOOD;
extern const morale_type MORALE_FOOD_HOT;
extern const morale_type MORALE_ATE_WITH_TABLE;
extern const morale_type MORALE_ATE_WITHOUT_TABLE;
extern const morale_type MORALE_MUSIC;
extern const morale_type MORALE_HONEY;
extern const morale_type MORALE_GAME;
extern const morale_type MORALE_MARLOSS;
extern const morale_type MORALE_MUTAGEN;
extern const morale_type MORALE_FEELING_GOOD;
extern const morale_type MORALE_SUPPORT;
extern const morale_type MORALE_PHOTOS;
extern const morale_type MORALE_CRAVING_NICOTINE;
extern const morale_type MORALE_CRAVING_CAFFEINE;
extern const morale_type MORALE_CRAVING_ALCOHOL;
extern const morale_type MORALE_CRAVING_OPIATE;
extern const morale_type MORALE_CRAVING_SPEED;
extern const morale_type MORALE_CRAVING_COCAINE;
extern const morale_type MORALE_CRAVING_CRACK;
extern const morale_type MORALE_CRAVING_MUTAGEN;
extern const morale_type MORALE_CRAVING_DIAZEPAM;
extern const morale_type MORALE_CRAVING_MARLOSS;
extern const morale_type MORALE_FOOD_BAD;
extern const morale_type MORALE_CANNIBAL;
extern const morale_type MORALE_VEGETARIAN;
extern const morale_type MORALE_MEATARIAN;
extern const morale_type MORALE_ANTIFRUIT;
extern const morale_type MORALE_LACTOSE;
extern const morale_type MORALE_ANTIJUNK;
extern const morale_type MORALE_ANTIWHEAT;
extern const morale_type MORALE_SWEETTOOTH;
extern const morale_type MORALE_NO_DIGEST;
extern const morale_type MORALE_WET;
extern const morale_type MORALE_DRIED_OFF;
extern const morale_type MORALE_COLD;
extern const morale_type MORALE_HOT;
extern const morale_type MORALE_FEELING_BAD;
extern const morale_type MORALE_KILLED_INNOCENT;
extern const morale_type MORALE_KILLED_FRIEND;
extern const morale_type MORALE_KILLED_MONSTER;
extern const morale_type MORALE_MUTILATE_CORPSE;
extern const morale_type MORALE_MUTAGEN_ELF;
extern const morale_type MORALE_MUTAGEN_CHIMERA;
extern const morale_type MORALE_MUTAGEN_MUTATION;
extern const morale_type MORALE_MOODSWING;
extern const morale_type MORALE_BOOK;
extern const morale_type MORALE_COMFY;
extern const morale_type MORALE_SCREAM;
extern const morale_type MORALE_PERM_MASOCHIST;
extern const morale_type MORALE_PERM_NOFACE;
extern const morale_type MORALE_PERM_FPMODE_ON;
extern const morale_type MORALE_PERM_HOARDER;
extern const morale_type MORALE_PERM_FANCY;
extern const morale_type MORALE_PERM_OPTIMIST;
extern const morale_type MORALE_PERM_BADTEMPER;
extern const morale_type MORALE_PERM_CONSTRAINED;
extern const morale_type MORALE_PERM_NOMAD;
extern const morale_type MORALE_GAME_FOUND_KITTEN;
extern const morale_type MORALE_HAIRCUT;
extern const morale_type MORALE_SHAVE;
extern const morale_type MORALE_CHAT;
extern const morale_type MORALE_VOMITED;
extern const morale_type MORALE_PLAY_WITH_PET;
extern const morale_type MORALE_PYROMANIA_STARTFIRE;
extern const morale_type MORALE_PYROMANIA_NEARFIRE;
extern const morale_type MORALE_PYROMANIA_NOFIRE;
extern const morale_type MORALE_KILLER_HAS_KILLED;
extern const morale_type MORALE_KILLER_NEED_TO_KILL;
extern const morale_type MORALE_PERM_FILTHY;
extern const morale_type MORALE_PERM_DEBUG;
extern const morale_type MORALE_BUTCHER;
extern const morale_type MORALE_GRAVEDIGGER;
extern const morale_type MORALE_FUNERAL;
extern const morale_type MORALE_TREE_COMMUNION;
extern const morale_type MORALE_ACCOMPLISHMENT;
extern const morale_type MORALE_FAILURE;

#endif // CATA_SRC_MORALE_TYPES_H
