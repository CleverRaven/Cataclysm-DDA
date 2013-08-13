#include "game.h"
#include "catajson.h"

#include <vector>
#include <map>

std::vector<dream> dreams;
std::map<std::string, std::vector<std::string> > mutations_category;

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
        new_trait.startingtrait = false;

        if (curr_trait.has("starting_trait")) {
            new_trait.startingtrait = curr_trait.get("starting_trait").as_bool();
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

        mutations_category[""].clear(); //dont delete this!

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

        if (mutationcurr.has("CATEGORY")) {
            catajson CATEGORY = mutationcurr.get("CATEGORY");
            if (CATEGORY.is_array()) {
                for (CATEGORY.set_begin(); CATEGORY.has_curr(); CATEGORY.next()) {
                    mutation_data[sMutation].category.push_back(CATEGORY.curr().as_string());
                    mutations_category[CATEGORY.curr().as_string()].push_back(sMutation);
                }
            }
        }
	}
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
		newdream.category		= dreamcurr.get("category").as_string();

		dreams.push_back(newdream);
	}
}
