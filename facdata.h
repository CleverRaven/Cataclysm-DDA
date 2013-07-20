#include <string>

std::string faction_adj_pos[15] = {
_("Shining"), _("Sacred"), _("Golden"), _("Holy"), _("Righteous"), _("Devoted"), _("Virtuous"),
_("Splendid"), _("Divine"), _("Radiant"), _("Noble"), _("Venerable"), _("Immaculate"),
_("Heroic"), _("Bright")};

std::string faction_adj_neu[15] = {
_("Original"), _("Crystal"), _("Metal"), _("Mighty"), _("Powerful"), _("Solid"), _("Stone"),
_("Firey"), _("Colossal"), _("Famous"), _("Supreme"), _("Invincible"), _("Unlimited"),
_("Great"), _("Electric")};

std::string faction_adj_bad[15] = {
_("Poisonous"), _("Deadly"), _("Foul"), _("Nefarious"), _("Wicked"), _("Vile"), _("Ruinous"),
_("Horror"), _("Devastating"), _("Vicious"), _("Sinister"), _("Baleful"), _("Pestilent"),
_("Pernicious"), _("Dread")};

std::string faction_noun_strong[15] = {
_("Fists"), _("Slayers"), _("Furies"), _("Dervishes"), _("Tigers"), _("Destroyers"),
_("Berserkers"), _("Samurai"), _("Valkyries"), _("Army"), _("Killers"), _("Paladins"),
_("Knights"), _("Warriors"), _("Huntsmen")};

std::string faction_noun_sneak[15] = {
_("Snakes"), _("Rats"), _("Assassins"), _("Ninja"), _("Agents"), _("Shadows"), _("Guerillas"),
_("Eliminators"), _("Snipers"), _("Smoke"), _("Arachnids"), _("Creepers"), _("Shade"),
_("Stalkers"), _("Eels")};

std::string faction_noun_crime[15] = {
_("Bandits"), _("Punks"), _("Family"), _("Mafia"), _("Mob"), _("Gang"), _("Vandals"), _("Sharks"),
_("Muggers"), _("Cutthroats"), _("Guild"), _("Faction"), _("Thugs"), _("Racket"), _("Crooks")};

std::string faction_noun_cult[15] = {
_("Brotherhood"), _("Church"), _("Ones"), _("Crucible"), _("Sect"), _("Creed"), _("Doctrine"),
_("Priests"), _("Tenet"), _("Monks"), _("Clerics"), _("Pastors"), _("Gnostics"), _("Elders"),
_("Inquisitors")};

std::string faction_noun_none[15] = {
_("Settlers"), _("People"), _("Men"), _("Faction"), _("Tribe"), _("Clan"), _("Society"), _("Folk"),
_("Nation"), _("Republic"), _("Colony"), _("State"), _("Kingdom"), _("Party"), _("Company")};


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
{_("basic survival"),		 0,	 0,	 0,	 0,	 0},
{_("financial wealth"),		 0,	-1,	 0,	 2,	-1},
{_("dominance of the region"),	-1,	 1,	-1,	 1,	-1},
{_("the extermination of monsters"),1,	 3,	-1,	-1,	-1},
{_("contact with unseen powers"),	-1,	 0,	 1,	 0,	 4},
{_("bringing the apocalypse"),	-5,	 1,	 2,	 0,	 7},
{_("general chaos and anarchy"),	-3,	 2,	-3,	 2,	-1},
{_("the cultivation of knowledge"), 2,	-3,	 2,	-1,	 0},
{_("harmony with nature"),		 2,	-2,	 0,	-1,	 2},
{_("rebuilding civilization"),	 2,	 1,	-2,	-2,	-4},
{_("spreading the fungus"),	-2,	 1,	 1,	 0,	 4}
};
// TOTAL:			-5	 3	-2	 0	 7

faction_value_datum facjob_data[NUM_FACJOBS] = {
// "They earn money via <name>"
//Name				Good	Str	Sneak	Crime	Cult
{"Null",		 	 0,	 0,	 0,	 0,	 0},
{_("protection rackets"),		-3,	 2,	-1,	 4,	 0},
{_("the sale of information"),	-1,	-1,	 4,	 1,	 0},
{_("their bustling trade centers"), 1,	-1,	-2,	-4,	-4},
{_("trade caravans"),		 2,	-1,	-1,	-3,	-2},
{_("scavenging supplies"),		 0,	-1,	 0,	-1,	-1},
{_("mercenary work"),		 0,	 3,	-1,	 1,	-1},
{_("assassinations"),		-1,	 2,	 2,	 1,	 1},
{_("raiding settlements"),		-4,	 4,	-3,	 3,	-2},
{_("the theft of property"),	-3,	-1,	 4,	 4,	 1},
{_("gambling parlors"),		-1,	-2,	-1,	 1,	-1},
{_("medical aid"),			 4,	-3,	-2,	-3,	 0},
{_("farming & selling food"),	 3,	-4,	-2,	-4,	 1},
{_("drug dealing"),		-2,	 0,	-1,	 2,	 0},
{_("selling manufactured goods"),	 1,	 0,	-1,	-2,	 0}
};
// TOTAL:			-5	-3	-5	 0	-6

faction_value_datum facval_data[NUM_FACVALS] = {
// "They are known for <name>"
//Name				Good	Str	Sneak	Crime	Cult
{"Null",		 	 0,	 0,	 0,	 0,	 0},
{_("their charitable nature"),	 5,	-1,	-1,	-2,	-2},
{_("their isolationism"),		 0,	-2,	 1,	 0,	 2},
{_("exploring extensively"),	 1,	 0,	 0,	-1,	-1},
{_("collecting rare artifacts"),	 0,	 1,	 1,	 0,	 3},
{_("their knowledge of bionics"),	 1,	 2,	 0,	 0,	 0},
{_("their libraries"),		 1,	-3,	 0,	-2,	 1},
{_("their elite training"),	 0,	 4,	 2,	 0,	 2},
{_("their robotics factories"),	 0,	 3,	-1,	 0,	-2},
{_("treachery"),			-3,	 0,	 1,	 3,	 0},
{_("the avoidance of drugs"),	 1,	 0,	 0,	-1,	 1},
{_("their adherance to the law"),	 2,	-1,	-1,	-4,	-1},
{_("their cruelty"),		-3,	 1,	-1,	 4,	 1}
};
// TOTALS:			 5	 4	 1	-3	 4
/* Note: It's nice to keep the totals around 0 for Good, and about even for the
 * other four.  It's okay if Good is slightly negative (after all, in a post-
 * apocalyptic world people might be a LITTLE less virtuous), and to keep
 * strength valued a bit higher than the others.
 */

