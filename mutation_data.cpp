#include "game.h"
#include "catajson.h"

#include <vector>
#include <map>

std::vector<dream> dreams;
std::map<std::string, std::vector<std::string> > mutations_category;

void game::init_traits_mutations()
{
	catajson cjMutationsRaw("data/raw/mutations.json");

	if (!json_good()) {
		throw (std::string)"data/raw/mutations.json wasn't found";
	}

	mutations_category[""].clear(); //dont delete this!

	for (cjMutationsRaw.set_begin(); cjMutationsRaw.has_curr(); cjMutationsRaw.next()) {
		catajson cjMutation = cjMutationsRaw.curr();

        std::string sMutation = cjMutation.get("id").as_string();

        trait new_trait;
        new_trait.name = cjMutation.get("name").as_string();
        new_trait.points = cjMutation.get("points").as_int();
        new_trait.visiblity = cjMutation.get("visibility").as_int();
        new_trait.ugliness = cjMutation.get("ugliness").as_int();
        new_trait.description = cjMutation.get("description").as_string();
        new_trait.startingtrait = false;

        if (cjMutation.has("starting_trait")) {
            new_trait.startingtrait = cjMutation.get("starting_trait").as_bool();
        }

        traits[sMutation] = new_trait;

		mutation_data[sMutation].valid = cjMutation.get("valid").as_bool();

        if (cjMutation.has("prereqs")) {
            catajson cjPrereqs = cjMutation.get("prereqs");
            if (cjPrereqs.is_array()) {
                for (cjPrereqs.set_begin(); cjPrereqs.has_curr(); cjPrereqs.next()) {
                    mutation_data[sMutation].prereqs.push_back(cjPrereqs.curr().as_string());
                }
            }
        }

        if (cjMutation.has("cancels")) {
            catajson cjCancels = cjMutation.get("cancels");
            if (cjCancels.is_array()) {
                for (cjCancels.set_begin(); cjCancels.has_curr(); cjCancels.next()) {
                    mutation_data[sMutation].cancels.push_back(cjCancels.curr().as_string());
                }
            }
		}

        if (cjMutation.has("changes_to")) {
            catajson cjChangesTo = cjMutation.get("changes_to");
            if (cjChangesTo.is_array()) {
                for (cjChangesTo.set_begin(); cjChangesTo.has_curr(); cjChangesTo.next()) {
                    mutation_data[sMutation].replacements.push_back(cjChangesTo.curr().as_string());
                }
            }
        }

        if (cjMutation.has("leads_to")) {
            catajson cjLeadsTo = cjMutation.get("leads_to");
            if (cjLeadsTo.is_array()) {
                for (cjLeadsTo.set_begin(); cjLeadsTo.has_curr(); cjLeadsTo.next()) {
                    mutation_data[sMutation].additions.push_back(cjLeadsTo.curr().as_string());
                }
            }
        }

        if (cjMutation.has("category")) {
            catajson cjCategory = cjMutation.get("category");
            if (cjCategory.is_array()) {
                for (cjCategory.set_begin(); cjCategory.has_curr(); cjCategory.next()) {
                    mutation_data[sMutation].category.push_back(cjCategory.curr().as_string());
                    mutations_category[cjCategory.curr().as_string()].push_back(sMutation);
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
