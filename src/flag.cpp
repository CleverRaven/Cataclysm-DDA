#include "flag.h"

#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "type_id.h"

const flag_id flag_NULL = flag_id( "null" ); // intentionally invalid flag

const flag_id flag_ACID( "ACID" );
const flag_id flag_ACID_IMMUNE( "ACID_IMMUNE" );
const flag_id flag_ACTIVATE_ON_PLACE( "ACTIVATE_ON_PLACE" );
const flag_id flag_ACTIVE_CLOAKING( "ACTIVE_CLOAKING" );
const flag_id flag_ACT_IN_FIRE( "ACT_IN_FIRE" );
const flag_id flag_ACT_ON_RANGED_HIT( "ACT_ON_RANGED_HIT" );
const flag_id flag_ALARMCLOCK( "ALARMCLOCK" );
const flag_id flag_ALARMED( "ALARMED" );
const flag_id flag_ALIGN_WORKBENCH( "ALIGN_WORKBENCH" );
const flag_id flag_ALLERGEN_EGG( "ALLERGEN_EGG" );
const flag_id flag_ALLERGEN_FRUIT( "ALLERGEN_FRUIT" );
const flag_id flag_ALLERGEN_JUNK( "ALLERGEN_JUNK" );
const flag_id flag_ALLERGEN_MEAT( "ALLERGEN_MEAT" );
const flag_id flag_ALLERGEN_MILK( "ALLERGEN_MILK" );
const flag_id flag_ALLERGEN_NUT( "ALLERGEN_NUT" );
const flag_id flag_ALLERGEN_VEGGY( "ALLERGEN_VEGGY" );
const flag_id flag_ALLERGEN_WHEAT( "ALLERGEN_WHEAT" );
const flag_id flag_ALLERGEN_WOOL( "ALLERGEN_WOOL" );
const flag_id flag_ALLOWS_NATURAL_ATTACKS( "ALLOWS_NATURAL_ATTACKS" );
const flag_id flag_ALLOWS_REMOTE_USE( "ALLOWS_REMOTE_USE" );
const flag_id flag_ALLOW_FIELD_EFFECT( "ALLOW_FIELD_EFFECT" );
const flag_id flag_ALWAYS_TWOHAND( "ALWAYS_TWOHAND" );
const flag_id flag_AURA( "AURA" );
const flag_id flag_AUTODOC( "AUTODOC" );
const flag_id flag_AUTODOC_COUCH( "AUTODOC_COUCH" );
const flag_id flag_AUTO_WALL_SYMBOL( "AUTO_WALL_SYMBOL" );
const flag_id flag_BAROMETER( "BAROMETER" );
const flag_id flag_BARRICADABLE_DOOR( "BARRICADABLE_DOOR" );
const flag_id flag_BARRICADABLE_DOOR_DAMAGED( "BARRICADABLE_DOOR_DAMAGED" );
const flag_id flag_BARRICADABLE_DOOR_REINFORCED( "BARRICADABLE_DOOR_REINFORCED" );
const flag_id flag_BARRICADABLE_WINDOW_CURTAINS( "BARRICADABLE_WINDOW_CURTAINS" );
const flag_id flag_BASH_IMMUNE( "BASH_IMMUNE" );
const flag_id flag_BELTED( "BELTED" );
const flag_id flag_BELT_CLIP( "BELT_CLIP" );
const flag_id flag_BIONIC_GUN( "BIONIC_GUN" );
const flag_id flag_BIONIC_INSTALLATION_DATA( "BIONIC_INSTALLATION_DATA" );
const flag_id flag_BIONIC_TOGGLED( "BIONIC_TOGGLED" );
const flag_id flag_BIONIC_WEAPON( "BIONIC_WEAPON" );
const flag_id flag_BIO_IMMUNE( "BIO_IMMUNE" );
const flag_id flag_BIPOD( "BIPOD" );
const flag_id flag_BIRD( "BIRD" );
const flag_id flag_BLED( "BLED" );
const flag_id flag_BLIND( "BLIND" );
const flag_id flag_BLOCKSDOOR( "BLOCKSDOOR" );
const flag_id flag_BLOCK_WHILE_WORN( "BLOCK_WHILE_WORN" );
const flag_id flag_BLOCK_WIND( "BLOCK_WIND" );
const flag_id flag_BOMB( "BOMB" );
const flag_id flag_BRASS_CATCHER( "BRASS_CATCHER" );
const flag_id flag_BULLET_IMMUNE( "BULLET_IMMUNE" );
const flag_id flag_BURNOUT( "BURNOUT" );
const flag_id flag_BURROWABLE( "BURROWABLE" );
const flag_id flag_BUTCHER_EQ( "BUTCHER_EQ" );
const flag_id flag_BYPRODUCT( "BYPRODUCT" );
const flag_id flag_CABLE_SPOOL( "CABLE_SPOOL" );
const flag_id flag_CAMERA_PRO( "CAMERA_PRO" );
const flag_id flag_CANNIBAL( "CANNIBAL" );
const flag_id flag_CANNIBALISM( "CANNIBALISM" );
const flag_id flag_CANT_HEAL_EVERYONE( "CANT_HEAL_EVERYONE" );
const flag_id flag_CANVAS_WALL( "CANVAS_WALL" );
const flag_id flag_CAN_SIT( "CAN_SIT" );
const flag_id flag_CARNIVORE_OK( "CARNIVORE_OK" );
const flag_id flag_CASING( "CASING" );
const flag_id flag_CATTLE( "CATTLE" );
const flag_id flag_CHAINFENCE( "CHAINFENCE" );
const flag_id flag_CHALLENGE( "CHALLENGE" );
const flag_id flag_CHARGEDIM( "CHARGEDIM" );
const flag_id flag_CHOCOLATE( "CHOCOLATE" );
const flag_id flag_CITY_START( "CITY_START" );
const flag_id flag_CLIMATE_CONTROL( "CLIMATE_CONTROL" );
const flag_id flag_CLIMBABLE( "CLIMBABLE" );
const flag_id flag_CLIMB_SIMPLE( "CLIMB_SIMPLE" );
const flag_id flag_COLD( "COLD" );
const flag_id flag_COLD_IMMUNE( "COLD_IMMUNE" );
const flag_id flag_COLLAPSES( "COLLAPSES" );
const flag_id flag_COLLAPSIBLE_STOCK( "COLLAPSIBLE_STOCK" );
const flag_id flag_COLLAR( "COLLAR" );
const flag_id flag_CONDUCTIVE( "CONDUCTIVE" );
const flag_id flag_CONNECT_TO_WALL( "CONNECT_TO_WALL" );
const flag_id flag_CONSOLE( "ONSOLE" );
const flag_id flag_CONSUMABLE( "CONSUMABLE" );
const flag_id flag_CONTAINER( "CONTAINER" );
const flag_id flag_COOKED( "COOKED" );
const flag_id flag_CORPSE( "CORPSE" );
const flag_id flag_CORROSIVE( "CORROSIVE" );
const flag_id flag_COUNTER( "COUNTER" );
const flag_id flag_CRUTCHES( "CRUTCHES" );
const flag_id flag_CURRENT( "CURRENT" );
const flag_id flag_CUSTOM_EXPLOSION( "CUSTOM_EXPLOSION" );
const flag_id flag_CUT_IMMUNE( "CUT_IMMUNE" );
const flag_id flag_DANGEROUS( "DANGEROUS" );
const flag_id flag_DEAF( "DEAF" );
const flag_id flag_DEEP_WATER( "DEEP_WATER" );
const flag_id flag_DESTROY_ITEM( "DESTROY_ITEM" );
const flag_id flag_DIAMOND( "DIAMOND" );
const flag_id flag_DIFFICULT_Z( "DIFFICULT_Z" );
const flag_id flag_DIGGABLE( "DIGGABLE" );
const flag_id flag_DIGGABLE_CAN_DEEPEN( "DIGGABLE_CAN_DEEPEN" );
const flag_id flag_DIG_TOOL( "DIG_TOOL" );
const flag_id flag_DIMENSIONAL_ANCHOR( "DIMENSIONAL_ANCHOR" );
const flag_id flag_DIRTY( "DIRTY" );
const flag_id flag_DISABLE_SIGHTS( "DISABLE_SIGHTS" );
const flag_id flag_DONT_REMOVE_ROTTEN( "DONT_REMOVE_ROTTEN" );
const flag_id flag_DOOR( "DOOR" );
const flag_id flag_DROP_ACTION_ONLY_IF_LIQUID( "DROP_ACTION_ONLY_IF_LIQUID" );
const flag_id flag_DURABLE_MELEE( "DURABLE_MELEE" );
const flag_id flag_EASY_DECONSTRUCT( "EASY_DECONSTRUCT" );
const flag_id flag_EATEN_COLD( "EATEN_COLD" );
const flag_id flag_EATEN_HOT( "EATEN_HOT" );
const flag_id flag_EDIBLE_FROZEN( "EDIBLE_FROZEN" );
const flag_id flag_EFFECT_IMPEDING( "EFFECT_IMPEDING" );
const flag_id flag_ELECTRIC_IMMUNE( "ELECTRIC_IMMUNE" );
const flag_id flag_ETHEREAL_ITEM( "ETHEREAL_ITEM" );
const flag_id flag_FAKE_MILL( "FAKE_MILL" );
const flag_id flag_FAKE_SMOKE( "FAKE_SMOKE" );
const flag_id flag_FANCY( "FANCY" );
const flag_id flag_FELINE( "FELINE" );
const flag_id flag_FERTILIZER( "FERTILIZER" );
const flag_id flag_FIELD_DRESS( "FIELD_DRESS" );
const flag_id flag_FIELD_DRESS_FAILED( "FIELD_DRESS_FAILED" );
const flag_id flag_FILTHY( "FILTHY" );
const flag_id flag_FIN( "FIN" );
const flag_id flag_FIRE( "FIRE" );
const flag_id flag_FIRESTARTER( "FIRESTARTER" );
const flag_id flag_FIREWOOD( "FIREWOOD" );
const flag_id flag_FIRE_100( "FIRE_100" );
const flag_id flag_FIRE_20( "FIRE_20" );
const flag_id flag_FIRE_50( "FIRE_50" );
const flag_id flag_FIRE_CONTAINER( "FIRE_CONTAINER" );
const flag_id flag_FIRE_TWOHAND( "FIRE_TWOHAND" );
const flag_id flag_FISHABLE( "FISHABLE" );
const flag_id flag_FISH_GOOD( "FISH_GOOD" );
const flag_id flag_FISH_POOR( "FISH_POOR" );
const flag_id flag_FIT( "FIT" );
const flag_id flag_FIX_FARSIGHT( "FIX_FARSIGHT" );
const flag_id flag_FIX_NEARSIGHT( "FIX_NEARSIGHT" );
const flag_id flag_FLAMING( "FLAMING" );
const flag_id flag_FLAMMABLE( "FLAMMABLE" );
const flag_id flag_FLAMMABLE_ASH( "FLAMMABLE_ASH" );
const flag_id flag_FLAMMABLE_HARD( "FLAMMABLE_HARD" );
const flag_id flag_FLASH_PROTECTION( "FLASH_PROTECTION" );
const flag_id flag_FLAT( "FLAT" );
const flag_id flag_FLAT_SURF( "FLAT_SURF" );
const flag_id flag_FLOTATION( "FLOTATION" );
const flag_id flag_FLOWER( "FLOWER" );
const flag_id flag_FORAGE_HALLU( "FORAGE_HALLU" );
const flag_id flag_FORAGE_POISON( "FORAGE_POISON" );
const flag_id flag_FRAGILE( "FRAGILE" );
const flag_id flag_FRAGILE_MELEE( "FRAGILE_MELEE" );
const flag_id flag_FREEZERBURN( "FREEZERBURN" );
const flag_id flag_FROZEN( "FROZEN" );
const flag_id flag_FUNGAL_VECTOR( "FUNGAL_VECTOR" );
const flag_id flag_FUNGUS( "FUNGUS" );
const flag_id flag_GAS_DISCOUNT( "GAS_DISCOUNT" );
const flag_id flag_GAS_PROOF( "GAS_PROOF" );
const flag_id flag_GIBBED( "GIBBED" );
const flag_id flag_GNV_EFFECT( "GNV_EFFECT" );
const flag_id flag_GOES_DOWN( "GOES_DOWN" );
const flag_id flag_GOES_UP( "GOES_UP" );
const flag_id flag_GROWTH_HARVEST( "GROWTH_HARVEST" );
const flag_id flag_GROWTH_MATURE( "GROWTH_MATURE" );
const flag_id flag_GROWTH_SEEDLING( "GROWTH_SEEDLING" );
const flag_id flag_HARD( "HARD" );
const flag_id flag_HARVESTED( "HARVESTED" );
const flag_id flag_HEAT_IMMUNE( "HEAT_IMMUNE" );
const flag_id flag_HELMET_COMPAT( "HELMET_COMPAT" );
const flag_id flag_HIDDEN_HALLU( "HIDDEN_HALLU" );
const flag_id flag_HIDDEN_POISON( "HIDDEN_POISON" );
const flag_id flag_HIDE_PLACE( "HIDE_PLACE" );
const flag_id flag_HOOD( "HOOD" );
const flag_id flag_HOT( "HOT" );
const flag_id flag_HURT_WHEN_WIELDED( "HURT_WHEN_WIELDED" );
const flag_id flag_HYGROMETER( "HYGROMETER" );
const flag_id flag_INDOORS( "INDOORS" );
const flag_id flag_INEDIBLE( "INEDIBLE" );
//const flag_id flag_INITIAL_PART( "INITIAL_PART" );
const flag_id flag_INSPIRATIONAL( "INSPIRATIONAL" );
const flag_id flag_INSTALL_DIFFICULT( "INSTALL_DIFFICULT" );
const flag_id flag_IN_CBM( "IN_CBM" );
const flag_id flag_IRREMOVABLE( "IRREMOVABLE" );
const flag_id flag_IR_EFFECT( "IR_EFFECT" );
const flag_id flag_IS_ARMOR( "IS_ARMOR" );
const flag_id flag_IS_PET_ARMOR( "IS_PET_ARMOR" );
const flag_id flag_IS_UPS( "IS_UPS" );
const flag_id flag_ITEM_BROKEN( "ITEM_BROKEN" );
const flag_id flag_LADDER( "LADDER" );
const flag_id flag_LEAK_ALWAYS( "LEAK_ALWAYS" );
const flag_id flag_LEAK_DAM( "LEAK_DAM" );
const flag_id flag_LIQUID( "LIQUID" );
const flag_id flag_LIQUIDCONT( "LIQUIDCONT" );
const flag_id flag_LITCIG( "LITCIG" );
const flag_id flag_LOCKED( "LOCKED" );
const flag_id flag_LUPINE( "LUPINE" );
const flag_id flag_MAGIC_FOCUS( "MAGIC_FOCUS" );
const flag_id flag_MAG_BELT( "MAG_BELT" );
const flag_id flag_MAG_BULKY( "MAG_BULKY" );
const flag_id flag_MAG_DESTROY( "MAG_DESTROY" );
const flag_id flag_MAG_EJECT( "MAG_EJECT" );
const flag_id flag_MC_ENCRYPTED( "MC_ENCRYPTED" );
const flag_id flag_MC_HAS_DATA( "MC_HAS_DATA" );
const flag_id flag_MC_MAY_BE_ENCRYPTED( "MC_MAY_BE_ENCRYPTED" );
const flag_id flag_MC_MOBILE( "MC_MOBILE" );
const flag_id flag_MC_RANDOM_STUFF( "MC_RANDOM_STUFF" );
const flag_id flag_MC_SCIENCE_STUFF( "MC_SCIENCE_STUFF" );
const flag_id flag_MC_TURN_USED( "MC_TURN_USED" );
const flag_id flag_MC_USED( "MC_USED" );
const flag_id flag_MECH_BAT( "MECH_BAT" );
const flag_id flag_MELTS( "MELTS" );
const flag_id flag_MESSY( "MESSY" );
const flag_id flag_MINEABLE( "MINEABLE" );
const flag_id flag_MISSION_ITEM( "MISSION_ITEM" );
const flag_id flag_MOUNTABLE( "MOUNTABLE" );
const flag_id flag_MOUNTED_GUN( "MOUNTED_GUN" );
const flag_id flag_MUSHY( "MUSHY" );
const flag_id flag_MUTE( "MUTE" );
const flag_id flag_MYCUS_OK( "MYCUS_OK" );
const flag_id flag_NANOFAB_TABLE( "NANOFAB_TABLE" );
const flag_id flag_NANOFAB_TEMPLATE( "NANOFAB_TEMPLATE" );
const flag_id flag_NEEDS_NO_LUBE( "NEEDS_NO_LUBE" );
const flag_id flag_NEEDS_UNFOLD( "NEEDS_UNFOLD" );
const flag_id flag_NEGATIVE_MONOTONY_OK( "NEGATIVE_MONOTONY_OK" );
const flag_id flag_NEVER_JAMS( "NEVER_JAMS" );
const flag_id flag_NOCOLLIDE( "NOCOLLIDE" );
const flag_id flag_NOITEM( "NOITEM" );
const flag_id flag_NONCONDUCTIVE( "NONCONDUCTIVE" );
const flag_id flag_NON_FOULING( "NON_FOULING" );
const flag_id flag_NOT_FOOTWEAR( "NOT_FOOTWEAR" );
const flag_id flag_NO_CVD( "NO_CVD" );
const flag_id flag_NO_DROP( "NO_DROP" );
const flag_id flag_NO_FLOOR( "NO_FLOOR" );
const flag_id flag_NO_INGEST( "NO_INGEST" );
const flag_id flag_NO_PACKED( "NO_PACKED" );
const flag_id flag_NO_PARASITES( "NO_PARASITES" );
const flag_id flag_NO_PICKUP_ON_EXAMINE( "NO_PICKUP_ON_EXAMINE" );
const flag_id flag_NO_QUICKDRAW( "NO_QUICKDRAW" );
const flag_id flag_NO_RELOAD( "NO_RELOAD" );
const flag_id flag_NO_REPAIR( "NO_REPAIR" );
const flag_id flag_NO_SALVAGE( "NO_SALVAGE" );
const flag_id flag_NO_SCENT( "NO_SCENT" );
const flag_id flag_NO_SELF_CONNECT( "NO_SELF_CONNECT" );
const flag_id flag_NO_SIGHT( "NO_SIGHT" );
const flag_id flag_NO_SPOIL( "NO_SPOIL" );
const flag_id flag_NO_STERILE( "NO_STERILE" );
const flag_id flag_NO_TAKEOFF( "NO_TAKEOFF" );
const flag_id flag_NO_UNLOAD( "NO_UNLOAD" );
const flag_id flag_NO_UNWIELD( "NO_UNWIELD" );
const flag_id flag_NPC_ACTIVATE( "NPC_ACTIVATE" );
const flag_id flag_NPC_ALT_ATTACK( "NPC_ALT_ATTACK" );
const flag_id flag_NPC_SAFE( "NPC_SAFE" );
const flag_id flag_NPC_THROWN( "NPC_THROWN" );
const flag_id flag_NPC_THROW_NOW( "NPC_THROW_NOW" );
const flag_id flag_NUTRIENT_OVERRIDE( "NUTRIENT_OVERRIDE" );
const flag_id flag_ONLY_ONE( "ONLY_ONE" );
const flag_id flag_OPENCLOSE_INSIDE( "OPENCLOSE_INSIDE" );
const flag_id flag_ORGANIC( "ORGANIC" );
const flag_id flag_OUTER( "OUTER" );
const flag_id flag_OVERSIZE( "OVERSIZE" );
const flag_id flag_PARTIAL_DEAF( "PARTIAL_DEAF" );
const flag_id flag_PAVEMENT( "PAVEMENT" );
const flag_id flag_PERFECT_LOCKPICK( "PERFECT_LOCKPICK" );
const flag_id flag_PERMEABLE( "PERMEABLE" );
const flag_id flag_PERPETUAL( "PERPETUAL" );
const flag_id flag_PERSONAL( "PERSONAL" );
const flag_id flag_PICKABLE( "PICKABLE" );
const flag_id flag_PLACE_ITEM( "PLACE_ITEM" );
const flag_id flag_PLACE_RANDOMLY( "PLACE_RANDOMLY" );
const flag_id flag_PLANT( "PLANT" );
const flag_id flag_PLANTABLE( "PLANTABLE" );
const flag_id flag_PLOWABLE( "PLOWABLE" );
const flag_id flag_POCKETS( "POCKETS" );
const flag_id flag_POLEARM( "POLEARM" );
const flag_id flag_POOLWATER( "POOLWATER" );
const flag_id flag_POWERARMOR_COMPATIBLE( "POWERARMOR_COMPATIBLE" );
const flag_id flag_POWERED( "POWERED" );
const flag_id flag_PREDATOR_FUN( "PREDATOR_FUN" );
const flag_id flag_PRIMITIVE_RANGED_WEAPON( "PRIMITIVE_RANGED_WEAPON" );
const flag_id flag_PROCESSING( "PROCESSING" );
const flag_id flag_PROCESSING_RESULT( "PROCESSING_RESULT" );
const flag_id flag_PSEUDO( "PSEUDO" );
const flag_id flag_PSYSHIELD_PARTIAL( "PSYSHIELD_PARTIAL" );
const flag_id flag_PULPED( "PULPED" );
const flag_id flag_PUMP_ACTION( "PUMP_ACTION" );
const flag_id flag_PUMP_RAIL_COMPATIBLE( "PUMP_RAIL_COMPATIBLE" );
const flag_id flag_QUARTERED( "QUARTERED" );
const flag_id flag_RADIOACTIVE( "RADIOACTIVE" );
const flag_id flag_RADIOCAR( "RADIOCAR" );
const flag_id flag_RADIOCARITEM( "RADIOCARITEM" );
const flag_id flag_RADIOSIGNAL_1( "RADIOSIGNAL_1" );
const flag_id flag_RADIOSIGNAL_2( "RADIOSIGNAL_2" );
const flag_id flag_RADIOSIGNAL_3( "RADIOSIGNAL_3" );
const flag_id flag_RADIO_ACTIVATION( "RADIO_ACTIVATION" );
const flag_id flag_RADIO_CONTAINER( "RADIO_CONTAINER" );
const flag_id flag_RADIO_INVOKE_PROC( "RADIO_INVOKE_PROC" );
const flag_id flag_RADIO_MOD( "RADIO_MOD" );
const flag_id flag_RADIO_MODABLE( "RADIO_MODABLE" );
const flag_id flag_RAD_PROOF( "RAD_PROOF" );
const flag_id flag_RAD_RESIST( "RAD_RESIST" );
const flag_id flag_RAIL( "RAIL" );
const flag_id flag_RAILING( "RAILING" );
const flag_id flag_RAINPROOF( "RAINPROOF" );
const flag_id flag_RAIN_PROTECT( "RAIN_PROTECT" );
const flag_id flag_RAMP( "RAMP" );
const flag_id flag_RAMP_DOWN( "RAMP_DOWN" );
const flag_id flag_RAMP_END( "RAMP_END" );
const flag_id flag_RAMP_UP( "RAMP_UP" );
const flag_id flag_RAW( "RAW" );
const flag_id flag_REACH( "REACH" );
const flag_id flag_REACH3( "REACH3" );
const flag_id flag_REACH_ATTACK( "REACH_ATTACK" );
const flag_id flag_REBREATHER( "REBREATHER" );
const flag_id flag_RECHARGE( "RECHARGE" );
const flag_id flag_REDUCED_BASHING( "REDUCED_BASHING" );
const flag_id flag_REDUCED_WEIGHT( "REDUCED_WEIGHT" );
const flag_id flag_REDUCE_SCENT( "REDUCE_SCENT" );
const flag_id flag_RELOAD_AND_SHOOT( "RELOAD_AND_SHOOT" );
const flag_id flag_RELOAD_EJECT( "RELOAD_EJECT" );
const flag_id flag_RELOAD_ONE( "RELOAD_ONE" );
const flag_id flag_REQUIRES_BALANCE( "REQUIRES_BALANCE" );
const flag_id flag_REQUIRES_TINDER( "REQUIRES_TINDER" );
const flag_id flag_RESTRICT_HANDS( "RESTRICT_HANDS" );
const flag_id flag_REVIVE_SPECIAL( "REVIVE_SPECIAL" );
const flag_id flag_ROAD( "ROAD" );
const flag_id flag_ROLLER_INLINE( "ROLLER_INLINE" );
const flag_id flag_ROLLER_ONE( "ROLLER_ONE" );
const flag_id flag_ROLLER_QUAD( "ROLLER_QUAD" );
const flag_id flag_ROUGH( "ROUGH" );
const flag_id flag_RUBBLE( "RUBBLE" );
const flag_id flag_SAFECRACK( "SAFECRACK" );
const flag_id flag_SALT_WATER( "SALT_WATER" );
const flag_id flag_SEALED( "SEALED" );
const flag_id flag_SEEN_FROM_ABOVE( "SEEN_FROM_ABOVE" );
const flag_id flag_SEMITANGIBLE( "SEMITANGIBLE" );
const flag_id flag_SHALLOW_WATER( "SHALLOW_WATER" );
const flag_id flag_SHARP( "SHARP" );
const flag_id flag_SHORT( "SHORT" );
const flag_id flag_SHRUB( "SHRUB" );
const flag_id flag_SIGN( "SIGN" );
const flag_id flag_SKINNED( "SKINNED" );
const flag_id flag_SKINTIGHT( "SKINTIGHT" );
const flag_id flag_SLEEP_AID( "SLEEP_AID" );
const flag_id flag_SLEEP_AID_CONTAINER( "SLEEP_AID_CONTAINER" );
const flag_id flag_SLEEP_IGNORE( "SLEEP_IGNORE" );
const flag_id flag_SLOWS_MOVEMENT( "SLOWS_MOVEMENT" );
const flag_id flag_SLOWS_THIRST( "SLOWS_THIRST" );
const flag_id flag_SLOW_WIELD( "SLOW_WIELD" );
const flag_id flag_SMALL_PASSAGE( "SMALL_PASSAGE" );
const flag_id flag_SMOKABLE( "SMOKABLE" );
const flag_id flag_SMOKED( "SMOKED" );
const flag_id flag_SOFT( "SOFT" );
const flag_id flag_SOLARPACK( "SOLARPACK" );
const flag_id flag_SOLARPACK_ON( "SOLARPACK_ON" );
const flag_id flag_SPEAR( "SPEAR" );
const flag_id flag_SPEEDLOADER( "SPEEDLOADER" );
const flag_id flag_SPLINT( "SPLINT" );
const flag_id flag_STAB( "STAB" );
const flag_id flag_STAB_IMMUNE( "STAB_IMMUNE" );
const flag_id flag_STRICT_HUMANITARIANISM( "STRICT_HUMANITARIANISM" );
const flag_id flag_STR_DRAW( "STR_DRAW" );
const flag_id flag_STR_RELOAD( "STR_RELOAD" );
const flag_id flag_STURDY( "STURDY" );
const flag_id flag_SUN_GLASSES( "SUN_GLASSES" );
const flag_id flag_SUN_ROOF_ABOVE( "SUN_ROOF_ABOVE" );
const flag_id flag_SUPER_FANCY( "SUPER_FANCY" );
const flag_id flag_SUPPORTS_ROOF( "SUPPORTS_ROOF" );
const flag_id flag_SUPPRESS_SMOKE( "SUPPRESS_SMOKE" );
const flag_id flag_SWIMMABLE( "SWIMMABLE" );
const flag_id flag_SWIM_GOGGLES( "SWIM_GOGGLES" );
const flag_id flag_TACK( "TACK" );
const flag_id flag_TANGLE( "TANGLE" );
const flag_id flag_TARDIS( "TARDIS" );
const flag_id flag_THERMOMETER( "THERMOMETER" );
const flag_id flag_THIN_OBSTACLE( "THIN_OBSTACLE" );
const flag_id flag_TIE_UP( "TIE_UP" );
const flag_id flag_TINDER( "TINDER" );
const flag_id flag_TINY( "TINY" );
const flag_id flag_TOBACCO( "TOBACCO" );
const flag_id flag_TOURNIQUET( "TOURNIQUET" );
const flag_id flag_TOW_CABLE( "TOW_CABLE" );
const flag_id flag_TRADER_AVOID( "TRADER_AVOID" );
const flag_id flag_TRADER_KEEP( "TRADER_KEEP" );
const flag_id flag_TRADER_KEEP_EQUIPPED( "TRADER_KEEP_EQUIPPED" );
const flag_id flag_TRANSLOCATOR( "TRANSLOCATOR" );
const flag_id flag_TRANSPARENT( "TRANSPARENT" );
const flag_id flag_TREE( "TREE" );
const flag_id flag_TWO_WAY_RADIO( "TWO_WAY_RADIO" );
const flag_id flag_UNARMED_WEAPON( "UNARMED_WEAPON" );
const flag_id flag_UNBREAKABLE_MELEE( "UNBREAKABLE_MELEE" );
const flag_id flag_UNDERSIZE( "UNDERSIZE" );
const flag_id flag_UNDERWATER_GUN( "UNDERWATER_GUN" );
const flag_id flag_UNRECOVERABLE( "UNRECOVERABLE" );
const flag_id flag_UNSTABLE( "UNSTABLE" );
const flag_id flag_URSINE_HONEY( "URSINE_HONEY" );
const flag_id flag_USABLE_FIRE( "USABLE_FIRE" );
const flag_id flag_USES_BIONIC_POWER( "USES_BIONIC_POWER" );
const flag_id flag_USE_EAT_VERB( "USE_EAT_VERB" );
const flag_id flag_USE_PLAYER_ENERGY( "USE_PLAYER_ENERGY" );
const flag_id flag_USE_UPS( "USE_UPS" );
const flag_id flag_VARSIZE( "VARSIZE" );
const flag_id flag_VEHICLE( "VEHICLE" );
const flag_id flag_WAIST( "WAIST" );
const flag_id flag_WALL( "WALL" );
const flag_id flag_WATCH( "WATCH" );
const flag_id flag_WATER( "WATER" );
const flag_id flag_WATERPROOF( "WATERPROOF" );
const flag_id flag_WATERPROOF_GUN( "WATERPROOF_GUN" );
const flag_id flag_WATER_BREAK( "WATER_BREAK" );
const flag_id flag_WATER_BREAK_ACTIVE( "WATER_BREAK_ACTIVE" );
const flag_id flag_WATER_DISSOLVE( "WATER_DISSOLVE" );
const flag_id flag_WATER_EXTINGUISH( "WATER_EXTINGUISH" );
const flag_id flag_WATER_FRIENDLY( "WATER_FRIENDLY" );
const flag_id flag_WET( "WET" );
const flag_id flag_WHIP( "WHIP" );
const flag_id flag_WINDOW( "WINDOW" );
const flag_id flag_WIND_EXTINGUISH( "WIND_EXTINGUISH" );
const flag_id flag_WOODFENCE( "WOODFENCE" );
const flag_id flag_WORKOUT_ARMS( "WORKOUT_ARMS" );
const flag_id flag_WORKOUT_LEGS( "WORKOUT_LEGS" );
const flag_id flag_WRITE_MESSAGE( "WRITE_MESSAGE" );
const flag_id flag_YOUNG( "YOUNG" );
const flag_id flag_ZERO_WEIGHT( "ZERO_WEIGHT" );
const flag_id flag_ZOOM( "ZOOM" );
const flag_id flag_Z_TRANSPARENT( "Z_TRANSPARENT" );
const flag_id flag_keep_items( "keep_items" );
const flag_id flag_wooled( "wooled" );
const flag_id json_flag_HIDDEN_ITEM( "HIDDEN_ITEM" );

/*
 * List of known flags, used in both terrain.json and furniture.json.
 * TRANSPARENT - Players and monsters can see through/past it. Also sets ter_t.transparent
 * FLAT - Player can build and move furniture on
 * CONTAINER - Items on this square are hidden until looted by the player
 * PLACE_ITEM - Valid terrain for place_item() to put items on
 * DOOR - Can be opened (used for NPC pathfinding)
 * FLAMMABLE - Can be lit on fire
 * FLAMMABLE_HARD - Harder to light on fire, but still possible
 * DIGGABLE - Digging monsters, seeding monsters, digging with shovel, etc
 * LIQUID - Blocks movement, but isn't a wall (lava, water, etc)
 * SWIMMABLE - Player and monsters can swim through it
 * SHARP - May do minor damage to players/monsters passing through it
 * ROUGH - May hurt the player's feet
 * SEALED - Can't use 'e' to retrieve items, must smash open first
 * NOITEM - Items 'fall off' this space
 * NO_SIGHT - When on this tile sight is reduced to 1
 * NO_SCENT - Scent on this tile (and thus scent diffusing through it) is reduced to 0. This acts like a wall for scent
 * MOUNTABLE - Player can fire mounted weapons from here (e.g. M2 Browning)
 * DESTROY_ITEM - Items that land here are destroyed
 * GOES_DOWN - Can use '>' to go down a level
 * GOES_UP - Can use '<' to go up a level
 * CONSOLE - Used as a computer
 * ALARMED - Sets off an alarm if smashed
 * SUPPORTS_ROOF - Used as a boundary for roof construction
 * MINEABLE - Able to broken with the jackhammer/pickaxe, but does not necessarily support a roof
 * INDOORS - Has roof over it; blocks rain, sunlight, etc.
 * COLLAPSES - Has a roof that can collapse
 * FLAMMABLE_ASH - Burns to ash rather than rubble.
 * REDUCE_SCENT - Reduces scent even more, only works if also bashable
 * FIRE_CONTAINER - Stops fire from spreading (brazier, wood stove, etc)
 * SUPPRESS_SMOKE - Prevents smoke from fires, used by ventilated wood stoves etc
 * PLANT - A "furniture" that grows and fruits
 * LIQUIDCONT - Furniture that contains liquid, allows for contents to be accessed in some checks even if SEALED
 * OPENCLOSE_INSIDE - If it's a door (with an 'open' or 'close' field), it can only be opened or closed if you're inside.
 * PERMEABLE - Allows gases to flow through unimpeded.
 * RAMP - Higher z-levels can be accessed from this tile
 * EASY_DECONSTRUCT - Player can deconstruct this without tools
 * HIDE_PLACE - Creature on this tile can't be seen by other creature not standing on adjacent tiles
 * BLOCK_WIND - This tile will partially block wind
 * FLAT_SURF - Furniture or terrain or vehicle part with flat hard surface (ex. table, but not chair; tree stump, etc.).
 *
 * Currently only used for Fungal conversions
 * WALL - This terrain is an upright obstacle
 * THIN_OBSTACLE - This terrain is a thin obstacle, i.e. fence
 * ORGANIC - This furniture is partly organic
 * FLOWER - This furniture is a flower
 * SHRUB - This terrain is a shrub
 * TREE - This terrain is a tree
 * HARVESTED - This terrain has been harvested so it won't bear any fruit
 * YOUNG - This terrain is a young tree
 * FUNGUS - Fungal covered
 *
 * Furniture only:
 * BLOCKSDOOR - This will boost map terrain's resistance to bashing if str_*_blocked is set (see map_bash_info)
 * WORKBENCH1/WORKBENCH2/WORKBENCH3 - This is an adequate/good/great workbench for crafting.  Must be paired with a workbench iexamine.
 */

static const std::unordered_map<flag_id, ter_bitflags> ter_bitflags_map = { {
        { flag_DESTROY_ITEM,             TFLAG_DESTROY_ITEM},   // add/spawn_item*()
        { flag_ROUGH,                    TFLAG_ROUGH },          // monmove
        { flag_UNSTABLE,                 TFLAG_UNSTABLE },       // monmove
        { flag_LIQUID,                   TFLAG_LIQUID },         // *move(), add/spawn_item*()
        { flag_FIRE_CONTAINER,           TFLAG_FIRE_CONTAINER }, // fire
        { flag_DIGGABLE,                 TFLAG_DIGGABLE },       // monmove
        { flag_SUPPRESS_SMOKE,           TFLAG_SUPPRESS_SMOKE }, // fire
        { flag_FLAMMABLE_HARD,           TFLAG_FLAMMABLE_HARD }, // fire
        { flag_SEALED,                   TFLAG_SEALED },         // Fire, acid
        { flag_ALLOW_FIELD_EFFECT,       TFLAG_ALLOW_FIELD_EFFECT }, // Fire, acid
        { flag_COLLAPSES,                TFLAG_COLLAPSES },      // building "remodeling"
        { flag_FLAMMABLE,                TFLAG_FLAMMABLE },      // fire bad! fire SLOW!
        { flag_REDUCE_SCENT,             TFLAG_REDUCE_SCENT },   // ...and the other half is update_scent
        { flag_INDOORS,                  TFLAG_INDOORS },        // vehicle gain_moves, weather
        { flag_SHARP,                    TFLAG_SHARP },          // monmove
        { flag_SUPPORTS_ROOF,            TFLAG_SUPPORTS_ROOF },  // and by building "remodeling" I mean hulkSMASH
        { flag_MINEABLE,                 TFLAG_MINEABLE },       // allows mining
        { flag_SWIMMABLE,                TFLAG_SWIMMABLE },      // monmove, many fields
        { flag_TRANSPARENT,              TFLAG_TRANSPARENT },    // map::is_transparent / lightmap
        { flag_NOITEM,                   TFLAG_NOITEM },         // add/spawn_item*()
        { flag_NO_SIGHT,                 TFLAG_NO_SIGHT },       // Sight reduced to 1 on this tile
        { flag_FLAMMABLE_ASH,            TFLAG_FLAMMABLE_ASH },  // oh hey fire. again.
        { flag_WALL,                     TFLAG_WALL },           // connects to other walls
        { flag_NO_SCENT,                 TFLAG_NO_SCENT },       // cannot have scent values, which prevents scent diffusion through this tile
        { flag_DEEP_WATER,               TFLAG_DEEP_WATER },     // Deep enough to submerge things
        { flag_SHALLOW_WATER,            TFLAG_SHALLOW_WATER },  // Water, but not deep enough to submerge the player
        { flag_CURRENT,                  TFLAG_CURRENT },        // Water is flowing.
        { flag_HARVESTED,                TFLAG_HARVESTED },      // harvested.  will not bear fruit.
        { flag_PERMEABLE,                TFLAG_PERMEABLE },      // gases can flow through.
        { flag_AUTO_WALL_SYMBOL,         TFLAG_AUTO_WALL_SYMBOL }, // automatically create the appropriate wall
        { flag_CONNECT_TO_WALL,          TFLAG_CONNECT_TO_WALL }, // superseded by ter_connects, retained for json backward compatibility
        { flag_CLIMBABLE,                TFLAG_CLIMBABLE },      // Can be climbed over
        { flag_GOES_DOWN,                TFLAG_GOES_DOWN },      // Allows non-flying creatures to move downwards
        { flag_GOES_UP,                  TFLAG_GOES_UP },        // Allows non-flying creatures to move upwards
        { flag_NO_FLOOR,                 TFLAG_NO_FLOOR },       // Things should fall when placed on this tile
        { flag_SEEN_FROM_ABOVE,          TFLAG_SEEN_FROM_ABOVE },// This should be visible if the tile above has no floor
        { flag_HIDE_PLACE,               TFLAG_HIDE_PLACE },     // Creature on this tile can't be seen by other creature not standing on adjacent tiles
        { flag_BLOCK_WIND,               TFLAG_BLOCK_WIND },     // This tile will partially block the wind.
        { flag_FLAT,                     TFLAG_FLAT },           // This tile is flat.
        { flag_RAMP,                     TFLAG_RAMP },           // Can be used to move up a z-level
        { flag_RAMP_DOWN,                TFLAG_RAMP_DOWN },      // Anything entering this tile moves down a z-level
        { flag_RAMP_UP,                  TFLAG_RAMP_UP },        // Anything entering this tile moves up a z-level
        { flag_RAIL,                     TFLAG_RAIL },           // Rail tile (used heavily)
        { flag_THIN_OBSTACLE,            TFLAG_THIN_OBSTACLE },  // Passable by players and monsters. Vehicles destroy it.
        { flag_Z_TRANSPARENT,            TFLAG_Z_TRANSPARENT },  // Doesn't block vision passing through the z-level
        { flag_SMALL_PASSAGE,            TFLAG_SMALL_PASSAGE },   // A small passage, that large or huge things cannot pass through
        { flag_SUN_ROOF_ABOVE,           TFLAG_SUN_ROOF_ABOVE },   // This furniture has a "fake roof" above, that blocks sunlight (see #44421).
        { flag_FUNGUS,                   TFLAG_FUNGUS }            // Fungal covered.
    }
};

static const std::unordered_map<flag_id, ter_connects> ter_connects_map = { {
        { flag_WALL,                     TERCONN_WALL },        // implied by TFLAG_CONNECT_TO_WALL, TFLAG_AUTO_WALL_SYMBOL or TFLAG_WALL
        { flag_CHAINFENCE,               TERCONN_CHAINFENCE },
        { flag_WOODFENCE,                TERCONN_WOODFENCE },
        { flag_RAILING,                  TERCONN_RAILING },
        { flag_WATER,                    TERCONN_WATER },
        { flag_POOLWATER,                TERCONN_POOLWATER },
        { flag_PAVEMENT,                 TERCONN_PAVEMENT },
        { flag_RAIL,                     TERCONN_RAIL },
        { flag_COUNTER,                  TERCONN_COUNTER },
        { flag_CANVAS_WALL,              TERCONN_CANVAS_WALL },
    }
};

namespace
{
generic_factory<json_flag> json_flags_all( "json_flags" );
} // namespace

/** @relates string_id */
template<>
bool flag_id::is_valid() const
{
    return json_flags_all.is_valid( *this );
}

/** @relates string_id */
template<>
const json_flag &flag_id::obj() const
{
    return json_flags_all.obj( *this );
}

json_flag::operator bool() const
{
    return id.is_valid();
}

const json_flag &json_flag::get( const std::string &id )
{
    static const json_flag null_value = json_flag();
    const flag_id f_id( id );
    return f_id.is_valid() ? *f_id : null_value;
}

void json_flag::load( const JsonObject &jo, const std::string & )
{
    // TODO: mark fields as mandatory where appropriate
    optional( jo, was_loaded, "info", info_ );
    optional( jo, was_loaded, "conflicts", conflicts_ );
    optional( jo, was_loaded, "inherit", inherit_, true );
    optional( jo, was_loaded, "craft_inherit", craft_inherit_, false );
    optional( jo, was_loaded, "requires_flag", requires_flag_ );
    optional( jo, was_loaded, "taste_mod", taste_mod_ );
    optional( jo, was_loaded, "restriction", restriction_ );
}

void json_flag::check_consistency()
{
    json_flags_all.check();
}

void json_flag::reset()
{
    json_flags_all.reset();
}

void json_flag::load_all( const JsonObject &jo, const std::string &src )
{
    json_flags_all.load( jo, src );
}

void json_flag::check() const
{
    for( const auto &conflicting : conflicts_ ) {
        if( !flag_id( conflicting ).is_valid() ) {
            debugmsg( "flag definition %s specifies unknown conflicting field %s", id.str(),
                      conflicting );
        }
    }
}

void json_flag::finalize_all()
{
    json_flags_all.finalize();
}

bool json_flag::is_ready()
{
    return !json_flags_all.empty();
}

const std::vector<json_flag> &json_flag::get_all()
{
    return json_flags_all.get_all();
}
