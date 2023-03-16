/** \page map_management Map Management
 *  \brief Coordinate systems, overmaps, maps, submaps
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

/*! @page Effects_Stat_Strength Stat: Strength
 *  @brief Effects of the Strength stat.
 * */
/*! @page Effects_Stat_Dexterity Stat: Dexterity
 *  @brief Effects of the Dexterity stat.
 * */
/*! @page Effects_Stat_Intelligence Stat: Intelligence
 *  @brief Effects of the Intelligence stat.
 * */
/*! @page Effects_Stat_Perception Stat: Perception
 *  @brief Effects of the Perception stat.
 * */

/*! @page Effects_Skill_Barter Skill: Barter
 *  @brief Effects of the Barter skill.
 * */
/*! @page Effects_Skill_Computer Skill: Computer
 *  @brief Effects of the Computer skill.
 * */
/*! @page Effects_Skill_Carpentry Skill: Carpentry
 *  @brief Effects of the Carpentry skill.
 * */
/*! @page Effects_Skill_Cooking Skill: Cooking
 *  @brief Effects of the Cooking skill.
 * */
/*! @page Effects_Skill_Driving Skill: Driving
 *  @brief Effects of the Driving skill.
 * */
/*! @page Effects_Skill_Electronics Skill: Electronics
 *  @brief Effects of the Electronics skill.
 * */
/*! @page Effects_Skill_Fabrication Skill: Fabrication
 *  @brief Effects of the Fabrication skill.
 * */
/*! @page Effects_Skill_Firstaid Skill: Firstaid
 *  @brief Effects of the Firstaid skill.
 * */
/*! @page Effects_Skill_Mechanics Skill: Mechanics
 *  @brief Effects of the Mechanics skill.
 * */
/*! @page Effects_Skill_Speech Skill: Speech
 *  @brief Effects of the Speech skill.
 * */
/*! @page Effects_Skill_Survival Skill: Survival
 *  @brief Effects of the Survival skill.
 * */
/*! @page Effects_Skill_Swimming Skill: Swimming
 *  @brief Effects of the Swimming skill.
 * */
/*! @page Effects_Skill_Tailor Skill: Tailor
 *  @brief Effects of the Tailor skill.
 * */
/*! @page Effects_Skill_Traps Skill: Traps
 *  @brief Effects of the Traps skill.
 * */
/*! @page Effects_Skill_Archery Skill: Archery
 *  @brief Effects of the Archery skill.
 * */
/*! @page Effects_Skill_Bashing Skill: Bashing
 *  @brief Effects of the Bashing skill.
 * */
/*! @page Effects_Skill_Cutting Skill: Cutting
 *  @brief Effects of the Cutting skill.
 * */
/*! @page Effects_Skill_Dodge Skill: Dodge
 *  @brief Effects of the Dodge skill.
 * */
/*! @page Effects_Skill_Gun Skill: Gun
 *  @brief Effects of the Gun skill.
 * */
/*! @page Effects_Skill_Launcher Skill: Launcher
 *  @brief Effects of the Launcher skill.
 * */
/*! @page Effects_Skill_Melee Skill: Melee
 *  @brief Effects of the Melee skill.
 * */
/*! @page Effects_Skill_Stabbing Skill: Stabbing
 *  @brief Effects of the Stabbing skill.
 * */
/*! @page Effects_Skill_Throw Skill: Throw
 *  @brief Effects of the Throw skill.
 * */
/*! @page Effects_Skill_Unarmed Skill: Unarmed
 *  @brief Effects of the Unarmed skill.
 * */
/*! @page Effects_Skill_Pistol Skill: Pistol
 *  @brief Effects of the Pistol skill.
 * */
/*! @page Effects_Skill_Rifle Skill: Rifle
 *  @brief Effects of the Rifle skill.
 * */
/*! @page Effects_Skill_Shotgun Skill: Shotgun
 *  @brief Effects of the Shotgun skill.
 * */
/*! @page Effects_Skill_Smg Skill: Smg
 *  @brief Effects of the Smg skill.
 * */
/*! @page Effects_Skill_Lockpick Skill: Lockpick
 *  @brief Effects of the Lock picking skill.
 * */
/*! @page Effects_Skill_Chemistry Skill: Chemistry
 *  @brief Effects of the Chemistry skill.
 * */

/*! @page Effects_Skill_Multiple Skill: Multiple
 *  @brief Effects that apply to multiple skills.
 * */

/*! @page Effects_Proficiency_Disarming Proficiency: Disarming
 *  @brief Effects of the Disarming proficiency
 * */
/*! @page Effects_Proficiency_Lockpicking Proficiency: Lockpicking
 *  @brief Effects of the Lockpicking proficiency
 * */
/*! @page Effects_Proficiency_Parkour Proficiency: Parkour
 *  @brief Effects of the Parkour proficiency
 * */
/*! @page Effects_Proficiency_Safecracking Proficiency: Safecracking
 *  @brief Effects of the Safecracking proficiency
 * */
/*! @page Effects_Proficiency_Spotting Proficiency: Spotting
 *  @brief Effects of the Spotting proficiency
 * */
/*! @page Effects_Proficiency_Traps Proficiency: Traps
 *  @brief Effects of the Traps proficiency
 * */
/*! @page Effects_Proficiency_TrapSetting Proficiency: TrapSetting
 *  @brief Effects of the TrapSetting proficiency
 * */
/*! @page Effects_Proficiency_Wound_Care Proficiency: Wound Care
 *  @brief Effects of the Wound Care proficiency
 * */

/*! @page Effects_Trait_Bad_Knees Trait: Bad Knees
 *  @brief Effects of the Bad Knees trait
 * */
/*! @page Effects_Trait_Clumsy Trait: Clumsy
 *  @brief Effects of the Clumsy trait
 * */
/*! @page Effects_Trait_Light_Step Trait: Light Step
 *  @brief Effects of the Light_Step trait
 * */

/*! @page Effects_Trait_Profession_Autodoc Profession: Autodoc
 *  @brief Effects of the Autodoc profession
 * */
/*! @page Effects_Trait_Profession_Churl Profession: Churl
 *  @brief Effects of the Churl profession
 * */
/*! @page Effects_Trait_Profession_Doctor Profession: Doctor
 *  @brief Effects of the Doctor profession
 * */

/*! @page Effects_Trait_Paws Trait: Paws
 *  @brief Effects of Paws traits
 * */
/*! @page Effects_Trait_Tail Trait: Tail
 *  @brief Effects of Tail traits
 * */
/*! @page Effects_Trait_Beak Trait: Beak
 *  @brief Effects of Beak traits
 * */
/*! @page Effects_Trait_Arm Trait: Arm
 *  @brief Effects of Arm traits
 * */
/*! @page Effects_Trait_Claws Trait: Claws
 *  @brief Effects of Claws traits
 * */
/*! @page Effects_Trait_Vines Trait: Vines
 *  @brief Effects of Vines traits
 * */

/*! @page Effects_Focus Focus
 *  @brief Effects of Focus
 *  @par asdf
 */
/*! @page Effects_Speed Speed
 *  @brief Effects of Speed
 *  @par
 *  Speed affects how fast the character performs most actions. Lower is faster.
 */
/*! @page Effects_Fatigue Fatigue
 *  @brief Effects of Fatigue
 */
