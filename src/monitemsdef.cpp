#include "game.h"
#include "setvector.h"

void game::init_monitems() {

    // This seems like a good candidate for json.

    // for zombies only: clothing is generated separately upon mondeath
    // monitems should therefore not include main clothing items but extra items
    // that zombies might be carrying

    setvector(&monitems["mon_zombie"],
              "livingroom", 5, "kitchen",  4, "bedroom", 16,
              "softdrugs",  5, "harddrugs", 1, "tools", 20, "trash", 7,
              "ammo", 1, "pistols", 1, "shotguns", 1, "smg", 1,
              NULL);

    setvector(&monitems["mon_zombie_child"], "child_items", 65, NULL);

    monitems["mon_zombie_shrieker"]   = monitems["mon_zombie"];
    monitems["mon_zombie_spitter"]    = monitems["mon_zombie"];
    monitems["mon_zombie_electric"]   = monitems["mon_zombie"];
    monitems["mon_zombie_brute"]      = monitems["mon_zombie"];
    monitems["mon_zombie_hulk"]       = monitems["mon_zombie"];
    monitems["mon_zombie_fungus"]     = monitems["mon_zombie"];
    monitems["mon_boomer"]            = monitems["mon_zombie"];
    monitems["mon_boomer_fungus"]     = monitems["mon_zombie"];
    monitems["mon_zombie_necro"]      = monitems["mon_zombie"];
    monitems["mon_zombie_grabber"]    = monitems["mon_zombie"];
    monitems["mon_zombie_master"]     = monitems["mon_zombie"];
    monitems["mon_zombie_hunter"]     = monitems["mon_zombie"];

    setvector(&monitems["mon_beekeeper"], "hive", 80, NULL);
    setvector(&monitems["mon_zombie_cop"], "cop_weapons", 20, NULL);

    setvector(&monitems["mon_zombie_scientist"],
              "harddrugs", 6, "chem_lab", 10,
              "teleport", 6, "goo", 8, "cloning_vat", 1,
              "dissection", 10, "electronics", 9, "bionics", 1,
              "radio", 2, "textbooks", 3, NULL);

    setvector(&monitems["mon_zombie_soldier"],
              "ammo", 10, "pistols", 5,
              "shotguns", 2, "smg", 5, "bots", 1,
              "launchers", 2, "mil_rifles", 10, "grenades", 5,
              "mil_accessories", 10, "mil_food", 5, "bionics_mil", 1,
              NULL);			  
			  
    setvector(&monitems["mon_biollante"], "biollante", 1, NULL);

    setvector(&monitems["mon_chud"],
              "subway", 40,"sewer", 20,"trash",  5,"bedroom",  1,
              "dresser",  5,"ammo", 18, NULL);
    monitems["mon_one_eye"]  = monitems["mon_chud"];

    setvector(&monitems["mon_bee"], "bees", 1, NULL);
    setvector(&monitems["mon_wasp"], "wasps", 1, NULL);
    setvector(&monitems["mon_eyebot"], "robots", 4, "eyebot", 1, NULL);
    setvector(&monitems["mon_manhack"], "robots", 4, "manhack", 1, NULL);
    setvector(&monitems["mon_skitterbot"], "robots", 4, "skitterbot", 1, NULL);
    setvector(&monitems["mon_secubot"], "robots", 4, "secubot", 1, NULL);
    setvector(&monitems["mon_copbot"], "robots", 4, "copbot", 1, NULL);
    setvector(&monitems["mon_molebot"], "robots", 4, "molebot", 1, NULL);
    setvector(&monitems["mon_tripod"], "robots", 4, "tripod", 1, NULL);
    setvector(&monitems["mon_chickenbot"], "robots", 4, "chickenbot", 1, NULL);
    setvector(&monitems["mon_tankbot"], "robots", 4, "tankbot", 1, NULL);
    setvector(&monitems["mon_turret"], "robots", 10, "turret", 1, NULL);
    setvector(&monitems["mon_laserturret"], "robots", 10, "laserturret", 1, NULL);
    setvector(&monitems["mon_fungal_fighter"], "fungal_sting", 1, NULL);
    setvector(&monitems["mon_shia"], "shia_stuff", 1, NULL);
	setvector(&monitems["mon_dog_zombie_cop"], "dog_cop", 1, NULL);
}
