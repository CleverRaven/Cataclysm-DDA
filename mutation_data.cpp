#include "game.h"
#include "setvector.h"
#include "catajson.h"

#include <vector>

std::vector<dream> dreams;

void game::init_traits()
{
    catajson traitsRaw("data/raw/traits.json",true);

    if(!json_good()) {
        throw (std::string)"data/raw/traits.json wasn't found";
    }

    for(traitsRaw.set_begin(); traitsRaw.has_curr() && json_good(); traitsRaw.next()) {
        catajson curr_trait = traitsRaw.curr();

        trait new_trait;
        new_trait.name = curr_trait.get("name").as_string();
        new_trait.points = curr_trait.get("points").as_int();
        new_trait.visiblity = curr_trait.get("visibility").as_int();
        new_trait.ugliness = curr_trait.get("ugliness").as_int();
        new_trait.description = curr_trait.get("description").as_string();

        if (curr_trait.has("starting_trait")) {
            if (curr_trait.get("starting_trait").as_bool()) {
                vTraitsGood.push_back(curr_trait.get("id").as_string());

            } else {
                vTraitsBad.push_back(curr_trait.get("id").as_string());
            }
        }

        traits[curr_trait.get("id").as_string()] = new_trait;
    }
}

void game::init_mutations()
{
	catajson mutationsRaw("data/raw/mutations.json");

	if (!json_good()) {
		throw (std::string)"data/raw/mutations.json wasn't found";
	}

	for (mutationsRaw.set_begin(); mutationsRaw.has_curr(); mutationsRaw.next()) {
		catajson mutationcurr = mutationsRaw.curr();

        std::string sMutation = mutationcurr.get("MUTATION").as_string();
		mutation_data[sMutation].valid = mutationcurr.get("VALID").as_bool();

        if (mutationcurr.has("PREREQS")) {
            catajson PREREQS = mutationcurr.get("PREREQS");
            if (PREREQS.is_array()) {
                for (PREREQS.set_begin(); PREREQS.has_curr(); PREREQS.next()) {
                    mutation_data[sMutation].prereqs.push_back(PREREQS.curr().as_string());
                }
            }
        }

        if (mutationcurr.has("CANCELS")) {
            catajson CANCELS = mutationcurr.get("CANCELS");
            if (CANCELS.is_array()) {
                for (CANCELS.set_begin(); CANCELS.has_curr(); CANCELS.next()) {
                    mutation_data[sMutation].cancels.push_back(CANCELS.curr().as_string());
                }
            }
		}

        if (mutationcurr.has("CHANGES_TO")) {
            catajson CHANGES_TO = mutationcurr.get("CHANGES_TO");
            if (CHANGES_TO.is_array()) {
                for (CHANGES_TO.set_begin(); CHANGES_TO.has_curr(); CHANGES_TO.next()) {
                    mutation_data[sMutation].replacements.push_back(CHANGES_TO.curr().as_string());
                }
            }
        }

        if (mutationcurr.has("LEADS_TO")) {
            catajson LEADS_TO = mutationcurr.get("LEADS_TO");
            if (LEADS_TO.is_array()) {
                for (LEADS_TO.set_begin(); LEADS_TO.has_curr(); LEADS_TO.next()) {
                    mutation_data[sMutation].additions.push_back(LEADS_TO.curr().as_string());
                }
            }
        }
	}
}

void game::init_mutations_cat()
{
	catajson mutationsRaw("data/raw/mutations_cat.json");

	if (!json_good()) {
		throw (std::string)"data/raw/mutations_cat.json wasn't found";
	}
/*
    //Category
    //Items
	for (mutationsRaw.set_begin(); mutationsRaw.has_curr(); mutationsRaw.next()) {
		catajson mutationcurr = mutationsRaw.curr();

        std::string sMutation = mutationcurr.get("MUTATION").as_string();
		mutation_data[sMutation].valid = mutationcurr.get("VALID").as_bool();

        catajson Items = mutationcurr.get("Items");
        if (Items.is_array()) {
            for (Items.set_begin(); Items.has_curr(); Items.next()) {
                mutation_data[sMutation].prereqs.push_back(Items.curr().as_string());
            }
        }
	}
	*/
}

void game::init_dreams()
{
	catajson dreams_file("data/raw/dreams.json");

	if (!json_good())
	{
		throw (std::string)"data/raw/dreams.json wasn't found";
	}

	for (dreams_file.set_begin(); dreams_file.has_curr(); dreams_file.next())
	{
		catajson dreamcurr = dreams_file.curr();

		dream newdream;

		catajson messages = dreamcurr.get("message");
		for (messages.set_begin(); messages.has_curr(); messages.next())
		{
			newdream.message.push_back(messages.curr().as_string());
		}
		newdream.strength		= dreamcurr.get("strength").as_int();
		newdream.category		= string_to_mutcat(dreamcurr.get("category").as_string());

		dreams.push_back(newdream);
	}
}

std::vector<std::string> mutations_from_category(mutation_category cat)
{
 std::vector<std::string> ret;
 switch (cat) {

 case MUTCAT_LIZARD:
  setvector(&ret,
"THICKSKIN", "INFRARED", "FANGS", "MEMBRANE", "SCALES", "TALONS",
"SLIT_NOSTRILS", "FORKED_TONGUE", "TROGLO", "WEBBED", "CARNIVORE",
"COLDBLOOD2", "STR_UP_2", "DEX_UP_2", NULL);
  break;

 case MUTCAT_BIRD:
  setvector(&ret,
"QUICK", "LIGHTEATER", "DEFT", "LIGHTSTEP", "BADBACK", "GLASSJAW",
"NIGHTVISION", "HOLLOW_BONES", "FEATHERS", "TALONS", "BEAK", "FLEET2",
"WINGS_BIRD", "COLDBLOOD", "DEX_UP_3", NULL);
  break;

 case MUTCAT_FISH:
  setvector(&ret,
"SMELLY2", "NIGHTVISION2", "FANGS", "MEMBRANE", "GILLS", "SLEEK_SCALES",
"TAIL_FIN", "DEFORMED", "THIRST", "WEBBED", "SLIMY", "COLDBLOOD2", NULL);
  break;

 case MUTCAT_BEAST:
  setvector(&ret,
"DEFT", "ANIMALEMPATH", "TERRIFYING", "ADRENALINE", "MYOPIC", "FORGETFUL",
"NIGHTVISION2", "FANGS", "FUR", "CLAWS", "PHEROMONE_MAMMAL",
"PADDED_FEET", "TAIL_FLUFFY", "SMELLY2", "DEFORMED2", "HUNGER", "TROGLO",
"CARNIVORE", "SNARL", "SHOUT3", "CANINE_EARS", "STR_UP_4", NULL);
  break;

 case MUTCAT_CATTLE:
  setvector(&ret,
"THICKSKIN", "DISRESISTANT", "NIGHTVISION", "FUR", "PHEROMONE_MAMMAL",
"HOOVES", "RUMINANT", "HORNS", "TAIL_LONG", "DEFORMED", "PONDEROUS2",
"CANINE_EARS", "STR_UP_2", NULL);

 case MUTCAT_INSECT:
  setvector(&ret,
"QUICK", "LIGHTEATER", "POISRESIST", "NIGHTVISION", "TERRIFYING",
"HEAVYSLEEPER", "NIGHTVISION2", "INFRARED", "CHITIN2", "PHEROMONE_INSECT",
"ANTENNAE", "WINGS_INSECT", "TAIL_STING", "MANDIBLES", "DEFORMED",
"TROGLO", "COLDBLOOD3", "STR_UP", "DEX_UP", NULL);
  break;

 case MUTCAT_PLANT:
  setvector(&ret,
"DISIMMUNE", "HEAVYSLEEPER", "BADHEARING", "FASTHEALER2", "BARK",
"THORNS", "LEAVES", "DEFORMED2", "PONDEROUS3", "STR_UP_2", NULL);
  break;

 case MUTCAT_SLIME:
  setvector(&ret,
"POISRESIST", "ROBUST", "CHEMIMBALANCE", "REGEN", "RADIOGENIC",
"DISIMMUNE", "POISONOUS", "SLIME_HANDS", "SMELLY2", "DEFORMED3",
"HOLLOW_BONES", "VOMITOUS", "HUNGER", "THIRST", "SORES", "TROGLO",
"WEBBED", "UNSTABLE", "RADIOACTIVE1", "SLIMY", "DEX_UP", "INT_UP", NULL);
  break;

 case MUTCAT_TROGLO:
  setvector(&ret,
"QUICK", "LIGHTEATER", "MYOPIC", "NIGHTVISION3", "INFRARED", "REGEN",
"DISIMMUNE", "POISONOUS", "SAPROVORE", "SLIT_NOSTRILS", "ALBINO",
"TROGLO3", "SLIMY", NULL);
  break;

 case MUTCAT_CEPHALOPOD:
  setvector(&ret,
"GILLS", "MOUTH_TENTACLES", "SLIT_NOSTRILS", "DEFORMED", "THIRST",
"BEAK", "SLIMY", "COLDBLOOD", "ARM_TENTACLES_8", "SHELL", "LEG_TENTACLES",
NULL);
  break;

 case MUTCAT_SPIDER:
  setvector(&ret,
"FLEET", "POISRESIST", "NIGHTVISION3", "INFRARED", "FUR", "CHITIN3",
"POISONOUS", "COMPOUND_EYES", "MANDIBLES", "WEB_WEAVER", "TROGLO",
"CARNIVORE", "COLDBLOOD", "DEX_UP_2", NULL);
  break;

 case MUTCAT_RAT:
  setvector(&ret,
"DISRESISTANT", "NIGHTVISION2", "FANGS", "FUR", "CLAWS", "TAIL_LONG",
"WHISKERS", "DEFORMED3", "VOMITOUS", "HUNGER", "TROGLO2", "GROWL", NULL);
  break;
 }

 return ret;
}

mutation_category string_to_mutcat(std::string input)
{
    mutation_category ret;
	if (input == "MUTCAT_NULL") {
        ret = MUTCAT_NULL;
	}
	else if (input ==  "MUTCAT_LIZARD") {
        ret = MUTCAT_LIZARD;
	}

	else if (input ==  "MUTCAT_BIRD") {
        ret = MUTCAT_BIRD;
	}

	else if (input ==  "MUTCAT_FISH") {
        ret = MUTCAT_FISH;
	}

	else if (input ==  "MUTCAT_BEAST") {
        ret = MUTCAT_BEAST;
	}

	else if (input ==  "MUTCAT_CATTLE") {
        ret = MUTCAT_CATTLE;
	}

	else if (input ==  "MUTCAT_INSECT") {
        ret = MUTCAT_INSECT;
	}

	else if (input ==  "MUTCAT_PLANT") {
        ret = MUTCAT_PLANT;
	}

	else if (input ==  "MUTCAT_SLIME") {
        ret = MUTCAT_SLIME;
	}

	else if (input ==  "MUTCAT_TROGLO") {
        ret = MUTCAT_TROGLO;
	}

	else if (input ==  "MUTCAT_CEPHALOPOD") {
        ret = MUTCAT_CEPHALOPOD;
	}

	else if (input ==  "MUTCAT_SPIDER") {
        ret = MUTCAT_SPIDER;
	}

	else if (input ==  "MUTCAT_RAT") {
        ret = MUTCAT_RAT;
	}
    return ret;
}
