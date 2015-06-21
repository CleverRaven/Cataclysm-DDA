#include "clzones.h"
#include "map.h"
#include "game.h"
#include "debug.h"
#include "output.h"
#include "mapsharing.h"
#include "translations.h"
#include "worldfactory.h"
#include "catacharset.h"
#include "ui.h"
#include <iostream>
#include <fstream>

zone_manager::zone_manager()
{
    //Add new zone types here
    //Use: if (g->checkZone("ZONE_NAME", posx, posy)) {
    //to check for a zone

    vZoneTypes.push_back( std::make_pair( _("No Auto Pickup"), "NO_AUTO_PICKUP" ) );
    vZoneTypes.push_back( std::make_pair( _("No NPC Pickup"),  "NO_NPC_PICKUP" ) );
}

void zone_manager::clZoneData::setName()
{
    const std::string sName = string_input_popup(_("Zone name:"), 55, this->sName, "", "", 15);

    this->sName = (sName == "") ? _("<no name>") : sName;
}

void zone_manager::clZoneData::setZoneType(
    const std::vector<std::pair<std::string, std::string> > &vZoneTypes )
{
    uimenu as_m;
    as_m.text = _("Select zone type:");

    for (unsigned int i = 0; i < vZoneTypes.size(); ++i) {
        as_m.entries.push_back(uimenu_entry(i + 1, true, (char)i + 1, vZoneTypes[i].first));
    }
    as_m.query();

    this->sZoneType = vZoneTypes[((as_m.ret >= 1) ? as_m.ret : 1) - 1].second;
}

void zone_manager::clZoneData::setEnabled(const bool p_bEnabled)
{
    this->bEnabled = p_bEnabled;
}

point zone_manager::clZoneData::getCenterPoint() const
{
    return point((pointStartXY.x + pointEndXY.x) / 2, (pointStartXY.y + pointEndXY.y) / 2);
}

std::string zone_manager::getNameFromType( const std::string &type ) const
{
    for( auto &elem : vZoneTypes ) {
        if( elem.second == type ) {
            return elem.first;
        }
    }

    return "Unknown Type";
}

bool zone_manager::hasType( const std::string &type ) const
{
    for( auto &elem : vZoneTypes ) {
        if( elem.second == type ) {
            return true;
        }
    }

    return false;
}

void zone_manager::cacheZoneData()
{
    mZones.clear();

    for( auto &elem : vZones ) {
        if( elem.getEnabled() ) {
            const std::string sType = elem.getZoneType();

            point pStart = elem.getStartPoint();
            point pEnd = elem.getEndPoint();

            //draw marked area
            for (int iY = pStart.y; iY <= pEnd.y; ++iY) {
                for (int iX = pStart.x; iX <= pEnd.x; ++iX) {
                    mZones[sType].insert( point{ iX, iY } );
                }
            }
        }
    }
}

bool zone_manager::hasZone( const std::string &type, const point point_input ) const
{
    const auto type_iter = mZones.find( type );
    if( type_iter == mZones.end() ) {
        return false;
    }

    const auto &point_set = type_iter->second;
    return point_set.find( point_input ) != point_set.end();
}

void zone_manager::serialize(JsonOut &json) const
{
    json.start_array();
    for( auto &elem : vZones ) {
        json.start_object();

        json.member( "name", elem.getName() );
        json.member( "type", elem.getZoneType() );
        json.member( "invert", elem.getInvert() );
        json.member( "enabled", elem.getEnabled() );

        point pointStart = elem.getStartPoint();
        point pointEnd = elem.getEndPoint();

        json.member("start_x", pointStart.x);
        json.member("start_y", pointStart.y);
        json.member("end_x", pointEnd.x);
        json.member("end_y", pointEnd.y);

        json.end_object();
    }

    json.end_array();
}

void zone_manager::deserialize(JsonIn &jsin)
{
    vZones.clear();

    jsin.start_array();
    while (!jsin.end_array()) {
        JsonObject joZone = jsin.get_object();

        const std::string sName = joZone.get_string("name");
        const std::string sType = joZone.get_string("type");

        const bool bInvert = joZone.get_bool("invert");
        const bool bEnabled = joZone.get_bool("enabled");

        const int iStartX = joZone.get_int("start_x");
        const int iStartY = joZone.get_int("start_y");
        const int iEndX = joZone.get_int("end_x");
        const int iEndY = joZone.get_int("end_y");

        if (hasType(sType)) {
            add(sName, sType, bInvert, bEnabled, point(iStartX, iStartY), point(iEndX, iEndY));
        }
    }

    cacheZoneData();
}


bool zone_manager::save_zones()
{
    cacheZoneData();

    std::string savefile = world_generator->active_world->world_path + "/" + base64_encode(
                               g->u.name) + ".zones.json";

    try {
        std::ofstream fout;
        fout.exceptions(std::ios::badbit | std::ios::failbit);

        fopen_exclusive(fout, savefile.c_str());
        if(!fout.is_open()) {
            return true; //trick game into thinking it was saved
        }

        fout << serialize();
        fclose_exclusive(fout, savefile.c_str());
        return true;

    } catch(std::ios::failure &) {
        popup(_("Failed to save zones to %s"), savefile.c_str());
        return false;
    }
}

void zone_manager::load_zones()
{
    std::string savefile = world_generator->active_world->world_path + "/" + base64_encode(
                               g->u.name) + ".zones.json";

    std::ifstream fin;
    fin.open(savefile.c_str(), std::ifstream::in | std::ifstream::binary);
    if(!fin.good()) {
        fin.close();
        return;
    }

    try {
        JsonIn jsin(fin);
        deserialize(jsin);
    } catch (std::string e) {
        DebugLog(D_ERROR, DC_ALL) << "load_zones: " << e;
    }

    fin.close();
}
