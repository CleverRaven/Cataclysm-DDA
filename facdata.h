#include <string>

std::string faction_adj_pos[15] = {
"Shining", "Sacred", "Golden", "Holy", "Righteous", "Devoted", "Virtuous",
"Splendid", "Divine", "Radiant", "Noble", "Venerable", "Immaculate",
"Heroic", "Bright"};

std::string faction_adj_neu[15] = {
"Original", "Crystal", "Metal", "Mighty", "Powerful", "Solid", "Stone",
"Firey", "Colossal", "Famous", "Supreme", "Invincible", "Unlimited",
"Great", "Electric"};

std::string faction_adj_bad[15] = {
"Poisonous", "Deadly", "Foul", "Nefarious", "Wicked", "Vile", "Ruinous",
"Horror", "Devastating", "Vicious", "Sinister", "Baleful", "Pestilent",
"Pernicious", "Dread"};

std::string faction_noun_strong[15] = {
"Fists", "Slayers", "Furies", "Dervishes", "Tigers", "Destroyers",
"Berserkers", "Samurai", "Valkyries", "Army", "Killers", "Paladins",
"Knights", "Warriors", "Huntsmen"};

std::string faction_noun_sneak[15] = {
"Snakes", "Rats", "Assassins", "Ninja", "Agents", "Shadows", "Guerillas",
"Eliminators", "Snipers", "Smoke", "Arachnids", "Creepers", "Shade",
"Stalkers", "Eels"};

std::string faction_noun_crime[15] = {
"Bandits", "Punks", "Family", "Mafia", "Mob", "Gang", "Vandals", "Sharks",
"Muggers", "Cutthroats", "Guild", "Faction", "Thugs", "Racket", "Crooks"};

std::string faction_noun_cult[15] = {
"Brotherhood", "Church", "Ones", "Crucible", "Sect", "Creed", "Doctrine",
"Priests", "Tenet", "Monks", "Clerics", "Pastors", "Gnostics", "Elders",
"Inquisitors"};

std::string faction_noun_none[15] = {
"Settlers", "People", "Men", "Faction", "Tribe", "Clan", "Society", "Folk",
"Nation", "Republic", "Colony", "State", "Kingdom", "Party", "Company"};


struct faction_value_datum {
 std::string name;
 int good;	// A measure of how "good" the value is (naming purposes &c)
 int strength;
 int sneak;
 int crime;
 int cult;
};

faction_value_datum facgoal_data[NUM_FACGOALS] = {
// "Their ultimate goal is <name>"
//Name				Good	Str	Sneak	Crime	Cult
{"Null",		 	 0,	 0,	 0,	 0,	 0},
{"basic survival",		 0,	 0,	 0,	 0,	 0},
{"financial wealth",		 0,	-1,	 0,	 2,	-1},
{"dominance of the region",	-1,	 1,	-1,	 1,	-1},
{"the extermination of monsters",1,	 3,	-1,	-1,	-1},
{"contact with unseen powers",	-1,	 0,	 1,	 0,	 4},
{"bringing the apocalypse",	-5,	 1,	 2,	 0,	 7},
{"general chaos and anarchy",	-3,	 2,	-3,	 2,	-1},
{"the cultivation of knowledge", 2,	-3,	 2,	-1,	 0},
{"harmony with nature",		 2,	-2,	 0,	-1,	 2},
{"rebuilding civilization",	 2,	 1,	-2,	-2,	-4},
{"spreading the fungus",	-2,	 1,	 1,	 0,	 4}
};
// TOTAL:			-5	 3	-2	 0	 7

faction_value_datum facjob_data[NUM_FACJOBS] = {
// "They earn money via <name>"
//Name				Good	Str	Sneak	Crime	Cult
{"Null",		 	 0,	 0,	 0,	 0,	 0},
{"protection rackets",		-3,	 2,	-1,	 4,	 0},
{"the sale of information",	-1,	-1,	 4,	 1,	 0},
{"their bustling trade centers", 1,	-1,	-2,	-4,	-4},
{"trade caravans",		 2,	-1,	-1,	-3,	-2},
{"scavenging supplies",		 0,	-1,	 0,	-1,	-1},
{"mercenary work",		 0,	 3,	-1,	 1,	-1},
{"assassinations",		-1,	 2,	 2,	 1,	 1},
{"raiding settlements",		-4,	 4,	-3,	 3,	-2},
{"the theft of property",	-3,	-1,	 4,	 4,	 1},
{"gambling parlors",		-1,	-2,	-1,	 1,	-1},
{"medical aid",			 4,	-3,	-2,	-3,	 0},
{"farming & selling food",	 3,	-4,	-2,	-4,	 1},
{"drug dealing",		-2,	 0,	-1,	 2,	 0},
{"selling manufactured goods",	 1,	 0,	-1,	-2,	 0}
};
// TOTAL:			-5	-3	-5	 0	-6

faction_value_datum facval_data[NUM_FACVALS] = {
// "They are known for <name>"
//Name				Good	Str	Sneak	Crime	Cult
{"Null",		 	 0,	 0,	 0,	 0,	 0},
{"their charitable nature",	 5,	-1,	-1,	-2,	-2},
{"their isolationism",		 0,	-2,	 1,	 0,	 2},
{"exploring extensively",	 1,	 0,	 0,	-1,	-1},
{"collecting rare artifacts",	 0,	 1,	 1,	 0,	 3},
{"their knowledge of bionics",	 1,	 2,	 0,	 0,	 0},
{"their libraries",		 1,	-3,	 0,	-2,	 1},
{"their elite training",	 0,	 4,	 2,	 0,	 2},
{"their robotics factories",	 0,	 3,	-1,	 0,	-2},
{"treachery",			-3,	 0,	 1,	 3,	 0},
{"the avoidance of drugs",	 1,	 0,	 0,	-1,	 1},
{"their adherance to the law",	 2,	-1,	-1,	-4,	-1},
{"their cruelty",		-3,	 1,	-1,	 4,	 1}
};
// TOTALS:			 5	 4	 1	-3	 4
/* Note: It's nice to keep the totals around 0 for Good, and about even for the
 * other four.  It's okay if Good is slightly negative (after all, in a post-
 * apocalyptic world people might be a LITTLE less virtuous), and to keep
 * strength valued a bit higher than the others.
 */

