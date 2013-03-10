#include "game.h"
#include "setvector.h"

void game::init_monitems()
{
 setvector(monitems[mon_ant],
	mi_bugs,	1, NULL);
 monitems[mon_ant_soldier]	= monitems[mon_ant];
 monitems[mon_ant_queen]	= monitems[mon_ant];

 setvector(monitems[mon_zombie],
	mi_livingroom,	5,mi_kitchen,	 4,mi_bedroom,	16,mi_dresser,	16,
	mi_softdrugs,	 5,mi_harddrugs, 1,mi_tools,	 20,mi_trash,	 7,
	mi_ammo,	 1,mi_pistols,	 1,mi_shotguns,	 1,mi_smg,	 1,
	NULL);
 monitems[mon_zombie_shrieker]	= monitems[mon_zombie];
 monitems[mon_zombie_spitter]	= monitems[mon_zombie];
 monitems[mon_zombie_electric]	= monitems[mon_zombie];
 monitems[mon_zombie_fast]	= monitems[mon_zombie];
 monitems[mon_zombie_brute]	= monitems[mon_zombie];
 monitems[mon_zombie_hulk]	= monitems[mon_zombie];
 monitems[mon_zombie_fungus]	= monitems[mon_zombie];
 monitems[mon_boomer]		= monitems[mon_zombie];
 monitems[mon_boomer_fungus]	= monitems[mon_zombie];
 monitems[mon_zombie_necro]	= monitems[mon_zombie];
 monitems[mon_zombie_grabber]	= monitems[mon_zombie];
 monitems[mon_zombie_master]	= monitems[mon_zombie];
 monitems[mon_zombie_child] = monitems[mon_zombie];

 setvector(monitems[mon_beekeeper],
        mi_hive, 80, NULL);

 setvector(monitems[mon_zombie_cop],
        mi_cop_weapons, 20, NULL);

 setvector(monitems[mon_zombie_scientist],
	mi_dresser,	10,mi_harddrugs,	 6,mi_chemistry,	10,
	mi_teleport,	 6,mi_goo,		 8,mi_cloning_vat,	 1,
	mi_dissection,	10,mi_electronics,	 9,mi_bionics,		 1,
	mi_radio,	 2,mi_textbooks,	 3,NULL);

 setvector(monitems[mon_zombie_soldier],
	mi_dresser,	 5,mi_ammo,		10,mi_pistols,		 5,
	mi_shotguns,	 2,mi_smg,		 5,mi_bots,		 1,
	mi_launchers,	 2,mi_mil_rifles,	10,mi_grenades,		 5,
	mi_mil_armor,	14,mi_mil_food,		 5,mi_bionics_mil,	 1,
	NULL);

 setvector(monitems[mon_biollante],
	mi_biollante, 1, NULL);

 setvector(monitems[mon_chud],
	mi_subway,	40,mi_sewer,	20,mi_trash,	 5,mi_bedroom,	 1,
	mi_dresser,	 5,mi_ammo,	18, NULL);
 monitems[mon_one_eye]		= monitems[mon_chud];

 setvector(monitems[mon_bee],
	mi_bees,	1, NULL);

 setvector(monitems[mon_wasp],
	mi_wasps,	1, NULL);

 setvector(monitems[mon_dragonfly],
	mi_bugs,	1, NULL);
 monitems[mon_centipede]	= monitems[mon_dragonfly];
 monitems[mon_spider_wolf]	= monitems[mon_dragonfly];
 monitems[mon_spider_web]	= monitems[mon_dragonfly];
 monitems[mon_spider_jumping]	= monitems[mon_dragonfly];
 monitems[mon_spider_trapdoor]	= monitems[mon_dragonfly];
 monitems[mon_spider_widow]	= monitems[mon_dragonfly];

 setvector(monitems[mon_eyebot],
           mi_robots,4, mi_eyebot, 1,NULL);

 setvector(monitems[mon_manhack],
           mi_robots,4, mi_manhack, 1, NULL);

 setvector(monitems[mon_skitterbot],
           mi_robots,4, mi_skitterbot, 1, NULL);

 setvector(monitems[mon_secubot],
           mi_robots,4, mi_secubot, 1, NULL);

 setvector(monitems[mon_copbot],
           mi_robots,4, mi_copbot, 1, NULL);

 setvector(monitems[mon_molebot],
           mi_robots,4, mi_molebot, 1, NULL);

 setvector(monitems[mon_tripod],
           mi_robots,4, mi_tripod, 1, NULL);

 setvector(monitems[mon_chickenbot],
           mi_robots,4, mi_chickenbot, 1, NULL);

 setvector(monitems[mon_tankbot],
           mi_robots,4, mi_tankbot, 1, NULL);

 setvector(monitems[mon_turret],
           mi_robots,4, mi_turret, 1, NULL);

}
