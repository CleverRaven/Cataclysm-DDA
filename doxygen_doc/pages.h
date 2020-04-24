/** \page map_management Map Management
 *
 * Cataclysm DDA uses a shifting coordinate system. This means that when the player moves to a new
 * submap, the (0, 0) coordinate moves from one submap to another, and the coordinates of all entities
 * on the map are shifted to accommodate this.
 *
 * Furthermore, the map is split into a hierarchical structure. At the top-level there are "overmaps",
 * which are very large chunks of the gameworld. The overmap is split into overmap tiles. When you use 'm'
 * to view the map in-game, what you see are overmap tiles. Each overmap tile is then again split into
 * four submaps (2x2). See \ref overmap and \ref submap.
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

/*! @page Effects_Stat_Strength
 *  @brief Cross referenced effects of the Strength stat.
 *  @par
 */
/*! @page Effects_Stat_Dexterity
 *  @brief Cross referenced effects of the Dexterity stat.
 *  @par
 */
/*! @page Effects_Stat_Intelligence
 *  @brief Cross referenced effects of the Intelligence stat.
 *  @par
 */
/*! @page Effects_Stat_Perception
 *  @brief Cross referenced effects of the Perception stat.
 *  @par
 */
/*! @page Effects_Skill_Barter
 *  @brief Cross referenced effects of the Barter skill.
 *  @par
 */
/*! @page Effects_Skill_Computer
 *  @brief Cross referenced effects of the Computer skill.
 *  @par
 */
/*! @page Effects_Skill_Carpentry
 *  @brief Cross referenced effects of the Carpentry skill.
 *  @par
 */
/*! @page Effects_Skill_Cooking
 *  @brief Cross referenced effects of the Cooking skill.
 *  @par
 */
/*! @page Effects_Skill_Driving
 *  @brief Cross referenced effects of the Driving skill.
 *  @par
 */
/*! @page Effects_Skill_Electronics
 *  @brief Cross referenced effects of the Electronics skill.
 *  @par
 */
/*! @page Effects_Skill_Fabrication
 *  @brief Cross referenced effects of the Fabrication skill.
 *  @par
 */
/*! @page Effects_Skill_Firstaid
 *  @brief Cross referenced effects of the Firstaid skill.
 *  @par
 */
/*! @page Effects_Skill_Mechanics
 *  @brief Cross referenced effects of the Mechanics skill.
 *  @par
 */
/*! @page Effects_Skill_Speech
 *  @brief Cross referenced effects of the Speech skill.
 *  @par
 */
/*! @page Effects_Skill_Survival
 *  @brief Cross referenced effects of the Survival skill.
 *  @par
 */
/*! @page Effects_Skill_Swimming
 *  @brief Cross referenced effects of the Swimming skill.
 *  @par
 */
/*! @page Effects_Skill_Tailor
 *  @brief Cross referenced effects of the Tailor skill.
 *  @par
 */
/*! @page Effects_Skill_Traps
 *  @brief Cross referenced effects of the Traps skill.
 *  @par
 */
/*! @page Effects_Skill_Archery
 *  @brief Cross referenced effects of the Archery skill.
 *  @par
 */
/*! @page Effects_Skill_Bashing
 *  @brief Cross referenced effects of the Bashing skill.
 *  @par
 */
/*! @page Effects_Skill_Cutting
 *  @brief Cross referenced effects of the Cutting skill.
 *  @par
 */
/*! @page Effects_Skill_Dodge
 *  @brief Cross referenced effects of the Dodge skill.
 *  @par
 */
/*! @page Effects_Skill_Gun
 *  @brief Cross referenced effects of the Gun skill.
 *  @par
 */
/*! @page Effects_Skill_Launcher
 *  @brief Cross referenced effects of the Launcher skill.
 *  @par
 */
/*! @page Effects_Skill_Melee
 *  @brief Cross referenced effects of the Melee skill.
 *  @par
 */
/*! @page Effects_Skill_Stabbing
 *  @brief Cross referenced effects of the Stabbing skill.
 *  @par
 */
/*! @page Effects_Skill_Throw
 *  @brief Cross referenced effects of the Throw skill.
 *  @par
 */
/*! @page Effects_Skill_Unarmed
 *  @brief Cross referenced effects of the Unarmed skill.
 *  @par
 */
/*! @page Effects_Skill_Pistol
 *  @brief Cross referenced effects of the Pistol skill.
 *  @par
 */
/*! @page Effects_Skill_Rifle
 *  @brief Cross referenced effects of the Rifle skill.
 *  @par
 */
/*! @page Effects_Skill_Shotgun
 *  @brief Cross referenced effects of the Shotgun skill.
 *  @par
 */
/*! @page Effects_Skill_Smg
 *  @brief Cross referenced effects of the Smg skill.
 *  @par
 */
 *! @page Effects_Skill_Lockpick
 *  @brief Cross referenced effects of the Lock picking skill.
 *  @par
 */
