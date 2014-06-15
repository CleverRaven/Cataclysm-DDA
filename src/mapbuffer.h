#include "line.h"
#include <map>
#include <list>

struct pointcomp {
    bool operator() (const tripoint &lhs, const tripoint &rhs) const
    {
        if (lhs.x < rhs.x) {
            return true;
        }
        if (lhs.x > rhs.x) {
            return false;
        }
        if (lhs.y < rhs.y) {
            return true;
        }
        if (lhs.y > rhs.y) {
            return false;
        }
        if (lhs.z < rhs.z) {
            return true;
        }
        if (lhs.z > rhs.z) {
            return false;
        }
        return false;
    };
};

struct submap;

/**
 * Store, buffer, save and load the entire world map.
 */
class mapbuffer
{
    public:
        mapbuffer();
        ~mapbuffer();

        /** Load the entire world from savefiles into submaps in this instance. **/
        void load(std::string worldname);
        /** Store all submaps in this instance into savefiles. **/
        void save( bool delete_after_save = false );

        /** Delete all buffered submaps. **/
        void reset();

        /** Add a new submap to the buffer.
         *
         * @param x, y, z The absolute world position in submap coordinates.
         */
        bool add_submap(int x, int y, int z, submap *sm);

        /** Get a submap stored in this buffer.
         *
         * @param x, y, z The absolute world position in submap coordinates.
         */
        submap *lookup_submap(int x, int y, int z);

    private:
        // There's a very good reason this is private,
        // if not handled carefully, this can erase in-use submaps and crash the game.
        void remove_submap( tripoint addr );
        submap *unserialize_submaps( const tripoint &p );
        void save_quad( const std::string &filename, const tripoint &om_addr,
                        std::list<tripoint> &submaps_to_delete, bool delete_after_save );
        std::map<tripoint, submap *, pointcomp> submaps;
        std::list<submap *> submap_list;
};

extern mapbuffer MAPBUFFER;
