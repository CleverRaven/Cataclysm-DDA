/** \page map_management Map Management
 *
 * Cataclysm DDA uses a shifting coordinate system. This means that when the player moves to a new
 * submap, the (0, 0) coordinate moves from one submap to another, and the coordinates of all entities
 * on the map are shifted to accommodate this.
 *
 * Furthermore, the map is split into a hierarchical structure. At the top-level there are "overmaps",
 * which are very large chunks of the gameworld. The overmap is split into overmap tiles. When you use 'm'
 * to view the map in-game, what you see are overmap tiles. Each overmap tile is then again split into
 * two submaps. See \ref overmap and \ref submap.
 *
 * Parallel to this, there is the \ref mapbuffer. The mapbuffer manages and stores all submaps. Note that
 * this is separate to the overmap, which stores overmap tiles. The mapbuffer simply loads all submaps that
 * have ever been created into memory and keeps them there in a list. It does not "sort" them into overmaps
 * or anything like that.
 *
 * Again parallel to this, there is \ref map and `g->m`. A map contains a 2D pointer-array of submaps, for instance
 * 11x11 submaps. These are usually centered around the player, which is the reason the coordinate system shifts.
 * For all these submaps, it also caches additional data, such as lighting, scent, etc. This is basically
 * where all the "on-screen" processing happens. Most of the source code modifies terrain, monsters and so on only by
 * creating a map instance and using its methods.
 *
 * \section related_classes Related Classes
 * \ref mapbuffer \ref map \ref submap
 */
