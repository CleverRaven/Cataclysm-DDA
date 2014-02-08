#include "map.h"
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

/**
 * Store, buffer, save and load the entire world map.
 */
class mapbuffer
{
    public:
        mapbuffer();
        ~mapbuffer();

        /** Tells the mapbuffer that there is some unsaved change.
         *
         *  The next time save_if_dirty is called, the mapbuffer will be saved.
         */
        void set_dirty();

        /** Clears the dirty flag. **/
        void make_volatile();

        /** Load the entire world from savefiles into submaps in this instance. **/
        void load(std::string worldname);
        /** Store all submaps in this instance into savefiles. **/
        void save( bool delete_after_save = false );

        /** Save only if the dirty flag is set. **/
        void save_if_dirty();

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

        /** Returns the amount of buffered submaps. **/
        int size();

    private:
        // There's a very good reason this is private,
        // if not handled carefully, this can erase in-use submaps and crash the game.
        void remove_submap( tripoint addr );
        int unserialize_keys( std::ifstream &fin );
        void unserialize_submaps( std::ifstream &fin, const int num_submaps );
        bool unserialize_legacy(std::ifstream &fin);
        void save_quad( std::ofstream &fout, const tripoint &om_addr, bool delete_after_save );
        std::map<tripoint, submap *, pointcomp> submaps;
        std::list<submap *> submap_list;
        bool dirty;
        std::map<int, int> ter_key;
        std::map<int, int> furn_key;
        std::map<int, int> trap_key;
};

extern mapbuffer MAPBUFFER;
