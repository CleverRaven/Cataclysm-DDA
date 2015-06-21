#ifndef CLZONES_H
#define CLZONES_H

#include "json.h"
#include "enums.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

/**
 * These are zones the player can designate.
 *
 * They currently don't serve much use, other than to designate
 * where to auto-pickup and where not to and restricting friendly npc pickup.
 */
class zone_manager : public JsonSerializer, public JsonDeserializer
{
    private:
        std::unordered_map<std::string, std::unordered_set<point> > mZones;
        std::vector<std::pair<std::string, std::string> > vZoneTypes;

    public:
        zone_manager();
        ~zone_manager() {};
        zone_manager(zone_manager &&) = default;
        zone_manager(const zone_manager &) = default;
        zone_manager &operator=(zone_manager &&) = default;
        zone_manager &operator=(const zone_manager &) = default;

        static zone_manager &get_manager() {
            static zone_manager manager;
            return manager;
        }

        class clZoneData
        {
            private:
                std::string sName;
                std::string sZoneType;
                bool bInvert;
                bool bEnabled;
                point pointStartXY;
                point pointEndXY;

            public:
                clZoneData()
                {
                    this->sName = "";
                    this->sZoneType = "";
                    this->bInvert = false;
                    this->bEnabled = false;
                    this->pointStartXY = point(-1, -1);
                    this->pointEndXY = point(-1, -1);
                }

                clZoneData(const std::string p_sName, const std::string p_sZoneType,
                           const bool p_bInvert, const bool p_bEnabled,
                           const point &p_pointStartXY, const point &p_pointEndXY)
                {
                    this->sName = p_sName;
                    this->sZoneType = p_sZoneType;
                    this->bInvert = p_bInvert;
                    this->bEnabled = p_bEnabled;
                    this->pointStartXY = p_pointStartXY;
                    this->pointEndXY = p_pointEndXY;
                }

                ~clZoneData() {};

                void setName();
                void setZoneType( const std::vector<std::pair<std::string, std::string> > &vZoneTypes );
                void setEnabled(const bool p_bEnabled);

                std::string getName() const
                {
                    return sName;
                }
                std::string getZoneType() const
                {
                    return sZoneType;
                }
                bool getInvert() const
                {
                    return bInvert;
                }
                bool getEnabled() const
                {
                    return bEnabled;
                }
                point getStartPoint() const
                {
                    return pointStartXY;
                }
                point getEndPoint() const
                {
                    return pointEndXY;
                }
                point getCenterPoint() const;
        };

        std::vector<clZoneData> vZones;

        void add(const std::string p_sName, const std::string p_sZoneType,
                 const bool p_bInvert, const bool p_bEnabled,
                 const point &p_pointStartXY, const point &p_pointEndXY)
        {
            vZones.push_back(clZoneData(p_sName, p_sZoneType,
                                        p_bInvert, p_bEnabled,
                                        p_pointStartXY, p_pointEndXY
                                       )
                            );
        }

        bool remove(const size_t iIndex)
        {
            if (iIndex < vZones.size()) {
                vZones.erase(vZones.begin() + iIndex);
                return true;
            }

            return false;
        }

        unsigned int size() const
        {
            return vZones.size();
        }
        std::vector<std::pair<std::string, std::string> > getZoneTypes() const
        {
            return vZoneTypes;
        }
        std::string getNameFromType( const std::string &p_sType ) const;
        bool hasType( const std::string &p_sType ) const;
        std::pair<point, point> bounding_box_type( const std::string &type ) const;
        void cacheZoneData();
        bool hasZone( const std::string &p_sType, const point p_pointInput ) const;

        bool save_zones();
        void load_zones();
        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const override;
        void deserialize(JsonIn &jsin) override;
};

#endif
