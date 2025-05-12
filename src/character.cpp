#include "character.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cwctype>
#include <functional>
#include <initializer_list>
#include <memory>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "addiction.h"
#include "anatomy.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bionics.h"
#include "cached_options.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character_attire.h"
#include "character_martial_arts.h"
#include "city.h"
#include "color.h"
#include "construction.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "current_map.h"
#include "cursesdef.h"
#include "debug.h"
#include "dialogue.h"
#include "disease.h"
#include "display.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "effect_source.h"
#include "enum_traits.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "fault.h"
#include "field.h"
#include "field_type.h"
#include "flag.h"
#include "fungal_effects.h"
#include "game.h"
#include "game_constants.h"
#include "gun_mode.h"
#include "handle_liquid.h"
#include "input_context.h"
#include "input_enums.h"
#include "inventory.h"
#include "item_location.h"
#include "item_pocket.h"
#include "item_stack.h"
#include "item_tname.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "lightmap.h"
#include "line.h"
#include "localized_comparator.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "map_selector.h"
#include "mapdata.h"
#include "martialarts.h"
#include "math_defines.h"
#include "math_parser_diag_value.h"
#include "memorial_logger.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "morale.h"
#include "move_mode.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overlay_ordering.h"
#include "overmap_types.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "profession.h"
#include "proficiency.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "scent_map.h"
#include "skill.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "submap.h"  // IWYU pragma: keep
#include "text_snippets.h"
#include "translation.h"
#include "translations.h"
#include "trap.h"
#include "uilist.h"
#include "ui_manager.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "viewer.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "weather_type.h"

class activity_actor;

static const activity_id ACT_AUTODRIVE( "ACT_AUTODRIVE" );
static const activity_id ACT_CONSUME_DRINK_MENU( "ACT_CONSUME_DRINK_MENU" );
static const activity_id ACT_CONSUME_FOOD_MENU( "ACT_CONSUME_FOOD_MENU" );
static const activity_id ACT_CONSUME_MEDS_MENU( "ACT_CONSUME_MEDS_MENU" );
static const activity_id ACT_EAT_MENU( "ACT_EAT_MENU" );
static const activity_id ACT_FISH( "ACT_FISH" );
static const activity_id ACT_GAME( "ACT_GAME" );
static const activity_id ACT_HAND_CRANK( "ACT_HAND_CRANK" );
static const activity_id ACT_HEATING( "ACT_HEATING" );
static const activity_id ACT_MEDITATE( "ACT_MEDITATE" );
static const activity_id ACT_MEND_ITEM( "ACT_MEND_ITEM" );
static const activity_id ACT_MOVE_ITEMS( "ACT_MOVE_ITEMS" );
static const activity_id ACT_OPERATION( "ACT_OPERATION" );
static const activity_id ACT_READ( "ACT_READ" );
static const activity_id ACT_SOCIALIZE( "ACT_SOCIALIZE" );
static const activity_id ACT_STUDY_SPELL( "ACT_STUDY_SPELL" );
static const activity_id ACT_TOOLMOD_ADD( "ACT_TOOLMOD_ADD" );
static const activity_id ACT_TRAVELLING( "ACT_TRAVELLING" );
static const activity_id ACT_TREE_COMMUNION( "ACT_TREE_COMMUNION" );
static const activity_id ACT_TRY_SLEEP( "ACT_TRY_SLEEP" );
static const activity_id ACT_VIBE( "ACT_VIBE" );
static const activity_id ACT_WAIT( "ACT_WAIT" );
static const activity_id ACT_WAIT_NPC( "ACT_WAIT_NPC" );
static const activity_id ACT_WAIT_STAMINA( "ACT_WAIT_STAMINA" );

static const addiction_id addiction_opiate( "opiate" );
static const addiction_id addiction_sleeping_pill( "sleeping pill" );

static const ammotype ammo_battery( "battery" );

static const anatomy_id anatomy_human_anatomy( "human_anatomy" );

static const bionic_id bio_gills( "bio_gills" );
static const bionic_id bio_ground_sonar( "bio_ground_sonar" );
static const bionic_id bio_memory( "bio_memory" );
static const bionic_id bio_ods( "bio_ods" );
static const bionic_id bio_railgun( "bio_railgun" );
static const bionic_id bio_shock_absorber( "bio_shock_absorber" );
static const bionic_id bio_sleep_shutdown( "bio_sleep_shutdown" );
static const bionic_id bio_soporific( "bio_soporific" );
static const bionic_id bio_synlungs( "bio_synlungs" );
static const bionic_id bio_targeting( "bio_targeting" );
static const bionic_id bio_uncanny_dodge( "bio_uncanny_dodge" );
static const bionic_id bio_ups( "bio_ups" );
static const bionic_id bio_voice( "bio_voice" );

static const character_modifier_id character_modifier_aim_speed_dex_mod( "aim_speed_dex_mod" );
static const character_modifier_id character_modifier_aim_speed_mod( "aim_speed_mod" );
static const character_modifier_id character_modifier_aim_speed_skill_mod( "aim_speed_skill_mod" );
static const character_modifier_id character_modifier_bleed_staunch_mod( "bleed_staunch_mod" );
static const character_modifier_id
character_modifier_crawl_speed_movecost_mod( "crawl_speed_movecost_mod" );
static const character_modifier_id character_modifier_limb_fall_mod( "limb_fall_mod" );
static const character_modifier_id character_modifier_limb_run_cost_mod( "limb_run_cost_mod" );
static const character_modifier_id character_modifier_limb_str_mod( "limb_str_mod" );
static const character_modifier_id
character_modifier_melee_stamina_cost_mod( "melee_stamina_cost_mod" );
static const character_modifier_id
character_modifier_move_mode_move_cost_mod( "move_mode_move_cost_mod" );
static const character_modifier_id
character_modifier_ranged_dispersion_vision_mod( "ranged_dispersion_vision_mod" );
static const character_modifier_id
character_modifier_stamina_move_cost_mod( "stamina_move_cost_mod" );
static const character_modifier_id
character_modifier_stamina_recovery_breathing_mod( "stamina_recovery_breathing_mod" );
static const character_modifier_id character_modifier_swim_mod( "swim_mod" );

static const damage_type_id damage_acid( "acid" );
static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_cut( "cut" );
static const damage_type_id damage_electric( "electric" );
static const damage_type_id damage_heat( "heat" );
static const damage_type_id damage_stab( "stab" );

static const effect_on_condition_id effect_on_condition_add_effect( "add_effect" );

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_alarm_clock( "alarm_clock" );
static const efftype_id effect_bandaged( "bandaged" );
static const efftype_id effect_beartrap( "beartrap" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_blood_spiders( "blood_spiders" );
static const efftype_id effect_bloodworms( "bloodworms" );
static const efftype_id effect_boomered( "boomered" );
static const efftype_id effect_brainworms( "brainworms" );
static const efftype_id effect_chafing( "chafing" );
static const efftype_id effect_common_cold( "common_cold" );
static const efftype_id effect_contacts( "contacts" );
static const efftype_id effect_controlled( "controlled" );
static const efftype_id effect_corroding( "corroding" );
static const efftype_id effect_cough_suppress( "cough_suppress" );
static const efftype_id effect_cramped_space( "cramped_space" );
static const efftype_id effect_darkness( "darkness" );
static const efftype_id effect_deaf( "deaf" );
static const efftype_id effect_dermatik( "dermatik" );
static const efftype_id effect_disinfected( "disinfected" );
static const efftype_id effect_disrupted_sleep( "disrupted_sleep" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_fearparalyze( "fearparalyze" );
static const efftype_id effect_flu( "flu" );
static const efftype_id effect_foodpoison( "foodpoison" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_glowing( "glowing" );
static const efftype_id effect_glowy_led( "glowy_led" );
static const efftype_id effect_harnessed( "harnessed" );
static const efftype_id effect_heavysnare( "heavysnare" );
static const efftype_id effect_in_pit( "in_pit" );
static const efftype_id effect_incorporeal( "incorporeal" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_jetinjector( "jetinjector" );
static const efftype_id effect_lack_sleep( "lack_sleep" );
static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_masked_scent( "masked_scent" );
static const efftype_id effect_mech_recon_vision( "mech_recon_vision" );
static const efftype_id effect_melatonin( "melatonin" );
static const efftype_id effect_mending( "mending" );
static const efftype_id effect_meth( "meth" );
static const efftype_id effect_monster_saddled( "monster_saddled" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_nausea( "nausea" );
static const efftype_id effect_no_sight( "no_sight" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_paincysts( "paincysts" );
static const efftype_id effect_pkill1_acetaminophen( "pkill1_acetaminophen" );
static const efftype_id effect_pkill1_generic( "pkill1_generic" );
static const efftype_id effect_pkill1_nsaid( "pkill1_nsaid" );
static const efftype_id effect_pkill2( "pkill2" );
static const efftype_id effect_pkill3( "pkill3" );
static const efftype_id effect_recently_coughed( "recently_coughed" );
static const efftype_id effect_recover( "recover" );
static const efftype_id effect_ridden( "ridden" );
static const efftype_id effect_riding( "riding" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_slept_through_alarm( "slept_through_alarm" );
static const efftype_id effect_stumbled_into_invisible( "stumbled_into_invisible" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_subaquatic_sonar( "subaquatic_sonar" );
static const efftype_id effect_tapeworm( "tapeworm" );
static const efftype_id effect_tied( "tied" );
static const efftype_id effect_transition_contacts( "transition_contacts" );
static const efftype_id effect_winded( "winded" );

static const fault_id fault_bionic_salvaged( "fault_bionic_salvaged" );

static const field_type_str_id field_fd_clairvoyant( "fd_clairvoyant" );

static const itype_id fuel_type_animal( "animal" );
static const itype_id fuel_type_muscle( "muscle" );
static const itype_id itype_UPS( "UPS" );
static const itype_id itype_apparatus( "apparatus" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_beartrap( "beartrap" );
static const itype_id itype_cookbook_human( "cookbook_human" );
static const itype_id itype_e_handcuffs( "e_handcuffs" );
static const itype_id itype_fire( "fire" );
static const itype_id itype_foodperson_mask( "foodperson_mask" );
static const itype_id itype_foodperson_mask_on( "foodperson_mask_on" );
static const itype_id itype_human_sample( "human_sample" );
static const itype_id itype_rm13_armor_on( "rm13_armor_on" );
static const itype_id itype_rope_6( "rope_6" );
static const itype_id itype_snare_trigger( "snare_trigger" );

static const json_character_flag json_flag_ACIDBLOOD( "ACIDBLOOD" );
static const json_character_flag json_flag_ALARMCLOCK( "ALARMCLOCK" );
static const json_character_flag json_flag_ALWAYS_HEAL( "ALWAYS_HEAL" );
static const json_character_flag json_flag_BIONIC_LIMB( "BIONIC_LIMB" );
static const json_character_flag json_flag_BLIND( "BLIND" );
static const json_character_flag json_flag_CANNOT_CHANGE_TEMPERATURE( "CANNOT_CHANGE_TEMPERATURE" );
static const json_character_flag json_flag_CANNOT_MOVE( "CANNOT_MOVE" );
static const json_character_flag json_flag_CANNOT_TAKE_DAMAGE( "CANNOT_TAKE_DAMAGE" );
static const json_character_flag json_flag_CLAIRVOYANCE( "CLAIRVOYANCE" );
static const json_character_flag json_flag_CLAIRVOYANCE_PLUS( "CLAIRVOYANCE_PLUS" );
static const json_character_flag json_flag_DEAF( "DEAF" );
static const json_character_flag json_flag_ENHANCED_VISION( "ENHANCED_VISION" );
static const json_character_flag json_flag_EYE_MEMBRANE( "EYE_MEMBRANE" );
static const json_character_flag json_flag_FEATHER_FALL( "FEATHER_FALL" );
static const json_character_flag json_flag_GLIDE( "GLIDE" );
static const json_character_flag json_flag_GLIDING( "GLIDING" );
static const json_character_flag json_flag_GRAB( "GRAB" );
static const json_character_flag json_flag_HEAL_OVERRIDE( "HEAL_OVERRIDE" );
static const json_character_flag json_flag_HEATSINK( "HEATSINK" );
static const json_character_flag json_flag_HYPEROPIC( "HYPEROPIC" );
static const json_character_flag json_flag_IMMUNE_HEARING_DAMAGE( "IMMUNE_HEARING_DAMAGE" );
static const json_character_flag json_flag_INFECTION_IMMUNE( "INFECTION_IMMUNE" );
static const json_character_flag json_flag_INSECTBLOOD( "INSECTBLOOD" );
static const json_character_flag json_flag_INVERTEBRATEBLOOD( "INVERTEBRATEBLOOD" );
static const json_character_flag json_flag_INVISIBLE( "INVISIBLE" );
static const json_character_flag json_flag_MYOPIC( "MYOPIC" );
static const json_character_flag json_flag_MYOPIC_IN_LIGHT( "MYOPIC_IN_LIGHT" );
static const json_character_flag json_flag_NIGHT_VISION( "NIGHT_VISION" );
static const json_character_flag json_flag_NON_THRESH( "NON_THRESH" );
static const json_character_flag json_flag_NO_RADIATION( "NO_RADIATION" );
static const json_character_flag json_flag_NO_THIRST( "NO_THIRST" );
static const json_character_flag json_flag_NVG_GREEN( "NVG_GREEN" );
static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );
static const json_character_flag json_flag_PLANTBLOOD( "PLANTBLOOD" );
static const json_character_flag json_flag_PRED2( "PRED2" );
static const json_character_flag json_flag_PRED3( "PRED3" );
static const json_character_flag json_flag_PRED4( "PRED4" );
static const json_character_flag json_flag_PSYCHOPATH( "PSYCHOPATH" );
static const json_character_flag json_flag_READ_IN_DARKNESS( "READ_IN_DARKNESS" );
static const json_character_flag json_flag_SAPIOVORE( "SAPIOVORE" );
static const json_character_flag json_flag_SEESLEEP( "SEESLEEP" );
static const json_character_flag json_flag_STEADY( "STEADY" );
static const json_character_flag json_flag_STOP_SLEEP_DEPRIVATION( "STOP_SLEEP_DEPRIVATION" );
static const json_character_flag json_flag_SUPER_CLAIRVOYANCE( "SUPER_CLAIRVOYANCE" );
static const json_character_flag json_flag_TOUGH_FEET( "TOUGH_FEET" );
static const json_character_flag json_flag_UNCANNY_DODGE( "UNCANNY_DODGE" );
static const json_character_flag json_flag_WALK_UNDERWATER( "WALK_UNDERWATER" );
static const json_character_flag json_flag_WATCH( "WATCH" );
static const json_character_flag json_flag_WATERWALKING( "WATERWALKING" );
static const json_character_flag json_flag_WEBBED_FEET( "WEBBED_FEET" );
static const json_character_flag json_flag_WEBBED_HANDS( "WEBBED_HANDS" );
static const json_character_flag json_flag_WINGS_2( "WINGS_2" );
static const json_character_flag json_flag_WING_ARMS( "WING_ARMS" );
static const json_character_flag json_flag_WING_GLIDE( "WING_GLIDE" );

static const limb_score_id limb_score_balance( "balance" );
static const limb_score_id limb_score_breathing( "breathing" );
static const limb_score_id limb_score_grip( "grip" );
static const limb_score_id limb_score_lift( "lift" );
static const limb_score_id limb_score_manip( "manip" );
static const limb_score_id limb_score_night_vis( "night_vis" );
static const limb_score_id limb_score_reaction( "reaction" );
static const limb_score_id limb_score_vision( "vision" );

static const material_id material_budget_steel( "budget_steel" );
static const material_id material_ch_steel( "ch_steel" );
static const material_id material_flesh( "flesh" );
static const material_id material_hc_steel( "hc_steel" );
static const material_id material_hflesh( "hflesh" );
static const material_id material_iron( "iron" );
static const material_id material_lc_steel( "lc_steel" );
static const material_id material_mc_steel( "mc_steel" );
static const material_id material_qt_steel( "qt_steel" );
static const material_id material_steel( "steel" );

static const morale_type morale_cold( "morale_cold" );
static const morale_type morale_hot( "morale_hot" );

static const move_mode_id move_mode_prone( "prone" );
static const move_mode_id move_mode_walk( "walk" );

static const mtype_id mon_player_blob( "mon_player_blob" );

static const proficiency_id proficiency_prof_parkour( "prof_parkour" );
static const proficiency_id proficiency_prof_spotting( "prof_spotting" );
static const proficiency_id proficiency_prof_traps( "prof_traps" );
static const proficiency_id proficiency_prof_trapsetting( "prof_trapsetting" );
static const proficiency_id proficiency_prof_wound_care( "prof_wound_care" );
static const proficiency_id proficiency_prof_wound_care_expert( "prof_wound_care_expert" );

static const quality_id qual_DRILL( "DRILL" );
static const quality_id qual_HAMMER( "HAMMER" );
static const quality_id qual_LIFT( "LIFT" );
static const quality_id qual_SCREW( "SCREW" );

static const scenttype_id scent_sc_human( "sc_human" );

static const skill_id skill_archery( "archery" );
static const skill_id skill_dodge( "dodge" );
static const skill_id skill_driving( "driving" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_pistol( "pistol" );
static const skill_id skill_speech( "speech" );
static const skill_id skill_swimming( "swimming" );
static const skill_id skill_throw( "throw" );

static const species_id species_HUMAN( "HUMAN" );

static const start_location_id start_location_sloc_shelter_a( "sloc_shelter_a" );

static const trait_id trait_ADRENALINE( "ADRENALINE" );
static const trait_id trait_BADBACK( "BADBACK" );
static const trait_id trait_BIRD_EYE( "BIRD_EYE" );
static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_CEPH_VISION( "CEPH_VISION" );
static const trait_id trait_CF_HAIR( "CF_HAIR" );
static const trait_id trait_CHEMIMBALANCE( "CHEMIMBALANCE" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_CLUMSY( "CLUMSY" );
static const trait_id trait_DEBUG_CLOAK( "DEBUG_CLOAK" );
static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_DEBUG_LS( "DEBUG_LS" );
static const trait_id trait_DEBUG_NIGHTVISION( "DEBUG_NIGHTVISION" );
static const trait_id trait_DEBUG_NODMG( "DEBUG_NODMG" );
static const trait_id trait_DEBUG_SILENT( "DEBUG_SILENT" );
static const trait_id trait_DEBUG_STAMINA( "DEBUG_STAMINA" );
static const trait_id trait_DEFT( "DEFT" );
static const trait_id trait_DISRESISTANT( "DISRESISTANT" );
static const trait_id trait_DOWN( "DOWN" );
static const trait_id trait_EATHEALTH( "EATHEALTH" );
static const trait_id trait_ELFA_FNV( "ELFA_FNV" );
static const trait_id trait_ELFA_NV( "ELFA_NV" );
static const trait_id trait_FACIAL_HAIR_NONE( "FACIAL_HAIR_NONE" );
static const trait_id trait_FAERIECREATURE( "FAERIECREATURE" );
static const trait_id trait_FAT( "FAT" );
static const trait_id trait_FEL_NV( "FEL_NV" );
static const trait_id trait_GILLS( "GILLS" );
static const trait_id trait_GILLS_CEPH( "GILLS_CEPH" );
static const trait_id trait_GLASSJAW( "GLASSJAW" );
static const trait_id trait_HATES_BOOKS( "HATES_BOOKS" );
static const trait_id trait_HEAVYSLEEPER( "HEAVYSLEEPER" );
static const trait_id trait_HEAVYSLEEPER2( "HEAVYSLEEPER2" );
static const trait_id trait_HIBERNATE( "HIBERNATE" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_INSOMNIA( "INSOMNIA" );
static const trait_id trait_LEG_TENT_BRACE( "LEG_TENT_BRACE" );
static const trait_id trait_LIGHTSTEP( "LIGHTSTEP" );
static const trait_id trait_LOVES_BOOKS( "LOVES_BOOKS" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_M_SKIN3( "M_SKIN3" );
static const trait_id trait_NIGHTVISION( "NIGHTVISION" );
static const trait_id trait_NIGHTVISION2( "NIGHTVISION2" );
static const trait_id trait_NIGHTVISION3( "NIGHTVISION3" );
static const trait_id trait_NOMAD( "NOMAD" );
static const trait_id trait_NOMAD2( "NOMAD2" );
static const trait_id trait_NOMAD3( "NOMAD3" );
static const trait_id trait_PACIFIST( "PACIFIST" );
static const trait_id trait_PARAIMMUNE( "PARAIMMUNE" );
static const trait_id trait_PAWS( "PAWS" );
static const trait_id trait_PAWS_LARGE( "PAWS_LARGE" );
static const trait_id trait_PER_SLIME( "PER_SLIME" );
static const trait_id trait_PER_SLIME_OK( "PER_SLIME_OK" );
static const trait_id trait_PROF_DICEMASTER( "PROF_DICEMASTER" );
static const trait_id trait_PROF_FOODP( "PROF_FOODP" );
static const trait_id trait_PROF_SKATER( "PROF_SKATER" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_QUILLS( "QUILLS" );
static const trait_id trait_ROOTS2( "ROOTS2" );
static const trait_id trait_ROOTS3( "ROOTS3" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_SAVANT( "SAVANT" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_SHELL3( "SHELL3" );
static const trait_id trait_SHOUT2( "SHOUT2" );
static const trait_id trait_SHOUT3( "SHOUT3" );
static const trait_id trait_SLIMESPAWNER( "SLIMESPAWNER" );
static const trait_id trait_SLIMY( "SLIMY" );
static const trait_id trait_SPINES( "SPINES" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );
static const trait_id trait_STRONGBACK( "STRONGBACK" );
static const trait_id trait_SUNLIGHT_DEPENDENT( "SUNLIGHT_DEPENDENT" );
static const trait_id trait_THORNS( "THORNS" );
static const trait_id trait_TRANSPIRATION( "TRANSPIRATION" );
static const trait_id trait_UNDINE_SLEEP_WATER( "UNDINE_SLEEP_WATER" );
static const trait_id trait_URSINE_EYE( "URSINE_EYE" );
static const trait_id trait_VISCOUS( "VISCOUS" );
static const trait_id trait_WATERSLEEP( "WATERSLEEP" );

static const vitamin_id vitamin_calcium( "calcium" );
static const vitamin_id vitamin_iron( "iron" );

static const std::set<material_id> ferric = { material_iron, material_steel, material_budget_steel, material_ch_steel, material_hc_steel, material_lc_steel, material_mc_steel, material_qt_steel };

static const std::string type_hair_color( "hair_color" );
static const std::string type_hair_style( "hair_style" );
static const std::string type_skin_tone( "skin_tone" );
static const std::string type_facial_hair( "facial_hair" );
static const std::string type_eye_color( "eye_color" );

namespace io
{

template<>
std::string enum_to_string<blood_type>( blood_type data )
{
    switch( data ) {
            // *INDENT-OFF*
        case blood_type::blood_O: return "O";
        case blood_type::blood_A: return "A";
        case blood_type::blood_B: return "B";
        case blood_type::blood_AB: return "AB";
            // *INDENT-ON*
        case blood_type::num_bt:
            break;
    }
    cata_fatal( "Invalid blood_type" );
}

} // namespace io

void Character::queue_effect( const std::string &name, const time_duration &delay,
                              const time_duration &effect_duration )
{
    global_variables::impl_t ctx = {
        { "effect", diag_value{ name } },
        { "duration", diag_value{ to_turns<int>( effect_duration ) } }
    };

    effect_on_conditions::queue_effect_on_condition( delay, effect_on_condition_add_effect, *this,
            ctx );
}

int Character::count_queued_effects( const std::string &effect ) const
{
    return std::count_if( queued_effect_on_conditions.list.begin(),
    queued_effect_on_conditions.list.end(), [&effect]( const queued_eoc & eoc ) {
        return eoc.eoc == effect_on_condition_add_effect &&
               eoc.context.at( "effect" ).str() == effect;
    } );
}

// *INDENT-OFF*
Character::Character() :
    id( -1 ),
    next_climate_control_check( calendar::before_time_starts ),
    last_climate_control_ret( false )
{
    randomize_blood();
    cached_organic_size = 1.0f;
    str_cur = 8;
    str_max = 8;
    dex_cur = 8;
    dex_max = 8;
    int_cur = 8;
    int_max = 8;
    per_cur = 8;
    per_max = 8;
    set_dodges_left(1);
    blocks_left = 1;
    str_bonus = 0;
    dex_bonus = 0;
    per_bonus = 0;
    int_bonus = 0;
    ppen_str = 0;
    ppen_dex = 0;
    ppen_int = 0;
    ppen_per = 0;
    ppen_spd = 0;
    lifestyle = 0;
    daily_health = 0;
    health_tally = 0;
    hunger = 0;
    thirst = 0;
    sleepiness = 0;
    sleep_deprivation = 0;
    daily_sleep = 0_turns;
    continuous_sleep = 0_turns;
    radiation = 0;
    slow_rad = 0;
    set_stim( 0 );
    arms_power_use = 0;
    legs_power_use = 0;
    arms_stam_mult = 1.0f;
    legs_stam_mult = 1.0f;
    set_stamina( 10000 ); //Temporary value for stamina. It will be reset later from external json option.
    cardio_acc = 1000; // Temporary cardio accumulator. It will be updated when reset_cardio_acc is called.
    set_anatomy( anatomy_human_anatomy );
    update_type_of_scent( true );
    pkill = 0;
    // 55 Mcal or 55k kcal
    healthy_calories = 55000000;
    base_cardio_acc = 1000;
    // this makes sure characters start with normal bmi
    stored_calories = healthy_calories - 1000000;
    initialize_stomach_contents();

    name.clear();
    custom_profession.clear();

    *path_settings = pathfinding_settings{ 0, 1000, 1000, 0, true, true, true, true, false, true, creature_size::medium };

    move_mode = move_mode_walk;
    next_expected_position = std::nullopt;
    invalidate_crafting_inventory();

    set_power_level( 0_kJ );
    cash = 0;
    scent = 500;
    male = true;
    prof = profession::has_initialized() ? profession::generic() :
           nullptr; //workaround for a potential structural limitation, see player::create
    start_location = start_location_sloc_shelter_a;
    moves = 100;
    oxygen = 0;
    in_vehicle = false;
    controlling_vehicle = false;
    grab_point = tripoint_rel_ms::zero;
    hauling = false;
    set_focus( 100 );
    last_item = itype_id::NULL_ID();
    sight_max = 9999;
    last_batch = 0;
    death_drops = true;
    nv_cached = false;
    leak_level = 0.0f;
    leak_level_dirty = true;
    volume = 0;
    set_value( "THIEF_MODE", "THIEF_ASK" );
    for( const auto &v : vitamin::all() ) {
        vitamin_levels[ v.first ] = 0;
        daily_vitamins[v.first] = { 0,0 };
    }
    // Only call these if game is initialized
    if( !!g && json_flag::is_ready() ) {
        recalc_sight_limits();
        trait_flag_cache.clear();
        bio_flag_cache.clear();
        calc_encumbrance();
        worn.recalc_ablative_blocking(this);
    }
}
// *INDENT-ON*

Character::~Character() = default;
Character::Character( Character && ) noexcept( map_is_noexcept ) = default;
Character &Character::operator=( Character && ) noexcept( list_is_noexcept ) = default;

void Character::setID( character_id i, bool force )
{
    if( id.is_valid() && !force ) {
        debugmsg( "tried to set id of a npc/player, but has already a id: %d", id.get_value() );
    } else if( !i.is_valid() && !force ) {
        debugmsg( "tried to set invalid id of a npc/player: %d", i.get_value() );
    } else {
        id = i;
    }
}

character_id Character::getID() const
{
    return this->id;
}

void Character::swap_character( Character &other )
{
    npc tmp_npc;
    Character &tmp = tmp_npc;
    tmp = std::move( other );
    other = std::move( *this );
    *this = std::move( tmp );
}

void Character::randomize_height()
{
    // Height distribution data is taken from CDC distributes statistics for the US population
    // https://github.com/CleverRaven/Cataclysm-DDA/pull/49270#issuecomment-861339732
    const int x = std::round( normal_roll( 168.35, 15.50 ) );
    init_height = clamp( x, Character::min_height(), Character::max_height() );
}

item_location Character::get_wielded_item() const
{
    return const_cast<Character *>( this )->get_wielded_item();
}

item_location Character::get_wielded_item()
{
    if( weapon.is_null() ) {
        return item_location();
    }
    return item_location( *this, &weapon );
}

void Character::set_wielded_item( const item &to_wield )
{
    weapon = to_wield;
}

std::vector<matype_id> Character::known_styles( bool teachable_only ) const
{
    return martial_arts_data->get_known_styles( teachable_only );
}

bool Character::has_martialart( const matype_id &m ) const
{
    return martial_arts_data->has_martialart( m );
}

int Character::get_oxygen_max() const
{
    return 30 + ( has_bionic( bio_synlungs ) ? 30 : 2 * str_cur );
}

bool Character::can_recover_oxygen() const
{
    return get_limb_score( limb_score_breathing ) > 0.5f && !is_underwater() &&
           !has_effect_with_flag( json_flag_GRAB ) && !( has_bionic( bio_synlungs ) &&
                   !has_active_bionic( bio_synlungs ) );
}

void Character::randomize_heartrate()
{
    // assume starting character is sedentary so we have some room to go down for cardio hr lowering
    avg_nat_bpm = rng_normal( 70, 90 );
}

void Character::randomize_blood()
{
    // Blood type distribution data is taken from this study on blood types of
    // COVID-19 patients presented to five major hospitals in Massachusetts:
    // https://www.ncbi.nlm.nih.gov/pmc/articles/PMC7354354/
    // and is adjusted for racial/ethnic distribution in game, which is defined in
    // data/json/npcs/appearance_trait_groups.json
    static const std::array<std::tuple<double, blood_type, bool>, 8> blood_type_distribution = {{
            std::make_tuple( 0.3821, blood_type::blood_O, true ),
            std::make_tuple( 0.0387, blood_type::blood_O, false ),
            std::make_tuple( 0.3380, blood_type::blood_A, true ),
            std::make_tuple( 0.0414, blood_type::blood_A, false ),
            std::make_tuple( 0.1361, blood_type::blood_B, true ),
            std::make_tuple( 0.0134, blood_type::blood_B, false ),
            std::make_tuple( 0.0437, blood_type::blood_AB, true ),
            std::make_tuple( 0.0066, blood_type::blood_AB, false )
        }
    };
    const double x = rng_float( 0.0, 1.0 );
    double cumulative_prob = 0.0;
    for( const std::tuple<double, blood_type, bool> &type : blood_type_distribution ) {
        cumulative_prob += std::get<0>( type );
        if( x <= cumulative_prob ) {
            my_blood_type = std::get<1>( type );
            blood_rh_factor = std::get<2>( type );
            return;
        }
    }
    my_blood_type = blood_type::blood_AB;
    blood_rh_factor = false;
}
void Character::randomize_cosmetics()
{
    randomize_cosmetic_trait( type_hair_color );
    randomize_cosmetic_trait( type_hair_style );
    randomize_cosmetic_trait( type_skin_tone );
    randomize_cosmetic_trait( type_eye_color );
    //arbitrary 50% chance to add beard to male characters
    if( male && one_in( 2 ) ) {
        randomize_cosmetic_trait( type_facial_hair );
    } else {
        set_mutation( trait_FACIAL_HAIR_NONE );
    }
}

void Character::starting_inv_damage_worn( int days )
{
    //damage equipment depending on days passed
    int chances_to_damage = std::max( days / 14 + rng( -3, 3 ), 0 );
    do {
        std::vector<item *> worn_items;
        worn.inv_dump( worn_items );
        if( !worn_items.empty() ) {
            item *to_damage = random_entry( worn_items );
            int damage_count = rng( 1, 3 );
            bool destroy = false;
            do {
                destroy = to_damage->inc_damage();
                if( destroy ) {
                    //if the clothing was destroyed in a simulated "dangerous situation", all contained items are lost
                    i_rem( to_damage );
                }
            } while( damage_count-- > 0 && !destroy );
        }
    } while( chances_to_damage-- > 0 );
}

field_type_id Character::bloodType() const
{
    if( has_flag( json_flag_ACIDBLOOD ) ) {
        return fd_acid;
    }
    if( has_flag( json_flag_PLANTBLOOD ) ) {
        return fd_blood_veggy;
    }
    if( has_flag( json_flag_INSECTBLOOD ) ) {
        return fd_blood_insect;
    }
    if( has_flag( json_flag_INVERTEBRATEBLOOD ) ) {
        return fd_blood_invertebrate;
    }
    return fd_blood;
}
field_type_id Character::gibType() const
{
    return fd_gibs_flesh;
}

bool Character::in_species( const species_id &spec ) const
{
    return spec == species_HUMAN;
}

bool Character::is_warm() const
{
    // TODO: is there a mutation (plant?) that makes a npc not warm blooded?
    return true;
}

const std::string &Character::symbol() const
{
    static const std::string character_symbol( "@" );
    return character_symbol;
}

void Character::mod_stat( const std::string &stat, float modifier )
{
    if( stat == "thirst" ) {
        mod_thirst( modifier );
    } else if( stat == "sleepiness" ) {
        mod_sleepiness( modifier );
    } else if( stat == "oxygen" ) {
        oxygen += modifier;
    } else if( stat == "stamina" ) {
        mod_stamina( modifier );
    } else if( stat == "str" ) {
        mod_str_bonus( modifier );
    } else if( stat == "dex" ) {
        mod_dex_bonus( modifier );
    } else if( stat == "per" ) {
        mod_per_bonus( modifier );
    } else if( stat == "int" ) {
        mod_int_bonus( modifier );
    } else if( stat == "healthy" ) {
        mod_livestyle( modifier );
    } else if( stat == "hunger" ) {
        mod_hunger( modifier );
    } else {
        Creature::mod_stat( stat, modifier );
    }
}

int Character::get_fat_to_hp() const
{
    int fat_to_hp = ( get_bmi_fat() - character_weight_category::normal );
    fat_to_hp = enchantment_cache->modify_value( enchant_vals::mod::FAT_TO_MAX_HP, fat_to_hp );
    return fat_to_hp;
}

creature_size Character::get_size() const
{
    return size_class;
}

std::string Character::disp_name( bool possessive, bool capitalize_first ) const
{
    return is_avatar() ? as_avatar()->display_name( possessive,
            capitalize_first ) : as_npc()->display_name( possessive );
}

std::string Character::disp_profession() const
{
    if( !custom_profession.empty() ) {
        return custom_profession;
    }
    if( is_npc() && as_npc()->myclass != npc_class_id::NULL_ID() ) {
        if( play_name_suffix ) {
            return play_name_suffix.value();
        }
    }
    if( prof != nullptr && prof != profession::generic() ) {
        return prof->gender_appropriate_name( male );
    }
    return "";
}

std::string Character::name_and_maybe_activity() const
{
    return disp_name( false, true );
}

std::string Character::skin_name() const
{
    // TODO: Return actual deflecting layer name
    return _( "armor" );
}

//message related stuff
void Character::add_msg_if_player( const std::string &msg ) const
{
    Messages::add_msg( msg );
}

void Character::add_msg_player_or_npc( const std::string &player_msg,
                                       const std::string &/*npc_msg*/ ) const
{
    Messages::add_msg( player_msg );
}

void Character::add_msg_if_player( const game_message_params &params, const std::string &msg ) const
{
    Messages::add_msg( params, msg );
}

void Character::add_msg_player_or_npc( const game_message_params &params,
                                       const std::string &player_msg,
                                       const std::string &/*npc_msg*/ ) const
{
    Messages::add_msg( params, player_msg );
}

void Character::add_msg_player_or_say( const std::string &player_msg,
                                       const std::string &/*npc_speech*/ ) const
{
    Messages::add_msg( player_msg );
}

void Character::add_msg_player_or_say( const game_message_params &params,
                                       const std::string &player_msg,
                                       const std::string &/*npc_speech*/ ) const
{
    Messages::add_msg( params, player_msg );
}

int Character::effective_dispersion( int dispersion, bool zoom ) const
{
    return get_character_parallax( zoom ) + dispersion;
}

int Character::get_character_parallax( bool zoom ) const
{
    /** @EFFECT_PER penalizes sight dispersion when low. */
    int character_parallax = zoom ? static_cast<int>( ranged_per_mod() * 0.25 ) : ranged_per_mod();
    character_parallax += get_modifier( character_modifier_ranged_dispersion_vision_mod );

    return std::max( static_cast<int>( std::round( character_parallax ) ), 0 );
}

static double modified_sight_speed( double aim_speed_modifier, double effective_sight_dispersion,
                                    double recoil )
{
    if( recoil <= effective_sight_dispersion ) {
        return 0;
    }
    // When recoil tends to effective_sight_dispersion, the aiming speed bonus will tend to 0
    // When recoil > 3 * effective_sight_dispersion + 1, attenuation_factor = 1
    // use 3 * effective_sight_dispersion + 1 instead of 3 * effective_sight_dispersion to avoid min=max
    if( effective_sight_dispersion < 0 ) {
        return 0;
    }
    double attenuation_factor = 1 - logarithmic_range( effective_sight_dispersion,
                                3 * effective_sight_dispersion + 1, recoil );

    return ( 10.0 + aim_speed_modifier ) * attenuation_factor;
}

int Character::point_shooting_limit( const item &gun )const
{
    // This value is not affected by PER, because the accuracy of aim shooting depends more on muscle memory and skill
    skill_id gun_skill = gun.gun_skill();
    if( gun_skill == skill_archery ) {
        return 30 + 220 / ( 1 + std::min( get_skill_level( gun_skill ), static_cast<float>( MAX_SKILL ) ) );
    } else {
        return 200 - 10 * std::min( get_skill_level( gun_skill ), static_cast<float>( MAX_SKILL ) );
    }
}

aim_mods_cache Character::gen_aim_mods_cache( const item &gun )const
{
    parallax_cache parallaxes{ get_character_parallax( true ), get_character_parallax( false ) };
    return { get_modifier( character_modifier_aim_speed_skill_mod, gun.gun_skill() ), get_modifier( character_modifier_aim_speed_dex_mod ), get_modifier( character_modifier_aim_speed_mod ), most_accurate_aiming_method_limit( gun ), aim_factor_from_volume( gun ), aim_factor_from_length( gun ), parallaxes };
}

double Character::fastest_aiming_method_speed( const item &gun, double recoil,
        const Target_attributes &target_attributes,
        const std::optional<std::reference_wrapper<const parallax_cache>> parallax_cache ) const
{
    // Get fastest aiming method that can be used to improve aim further below @ref recoil.

    skill_id gun_skill = gun.gun_skill();
    // aim with instinct (point shooting)

    // When it is a negative number, the effect of reducing the aiming speed is very significant.
    // When it is a positive number, a large value is needed to significantly increase the aiming speed
    double point_shooting_speed_modifier = 0;
    if( gun_skill == skill_pistol ) {
        point_shooting_speed_modifier = 10 + 4 * get_skill_level( gun_skill );
    } else {
        point_shooting_speed_modifier = get_skill_level( gun_skill );
    }

    double aim_speed_modifier = modified_sight_speed( point_shooting_speed_modifier,
                                point_shooting_limit( gun ), recoil );
    // aim with iron sight
    if( !gun.has_flag( flag_DISABLE_SIGHTS ) ) {
        const int iron_sight_FOV = 480;
        int effective_iron_sight_dispersion = effective_dispersion( gun.type->gun->sight_dispersion );
        double iron_sight_speed = modified_sight_speed( 0, effective_iron_sight_dispersion, recoil );
        if( effective_iron_sight_dispersion < recoil && iron_sight_speed > aim_speed_modifier &&
            recoil <= iron_sight_FOV ) {
            aim_speed_modifier = iron_sight_speed;
        }
    }

    // aim with other sights

    // to check whether laser sights are available
    const int base_distance = 10;
    const float light_limit = 120.0f;
    bool laser_light_available = target_attributes.range <= ( base_distance + per_cur ) * std::max(
                                     1.0f - target_attributes.light / light_limit, 0.0f ) && target_attributes.visible;
    // There are only two kinds of parallaxes, one with zoom and one without. So cache them.
    std::vector<std::optional<int>> parallaxes;
    parallaxes.resize( 2 );
    if( parallax_cache.has_value() ) {
        parallaxes[0].emplace( parallax_cache.value().get().parallax_without_zoom );
        parallaxes[1].emplace( parallax_cache.value().get().parallax_with_zoom );
    }
    for( const item *e : gun.gunmods() ) {
        const islot_gunmod &mod = *e->type->gunmod;
        if( mod.sight_dispersion < 0 || mod.field_of_view <= 0 ) {
            continue; // skip gunmods which don't provide a sight
        }
        if( e->has_flag( flag_LASER_SIGHT ) && !laser_light_available ) {
            continue;
        }
        bool zoom = e->has_flag( flag_ZOOM );
        // zoom==true will access parallaxes[1], zoom==false will access parallaxes[0].
        int parallax = parallaxes[static_cast<int>( zoom )].has_value() ? parallaxes[static_cast<int>
                       ( zoom )].value() : get_character_parallax( zoom );
        parallaxes[static_cast<int>( zoom )].emplace( parallax );
        // Maunal expansion of effective_dispersion() for performance.
        double e_effective_dispersion = parallax + mod.sight_dispersion;

        // The character can hardly get the aiming speed bonus from a non-magnifier sight when aiming at a target that is too far or too small
        double effective_aim_speed_modifier = 4 * parallax >
                                              target_attributes.size_in_moa ? std::min( 0.0, mod.aim_speed_modifier ) : mod.aim_speed_modifier;
        if( e_effective_dispersion < recoil && recoil <= mod.field_of_view ) {
            double e_speed = recoil < mod.field_of_view ? modified_sight_speed( effective_aim_speed_modifier,
                             e_effective_dispersion, recoil ) : 0;
            if( e_speed > aim_speed_modifier ) {
                aim_speed_modifier = e_speed;
            }
        }

    }
    return aim_speed_modifier;
}

int Character::most_accurate_aiming_method_limit( const item &gun ) const
{
    if( !gun.is_gun() ) {
        return 0;
    }

    int limit = point_shooting_limit( gun );

    if( !gun.has_flag( flag_DISABLE_SIGHTS ) ) {
        int iron_sight_limit = effective_dispersion( gun.type->gun->sight_dispersion );
        if( limit > iron_sight_limit ) {
            limit = iron_sight_limit;
        }
    }

    for( const item *e : gun.gunmods() ) {
        const islot_gunmod &mod = *e->type->gunmod;
        if( mod.field_of_view > 0 && mod.sight_dispersion >= 0 ) {
            limit = std::min( limit, effective_dispersion( mod.sight_dispersion, e->has_flag( flag_ZOOM ) ) );
        }
    }

    return limit;
}

double Character::aim_factor_from_volume( const item &gun ) const
{
    skill_id gun_skill = gun.gun_skill();
    double wielded_volume = gun.volume() / 1_ml;
    // this is only here for mod support
    if( gun.has_flag( flag_COLLAPSIBLE_STOCK ) ) {
        // use the unfolded volume
        wielded_volume += gun.collapsed_volume_delta() / 1_ml;
    }

    double factor = gun_skill == skill_pistol ? 4 : 1;
    double min_volume_without_debuff =  800;
    if( wielded_volume > min_volume_without_debuff ) {
        factor *= std::pow( min_volume_without_debuff / wielded_volume, 0.333333 );
    }

    return std::max( factor, 0.2 ) ;
}

static bool is_obstacle( tripoint_bub_ms pos )
{
    return get_map().coverage( pos ) >= 50;
}

double Character::aim_factor_from_length( const item &gun ) const
{
    tripoint_bub_ms cur_pos = pos_bub();
    bool nw_to_se = is_obstacle( cur_pos + tripoint::south_east ) &&
                    is_obstacle( cur_pos + tripoint::north_west );
    bool w_to_e = is_obstacle( cur_pos + tripoint::west ) &&
                  is_obstacle( cur_pos + tripoint::east );
    bool sw_to_ne = is_obstacle( cur_pos + tripoint::south_west ) &&
                    is_obstacle( cur_pos + tripoint::north_east );
    bool n_to_s = is_obstacle( cur_pos + tripoint::north ) &&
                  is_obstacle( cur_pos + tripoint::south );
    double wielded_length = gun.length() / 1_mm;
    double factor = 1.0;

    if( nw_to_se || w_to_e || sw_to_ne || n_to_s ) {
        factor = 1.0 - static_cast<float>( ( wielded_length - 300 ) / 1000 );
        factor =  std::min( factor, 1.0 );
    }
    return std::max( factor, 0.2 ) ;
}

double Character::aim_per_move( const item &gun, double recoil,
                                const Target_attributes &target_attributes,
                                std::optional<std::reference_wrapper<const aim_mods_cache>> aim_cache ) const
{
    if( !gun.is_gun() ) {
        return 0.0;
    }
    bool use_cache = aim_cache.has_value();
    double sight_speed_modifier = fastest_aiming_method_speed( gun, recoil, target_attributes,
                                  use_cache ? std::make_optional( std::ref( aim_cache.value().get().parallaxes ) ) : std::nullopt );
    int limit = use_cache ? aim_cache.value().get().limit :
                most_accurate_aiming_method_limit( gun );
    if( sight_speed_modifier == INT_MIN ) {
        // No suitable sights (already at maximum aim).
        return 0;
    }

    // Overall strategy for determining aim speed is to sum the factors that contribute to it,
    // then scale that speed by current recoil level.
    // Player capabilities make aiming faster, and aim speed slows down as it approaches 0.
    // Base speed is non-zero to prevent extreme rate changes as aim speed approaches 0.
    double aim_speed = 10.0;

    skill_id gun_skill = gun.gun_skill();

    aim_speed += sight_speed_modifier;

    if( !use_cache ) {
        // Ranges [-1.5 - 3.5] for archery Ranges [0 - 2.5] for others
        aim_speed += get_modifier( character_modifier_aim_speed_skill_mod, gun_skill );

        /** @EFFECT_DEX increases aiming speed */
        // every DEX increases 0.5 aim_speed
        aim_speed += get_modifier( character_modifier_aim_speed_dex_mod );

        aim_speed *= get_modifier( character_modifier_aim_speed_mod );
    } else {
        aim_speed += aim_cache.value().get().aim_speed_skill_mod;

        aim_speed += aim_cache.value().get().aim_speed_dex_mod;

        aim_speed *= aim_cache.value().get().aim_speed_mod;
    }

    // finally multiply everything by a harsh function that is eliminated by 7.5 gunskill
    aim_speed /= std::max( 1.0, 2.5 - 0.2 * get_skill_level( gun_skill ) );
    // Use a milder attenuation function to replace the previous logarithmic attenuation function when recoil is closed to 0.
    aim_speed *= std::max( recoil / MAX_RECOIL, 1 - logarithmic_range( 0, MAX_RECOIL, recoil ) );

    // add 4 max aim speed per skill up to 5 skill, then 1 per skill for skill 5-10
    double base_aim_speed_cap = 5.0 +  1.0 * get_skill_level( gun_skill ) + std::max( 10.0,
                                3.0 * get_skill_level( gun_skill ) );
    if( !use_cache ) {
        // This upper limit usually only affects the first half of the aiming process
        // Pistols have a much higher aiming speed limit
        aim_speed = std::min( aim_speed, base_aim_speed_cap * aim_factor_from_volume( gun ) );

        // When the character is in an open area, it will not be affected by the length of the weapon.
        // This upper limit usually only affects the first half of the aiming process
        // Weapons shorter than carbine are usually not affected by it
        aim_speed = std::min( aim_speed, base_aim_speed_cap * aim_factor_from_length( gun ) );
    } else {
        aim_speed = std::min( aim_speed,
                              base_aim_speed_cap * aim_cache.value().get().aim_factor_from_volume );

        aim_speed = std::min( aim_speed,
                              base_aim_speed_cap * aim_cache.value().get().aim_factor_from_length );
    }
    // Just a raw scaling factor.
    aim_speed *= 2.4;

    // Minimum improvement is 0.01MoA.  This is just to prevent data anomalies
    aim_speed = std::max( aim_speed, MIN_RECOIL_IMPROVEMENT );

    // Never improve by more than the currently used sights permit.
    return std::min( aim_speed, recoil - limit );
}

int Character::get_dodges_left() const
{
    return dodges_left;
}

void Character::set_dodges_left( int dodges )
{
    dodges_left = dodges;
}

void Character::mod_dodges_left( int mod )
{
    dodges_left += mod;
}

void Character::consume_dodge_attempts()
{
    if( get_dodges_left() > 0 ) {
        mod_dodges_left( -1 );
    }
}

ret_val<void> Character::can_try_dodge( bool ignore_dodges_left ) const
{
    //If we're asleep or busy we can't dodge
    if( in_sleep_state() || has_effect( effect_narcosis ) ||
        has_effect( effect_winded ) || has_effect( effect_fearparalyze ) || is_driving() ||
        has_flag( json_flag_CANNOT_MOVE ) ) {
        add_msg_debug( debugmode::DF_MELEE, "Unable to dodge (sleeping, winded, or driving)" );
        return ret_val<void>::make_failure();
    }
    //If stamina is too low we can't dodge
    if( get_stamina_dodge_modifier() <= 0.11 ) {
        add_msg_debug( debugmode::DF_MELEE, "Stamina too low to dodge.  Stamina: %d", get_stamina() );
        add_msg_debug( debugmode::DF_MELEE, "Stamina dodge modifier: %f", get_stamina_dodge_modifier() );
        return ret_val<void>::make_failure( !is_npc() ? _( "Your stamina is too low to attempt to dodge." )
                                            :
                                            _( "<npcname>'s stamina is too low to attempt to dodge." ) );
    }
    // Ensure no attempt to dodge without sources of extra dodges, eg martial arts
    if( get_dodges_left() <= 0 && !ignore_dodges_left ) {
        add_msg_debug( debugmode::DF_MELEE, "No remaining dodge attempts" );
        return ret_val<void>::make_failure();
    }
    return ret_val<void>::make_success();
}

float Character::get_stamina_dodge_modifier() const
{
    const double stamina = get_stamina();
    const double stamina_min = get_stamina_max() * 0.1;
    const double stamina_max = get_stamina_max() * 0.9;
    const double stamina_logistic = 1.0 - logarithmic_range( stamina_min, stamina_max, stamina );
    return stamina_logistic;
}

void Character::tally_organic_size()
{
    float ret = 0.0;
    for( const bodypart_id &part : get_all_body_parts() ) {
        if( !part->has_flag( json_flag_BIONIC_LIMB ) ) {
            ret += part->hit_size;
        }
    }
    cached_organic_size = ret / 100.0f;
    //return creature_anatomy->get_organic_size_sum() / 100.0f;
}

float Character::get_cached_organic_size() const
{
    return cached_organic_size;
}

int Character::sight_range( float light_level ) const
{
    if( light_level == 0 ) {
        return 1;
    }
    /* Via Beer-Lambert we have:
     * light_level * (1 / exp( LIGHT_TRANSPARENCY_OPEN_AIR * distance) ) <= LIGHT_AMBIENT_LOW
     * Solving for distance:
     * 1 / exp( LIGHT_TRANSPARENCY_OPEN_AIR * distance ) <= LIGHT_AMBIENT_LOW / light_level
     * 1 <= exp( LIGHT_TRANSPARENCY_OPEN_AIR * distance ) * LIGHT_AMBIENT_LOW / light_level
     * light_level <= exp( LIGHT_TRANSPARENCY_OPEN_AIR * distance ) * LIGHT_AMBIENT_LOW
     * log(light_level) <= LIGHT_TRANSPARENCY_OPEN_AIR * distance + log(LIGHT_AMBIENT_LOW)
     * log(light_level) - log(LIGHT_AMBIENT_LOW) <= LIGHT_TRANSPARENCY_OPEN_AIR * distance
     * log(LIGHT_AMBIENT_LOW / light_level) <= LIGHT_TRANSPARENCY_OPEN_AIR * distance
     * log(LIGHT_AMBIENT_LOW / light_level) * (1 / LIGHT_TRANSPARENCY_OPEN_AIR) <= distance
     */

    int range = static_cast<int>( -std::log( get_vision_threshold( get_map().ambient_light_at(
                                      pos_bub() ) ) / light_level ) / LIGHT_TRANSPARENCY_OPEN_AIR );

    // Clamp to [1, sight_max].
    return clamp( range, 1, sight_max );
}

int Character::unimpaired_range() const
{
    return std::min( sight_max, MAX_VIEW_DISTANCE );
}

bool Character::overmap_los( const tripoint_abs_omt &omt, int sight_points ) const
{
    const tripoint_abs_omt ompos = pos_abs_omt();
    const point_rel_omt offset = omt.xy() - ompos.xy();
    if( offset.x() < -sight_points || offset.x() > sight_points ||
        offset.y() < -sight_points || offset.y() > sight_points ) {
        // Outside maximum sight range
        return false;
    }

    const std::vector<tripoint_abs_omt> line = line_to( ompos, omt );
    for( size_t i = 0; i < line.size() && sight_points >= 0; i++ ) {
        const tripoint_abs_omt &pt = line[i];
        const oter_id &ter = overmap_buffer.ter( pt );
        sight_points -= static_cast<int>( ter->get_see_cost() );
        if( sight_points < 0 ) {
            return false;
        }
    }
    return true;
}

int Character::overmap_sight_range( float light_level ) const
{
    // How many map tiles I can see given the light??
    int sight = sight_range( light_level );
    // What are these doing???
    if( sight < SEEX ) {
        sight = 0;
    }
    if( sight <= SEEX * 4 ) {
        sight /= ( SEEX / 2 );
    }

    // Clamp it for some reason?
    if( sight > 0 ) {
        sight = 6;
    }

    // enchantment modifiers
    sight += enchantment_cache->get_value_add( enchant_vals::mod::OVERMAP_SIGHT );
    float multiplier = 1 + enchantment_cache->get_value_multiply( enchant_vals::mod::OVERMAP_SIGHT );

    // If sight got changed due OVERMAP_SIGHT, process the rest of the modifiers, otherwise skip them
    if( sight > 0 ) {
        // The higher your perception, the farther you can see.
        sight += static_cast<int>( get_per() / 2 );
    }

    if( sight == 0 ) {
        return 0;
    }

    return std::max<int>( std::round( sight * multiplier ), 3 );
}

int Character::overmap_modified_sight_range( float light_level ) const
{
    int sight = overmap_sight_range( light_level );

    // The higher up you are, the farther you can see.
    sight += std::max( 0, posz() ) * 2;

    // Binoculars double your sight range.
    // When adding checks here, also call game::update_overmap_seen at the place they first become true
    const bool has_optic = cache_has_item_with( flag_ZOOM ) ||
                           has_flag( json_flag_ENHANCED_VISION ) ||
                           ( is_mounted() && mounted_creature->has_flag( mon_flag_MECH_RECON_VISION ) ) ||
                           get_map().veh_at( pos_bub() ).avail_part_with_feature( "ENHANCED_VISION" ).has_value();

    const bool has_scoped_gun = cache_has_item_with( "is_gun", &item::is_gun, [&]( const item & gun ) {
        for( const item *mod : gun.gunmods() ) {
            if( mod->has_flag( flag_ZOOM ) ) {
                return true;
            }
        }
        return false;
    } );

    if( has_optic || has_scoped_gun ) {
        sight *= 2;
    }

    if( sight == 0 ) {
        return 0;
    }

    return std::max( sight, 3 );
}

int Character::clairvoyance() const
{
    if( vision_mode_cache[VISION_CLAIRVOYANCE_SUPER] ) {
        return MAX_CLAIRVOYANCE;
    }

    if( vision_mode_cache[VISION_CLAIRVOYANCE_PLUS] ) {
        return 8;
    }

    if( vision_mode_cache[VISION_CLAIRVOYANCE] ) {
        return 3;
    }

    return 0;
}

bool Character::sight_impaired() const
{
    const bool in_light = get_map().ambient_light_at( pos_bub() ) > LIGHT_AMBIENT_LIT;
    return ( ( has_effect( effect_boomered ) || has_effect( effect_no_sight ) ||
               has_effect( effect_darkness ) ) &&
             ( !has_trait( trait_PER_SLIME_OK ) ) ) ||
           ( underwater && !has_flag( json_flag_EYE_MEMBRANE ) &&
             !worn_with_flag( flag_SWIM_GOGGLES ) ) ||
           ( ( has_flag( json_flag_MYOPIC ) || ( in_light && has_flag( json_flag_MYOPIC_IN_LIGHT ) ) ) &&
             !worn_with_flag( flag_FIX_NEARSIGHT ) &&
             !has_effect( effect_contacts ) &&
             !has_effect( effect_transition_contacts ) &&
             !has_flag( json_flag_ENHANCED_VISION ) ) ||
           has_trait( trait_PER_SLIME ) || is_blind();
}

bool Character::has_alarm_clock() const
{
    map &here = get_map();
    if( cache_has_item_with_flag( flag_ALARMCLOCK, true ) ) {
        return true;
    }
    const optional_vpart_position &vp = here.veh_at( pos_bub() );
    return ( vp && !empty( vp->vehicle().get_avail_parts( "ALARMCLOCK" ) ) ) ||
           has_flag( json_flag_ALARMCLOCK );
}

bool Character::has_watch() const
{
    map &here = get_map();
    if( cache_has_item_with_flag( flag_WATCH, true ) ) {
        return true;
    }
    const optional_vpart_position &vp = here.veh_at( pos_bub() );
    return ( vp && !empty( vp->vehicle().get_avail_parts( "WATCH" ) ) ) ||
           has_flag( json_flag_WATCH );
}

void Character::react_to_felt_pain( int intensity )
{
    if( intensity <= 0 ) {
        return;
    }
    if( uistate.distraction_pain && is_avatar() && intensity >= 2 ) {
        g->cancel_activity_or_ignore_query( distraction_type::pain, _( "Ouch, something hurts!" ) );
    }
    // Only a large pain burst will actually wake people while sleeping.
    if( has_effect( effect_sleep ) && get_effect( effect_sleep ).get_duration() > 0_turns &&
        !has_effect( effect_narcosis ) ) {
        int pain_thresh = rng( 3, 5 );

        if( has_bionic( bio_sleep_shutdown ) ) {
            pain_thresh += 999;
        } else if( has_trait( trait_HEAVYSLEEPER ) ) {
            pain_thresh += 2;
        } else if( has_trait( trait_HEAVYSLEEPER2 ) ) {
            pain_thresh += 5;
        }

        if( intensity >= pain_thresh ) {
            wake_up();
        }
    }
}

void Character::action_taken()
{
    nv_cached = false;
}

int Character::swim_speed() const
{
    int ret;
    if( is_mounted() ) {
        monster *mon = mounted_creature.get();
        // no difference in swim speed by monster type yet.
        // TODO: difference in swim speed by monster type.
        // No monsters are currently mountable and can swim, though mods may allow this.
        if( mon->swims() ) {
            ret = 25;
            ret += get_weight() / 120_gram - 50 * ( mon->get_size() - 1 );
            return ret;
        }
    }
    const body_part_set usable = exclusive_flag_coverage( flag_ALLOWS_NATURAL_ATTACKS );
    float hand_bonus_mult = ( usable.test( body_part_hand_l ) ? 0.5f : 0.0f ) +
                            ( usable.test( body_part_hand_r ) ? 0.5f : 0.0f );

    // base swim speed.
    float swim_speed_mult = enchantment_cache->modify_value( enchant_vals::mod::MOVECOST_SWIM_MOD, 1 );
    ret = ( 440 * swim_speed_mult ) + weight_carried() /
          ( 60_gram / swim_speed_mult ) - 50 * get_skill_level( skill_swimming );
    /** @EFFECT_STR increases swim speed bonus from PAWS */
    if( has_trait( trait_PAWS ) ) {
        ret -= hand_bonus_mult * ( 20 + str_cur * 3 );
    }
    /** @EFFECT_STR increases swim speed bonus from PAWS_LARGE */
    if( has_trait( trait_PAWS_LARGE ) ) {
        ret -= hand_bonus_mult * ( 20 + str_cur * 4 );
    }
    /** @EFFECT_STR increases swim speed bonus from swim_fins */
    if( worn_with_flag( flag_FIN, body_part_foot_l ) ||
        worn_with_flag( flag_FIN, body_part_foot_r ) ) {
        if( worn_with_flag( flag_FIN, body_part_foot_l ) &&
            worn_with_flag( flag_FIN, body_part_foot_r ) ) {
            ret -= ( 15 * str_cur );
        } else {
            ret -= ( 15 * str_cur ) / 2;
        }
    }
    /** @EFFECT_STR increases swim speed bonus from WEBBED and WEBBED_FEET */
    float webbing_factor = 60 + str_cur * 5;
    if( has_flag( json_flag_WEBBED_HANDS ) ) {
        ret -= hand_bonus_mult * webbing_factor * 0.5f;
    }
    if( has_flag( json_flag_WEBBED_FEET ) && is_barefoot() ) {
        ret -= webbing_factor * 0.5f;
    }
    /** @EFFECT_SWIMMING increases swim speed */
    ret *= get_modifier( character_modifier_swim_mod );
    ret += worn.swim_modifier( round( get_skill_level( skill_swimming ) ) );
    /** @EFFECT_STR increases swim speed */

    /** @EFFECT_DEX increases swim speed */
    ret -= str_cur * 6 + dex_cur * 4;
    if( worn_with_flag( flag_FLOTATION ) ) {
        ret = std::min( ret, 400 );
        ret = std::max( ret, 200 );
    }
    // If (ret > 500), we can not swim; so do not apply the underwater bonus.
    if( underwater && ret < 500 ) {
        ret -= 50;
    }

    ret += move_mode->swim_speed_mod();

    if( ret < 30 ) {
        ret = 30;
    }
    return ret;
}

bool Character::is_on_ground() const
{
    return ( !enough_working_legs() && !weapon.has_flag( flag_CRUTCHES ) ) ||
           has_effect( effect_downed ) || is_prone();
}

bool Character::can_stash( const item &it, bool ignore_pkt_settings )
{
    return best_pocket( it, nullptr, ignore_pkt_settings ).second != nullptr;
}

bool Character::can_stash( const item &it, int &copies_remaining, bool ignore_pkt_settings )
{
    bool stashed_any = false;

    for( item_location loc : top_items_loc() ) {
        if( loc->can_contain( it, copies_remaining, false, false, ignore_pkt_settings ).success() ) {
            stashed_any = true;
        }
        if( copies_remaining <= 0 ) {
            return stashed_any;
        }
    }
    return stashed_any;
}

bool Character::can_stash_partial( const item &it, int &copies_remaining, bool ignore_pkt_settings )
{
    item copy = it;
    if( it.count_by_charges() ) {
        copy.charges = 1;
    }

    return can_stash( copy, copies_remaining, ignore_pkt_settings );
}

bool Character::can_stash_partial( const item &it, bool ignore_pkt_settings )
{
    item copy = it;
    if( it.count_by_charges() ) {
        copy.charges = 1;
    }

    return can_stash( copy, ignore_pkt_settings );
}

void Character::cancel_stashed_activity()
{
    stashed_outbounds_activity = player_activity();
    stashed_outbounds_backlog = player_activity();
}

player_activity Character::get_stashed_activity() const
{
    return stashed_outbounds_activity;
}

void Character::set_stashed_activity( const player_activity &act, const player_activity &act_back )
{
    stashed_outbounds_activity = act;
    stashed_outbounds_backlog = act_back;
}

bool Character::has_stashed_activity() const
{
    return static_cast<bool>( stashed_outbounds_activity );
}

void Character::assign_stashed_activity()
{
    activity = stashed_outbounds_activity;
    backlog.push_front( stashed_outbounds_backlog );
    cancel_stashed_activity();
}

bool Character::check_outbounds_activity( const player_activity &act, bool check_only )
{
    map &here = get_map();
    if( ( act.placement != tripoint_abs_ms() && act.placement != player_activity::invalid_place &&
          !here.inbounds( here.get_bub( act.placement ) ) ) || ( !act.coords.empty() &&
                  !here.inbounds( here.get_bub( act.coords.back() ) ) ) ) {
        if( is_npc() && !check_only ) {
            // stash activity for when reloaded.
            stashed_outbounds_activity = act;
            if( !backlog.empty() ) {
                stashed_outbounds_backlog = backlog.front();
            }
            activity = player_activity();
        }
        add_msg_debug( debugmode::DF_CHARACTER,
                       "npc %s at pos %s, activity target is not inbounds at %s therefore "
                       "activity was stashed",
                       disp_name(), pos_bub().to_string_writable(),
                       act.placement.to_string_writable() );
        return true;
    }
    return false;
}

void Character::set_destination_activity( const player_activity &new_destination_activity )
{
    destination_activity = new_destination_activity;
}

void Character::clear_destination_activity()
{
    destination_activity = player_activity();
}

player_activity Character::get_destination_activity() const
{
    return destination_activity;
}

void Character::mount_creature( monster &z )
{
    map &here = get_map();

    tripoint_bub_ms pnt = z.pos_bub();
    shared_ptr_fast<monster> mons = g->shared_from( z );
    if( mons == nullptr ) {
        add_msg_debug( debugmode::DF_CHARACTER, "mount_creature(): monster not found in critter_tracker" );
        return;
    }

    add_effect( effect_riding, 1_turns, true );
    z.add_effect( effect_ridden, 1_turns, true );
    if( z.has_effect( effect_tied ) ) {
        z.remove_effect( effect_tied );
        if( z.tied_item ) {
            i_add( *z.tied_item );
            z.tied_item.reset();
        }
    }
    z.mounted_player_id = getID();
    if( z.has_effect( effect_harnessed ) ) {
        z.remove_effect( effect_harnessed );
        add_msg_if_player( m_info, _( "You remove the %s's harness." ), z.get_name() );
    }
    mounted_creature = mons;
    mons->mounted_player = this;
    avatar &player_avatar = get_avatar();
    if( is_avatar() && player_avatar.get_grab_type() != object_type::NONE ) {
        add_msg( m_warning, _( "You let go of the grabbed object." ) );
        player_avatar.grab( object_type::NONE );
    }
    add_msg_if_player( m_good, _( "You climb on the %s." ), z.get_name() );
    if( z.has_flag( mon_flag_RIDEABLE_MECH ) ) {
        if( !z.type->mech_weapon.is_empty() ) {
            item mechwep = item( z.type->mech_weapon );
            set_wielded_item( mechwep );
        }
        add_msg_if_player( m_good, _( "You hear your %s whir to life." ), z.get_name() );
    }
    if( is_avatar() ) {
        if( player_avatar.is_hauling() ) {
            player_avatar.stop_hauling();
        }
        g->place_player( pnt );
    } else {
        npc &guy = dynamic_cast<npc &>( *this );
        guy.setpos( here, pnt );
    }
    z.facing = facing;
    // some rideable mechs have night-vision
    recalc_sight_limits();
    if( is_avatar() && z.has_flag( mon_flag_MECH_RECON_VISION ) ) {
        add_effect( effect_mech_recon_vision, 1_turns, true );
        // mech night-vision counts as optics for overmap sight range.
        g->update_overmap_seen();
    }
    mod_moves( -100 );
}

bool Character::check_mount_will_move( const tripoint_bub_ms &dest_loc )
{
    const map &here = get_map();

    if( !is_mounted() || mounted_creature->has_flag( mon_flag_COMBAT_MOUNT ) ) {
        return true;
    }
    if( mounted_creature && mounted_creature->type->has_fear_trigger( mon_trigger::HOSTILE_CLOSE ) ) {
        for( const monster &critter : g->all_monsters() ) {
            if( critter.is_hallucination() ) {
                continue;
            }
            Attitude att = critter.attitude_to( *this );
            if( att == Attitude::HOSTILE && sees( here, critter ) &&
                rl_dist( pos_abs(), critter.pos_abs() ) <= 15 &&
                rl_dist( dest_loc, critter.pos_bub( here ) ) < rl_dist( pos_abs(), critter.pos_abs() ) ) {
                add_msg_if_player( _( "You fail to budge your %s!" ), mounted_creature->get_name() );
                return false;
            }
        }
    }
    return true;
}

bool Character::check_mount_is_spooked()
{
    const map &here = get_map();

    if( !is_mounted() ) {
        return false;
    }
    // chance to spook per monster nearby:
    // base 1% per turn.
    // + 1% per square closer than 15 distance. (1% - 15%)
    // * 2 if hostile monster is bigger than or same size as mounted creature.
    // -0.25% per point of dexterity (low -1%, average -2%, high -3%, extreme -3.5%)
    // -0.1% per point of strength ( low -0.4%, average -0.8%, high -1.2%, extreme -1.4% )
    // / 2 if horse has full tack and saddle.
    // / 2 if mount has the monster flag COMBAT_MOUNT
    // Monster in spear reach monster and average stat (8) player on saddled horse, 14% -2% -0.8% / 2 = ~5%
    if( mounted_creature && mounted_creature->type->has_fear_trigger( mon_trigger::HOSTILE_CLOSE ) ) {
        const creature_size mount_size = mounted_creature->get_size();
        const bool saddled = mounted_creature->has_effect( effect_monster_saddled );
        const bool combat_mount = mounted_creature->has_flag( mon_flag_COMBAT_MOUNT );
        for( const monster &critter : g->all_monsters() ) {
            if( critter.is_hallucination() ) {
                continue;
            }
            double chance = 1.0;
            Attitude att = critter.attitude_to( *this );
            // actually too close now - horse might spook.
            if( att == Attitude::HOSTILE && sees( here, critter ) &&
                rl_dist( pos_abs(), critter.pos_abs() ) <= 10 ) {
                chance += 10 - rl_dist( pos_abs(), critter.pos_abs() );
                if( critter.get_size() >= mount_size ) {
                    chance *= 2;
                }
                chance -= 0.25 * get_dex();
                chance -= 0.1 * get_str();
                chance *= get_limb_score( limb_score_grip );
                if( saddled ) {
                    chance /= 2;
                }
                if( combat_mount ) {
                    chance /= 2;
                }
                chance = std::max( 1.0, chance );
                if( x_in_y( chance, 100.0 ) ) {
                    forced_dismount();
                    return true;
                }
            }
        }
    }
    return false;
}

bool Character::cant_do_mounted( bool msg ) const
{
    if( is_mounted() ) {
        if( msg ) {
            add_msg_player_or_npc( m_info, _( "You can't do that while mounted." ),
                                   _( "<npcname> can't do that while mounted." ) );
        }
        return true;
    }
    return false;
}

bool Character::is_mounted() const
{
    return has_effect( effect_riding ) && mounted_creature;
}

void Character::forced_dismount()
{
    map &here = get_map();

    remove_effect( effect_riding );
    remove_effect( effect_mech_recon_vision );
    bool mech = false;
    if( mounted_creature ) {
        auto *mon = mounted_creature.get();
        if( mon->has_flag( mon_flag_RIDEABLE_MECH ) && !mon->type->mech_weapon.is_empty() ) {
            mech = true;
            remove_item( weapon );
        }
        mon->mounted_player_id = character_id();
        mon->remove_effect( effect_ridden );
        mon->add_effect( effect_controlled, 5_turns );
        mounted_creature = nullptr;
        mon->mounted_player = nullptr;
    }
    std::vector<tripoint_bub_ms> valid;
    valid.reserve( 8 );
    for( const tripoint_bub_ms &jk : here.points_in_radius( pos_bub( here ), 1 ) ) {
        if( g->is_empty( jk ) ) {
            valid.push_back( jk );
        }
    }
    if( !valid.empty() ) {
        setpos( here, random_entry( valid ) );
        if( mech ) {
            add_msg_player_or_npc( m_bad, _( "You are ejected from your mech!" ),
                                   _( "<npcname> is ejected from their mech!" ) );
        } else {
            add_msg_player_or_npc( m_bad, _( "You fall off your mount!" ),
                                   _( "<npcname> falls off their mount!" ) );
        }
        const int dodge = get_dodge();
        const int damage = std::max( 0, rng( 1, 20 ) - rng( dodge, dodge * 2 ) );
        bodypart_id hit = bodypart_str_id::NULL_ID();
        switch( rng( 1, 10 ) ) {
            case  1:
                if( one_in( 2 ) ) {
                    hit = body_part_foot_l;
                } else {
                    hit = body_part_foot_r;
                }
                break;
            case  2:
            case  3:
            case  4:
                if( one_in( 2 ) ) {
                    hit = body_part_leg_l;
                } else {
                    hit = body_part_leg_r;
                }
                break;
            case  5:
            case  6:
            case  7:
                if( one_in( 2 ) ) {
                    hit = body_part_arm_l;
                } else {
                    hit = body_part_arm_r;
                }
                break;
            case  8:
            case  9:
                hit = body_part_torso;
                break;
            case 10:
                hit = body_part_head;
                break;
        }
        if( damage > 0 ) {
            add_msg_if_player( m_bad, _( "You hurt yourself!" ) );
            // FIXME: Hardcoded damage type
            deal_damage( nullptr, hit, damage_instance( damage_bash, damage ) );
            if( is_avatar() ) {
                get_memorial().add(
                    pgettext( "memorial_male", "Fell off a mount." ),
                    pgettext( "memorial_female", "Fell off a mount." ) );
            }
            check_dead_state( &here );
        }
        add_effect( effect_downed, 5_turns, true );
    } else {
        add_msg_debug( debugmode::DF_CHARACTER,
                       "Forced_dismount could not find a square to deposit player" );
    }
    if( is_avatar() ) {
        avatar &player_character = get_avatar();
        if( player_character.get_grab_type() != object_type::NONE ) {
            add_msg( m_warning, _( "You let go of the grabbed object." ) );
            player_character.grab( object_type::NONE );
        }
        set_movement_mode( move_mode_walk );
        if( player_character.is_auto_moving() || player_character.has_destination() ||
            player_character.has_destination_activity() ) {
            player_character.abort_automove();
        }
        g->update_map( player_character );
    }
    if( activity ) {
        cancel_activity();
    }
    mod_moves( -get_speed() * 1.5 );
}

void Character::dismount()
{
    map &here = get_map();

    if( !is_mounted() ) {
        add_msg_debug( debugmode::DF_CHARACTER, "dismount called when not riding" );
        return;
    }
    if( const std::optional<tripoint_bub_ms> pnt = choose_adjacent( _( "Dismount where?" ) ) ) {
        if( !g->is_empty( *pnt ) ) {
            add_msg( m_warning, _( "You cannot dismount there!" ) );
            return;
        }

        remove_effect( effect_riding );
        remove_effect( effect_mech_recon_vision );
        monster *critter = mounted_creature.get();
        critter->mounted_player_id = character_id();
        if( critter->has_flag( mon_flag_RIDEABLE_MECH ) && !critter->type->mech_weapon.is_empty() &&
            weapon.typeId() == critter->type->mech_weapon ) {
            remove_item( weapon );
        }
        avatar &player_character = get_avatar();
        if( is_avatar() && player_character.get_grab_type() != object_type::NONE ) {
            add_msg( m_warning, _( "You let go of the grabbed object." ) );
            player_character.grab( object_type::NONE );
        }
        critter->remove_effect( effect_ridden );
        critter->add_effect( effect_controlled, 5_turns );
        mounted_creature = nullptr;
        critter->mounted_player = nullptr;
        setpos( here, *pnt );
        mod_moves( -100 );
        set_movement_mode( move_mode_walk );
    }
}

float Character::stability_roll() const
{
    /** @EFFECT_STR improves player stability roll */

    /** Balance and reaction scores influence stability rolls */

    /** @EFFECT_MELEE improves player stability roll */
    return ( get_melee() + get_str() ) * ( ( get_limb_score( limb_score_balance ) * 3 + get_limb_score(
            limb_score_reaction ) ) / 4.0f );
}

bool Character::is_dead_state() const
{
    if( cached_dead_state.has_value() ) {
        return cached_dead_state.value();
    }

    cached_dead_state = false;
    // we want to warn the player with a debug message if they are invincible. this should be unimportant once wounds exist and bleeding is how you die.
    bool has_vitals = false;
    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        if( bp->is_vital ) {
            has_vitals = true;
            if( get_part_hp_cur( bp ) <= 0 ) {
                cached_dead_state = true;
                return cached_dead_state.value();
            }
        }
    }
    if( !has_vitals ) {
        debugmsg( _( "WARNING!  %s has no vital part and is invincible." ), disp_name() );
    }
    return false;
}

void Character::set_part_hp_cur( const bodypart_id &id, int set )
{
    bool is_broken_before = get_part_hp_cur( id ) <= 0;
    Creature::set_part_hp_cur( id, set );
    bool is_broken_after = get_part_hp_cur( id ) <= 0;
    // detect unbroken->broken (possible death) as well as broken->unbroken (possible revival)
    if( is_broken_before != is_broken_after ) {
        cached_dead_state.reset();
    }
}

void Character::mod_part_hp_cur( const bodypart_id &id, int set )
{
    bool is_broken_before = get_part_hp_cur( id ) <= 0;
    Creature::mod_part_hp_cur( id, set );
    bool is_broken_after = get_part_hp_cur( id ) <= 0;
    // detect unbroken->broken (possible death) as well as broken->unbroken (possible revival)
    if( is_broken_before != is_broken_after ) {
        cached_dead_state.reset();
    }
}

void Character::on_try_dodge()
{
    ret_val<void> can_dodge = can_try_dodge();
    if( !can_dodge.success() ) {
        add_msg( m_bad, can_dodge.c_str() );
        return;
    }

    // Each attempt consumes an available dodge
    consume_dodge_attempts();

    const int base_burn_rate = get_option<int>( STATIC( "PLAYER_BASE_STAMINA_BURN_RATE" ) );
    const float dodge_skill_modifier = ( 20.0f - get_skill_level( skill_dodge ) ) / 20.0f;
    burn_energy_legs( - std::floor( static_cast<float>( base_burn_rate ) * 6.0f *
                                    dodge_skill_modifier ) );
    set_activity_level( EXTRA_EXERCISE );
}

void Character::on_dodge( Creature *source, float difficulty, float training_level )
{
    // Make sure we're not practicing dodge in situation where we can't dodge
    // We can ignore dodges_left because it was already checked in get_dodge()
    if( !can_try_dodge( true ).success() ) {
        return;
    }

    // dodging throws of our aim unless we are either skilled at dodging or using a small weapon
    if( is_armed() && weapon.is_gun() ) {
        recoil += std::max( weapon.volume() / 250_ml - get_skill_level( skill_dodge ), 0.0f ) * rng( 0,
                  100 );
        recoil = std::min( MAX_RECOIL, recoil );
    }

    // Even if we are not to train still call practice to prevent skill rust
    difficulty = std::max( difficulty, 0.0f );

    // If training_level is set, treat that as the difficulty instead
    if( training_level != 0.0f ) {
        difficulty = training_level;
    }

    practice( skill_dodge, difficulty * 2, difficulty );
    martial_arts_data->ma_ondodge_effects( *this );

    // For adjacent attackers check for techniques usable upon successful dodge
    if( source && square_dist( pos_bub(), source->pos_bub() ) == 1 ) {
        matec_id tec = std::get<0>( pick_technique( *source, used_weapon(), false, true, false ) );

        if( tec != tec_none && !is_dead_state() ) {
            if( get_stamina() < get_stamina_max() / 3 ) {
                add_msg( m_bad, _( "You try to counterattack, but you are too exhausted!" ) );
            } else {
                melee_attack( *source, false, tec );
            }
        }
    }
}

float Character::get_melee() const
{
    return get_skill_level( skill_melee );
}

bool Character::uncanny_dodge()
{
    map &here = get_map();

    bool is_u = is_avatar();
    bool seen = get_player_view().sees( here, *this );

    const units::energy trigger_cost = bio_uncanny_dodge->power_trigger;

    const bool can_dodge_bio = get_power_level() >= trigger_cost &&
                               has_active_bionic( bio_uncanny_dodge );
    const bool can_dodge_stam = get_stamina() >= 300 && ( has_trait_flag( json_flag_UNCANNY_DODGE ) ||
                                has_effect_with_flag( json_flag_UNCANNY_DODGE ) );
    const bool can_dodge_both = get_power_level() >= ( trigger_cost / 2 ) &&
                                has_active_bionic( bio_uncanny_dodge ) &&
                                get_stamina() >= 150 && has_trait_flag( json_flag_UNCANNY_DODGE );

    if( !( can_dodge_bio || can_dodge_stam || can_dodge_both ) ) {
        return false;
    }
    tripoint_bub_ms adjacent{ adjacent_tile() };

    const optional_vpart_position veh_part = here.veh_at( pos_bub( here ) );
    if( in_vehicle && veh_part && veh_part.avail_part_with_feature( "SEATBELT" ) ) {
        return false;
    }

    //uncanny dodge triggered in car and wasn't secured by seatbelt
    if( in_vehicle && veh_part ) {
        here.unboard_vehicle( pos_bub( here ) );
    }
    if( adjacent.xy() != pos_bub().xy() ) {
        set_pos_bub_only( here, adjacent );

        //landed in a vehicle tile
        if( here.veh_at( pos_bub( here ) ) ) {
            here.board_vehicle( pos_bub( here ), this );
        }

        if( is_u ) {
            add_msg( _( "Time seems to slow down, and you instinctively dodge!" ) );
        } else if( seen ) {
            add_msg( _( "%s dodges… so fast!" ), this->disp_name() );

        }

        if( can_dodge_both ) {
            mod_power_level( -trigger_cost / 2 );
            burn_energy_legs( -150 );
        } else if( can_dodge_bio ) {
            mod_power_level( -trigger_cost );
        } else if( can_dodge_stam ) {
            burn_energy_legs( -300 );
        }
        return true;
    }
    if( is_u ) {
        add_msg( _( "You try to dodge, but there's no room!" ) );
    } else if( seen ) {
        add_msg( _( "%s tries to dodge, but there's no room!" ), this->disp_name() );
    }
    return false;
}

bool Character::check_avoid_friendly_fire() const
{
    double chance = enchantment_cache->modify_value( enchant_vals::mod::AVOID_FRIENDRY_FIRE, 0.0 );
    return rng( 0, 99 ) < chance * 100.0;
}

void Character::handle_skill_warning( const skill_id &id, bool force_warning )
{
    //remind the player intermittently that no skill gain takes place
    if( is_avatar() && ( force_warning || one_in( 5 ) ) ) {
        SkillLevel &level = get_skill_level_object( id );

        const Skill &skill = id.obj();
        std::string skill_name = skill.name();
        int curLevel = level.level();

        add_msg( m_info, _( "This task is too simple to train your %s beyond %d." ),
                 skill_name, curLevel );
    }
}

bool Character::has_min_manipulators() const
{
    return get_limb_score( limb_score_manip ) > MIN_MANIPULATOR_SCORE;
}

/** Returns true if the character has two functioning arms */
bool Character::has_two_arms_lifting() const
{
    // 0.5f is one "standard" arm, so if you have more than that you barely qualify.
    return get_limb_score( limb_score_lift, body_part_type::type::arm ) > 0.5f;
}

std::set<matec_id> Character::get_limb_techs() const
{
    std::set<matec_id> result;
    for( const bodypart_id &part : get_all_body_parts() ) {
        const bodypart *bp = get_part( part );
        if( !bp->is_limb_overencumbered( *this ) && bp->get_hp_cur() > part->health_limit ) {
            std::set<matec_id> part_tech = get_part( part )->get_limb_techs( *this );
            result.insert( part_tech.begin(), part_tech.end() );
        }
    }
    return result;
}

int Character::get_working_arm_count() const
{
    int limb_count = 0;
    body_part_type::type arm_type = body_part_type::type::arm;
    for( const bodypart_id &part : get_all_body_parts_of_type( arm_type ) ) {
        // Almost broken or overencumbered arms don't count
        if( get_part( part )->get_limb_score( *this,
                                              limb_score_lift ) * part->limbtypes.at( arm_type ) >= 0.1 &&
            !get_part( part )->is_limb_overencumbered( *this ) ) {
            limb_count++;
        }
    }
    return limb_count;
}

// working is defined here as not broken
bool Character::enough_working_legs() const
{
    int limb_count = 0;
    int working_limb_count = 0;
    for( const bodypart_id &part : get_all_body_parts() ) {
        if( part->primary_limb_type() == body_part_type::type::leg ) {
            limb_count++;
            if( !is_limb_broken( part ) ) {
                working_limb_count++;
            }
        }
    }

    return working_limb_count == limb_count;
}

// working is defined here as not broken
int Character::get_working_leg_count() const
{
    int working_limb_count = 0;
    for( const bodypart_id &part : get_all_body_parts() ) {
        if( part->primary_limb_type() == body_part_type::type::leg ) {
            if( !is_limb_broken( part ) ) {
                working_limb_count++;
            }
        }
    }

    return working_limb_count;
}

// this is the source of truth on if a limb is broken so all code to determine
// if a limb is broken should point here to make any future changes to breaking easier
bool Character::is_limb_broken( const bodypart_id &limb ) const
{
    return get_part_hp_cur( limb ) == 0;
}

bool Character::can_run() const
{
    return get_stamina() > 0 && !has_effect( effect_winded ) && enough_working_legs();
}

move_mode_id Character::current_movement_mode() const
{
    return move_mode;
}

bool Character::movement_mode_is( const move_mode_id &mode ) const
{
    return move_mode == mode;
}

bool Character::is_running() const
{
    return move_mode->type() == move_mode_type::RUNNING;
}

bool Character::is_walking() const
{
    return move_mode->type() == move_mode_type::WALKING;
}

bool Character::is_crouching() const
{
    return move_mode->type() == move_mode_type::CROUCHING;
}

bool Character::is_prone() const
{
    return move_mode->type() == move_mode_type::PRONE;
}

int Character::footstep_sound() const
{
    if( has_trait( trait_DEBUG_SILENT ) ) {
        return 0;
    }

    double volume = is_stealthy() ? 3 : 6;
    if( volume > 0 ) {
        volume *= current_movement_mode()->sound_mult();
        if( is_mounted() ) {
            monster *mons = mounted_creature.get();
            switch( mons->get_size() ) {
                case creature_size::tiny:
                    volume = 0; // No sound for the tinies
                    break;
                case creature_size::small:
                    volume /= 3;
                    break;
                case creature_size::medium:
                    break;
                case creature_size::large:
                    volume *= 1.5;
                    break;
                case creature_size::huge:
                    volume *= 2;
                    break;
                default:
                    break;
            }
            if( mons->has_flag( mon_flag_LOUDMOVES ) ) {
                volume += 6;
            } else if( mons->has_flag( mon_flag_QUIETMOVES ) ) {
                volume /= 2;
            }
        } else {
            volume = calculate_by_enchantment( volume, enchant_vals::mod::FOOTSTEP_NOISE );
        }
    }
    return std::round( volume );
}

int Character::clatter_sound() const
{
    return worn.clatter_sound();
}

void Character::make_footstep_noise() const
{
    if( is_hallucination() ) {
        return;
    }

    const int volume = footstep_sound();
    if( volume <= 0 ) {
        return;
    }
    if( is_mounted() ) {
        sounds::sound( pos_bub(), volume, sounds::sound_t::movement,
                       mounted_creature.get()->type->get_footsteps(),
                       false, "none", "none" );
    } else {
        sounds::sound( pos_bub(), volume, sounds::sound_t::movement, _( "footsteps" ), true,
                       "none", "none" );    // Sound of footsteps may awaken nearby monsters
    }
    sfx::do_footstep();
}

void Character::make_clatter_sound() const
{

    const int volume = clatter_sound();
    if( volume <= 0 ) {
        return;
    }
    sounds::sound( pos_bub(), volume, sounds::sound_t::movement, _( "clattering equipment" ), true,
                   "none", "none" );   // Sound of footsteps may awaken nearby monsters
}

steed_type Character::get_steed_type() const
{
    steed_type steed;
    if( is_mounted() ) {
        if( mounted_creature->has_flag( mon_flag_RIDEABLE_MECH ) ) {
            steed = steed_type::MECH;
        } else {
            steed = steed_type::ANIMAL;
        }
    } else {
        steed = steed_type::NONE;
    }
    return steed;
}

bool Character::can_switch_to( const move_mode_id &mode ) const
{
    // Only running modes are restricted at the moment and only when its your legs doing the running
    return get_steed_type() != steed_type::NONE || mode->type() != move_mode_type::RUNNING || can_run();
}

void Character::expose_to_disease( const diseasetype_id &dis_type )
{
    const std::optional<int> &healt_thresh = dis_type->health_threshold;
    if( healt_thresh && healt_thresh.value() < get_lifestyle() ) {
        return;
    }
    const std::set<bodypart_str_id> &bps = dis_type->affected_bodyparts;
    if( !bps.empty() ) {
        for( const bodypart_str_id &bp : bps ) {
            add_effect( dis_type->symptoms, rng( dis_type->min_duration, dis_type->max_duration ), bp.id(),
                        false,
                        rng( dis_type->min_intensity, dis_type->max_intensity ) );
        }
    } else {
        add_effect( dis_type->symptoms, rng( dis_type->min_duration, dis_type->max_duration ),
                    bodypart_str_id::NULL_ID(),
                    false,
                    rng( dis_type->min_intensity, dis_type->max_intensity ) );
    }
}

void Character::process_turn()
{
    map &here = get_map();
    // Has to happen before reset_stats
    clear_miss_reasons();
    migrate_items_to_storage( false );

    for( bionic &i : *my_bionics ) {
        if( i.incapacitated_time > 0_turns ) {
            i.incapacitated_time -= 1_turns;
            if( i.incapacitated_time == 0_turns ) {
                add_msg_if_player( m_bad, _( "Your %s bionic comes back online." ), i.info().name );
            }
        }
    }

    for( const trait_id &mut : get_functioning_mutations() ) {
        mutation_reflex_trigger( mut );
    }
    //Creature::process_turn() uses speed to restore moves, but for Character, speed has not yet been calculated.
    //So Creature::process_turn() cannot be used directly here.
    //Exposed the content of Creature::process_turn() except restore moves here,
    //and do the reset moves later after recalc_speed_bonus().
    //The following is the content of Creature::process_turn() except restore moves
    decrement_summon_timer();
    if( is_dead_state() ) {
        return;
    }
    reset_bonuses();

    process_effects();

    process_damage_over_time();

    // Call this in case any effects have changed our stats
    reset_stats();
    //The above is the content of Creature::process_turn() except reset moves

    // If we're actively handling something we can't just drop it on the ground
    // in the middle of handling it
    if( activity.targets.empty() && activity.do_drop_invalid_inventory() ) {
        drop_invalid_inventory();
    }
    if( leak_level_dirty ) {
        calculate_leak_level();
    }
    process_items( &here );
    leak_items();
    // Didn't just pick something up
    last_item = itype_id::NULL_ID();

    cache_visit_items_with( "is_relic", &item::is_relic, [this]( item & it ) {
        it.process_relic( this, pos_bub() );
    } );

    suffer();
    recalc_speed_bonus();
    //
    if( !has_effect( effect_ridden ) ) {
        moves += get_speed();
    }
    // NPCs currently don't make any use of their scent, pointless to calculate it
    // TODO: make use of NPC scent.
    if( !is_npc() ) {
        if( !has_effect( effect_masked_scent ) ) {
            restore_scent();
        }
        const int mask_intensity = get_effect_int( effect_masked_scent );

        // Set our scent towards the norm
        int norm_scent = 500;
        int temp_norm_scent = INT_MIN;
        bool found_intensity = false;
        for( const trait_id &mut : get_functioning_mutations() ) {
            const std::optional<int> &scent_intensity = mut->scent_intensity;
            if( scent_intensity ) {
                found_intensity = true;
                temp_norm_scent = std::max( temp_norm_scent, *scent_intensity );
            }
        }
        if( found_intensity ) {
            norm_scent = temp_norm_scent;
        }

        norm_scent = enchantment_cache->modify_value( enchant_vals::mod::SCENT_MASK, norm_scent );
        //mask from scent altering items;
        norm_scent += mask_intensity;

        // Scent increases fast at first, and slows down as it approaches normal levels.
        // Estimate it will take about norm_scent * 2 turns to go from 0 - norm_scent / 2
        // Without smelly trait this is about 1.5 hrs. Slows down significantly after that.
        if( scent < rng( 0, norm_scent ) ) {
            scent++;
        }

        // Unusually high scent decreases steadily until it reaches normal levels.
        if( scent > norm_scent ) {
            scent--;
        }
    }

    // We can dodge again! Assuming we can actually move...
    if( in_sleep_state() ) {
        blocks_left = 0;
        set_dodges_left( 0 );
    } else if( moves > 0 ) {
        blocks_left = get_num_blocks();
        set_dodges_left( get_num_dodges() );
    }

    // auto-learning. This is here because skill-increases happens all over the place:
    // SkillLevel::readBook (has no connection to the skill or the player),
    // player::read, player::practice, ...
    // Check for spontaneous discovery of martial art styles
    for( auto &style : autolearn_martialart_types() ) {
        const matype_id &ma( style );

        if( !martial_arts_data->has_martialart( ma ) && can_autolearn( ma ) ) {
            martial_arts_data->add_martialart( ma );
            add_msg_if_player( m_info, _( "You have learned a new style: %s!" ), ma.obj().name );
        }
    }

    // Update time spent conscious in this overmap tile for the Nomad traits.
    if( !is_npc() && ( has_trait( trait_NOMAD ) || has_trait( trait_NOMAD2 ) ||
                       has_trait( trait_NOMAD3 ) ) &&
        !has_effect( effect_sleep ) && !has_effect( effect_narcosis ) ) {
        const tripoint_abs_omt ompos = pos_abs_omt();
        const point_abs_omt pos = ompos.xy();
        if( overmap_time.find( pos ) == overmap_time.end() ) {
            overmap_time[pos] = 1_turns;
        } else {
            overmap_time[pos] += 1_turns;
        }
    }
    // Decay time spent in other overmap tiles.
    if( !is_npc() && calendar::once_every( 1_hours ) ) {
        const tripoint_abs_omt ompos = pos_abs_omt();
        const time_point now = calendar::turn;
        time_duration decay_time = 0_days;
        if( has_trait( trait_NOMAD ) ) {
            decay_time = 7_days;
        } else if( has_trait( trait_NOMAD2 ) ) {
            decay_time = 14_days;
        } else if( has_trait( trait_NOMAD3 ) ) {
            decay_time = 28_days;
        }
        auto it = overmap_time.begin();
        while( it != overmap_time.end() ) {
            if( it->first == ompos.xy() ) {
                ++it;
                continue;
            }
            // Find the amount of time passed since the player touched any of the overmap tile's submaps.
            const tripoint_abs_omt tpt( it->first, 0 );
            const time_point last_touched = overmap_buffer.scent_at( tpt ).creation_time;
            const time_duration since_visit = now - last_touched;
            // If the player has spent little time in this overmap tile, let it decay after just an hour instead of the usual extended decay time.
            const time_duration modified_decay_time = it->second > 5_minutes ? decay_time : 1_hours;
            if( since_visit > modified_decay_time ) {
                // Reduce the tracked time spent in this overmap tile.
                const time_duration decay_amount = std::min( since_visit - modified_decay_time, 1_hours );
                const time_duration updated_value = it->second - decay_amount;
                if( updated_value <= 0_turns ) {
                    // We can stop tracking this tile if there's no longer any time recorded there.
                    it = overmap_time.erase( it );
                    continue;
                } else {
                    it->second = updated_value;
                }
            }
            ++it;
        }
    }
    effect_on_conditions::process_effect_on_conditions( *this );
}

void Character::recalc_hp()
{
    // Mutated toughness stacks with starting, by design.
    float hp_mod = 1.0f + enchantment_cache->get_value_multiply( enchant_vals::mod::MAX_HP );
    float hp_adjustment = enchantment_cache->get_value_add( enchant_vals::mod::MAX_HP );
    calc_all_parts_hp( hp_mod, hp_adjustment, get_str_base(), get_dex_base(), get_per_base(),
                       get_int_base(), get_lifestyle(), get_fat_to_hp() );
    cached_dead_state.reset();
}

void Character::calc_all_parts_hp( float hp_mod, float hp_adjustment, int str_max, int dex_max,
                                   int per_max, int int_max, int healthy_mod, int fat_to_max_hp )
{
    for( std::pair<const bodypart_str_id, bodypart> &part : body ) {
        int new_max = ( part.first->base_hp + str_max * part.first->hp_mods.str_mod + dex_max *
                        part.first->hp_mods.dex_mod + int_max * part.first->hp_mods.int_mod + per_max *
                        part.first->hp_mods.per_mod + part.first->hp_mods.health_mod * healthy_mod + fat_to_max_hp +
                        hp_adjustment ) * hp_mod;

        if( has_trait( trait_GLASSJAW ) && part.first == body_part_head ) {
            new_max *= 0.8;
        }

        new_max = std::max( 1, new_max );

        float max_hp_ratio = static_cast<float>( new_max ) /
                             static_cast<float>( part.second.get_hp_max() );

        int new_cur = std::ceil( static_cast<float>( part.second.get_hp_cur() ) * max_hp_ratio );

        part.second.set_hp_max( std::max( new_max, 1 ) );
        part.second.set_hp_cur( std::max( std::min( new_cur, new_max ), 0 ) );
    }
}

// This must be called when any of the following change:
// - effects
// - bionics
// - traits
// - underwater
// - clothes
// With the exception of clothes, all changes to these character attributes must
// occur through a function in this class which calls this function. Clothes are
// typically added/removed with wear() and takeoff(), but direct access to the
// 'wears' vector is still allowed due to refactor exhaustion.
void Character::recalc_sight_limits()
{
    sight_max = 9999;
    vision_mode_cache.reset();
    const bool in_light = get_map().ambient_light_at( pos_bub() ) > LIGHT_AMBIENT_LIT;
    bool in_shell = has_active_mutation( trait_SHELL2 ) ||
                    has_active_mutation( trait_SHELL3 );

    // Set sight_max.
    if( is_blind() || ( in_sleep_state() && !has_flag( json_flag_SEESLEEP ) && is_avatar() ) ||
        has_effect( effect_narcosis ) ) {
        sight_max = 0;
    } else if( has_effect( effect_boomered ) && ( !has_trait( trait_PER_SLIME_OK ) ) ) {
        sight_max = 1;
        vision_mode_cache.set( BOOMERED );
    } else if( has_effect( effect_in_pit ) || has_effect( effect_no_sight ) ||
               ( underwater && !has_flag( json_flag_EYE_MEMBRANE ) && !worn_with_flag( flag_SWIM_GOGGLES ) ) ) {
        sight_max = 1;
    } else if( in_shell ) { // NOLINT(bugprone-branch-clone)
        // You can kinda see out a bit.
        sight_max = 2;
    } else if( has_trait( trait_PER_SLIME ) ) {
        sight_max = 8;
    } else if( ( has_flag( json_flag_MYOPIC ) || ( in_light &&
                 has_flag( json_flag_MYOPIC_IN_LIGHT ) ) ) &&
               !worn_with_flag( flag_FIX_NEARSIGHT ) && !has_effect( effect_contacts ) &&
               !has_effect( effect_transition_contacts ) ) {
        sight_max = 12;
    } else if( has_effect( effect_darkness ) ) {
        vision_mode_cache.set( DARKNESS );
        sight_max = 10;
    }

    // Debug-only NV, by vache's request
    if( has_trait( trait_DEBUG_NIGHTVISION ) ) {
        vision_mode_cache.set( DEBUG_NIGHTVISION );
    }
    if( has_nv_goggles() ) {
        vision_mode_cache.set( NV_GOGGLES );
    }
    if( has_active_mutation( trait_NIGHTVISION3 ) || is_wearing( itype_rm13_armor_on ) ||
        ( is_mounted() && mounted_creature->has_flag( mon_flag_MECH_RECON_VISION ) ) ) {
        vision_mode_cache.set( NIGHTVISION_3 );
    }
    if( has_active_mutation( trait_ELFA_FNV ) ) {
        vision_mode_cache.set( FULL_ELFA_VISION );
    }
    if( has_active_mutation( trait_CEPH_VISION ) ) {
        vision_mode_cache.set( CEPH_VISION );
    }
    if( has_active_mutation( trait_ELFA_NV ) ) {
        vision_mode_cache.set( ELFA_VISION );
    }
    if( has_active_mutation( trait_NIGHTVISION2 ) ) {
        vision_mode_cache.set( NIGHTVISION_2 );
    }
    if( has_active_mutation( trait_FEL_NV ) ) {
        vision_mode_cache.set( FELINE_VISION );
    }
    if( has_active_mutation( trait_URSINE_EYE ) ) {
        vision_mode_cache.set( URSINE_VISION );
    }
    if( has_active_mutation( trait_NIGHTVISION ) ) {
        vision_mode_cache.set( NIGHTVISION_1 );
    }
    if( has_trait( trait_BIRD_EYE ) ) {
        vision_mode_cache.set( BIRD_EYE );
    }

    if( has_flag( json_flag_SUPER_CLAIRVOYANCE ) ) {
        vision_mode_cache.set( VISION_CLAIRVOYANCE_SUPER );
    } else if( has_flag( json_flag_CLAIRVOYANCE_PLUS ) ) {
        vision_mode_cache.set( VISION_CLAIRVOYANCE_PLUS );
    } else if( has_flag( json_flag_CLAIRVOYANCE ) ) {
        vision_mode_cache.set( VISION_CLAIRVOYANCE );
    }
}

static float threshold_for_range( float range )
{
    constexpr float epsilon = 0.01f;
    return LIGHT_AMBIENT_MINIMAL / std::exp( range * LIGHT_TRANSPARENCY_OPEN_AIR ) - epsilon;
}

float Character::get_vision_threshold( float light_level ) const
{
    if( vision_mode_cache[DEBUG_NIGHTVISION] ) {
        // Debug vision always works with absurdly little light.
        return 0.01;
    }

    // As light_level goes from LIGHT_AMBIENT_MINIMAL to LIGHT_AMBIENT_LIT,
    // dimming goes from 1.0 to 2.0.
    const float dimming_from_light = 1.0f + ( ( light_level - LIGHT_AMBIENT_MINIMAL ) /
                                     ( LIGHT_AMBIENT_LIT - LIGHT_AMBIENT_MINIMAL ) );

    float range = get_per() / 3.0f;
    if( vision_mode_cache[NIGHTVISION_3] || vision_mode_cache[FULL_ELFA_VISION] ||
        vision_mode_cache[CEPH_VISION] ) {
        range += 10;
    } else if( vision_mode_cache[NIGHTVISION_2] || vision_mode_cache[FELINE_VISION] ||
               vision_mode_cache[URSINE_VISION] || vision_mode_cache[ELFA_VISION] ) {
        range += 4.5;
    } else if( vision_mode_cache[NIGHTVISION_1] ) {
        range += 2;
    }

    if( vision_mode_cache[BIRD_EYE] ) {
        range++;
    }

    // bionic night vision and other old night vision flagged items
    if( worn_with_flag( flag_GNV_EFFECT ) || has_flag( json_flag_NIGHT_VISION ) ) {
        range += 10;
    } else {
        range = enchantment_cache->modify_value( enchant_vals::mod::NIGHT_VIS, range );
    }

    // Clamp range to 1+, so that we can always see where we are
    range = std::max( 1.0f, range * get_limb_score( limb_score_night_vis ) );

    return std::min( LIGHT_AMBIENT_LOW,
                     threshold_for_range( range ) * dimming_from_light );
}

void Character::flag_encumbrance()
{
    check_encumbrance = true;
}

void Character::check_item_encumbrance_flag()
{
    if( worn.check_item_encumbrance_flag( check_encumbrance ) ) {
        calc_encumbrance();
    }
}

bool Character::natural_attack_restricted_on( const bodypart_id &bp ) const
{
    return worn.natural_attack_restricted_on( bp );
}

bool Character::natural_attack_restricted_on( const sub_bodypart_id &bp ) const
{
    return worn.natural_attack_restricted_on( bp );
}

std::vector<const item *> Character::get_pseudo_items() const
{
    if( !pseudo_items_valid ) {
        pseudo_items.clear();

        for( const bionic &bio : *my_bionics ) {
            std::vector<const item *> pseudos = bio.get_available_pseudo_items();
            for( const item *pseudo : pseudos ) {
                pseudo_items.push_back( pseudo );
            }
        }

        pseudo_items_valid = true;
    }
    return pseudo_items;
}

bool Character::practice( const skill_id &id, int amount, int cap, bool suppress_warning,
                          bool allow_multilevel )
{
    SkillLevel &level = get_skill_level_object( id );
    const Skill &skill = id.obj();
    if( !level.can_train() || in_sleep_state() ||
        ( static_cast<int>( get_skill_level( id ) ) >= MAX_SKILL ) ) {
        // Do not practice if: cannot train, asleep, or at effective skill cap
        // Leaving as a skill method rather than global for possible future skill cap setting
        return false;
    }

    // Your ability to "catch up" skill experience to knowledge is mostly a function of intelligence,
    // but perception also plays a role, representing both memory/attentiveness and catching on to how
    // the two apply to each other.
    float catchup_modifier = 1.0f + ( 2.0f * get_int() + get_per() ) / 24.0f; // 2 for an average person
    float knowledge_modifier = 1.0f + get_int() /
                               40.0f; // 1.2 for an average person, always a bit higher than base amount

    const auto highest_skill = [&]() {
        std::pair<skill_id, int> result( skill_id::NULL_ID(), -1 );
        for( const auto &pair : *_skills ) {
            const SkillLevel &lobj = pair.second;
            if( lobj.level() > result.second ) {
                result = std::make_pair( pair.first, lobj.level() );
            }
        }
        return result.first;
    };

    const bool isSavant = has_trait( trait_SAVANT );
    const skill_id savantSkill = isSavant ? highest_skill() : skill_id::NULL_ID();

    amount = adjust_for_focus( std::min( 1000, amount ) ) * 100.0f;

    if( has_trait( trait_PACIFIST ) && skill.is_combat_skill() ) {
        amount /= 3.0f;
    }

    catchup_modifier = enchantment_cache->modify_value( enchant_vals::mod::COMBAT_CATCHUP,
                       catchup_modifier );

    if( isSavant && id != savantSkill ) {
        amount *= 0.5f;
    }

    if( amount > 0 &&
        static_cast<int>( get_skill_level( id ) ) > cap ) { //blunt grinding cap implementation for crafting
        amount = 0;
        if( !suppress_warning ) {
            handle_skill_warning( id, false );
        }
    }
    bool level_up = false;
    if( amount > 0 && level.isTraining() ) {
        int old_practical_level = static_cast<int>( get_skill_level( id ) );
        int old_theoretical_level = get_knowledge_level( id );
        get_skill_level_object( id ).train( amount, catchup_modifier, knowledge_modifier,
                                            allow_multilevel );
        int new_practical_level = static_cast<int>( get_skill_level( id ) );
        int new_theoretical_level = get_knowledge_level( id );
        std::string skill_name = skill.name();
        if( new_practical_level > old_practical_level ) {
            get_event_bus().send<event_type::gains_skill_level>( getID(), id, new_practical_level );
        }
        if( is_avatar() && new_practical_level > old_practical_level ) {
            add_msg( m_good, _( "Your practical skill in %s has increased to %d!" ), skill_name,
                     new_practical_level );
            level_up = true;
        }
        if( is_avatar() && new_theoretical_level > old_theoretical_level ) {
            add_msg( m_good, _( "Your theoretical understanding of %s has increased to %d!" ), skill_name,
                     new_theoretical_level );
        }
        if( is_avatar() && new_practical_level > cap ) {
            //inform player immediately that the current recipe can't be used to train further
            add_msg( m_info, _( "You feel that %s tasks of this level are becoming trivial." ),
                     skill_name );
        }

        // Apex Predators don't think about much other than killing.
        // They don't lose Focus when practicing combat skills.
        if( !( has_flag( json_flag_PRED4 ) && skill.is_combat_skill() ) ) {
            // Base reduction on the larger of 1% of total, or practice amount.
            // The latter kicks in when long actions like crafting
            // apply many turns of gains at once.
            int focus_drain = std::max( focus_pool / 100, amount );

            // The purpose of having this squared is that it makes focus drain dramatically slower
            // as it approaches zero. As such, the square function would not be used if the drain is
            // larger or equal to 1000 to avoid the runaway, and the original drain gets applied instead.
            if( focus_drain >= 1000 ) {
                focus_pool -= focus_drain;
            } else {
                focus_pool -= ( focus_drain * focus_drain ) / 1000;
            }
        }
        focus_pool = std::max( focus_pool, 0 );
    }

    get_skill_level_object( id ).practice();
    return level_up;
}

// Returned values range from 1.0 (unimpeded vision) to 11.0 (totally blind).
//  1.0 is LIGHT_AMBIENT_LIT or brighter
//  4.0 is a dark clear night, barely bright enough for reading and crafting
//  6.0 is LIGHT_AMBIENT_DIM
//  7.3 is LIGHT_AMBIENT_MINIMAL, a dark cloudy night, unlit indoors
// 11.0 is zero light or blindness
float Character::fine_detail_vision_mod( const tripoint_bub_ms &p ) const
{
    // PER_SLIME_OK implies you can get enough eyes around the bile
    // that you can generally see.  There still will be the haze, but
    // it's annoying rather than limiting.
    if( is_blind() ||
        ( ( has_effect( effect_boomered ) || has_effect( effect_darkness ) ) &&
          !has_trait( trait_PER_SLIME_OK ) ) ) {
        return 11.0;
    }
    // Scale linearly as light level approaches LIGHT_AMBIENT_LIT.
    // If we're actually a source of light, assume we can direct it where we need it.
    // Therefore give a hefty bonus relative to ambient light.
    float own_light = std::max( 1.0f, LIGHT_AMBIENT_LIT - active_light() - 2.0f );

    // Same calculation as above, but with a result 3 lower.
    float ambient_light{};
    tripoint_bub_ms const check_p = p.is_invalid() ? pos_bub() : p;
    tripoint_bub_ms const avatar_p = get_avatar().pos_bub();
    if( is_avatar() || check_p.z() == avatar_p.z() ) {
        ambient_light = std::max( 1.0f,
                                  LIGHT_AMBIENT_LIT - get_map().ambient_light_at( check_p ) + 1.0f );
    } else {
        // light map is not calculated outside the player character's z-level
        // even if fov_3d_z_range > 0, and building light map on multiple levels
        // could be expensive, so make NPCs able to see things in this case to
        // not interfere with NPC activity.
        ambient_light = 1.0f;
    }

    return std::min( own_light, ambient_light );
}

units::energy Character::get_power_level() const
{
    return power_level;
}

units::energy Character::get_max_power_level() const
{
    units::energy val = enchantment_cache->modify_value( enchant_vals::mod::BIONIC_POWER,
                        max_power_level_cached + max_power_level_modifier );
    return clamp( val, 0_kJ, units::energy::max() );
}

void Character::set_power_level( const units::energy &npower )
{
    power_level = clamp( npower, 0_kJ, get_max_power_level() );
}

void Character::set_max_power_level_modifier( const units::energy &capacity )
{
    max_power_level_modifier = clamp( capacity, units::energy::min(), units::energy::max() );
}

void Character::set_max_power_level( const units::energy &capacity )
{
    max_power_level_modifier = clamp( capacity - max_power_level_cached, units::energy::min(),
                                      units::energy::max() );
}

void Character::mod_power_level( const units::energy &npower )
{
    set_power_level( power_level + npower );
    if( npower < 0_kJ && !has_power() ) {
        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( !bp->no_power_effect.is_null() ) {
                add_effect( bp->no_power_effect, 5_turns );
            }
        }
    }
}

void Character::mod_max_power_level_modifier( const units::energy &npower )
{
    max_power_level_modifier = clamp( max_power_level_modifier + npower, units::energy::min(),
                                      units::energy::max() );
}

bool Character::is_max_power() const
{
    return get_power_level() >= get_max_power_level();
}

bool Character::has_power() const
{
    return get_power_level() > 0_kJ;
}

bool Character::has_max_power() const
{
    return get_max_power_level() > 0_kJ;
}

bool Character::enough_power_for( const bionic_id &bid ) const
{
    return get_power_level() >= bid->power_activate;
}

std::vector<skill_id> Character::skills_offered_to( const Character *you ) const
{
    std::vector<skill_id> ret;
    for( const auto &pair : *_skills ) {
        const skill_id &id = pair.first;
        if( id->is_teachable() && pair.second.level() > 0 &&
            ( !you || you->get_knowledge_level( id ) < pair.second.level() ) ) {
            ret.push_back( id );
        }
    }
    return ret;
}

std::vector<matype_id> Character::styles_offered_to( const Character *you ) const
{
    if( you ) {
        return you->martial_arts_data->get_unknown_styles( *martial_arts_data, true );
    }
    return martial_arts_data->get_known_styles( true );
}

std::vector<spell_id> Character::spells_offered_to( const Character *you ) const
{
    std::vector<spell_id> teachable;
    for( const spell_id &sp : magic->spells() ) {
        const spell &teacher_spell = magic->get_spell( sp );
        if( sp->teachable && ( !you || you->magic->can_learn_spell( *you, sp ) ) ) {
            if( you && you->magic->knows_spell( sp ) ) {
                const spell &student_spell = you->magic->get_spell( sp );
                if( student_spell.is_max_level( *you ) ||
                    student_spell.get_level() >= teacher_spell.get_level() ) {
                    continue;
                }
            }
            teachable.emplace_back( sp );
        }
    }
    return teachable;
}

void Character::conduct_blood_analysis()
{
    std::vector<std::string> effect_descriptions;
    std::vector<nc_color> colors;

    for( auto &elem : *effects ) {
        if( elem.first->get_blood_analysis_description().empty() ) {
            continue;
        }
        effect_descriptions.emplace_back( elem.first->get_blood_analysis_description() );
        colors.emplace_back( elem.first->get_rating() == m_good ? c_green : c_red );
    }

    const int win_w = 46;
    size_t win_h = 0;
    catacurses::window w;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        win_h = std::min( static_cast<size_t>( TERMY ),
                          std::max<size_t>( 1, effect_descriptions.size() ) + 2 );
        w = catacurses::newwin( win_h, win_w,
                                point( ( TERMX - win_w ) / 2, ( TERMY - win_h ) / 2 ) );
        ui.position_from_window( w );
    } );
    ui.mark_resize();
    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_border( w, c_red, string_format( " %s ", _( "Blood Test Results" ) ) );
        if( effect_descriptions.empty() ) {
            trim_and_print( w, point( 2, 1 ), win_w - 3, c_white, _( "No effects." ) );
        } else {
            for( size_t line = 1; line < ( win_h - 1 ) && line <= effect_descriptions.size(); ++line ) {
                trim_and_print( w, point( 2, line ), win_w - 3, colors[line - 1], effect_descriptions[line - 1] );
            }
        }
        wnoutrefresh( w );
    } );
    input_context ctxt( "BLOOD_TEST_RESULTS" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    bool stop = false;
    // Display new messages
    g->invalidate_main_ui_adaptor();
    while( !stop ) {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "CONFIRM" || action == "QUIT" ) {
            stop = true;
        }
    }
}

int Character::get_standard_stamina_cost( const item *thrown_item ) const
{
    // Previously calculated as 2_gram * std::max( 1, str_cur )
    // using 16_gram normalizes it to 8 str. Same effort expenditure
    // for each strike, regardless of weight. This is compensated
    // for by the additional move cost as weapon weight increases
    //If the item is thrown, override with the thrown item instead.

    item current_weapon = used_weapon() ? *used_weapon() : null_item_reference();

    const int weight_cost = ( thrown_item == nullptr ) ? current_weapon.weight() /
                            16_gram : thrown_item->weight() / 16_gram;
    return ( weight_cost + 50 ) * -1 * get_modifier( character_modifier_melee_stamina_cost_mod );
}

std::vector<item_location> Character::nearby( const
        std::function<bool( const item *, const item * )> &func, int radius ) const
{
    map &here = get_map();
    std::vector<item_location> res;

    visit_items( [&]( const item * e, const item * parent ) {
        if( func( e, parent ) ) {
            res.emplace_back( const_cast<Character &>( *this ), const_cast<item *>( e ) );
        }
        return VisitResponse::NEXT;
    } );

    for( const map_cursor &cur : map_selector( pos_bub( here ), radius ) ) {
        cur.visit_items( [&]( const item * e, const item * parent ) {
            if( func( e, parent ) ) {
                res.emplace_back( cur, const_cast<item *>( e ) );
            }
            return VisitResponse::NEXT;
        } );
    }

    for( const vehicle_cursor &cur : vehicle_selector( here, pos_bub( here ), radius ) ) {
        cur.visit_items( [&]( const item * e, const item * parent ) {
            if( func( e, parent ) ) {
                res.emplace_back( cur, const_cast<item *>( e ) );
            }
            return VisitResponse::NEXT;
        } );
    }

    return res;
}

std::list<item> Character::remove_worn_items_with( const std::function<bool( item & )> &filter )
{
    invalidate_inventory_validity_cache();
    invalidate_leak_level_cache();
    return worn.remove_worn_items_with( filter, *this );
}

void Character::clear_worn()
{
    worn.worn.clear();
    inv_search_caches.clear();
}

std::list<item *> Character::get_dependent_worn_items( const item &it )
{
    std::list<item *> dependent;

    if( is_worn( it ) ) {
        worn.add_dependent_item( dependent, it );
    }

    return dependent;
}

item Character::remove_weapon()
{
    item tmp = weapon;
    weapon = item();
    get_event_bus().send<event_type::character_wields_item>( getID(), weapon.typeId() );
    cached_info.erase( "weapon_value" );
    invalidate_weight_carried_cache();
    return tmp;
}

void Character::remove_mission_items( int mission_id )
{
    if( mission_id == -1 ) {
        return;
    }
    remove_items_with( has_mission_item_filter { mission_id } );
}

void Character::on_move( const tripoint_abs_ms &old_pos )
{
    Creature::on_move( old_pos );
    // Ugly to compare a tripoint_bub_ms with a tripoint_abs_ms, but the 'z' component
    // is the same regardless of the x/y reference point.
    if( this->posz() != old_pos.z() ) {
        // Make sure caches are rebuilt in a timely manner to ensure companions aren't
        // caught in the "dark" because the caches aren't updated on their next move.
        get_map().build_map_cache( this->posz() );
    }
    // In case we've moved out of range of lifting assist.
    if( using_lifting_assist ) {
        invalidate_weight_carried_cache();
    }
}

units::volume Character::get_total_volume() const
{
    item_location wep = get_wielded_item();
    units::volume wep_volume = wep ? wep->volume() : 0_ml;
    // Note: Does not measure volume of worn items that do not themselves contain anything
    return get_base_volume() + volume_carried() + wep_volume;
}

units::volume Character::get_base_volume() const
{
    // The formula used here to calculate base volume for a human body is
    //     BV = W / BD
    // Where:
    // * BV is the body volume in liters
    // * W  is the weight in kilograms
    // * BD is the body density (kg/L), estimated using the Brozek formula:
    //     BD = 1.097 – 0.00046971 * W + 0.00000056 * W^2 – 0.00012828 * H
    // See
    //   https://en.wikipedia.org/wiki/Body_fat_percentage
    //   https://calculator.academy/body-volume-calculator/
    const int your_height = height();
    const double your_weight = units::to_kilogram( bodyweight() );
    const double your_density = 1.097 - 0.00046971 * your_weight
                                + 0.00000056 * std::pow( your_weight, 2 )
                                - 0.00012828 * your_height;
    units::volume your_base_volume = units::from_liter( your_weight / your_density );
    return your_base_volume;
}

units::mass Character::weight_carried() const
{
    if( cached_weight_carried ) {
        return *cached_weight_carried;
    }
    cached_weight_carried = weight_carried_with_tweaks( item_tweaks() );
    return *cached_weight_carried;
}

void Character::invalidate_weight_carried_cache()
{
    cached_weight_carried = std::nullopt;
}

units::mass Character::best_nearby_lifting_assist() const
{
    return best_nearby_lifting_assist( this->pos_bub() );
}

units::mass Character::best_nearby_lifting_assist( const tripoint_bub_ms &world_pos ) const
{
    map &here = get_map();
    int mech_lift = 0;
    if( is_mounted() ) {
        auto *mons = mounted_creature.get();
        if( mons->has_flag( mon_flag_RIDEABLE_MECH ) ) {
            mech_lift = mons->mech_str_addition() + 10;
        }
    }
    int lift_quality = std::max( { this->max_quality( qual_LIFT ), mech_lift,
                                   map_selector( this->pos_bub(), PICKUP_RANGE ).max_quality( qual_LIFT ),
                                   vehicle_selector( here, world_pos, 4, true, true ).max_quality( qual_LIFT )
                                 } );
    return lifting_quality_to_mass( lift_quality );
}

units::mass Character::weight_carried_with_tweaks( const std::vector<std::pair<item_location, int>>
        &locations ) const
{
    std::map<const item *, int> dropping;
    for( const std::pair<item_location, int> &location_pair : locations ) {
        dropping.emplace( location_pair.first.get_item(), location_pair.second );
    }
    return weight_carried_with_tweaks( item_tweaks( { dropping } ) );
}

units::mass Character::weight_carried_with_tweaks( const item_tweaks &tweaks ) const
{
    const std::map<const item *, int> empty;
    const std::map<const item *, int> &without = tweaks.without_items ? tweaks.without_items->get() :
            empty;

    // Worn items
    units::mass ret = worn.weight_carried_with_tweaks( without );

    // Wielded item
    units::mass weaponweight = 0_gram;
    if( !without.count( &weapon ) ) {
        weaponweight += weapon.weight();
        for( const item *i : weapon.all_items_ptr( pocket_type::CONTAINER ) ) {
            if( i->count_by_charges() ) {
                weaponweight -= get_selected_stack_weight( i, without );
            } else if( without.count( i ) ) {
                weaponweight -= i->weight();
            }
        }
    } else if( weapon.count_by_charges() ) {
        weaponweight += weapon.weight() - get_selected_stack_weight( &weapon, without );
    }

    // Exclude wielded item if using lifting tool
    if( ( weaponweight + ret <= weight_capacity() ) || ( g->new_game ||
            best_nearby_lifting_assist() < weaponweight ) ) {
        ret += weaponweight;
        using_lifting_assist = false;
    } else {
        using_lifting_assist = true;
    }

    return ret;
}

units::volume Character::volume_carried_with_tweaks( const
        std::vector<std::pair<item_location, int>>
        &locations ) const
{
    std::map<const item *, int> dropping;
    for( const std::pair<item_location, int> &location_pair : locations ) {
        dropping.emplace( location_pair.first.get_item(), location_pair.second );
    }
    return volume_carried_with_tweaks( item_tweaks( { dropping } ) );
}

units::volume Character::volume_carried_with_tweaks( const item_tweaks &tweaks ) const
{
    const std::map<const item *, int> empty;
    const std::map<const item *, int> &without = tweaks.without_items ? tweaks.without_items->get() :
            empty;

    // Worn items
    units::volume ret = worn.contents_volume_with_tweaks( without );

    // Wielded item
    if( !without.count( &weapon ) ) {
        ret += weapon.get_contents_volume_with_tweaks( without );
    }

    return ret;
}

units::mass Character::weight_capacity() const
{
    // Get base capacity from creature,
    // then apply enchantment.
    units::mass ret = Creature::weight_capacity();
    // Not using get_str() so pain and other temporary effects won't decrease carrying capacity;
    // transiently reducing carry weight is unlikely to have any play impact besides being very annoying.
    /** @EFFECT_STR increases carrying capacity */
    ret += ( get_str_base() + get_str_bonus() ) * 4_kilogram;

    ret = enchantment_cache->modify_value( enchant_vals::mod::CARRY_WEIGHT, ret );

    if( ret < 0_gram ) {
        ret = 0_gram;
    }
    if( is_mounted() ) {
        auto *mons = mounted_creature.get();
        // the mech has an effective strength for other purposes, like hitting.
        // but for lifting, its effective strength is even higher, due to its sturdy construction, leverage,
        // and being built entirely for that purpose with hydraulics etc.
        ret = mons->mech_str_addition() == 0 ? ret : ( mons->mech_str_addition() + 10 ) * 4_kilogram;
    }
    return ret;
}

units::mass Character::max_pickup_capacity() const
{
    return weight_capacity() * 4;
}

bool Character::can_pickVolume( const item &it, bool, const item *avoid,
                                const bool ignore_pkt_settings ) const
{
    if( ( avoid == nullptr || &weapon != avoid ) &&
        weapon.can_contain( it, false, false, ignore_pkt_settings ).success() ) {
        return true;
    }
    if( worn.can_pickVolume( it, ignore_pkt_settings ) ) {
        return true;
    }
    return false;
}

bool Character::can_pickVolume_partial( const item &it, bool, const item *avoid,
                                        const bool ignore_pkt_settings, const bool ignore_non_container_pocket ) const
{
    item copy = it;
    if( it.count_by_charges() ) {
        copy.charges = 1;
    }

    if( ( avoid == nullptr || &weapon != avoid ) &&
        weapon.can_contain( copy, false, false, ignore_pkt_settings, ignore_non_container_pocket
                          ).success() ) {
        return true;
    }

    return worn.can_pickVolume( copy, ignore_pkt_settings, ignore_non_container_pocket );
}

bool Character::can_pickWeight( const item &it, bool safe ) const
{
    if( !safe ) {
        return ( weight_carried() + it.weight() <= max_pickup_capacity() );
    } else {
        return ( weight_carried() + it.weight() <= weight_capacity() );
    }
}

bool Character::can_pickWeight_partial( const item &it, bool safe ) const
{
    item copy = it;
    if( it.count_by_charges() ) {
        copy.charges = 1;
    }

    return can_pickWeight( copy, safe );
}

bool Character::can_use( const item &it, const item &context ) const
{
    if( has_effect( effect_incorporeal ) ) {
        add_msg_player_or_npc( m_bad, _( "You can't use anything while incorporeal." ),
                               _( "<npcname> can't use anything while incorporeal." ) );
        return false;
    }
    const item &ctx = !context.is_null() ? context : it;

    if( !meets_requirements( it, ctx ) ) {
        const std::string unmet( enumerate_unmet_requirements( it, ctx ) );

        if( &it == &ctx ) {
            //~ %1$s - list of unmet requirements, %2$s - item name.
            add_msg_player_or_npc( m_bad, _( "You need at least %1$s to use this %2$s." ),
                                   _( "<npcname> needs at least %1$s to use this %2$s." ),
                                   unmet, it.tname() );
        } else {
            //~ %1$s - list of unmet requirements, %2$s - item name, %3$s - indirect item name.
            add_msg_player_or_npc( m_bad, _( "You need at least %1$s to use this %2$s with your %3$s." ),
                                   _( "<npcname> needs at least %1$s to use this %2$s with their %3$s." ),
                                   unmet, it.tname(), ctx.tname() );
        }

        return false;
    }

    return true;
}

ret_val<void> Character::can_unwield( const item &it ) const
{
    if( it.has_flag( flag_NO_UNWIELD ) ) {
        // check if "it" is currently wielded fake bionic weapon that can be deactivated
        std::optional<bionic *> bio_opt = find_bionic_by_uid( get_weapon_bionic_uid() );
        if( !is_wielding( it ) || it.ethereal || !bio_opt ||
            !can_deactivate_bionic( **bio_opt ).success() ) {
            return ret_val<void>::make_failure( _( "You cannot unwield your %s." ), it.tname() );
        }
    }

    return ret_val<void>::make_success();
}

void Character::invalidate_inventory_validity_cache()
{
    cache_inventory_is_valid = false;
}
bool Character::is_wielding( const item &target ) const
{
    return &weapon == &target;
}

std::vector<std::pair<std::string, std::string>> Character::get_overlay_ids() const
{
    std::vector<std::pair<std::string, std::string>> rval;
    std::multimap<int, std::pair<std::string, std::string>> mutation_sorting;
    int order;
    std::string overlay_id;
    std::string variant;
    // first get effects
    if( show_creature_overlay_icons ) {
        for( const auto &eff_pr : *effects ) {
            rval.emplace_back( "effect_" + eff_pr.first.str(), "" );
        }
    }

    // then get mutations
    for( const auto &mut : cached_mutations ) {
        if( mut.second.corrupted > 0 || !mut.second.show_sprite ) {
            continue;
        }
        overlay_id = ( mut.second.powered ? "active_" : "" ) + mut.first.str();
        if( mut.second.variant != nullptr ) {
            variant = mut.second.variant->id;
        }
        order = get_overlay_order_of_mutation( overlay_id );
        mutation_sorting.emplace( order, std::pair<std::string, std::string> { overlay_id, variant } );
    }

    // then get bionics
    for( const bionic &bio : *my_bionics ) {
        if( !bio.show_sprite ) {
            continue;
        }
        overlay_id = ( bio.powered ? "active_" : "" ) + bio.id.str();
        order = get_overlay_order_of_mutation( overlay_id );
        mutation_sorting.emplace( order, std::pair<std::string, std::string> { overlay_id, "" } );
    }

    for( auto &mutorder : mutation_sorting ) {
        rval.emplace_back( "mutation_" + mutorder.second.first, mutorder.second.second );
    }

    // next clothing
    worn.get_overlay_ids( rval );

    // last weapon
    // TODO: might there be clothing that covers the weapon?
    if( is_armed() ) {
        const std::string variant = weapon.has_itype_variant() ? weapon.itype_variant().id : "";
        rval.emplace_back( "wielded_" + weapon.typeId().str(), variant );
    }

    if( !is_walking() && show_creature_overlay_icons ) {
        rval.emplace_back( move_mode.str(), "" );
    }

    return rval;
}

std::vector<std::pair<std::string, std::string>> Character::get_overlay_ids_when_override_look()
        const
{
    std::vector<std::pair<std::string, std::string>> rval;
    if( !show_creature_overlay_icons ) {
        return rval;
    }
    // first get effects
    for( const auto &eff_pr : *effects ) {
        rval.emplace_back( "effect_" + eff_pr.first.str(), "" );
    }
    // then move_move sign
    if( !is_walking() ) {
        rval.emplace_back( move_mode.str(), "" );
    }
    return rval;
}

void Character::zero_all_skills()
{
    for( Skill &skill : Skill::skills ) {
        set_skill_level( skill.ident(), 0 );
    }
}

SkillLevelMap Character::get_all_skills() const
{
    SkillLevelMap skills = *_skills;
    for( std::pair<const skill_id, SkillLevel> &sk : skills ) {
        sk.second.level( std::round( enchantment_cache->modify_value( sk.first, sk.second.level() ) ) );
    }
    return skills;
}

const SkillLevel &Character::get_skill_level_object( const skill_id &ident ) const
{
    return _skills->get_skill_level_object( ident );
}

SkillLevel &Character::get_skill_level_object( const skill_id &ident )
{
    return _skills->get_skill_level_object( ident );
}

float Character::get_skill_level( const skill_id &ident ) const
{
    return enchantment_cache->modify_value( ident,
                                            _skills->get_skill_level( ident ) + _skills->get_progress_level( ident ) );
}

float Character::get_skill_level( const skill_id &ident, const item &context ) const
{
    return enchantment_cache->modify_value( ident, _skills->get_skill_level( ident,
                                            context ) + _skills->get_progress_level( ident,
                                                    context ) );
}

int Character::get_knowledge_level( const skill_id &ident ) const
{
    return _skills->get_knowledge_level( ident );
}

float Character::get_knowledge_plus_progress( const skill_id &ident ) const
{
    return _skills->get_knowledge_level( ident ) + _skills->get_knowledge_progress_level( ident );
}

int Character::get_knowledge_level( const skill_id &ident, const item &context ) const
{
    return _skills->get_knowledge_level( ident, context );
}

float Character::get_average_skill_level( const skill_id &ident ) const
{
    return ( get_skill_level( ident ) + get_knowledge_plus_progress( ident ) ) / 2;
}

float Character::get_greater_skill_or_knowledge_level( const skill_id &ident ) const
{
    return get_skill_level( ident ) > get_knowledge_plus_progress( ident ) ? get_skill_level(
               ident ) : get_knowledge_plus_progress( ident );
}

void Character::set_skill_level( const skill_id &ident, const int level )
{
    get_skill_level_object( ident ).level( level );
}

void Character::mod_skill_level( const skill_id &ident, const int delta )
{
    _skills->mod_skill_level( ident, delta );
}
void Character::set_knowledge_level( const skill_id &ident, const int level )
{
    get_skill_level_object( ident ).knowledgeLevel( level );
}

void Character::mod_knowledge_level( const skill_id &ident, const int delta )
{
    _skills->mod_knowledge_level( ident, delta );
}

std::string Character::enumerate_unmet_requirements( const item &it, const item &context ) const
{
    std::vector<std::string> unmet_reqs;

    const auto check_req = [ &unmet_reqs ]( const std::string & name, int cur, int req ) {
        if( cur < req ) {
            unmet_reqs.push_back( string_format( "%s %d", name, req ) );
        }
    };

    check_req( _( "strength" ),     get_str(), it.get_min_str() );
    check_req( _( "dexterity" ),    get_dex(), it.type->min_dex );
    check_req( _( "intelligence" ), get_int(), it.type->min_int );
    check_req( _( "perception" ),   get_per(), it.type->min_per );

    for( const auto &elem : it.type->min_skills ) {
        check_req( context.contextualize_skill( elem.first )->name(),
                   static_cast<int>( get_skill_level( elem.first, context ) ),
                   elem.second );
    }

    return enumerate_as_string( unmet_reqs );
}

int Character::read_speed() const
{
    // Stat window shows stat effects on based on current stat
    const float intel = 2.0f + get_int() / 8.0f;
    // 4 int = 72 seconds, 8 int = 60 seconds, 12 int = 51 seconds, 16 int = 45 seconds, 20 int = 40 seconds
    /** @EFFECT_INT affects reading speed by an decreasing amount the higher intelligence goes, intially about 9% per point at 4 int to lower than 4% at 20+ int */
    time_duration ret = 180_seconds / intel;

    ret = enchantment_cache->modify_value( enchant_vals::mod::READING_SPEED_MULTIPLIER, ret );
    if( ret < 1_seconds ) {
        ret = 1_seconds;
    }
    return ret * 100 / 1_minutes;
}

bool Character::meets_skill_requirements( const std::map<skill_id, int> &req,
        const item &context ) const
{
    return _skills->meets_skill_requirements( req, context );
}

bool Character::meets_skill_requirements( const construction &con ) const
{
    return std::all_of( con.required_skills.begin(), con.required_skills.end(),
    [&]( const std::pair<skill_id, int> &pr ) {
        return get_knowledge_level( pr.first ) >= pr.second;
    } );
}

bool Character::meets_stat_requirements( const item &it ) const
{
    return get_str() >= it.get_min_str() &&
           get_dex() >= it.type->min_dex &&
           get_int() >= it.type->min_int &&
           get_per() >= it.type->min_per;
}

bool Character::meets_requirements( const item &it, const item &context ) const
{
    const item &ctx = !context.is_null() ? context : it;
    return meets_stat_requirements( it ) && meets_skill_requirements( it.type->min_skills, ctx );
}

void Character::normalize()
{
    Creature::normalize();

    activity_history.weary_clear();
    martial_arts_data->reset_style();
    weapon = item( itype_id::NULL_ID(), calendar::turn_zero );

    set_body();
    recalc_hp();
    set_all_parts_temp_conv( BODYTEMP_NORM );
    set_stamina( get_stamina_max() );
    oxygen = get_oxygen_max();
}

// Actual player death is mostly handled in game::is_game_over
void Character::die( map *, Creature *nkiller )
{
    g->set_critter_died();
    set_all_parts_hp_cur( 0 );
    set_killer( nkiller );
    set_time_died( calendar::turn );

    if( has_effect( effect_heavysnare ) ) {
        inv->add_item( item( itype_rope_6, calendar::turn_zero ) );
        inv->add_item( item( itype_snare_trigger, calendar::turn_zero ) );
    }
    if( has_effect( effect_beartrap ) ) {
        inv->add_item( item( itype_beartrap, calendar::turn_zero ) );
    }
    mission::on_creature_death( *this );
}

void Character::prevent_death()
{
    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        if( bp->is_vital ) {
            if( get_part_hp_cur( bp ) <= 0 ) {
                set_part_hp_cur( bp, 1 );
            }
        }
    }
    cached_dead_state.reset();
}

std::pair<bodypart_id, int> Character::best_part_to_smash() const
{
    std::pair<bodypart_id, int> best_part_to_smash = {bodypart_str_id::NULL_ID().id(), 0};
    int tmp_bash_armor = 0;
    for( const bodypart_id &bp : get_all_body_parts() ) {
        tmp_bash_armor += worn.damage_resist( damage_bash, bp );
        for( const trait_id &mut : get_functioning_mutations() ) {
            const resistances &res = mut->damage_resistance( bp );
            tmp_bash_armor += std::floor( res.type_resist( damage_bash ) );
        }
        if( tmp_bash_armor > best_part_to_smash.second ) {
            best_part_to_smash = {bp, tmp_bash_armor};
        }
    }

    return best_part_to_smash;
}

int Character::smash_ability() const
{
    int ret = get_arm_str();
    ///\EFFECT_STR increases smashing capability
    if( is_mounted() ) {
        auto *mon = mounted_creature.get();
        ret += mon->mech_str_addition() + mon->type->melee_dice * mon->type->melee_sides;
    } else if( get_wielded_item() ) {
        ret += get_wielded_item()->damage_melee( damage_bash );
        ret = enchantment_cache->modify_melee_damage( damage_bash, ret );
    }

    if( !has_weapon() ) {
        std::pair<bodypart_id, int> best_part = best_part_to_smash();
        const int min_ret = ret * best_part.first->smash_efficiency;
        const int max_ret = ret * ( 1.0f + best_part.first->smash_efficiency );
        ret = std::min( best_part.second + min_ret, max_ret );
    }
    ret = calculate_by_enchantment( ret, enchant_vals::mod::MELEE_DAMAGE, true );

    return ret;
}

void Character::do_skill_rust()
{
    for( std::pair<const skill_id, SkillLevel> &pair : *_skills ) {
        const Skill &aSkill = *pair.first;
        SkillLevel &skill_level_obj = pair.second;

        if( aSkill.is_combat_skill() &&
            ( ( has_flag( json_flag_PRED2 ) && calendar::once_every( 8_hours ) ) ||
              ( has_flag( json_flag_PRED3 ) && calendar::once_every( 4_hours ) ) ||
              ( has_flag( json_flag_PRED4 ) && calendar::once_every( 3_hours ) ) ) ) {
            // Their brain is optimized to remember this
            if( one_in( 13 ) ) {
                // They've already passed the roll to avoid rust at
                // this point, but print a message about it now and
                // then.
                //
                // 13 combat skills.
                // This means PRED2/PRED3/PRED4 think of hunting on
                // average every 8/4/3 hours, enough for immersion
                // without becoming an annoyance.
                //
                add_msg_if_player( _( "Your heart races as you recall your most recent hunt." ) );
                mod_stim( 1 );
            }
            continue;
        }

        const int rust_resist = enchantment_cache->get_value_add( enchant_vals::mod::SKILL_RUST_RESIST );
        const float rust_resist_mult = 1.0f + enchantment_cache->get_value_multiply(
                                           enchant_vals::mod::SKILL_RUST_RESIST );
        if( skill_level_obj.rust( rust_resist, rust_resist_mult ) ) {
            mod_power_level( -bio_memory->power_trigger );
        }
    }
}

int Character::focus_equilibrium_sleepiness_cap( int equilibrium ) const
{
    if( get_sleepiness() >= sleepiness_levels::MASSIVE_SLEEPINESS && equilibrium > 20 ) {
        return 20;
    } else if( get_sleepiness() >= sleepiness_levels::EXHAUSTED && equilibrium > 40 ) {
        return 40;
    } else if( get_sleepiness() >= sleepiness_levels::DEAD_TIRED && equilibrium > 60 ) {
        return 60;
    } else if( get_sleepiness() >= sleepiness_levels::TIRED && equilibrium > 80 ) {
        return 80;
    }
    return equilibrium;
}

int Character::calc_focus_equilibrium( bool ignore_pain ) const
{
    int focus_equilibrium = 100;

    if( activity.id() == ACT_READ ) {
        const item_location book = activity.targets[0];
        if( book && book->is_book() ) {
            const cata::value_ptr<islot_book> &bt = book->type->book;
            // apply a penalty when we're actually learning something
            const SkillLevel &skill_level = get_skill_level_object( bt->skill );
            if( skill_level.can_train() && skill_level < bt->level ) {
                focus_equilibrium -= 50;
            }
        }
    }

    int eff_morale = get_morale_level();
    // Factor in perceived pain, since it's harder to rest your mind while your body hurts.
    // Cenobites don't mind, though
    if( !ignore_pain && !has_trait( trait_CENOBITE ) ) {
        int perceived_pain = get_perceived_pain();
        if( has_trait( trait_MASOCHIST ) ) {
            if( perceived_pain > 20 ) {
                eff_morale = eff_morale - ( perceived_pain - 20 );
            }
        } else {
            eff_morale = eff_morale - perceived_pain;
        }
    }

    if( eff_morale < -99 ) {
        // At very low morale, focus is at it's minimum
        focus_equilibrium = 1;
    } else if( eff_morale <= 50 ) {
        // At -99 to +50 morale, each point of morale gives or takes 1 point of focus
        focus_equilibrium += eff_morale;
    } else {
        /* Above 50 morale, we apply strong diminishing returns.
        * Each block of 50 takes twice as many morale points as the previous one:
        * 150 focus at 50 morale (as before)
        * 200 focus at 150 morale (100 more morale)
        * 250 focus at 350 morale (200 more morale)
        * ...
        * Cap out at 400% focus gain with 3,150+ morale, mostly as a sanity check.
        */

        int block_multiplier = 1;
        int morale_left = eff_morale;
        while( focus_equilibrium < 400 ) {
            if( morale_left > 50 * block_multiplier ) {
                // We can afford the entire block.  Get it and continue.
                morale_left -= 50 * block_multiplier;
                focus_equilibrium += 50;
                block_multiplier *= 2;
            } else {
                // We can't afford the entire block.  Each block_multiplier morale
                // points give 1 focus, and then we're done.
                focus_equilibrium += morale_left / block_multiplier;
                break;
            }
        }
    }

    // This should be redundant, but just in case...
    if( focus_equilibrium < 1 ) {
        focus_equilibrium = 1;
    } else if( focus_equilibrium > 400 ) {
        focus_equilibrium = 400;
    }
    return focus_equilibrium;
}

int Character::calc_focus_change() const
{
    return focus_equilibrium_sleepiness_cap( calc_focus_equilibrium() ) - get_focus();
}

void Character::update_mental_focus()
{
    // calc_focus_change() returns percentile focus, applying it directly
    // to focus pool is an implicit / 100.
    focus_pool += 10 * calc_focus_change();
}

void Character::reset_stats()
{
    if( calendar::once_every( 1_minutes ) ) {
        update_mental_focus();
    }

    mod_dodge_bonus( enchantment_cache->modify_value( enchant_vals::mod::DODGE_CHANCE, 0 ) );

    /** @EFFECT_STR_MAX above 15 decreases Dodge bonus by 1 (NEGATIVE) */
    if( str_max >= 16 ) {
        mod_dodge_bonus( -1 );   // Penalty if we're huge
    }
    /** @EFFECT_STR_MAX below 6 increases Dodge bonus by 1 */
    else if( str_max <= 5 ) {
        mod_dodge_bonus( 1 );   // Bonus if we're small
    }

    nv_cached = false;

    // Reset our stats to normal levels
    // Any persistent buffs/debuffs will take place in effects,
    // player::suffer(), etc.

    // repopulate the stat fields
    str_cur = str_max + get_str_bonus();
    dex_cur = dex_max + get_dex_bonus();
    per_cur = per_max + get_per_bonus();
    int_cur = int_max + get_int_bonus();

    // Floor for our stats.  No stat changes should occur after this!
    if( dex_cur < 0 ) {
        dex_cur = 0;
    }
    if( str_cur < 0 ) {
        str_cur = 0;
    }
    if( per_cur < 0 ) {
        per_cur = 0;
    }
    if( int_cur < 0 ) {
        int_cur = 0;
    }
}

void Character::reset()
{
    // TODO: Move reset_stats here, remove it from Creature
    reset_bonuses();
    // Apply bonuses from hardcoded effects
    mod_str_bonus( str_bonus_hardcoded );
    mod_dex_bonus( dex_bonus_hardcoded );
    mod_int_bonus( int_bonus_hardcoded );
    mod_per_bonus( per_bonus_hardcoded );
    reset_stats();
    recalc_speed_bonus();
}

bool Character::has_nv_goggles()
{
    static bool nv = false;

    if( !nv_cached ) {
        nv_cached = true;
        nv = worn_with_flag( flag_GNV_EFFECT ) || has_flag( json_flag_NIGHT_VISION ) ||
             worn_with_flag( json_flag_NVG_GREEN ) || has_worn_module_with_flag( json_flag_NVG_GREEN );
    }
    return nv;
}

bool Character::has_worn_module_with_flag( const flag_id &f )
{
    std::vector<item *> flag_items = cache_get_items_with( f );
    bool has_flag = std::any_of( flag_items.begin(), flag_items.end(),
    [this]( item * i ) {
        return is_worn_module( *i );
    } );
    return has_flag;
}

void Character::calc_discomfort()
{
    // clear all instances of discomfort
    remove_effect( effect_chafing );
    for( const bodypart_id &bp : worn.where_discomfort( *this ) ) {
        if( bp->feels_discomfort ) {
            add_effect( effect_chafing, 1_turns, bp, true, 1 );
        }
    }
}

void Character::calc_encumbrance()
{
    calc_encumbrance( item() );
}

void Character::calc_encumbrance( const item &new_item )
{
    std::map<bodypart_id, encumbrance_data> enc;
    worn.item_encumb( enc, new_item, *this );
    mut_cbm_encumb( enc );
    calc_bmi_encumb( enc );

    for( const std::pair<const bodypart_id, encumbrance_data> &elem : enc ) {
        set_part_encumbrance_data( elem.first, elem.second );
    }

}

units::mass Character::get_weight() const
{
    units::mass ret = 0_gram;
    units::mass wornWeight = worn.weight();

    ret += bodyweight();       // The base weight of the player's body
    ret += inv->weight();           // Weight of the stored inventory
    ret += wornWeight;             // Weight of worn items
    ret += weapon.weight();        // Weight of wielded item
    ret += bionics_weight();       // Weight of installed bionics
    return ret;
}

std::pair<int, int> Character::climate_control_strength() const
{
    // In warmth points
    const int DEFAULT_STRENGTH = 50;

    int power_heat = enchantment_cache->get_value_add( enchant_vals::mod::CLIMATE_CONTROL_HEAT );
    int power_chill = enchantment_cache->get_value_add( enchant_vals::mod::CLIMATE_CONTROL_CHILL );

    map &here = get_map();
    if( has_trait( trait_M_SKIN3 ) &&
        here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FUNGUS, pos_bub() ) &&
        in_sleep_state() ) {
        power_heat += DEFAULT_STRENGTH;
        power_chill += DEFAULT_STRENGTH;
    }

    bool regulated_area = false;
    if( calendar::turn >= next_climate_control_check ) {
        // save CPU and simulate acclimation.
        next_climate_control_check = calendar::turn + 20_turns;
        if( const optional_vpart_position vp = here.veh_at( pos_bub() ) ) {
            // TODO: (?) Force player to scrounge together an AC unit
            regulated_area = (
                                 (
                                     vp->is_inside() && // Already checks for opened doors
                                     vp->vehicle().engine_on &&
                                     vp->vehicle().has_engine_type_not( fuel_type_muscle, true ) &&
                                     vp->vehicle().has_engine_type_not( fuel_type_animal, true )
                                 ) ||
                                 // Also check for a working alternator. Muscle or animal could be powering it.
                                 (
                                     vp->is_inside() &&
                                     vp->vehicle().total_alternator_epower( here ) > 0_W
                                 )
                             );
        }
        // TODO: AC check for when building power is implemented
        last_climate_control_ret = regulated_area;
        if( !regulated_area ) {
            // Takes longer to cool down / warm up with AC, than it does to step outside and feel cruddy.
            next_climate_control_check += 40_turns;
        }
    } else {
        regulated_area = last_climate_control_ret;
    }

    if( regulated_area ) {
        power_heat += DEFAULT_STRENGTH;
        power_chill += DEFAULT_STRENGTH;
    }

    return { power_heat, power_chill };
}

std::map<bodypart_id, int> Character::get_wind_resistance( const std::map <bodypart_id,
        std::vector<const item *>> &clothing_map ) const
{

    std::map<bodypart_id, int> ret;
    for( const bodypart_id &bp : get_all_body_parts() ) {
        ret.emplace( bp, 0 );
    }
    bool in_shell = has_active_mutation( trait_SHELL2 ) ||
                    has_active_mutation( trait_SHELL3 );
    // Your shell provides complete wind protection if you're inside it
    if( in_shell ) { // NOLINT(bugprone-branch-clone)
        for( std::pair<const bodypart_id, int> &this_bp : ret ) {
            this_bp.second = 100;
        }
        return ret;
    }

    for( const std::pair<const bodypart_id, std::vector<const item *>> &on_bp : clothing_map ) {
        const bodypart_id &bp = on_bp.first;

        int coverage = 0;
        float totalExposed = 1.0f;
        int penalty = 100;

        for( const item *it : on_bp.second ) {
            const item &i = *it;
            penalty = 100 - i.wind_resist();
            coverage = std::max( 0, i.get_coverage( bp ) - penalty );
            totalExposed *= ( 1.0 - coverage / 100.0 ); // Coverage is between 0 and 1?
        }

        ret[bp] = 100 - totalExposed * 100;
    }

    return ret;
}

void layer_details::reset()
{
    *this = layer_details();
}

// The stacking penalty applies by doubling the encumbrance of
// each item except the highest encumbrance one.
// So we add them together and then subtract out the highest.
int layer_details::layer( const int encumbrance, bool conflicts )
{
    /*
     * We should only get to this point with an encumbrance value of 0
     * if the item is 'semitangible'. A normal item has a minimum of
     * 2 encumbrance for layer penalty purposes.
     * ( even if normally its encumbrance is 0 )
     */
    if( encumbrance == 0 ) {
        return total; // skip over the other logic because this item doesn't count
    }

    // if this layer has conflicting pieces then add that information
    if( conflicts ) {
        is_conflicting = true;
    }

    pieces.push_back( encumbrance );

    int current = total;
    if( encumbrance > max ) {
        total += max;   // *now* the old max is counted, just ignore the new max
        max = encumbrance;
    } else {
        total += encumbrance;
    }
    return total - current;
}

int Character::avg_encumb_of_limb_type( body_part_type::type part_type ) const
{
    float limb_encumb = 0.0f;
    int num_limbs = 0;
    for( const bodypart_id &part : get_all_body_parts_of_type( part_type,
            get_body_part_flags::primary_type ) ) {
        limb_encumb += encumb( part );
        num_limbs++;
    }
    if( num_limbs == 0 ) {
        return 0;
    }
    limb_encumb /= num_limbs;
    return std::round( limb_encumb );
}

int Character::encumb( const bodypart_id &bp ) const
{
    if( !has_part( bp, body_part_filter::equivalent ) ) {
        debugmsg( "INFO: Tried to check encumbrance of a bodypart that does not exist." );
        return 0;
    }
    return get_part_encumbrance( bp );
}

void Character::apply_mut_encumbrance( std::map<bodypart_id, encumbrance_data> &vals ) const
{
    const std::vector<trait_id> all_muts = get_functioning_mutations();
    std::map<bodypart_str_id, float> total_enc;

    // Lower penalty for bps covered only by XL or unrestricted armor
    // Initialized on demand for performance reasons:
    // (calculation is costly, most of players and npcs are don't have encumbering mutations)

    for( const trait_id &mut : all_muts ) {
        for( const std::pair<const bodypart_str_id, int> &enc : mut->encumbrance_always ) {
            total_enc[enc.first] += enc.second;
        }
        for( const std::pair<const bodypart_str_id, int> &enc : mut->encumbrance_covered ) {
            if( wearing_fitting_on( enc.first ) ) {
                total_enc[enc.first] += enc.second;
            }
        }
    }

    for( const trait_id &mut : all_muts ) {
        for( const std::pair<const bodypart_str_id, float> &enc : mut->encumbrance_multiplier_always ) {
            if( total_enc[enc.first] > 0 ) {
                total_enc[enc.first] *= enc.second;
            }
        }
    }

    for( const std::pair<const bodypart_str_id, float> &enc : total_enc ) {
        vals[enc.first.id()].encumbrance += enc.second;
    }
}

void Character::mut_cbm_encumb( std::map<bodypart_id, encumbrance_data> &vals ) const
{
    for( const bionic_id &bid : get_bionics() ) {
        for( const std::pair<const bodypart_str_id, int> &element : bid->encumbrance ) {
            vals[element.first.id()].encumbrance += element.second;
        }
    }

    if( has_active_bionic( bio_shock_absorber ) ) {
        for( std::pair<const bodypart_id, encumbrance_data> &val : vals ) {
            val.second.encumbrance += 3; // Slight encumbrance to all parts except eyes
        }
        vals[body_part_eyes].encumbrance -= 3;
    }

    apply_mut_encumbrance( vals );
}

void Character::calc_bmi_encumb( std::map<bodypart_id, encumbrance_data> &vals ) const
{
    for( const std::pair<const bodypart_str_id, bodypart> &elem : get_body() ) {
        int penalty = std::floor( elem.second.get_bmi_encumbrance_scalar() * std::max( 0.0f,
                                  get_bmi_fat() - static_cast<float>( elem.second.get_bmi_encumbrance_threshold() ) ) );
        vals[elem.first.id()].encumbrance += penalty;
    }
}

body_part_set Character::exclusive_flag_coverage( const flag_id &flag ) const
{
    body_part_set ret;
    ret.fill( get_all_body_parts() );
    return worn.exclusive_flag_coverage( ret, flag );
}

/*
 * Innate stats getters
 */

// get_stat() always gets total (current) value, NEVER just the base
// get_stat_bonus() is always just the bonus amount
int Character::get_str() const
{
    return std::max( 0, get_str_base() + str_bonus );
}
int Character::get_dex() const
{
    return std::max( 0, get_dex_base() + dex_bonus );
}
int Character::get_per() const
{
    return std::max( 0, get_per_base() + per_bonus );
}
int Character::get_int() const
{
    return std::max( 0, get_int_base() + int_bonus );
}

int Character::get_str_base() const
{
    return str_max;
}
int Character::get_dex_base() const
{
    return dex_max;
}
int Character::get_per_base() const
{
    return per_max;
}
int Character::get_int_base() const
{
    return int_max;
}

int Character::get_str_bonus() const
{
    return str_bonus;
}
int Character::get_dex_bonus() const
{
    return dex_bonus;
}
int Character::get_per_bonus() const
{
    return per_bonus;
}
int Character::get_int_bonus() const
{
    return int_bonus;
}

static int get_speedydex_bonus( const int dex )
{
    static const std::string speedydex_min_dex( "SPEEDYDEX_MIN_DEX" );
    static const std::string speedydex_dex_speed( "SPEEDYDEX_DEX_SPEED" );
    // this is the number to be multiplied by the increment
    const int modified_dex = std::max( dex - get_option<int>( speedydex_min_dex ), 0 );
    return modified_dex * get_option<int>( speedydex_dex_speed );
}

int Character::get_enchantment_speed_bonus() const
{
    return enchantment_speed_bonus;
}

int Character::get_speed() const
{
    if( has_flag( json_flag_STEADY ) ) {
        return get_speed_base() + std::max( 0, get_speed_bonus() );
    }
    return Creature::get_speed();
}

int Character::get_arm_str() const
{
    return str_cur * get_modifier( character_modifier_limb_str_mod );
}

int Character::get_eff_per() const
{
    return ( Character::get_per() + int( Character::has_proficiency(
            proficiency_prof_spotting ) ) *
             Character::get_per_base() ) * get_limb_score( limb_score_vision ) ;
}

int Character::ranged_dex_mod() const
{
    ///\EFFECT_DEX <20 increases ranged penalty
    return std::max( ( 20.0 - get_dex() ) * 0.5, 0.0 );
}

int Character::ranged_per_mod() const
{
    ///\EFFECT_PER <20 increases ranged aiming penalty.
    return std::max( ( 20.0 - get_per() ) * 1.2, 0.0 );
}

int Character::get_lifestyle() const
{
    // gets your health_tally variable + the factor of your bmi on your healthiness.

    // being over or underweight makes your "effective healthiness" lower.
    // for example, you have a lifestyle of 0 and a bmi_fat of 15 (border between obese and very obese)
    // 0 - 25 - 0 = -25. So your "effective lifestyle" is -25 (feel a little cruddy)
    // ex2 you have a lifestyle of 50 and a bmi_fat of 15 because you get exercise and eat well
    // 50 - 25 - 0 = 25. So your "effective lifestyle" is 25 (feel decent despite being obese)
    // ex3 you have a lifestyle of -50 and a bmi_fat of 15 because you eat candy and don't exercise
    // -50 - 25 - 0 = -75. So your "effective lifestyle" is -75 (you'd feel crappy normally but because of your weight you feel worse)

    const float bmi = get_bmi_fat();
    int over_factor = std::round( std::max( 0.0f,
                                            5 * ( bmi - character_weight_category::obese ) ) );
    int under_factor = std::round( std::max( 0.0f,
                                   50 * ( character_weight_category::normal - bmi ) ) );
    return std::max( lifestyle - ( over_factor + under_factor ), -200 );
}

int Character::get_daily_health() const
{
    return daily_health;
}

int Character::get_health_tally() const
{
    return health_tally;
}

/*
 * Innate stats setters
 */

void Character::set_str_bonus( int nstr )
{
    str_bonus = nstr;
    str_cur = std::max( 0, str_max + str_bonus );
}
void Character::set_dex_bonus( int ndex )
{
    dex_bonus = ndex;
    dex_cur = std::max( 0, dex_max + dex_bonus );
}
void Character::set_per_bonus( int nper )
{
    per_bonus = nper;
    per_cur = std::max( 0, per_max + per_bonus );
}
void Character::set_int_bonus( int nint )
{
    int_bonus = nint;
    int_cur = std::max( 0, int_max + int_bonus );
}
void Character::mod_str_bonus( int nstr )
{
    str_bonus += nstr;
    str_cur = std::max( 0, str_max + str_bonus );
}
void Character::mod_dex_bonus( int ndex )
{
    dex_bonus += ndex;
    dex_cur = std::max( 0, dex_max + dex_bonus );
}
void Character::mod_per_bonus( int nper )
{
    per_bonus += nper;
    per_cur = std::max( 0, per_max + per_bonus );
}
void Character::mod_int_bonus( int nint )
{
    int_bonus += nint;
    int_cur = std::max( 0, int_max + int_bonus );
}

namespace io
{
template<>
std::string enum_to_string<character_stat>( character_stat data )
{
    switch( data ) {
        // *INDENT-OFF*
    case character_stat::STRENGTH:     return "STR";
    case character_stat::DEXTERITY:    return "DEX";
    case character_stat::INTELLIGENCE: return "INT";
    case character_stat::PERCEPTION:   return "PER";

        // *INDENT-ON*
        case character_stat::DUMMY_STAT:
            break;
    }
    cata_fatal( "Invalid character_stat" );
}
} // namespace io

void Character::set_lifestyle( int nhealthy )
{
    lifestyle = nhealthy;
}
void Character::mod_livestyle( int nhealthy )
{
    lifestyle += enchantment_cache->modify_value( enchant_vals::mod::HEALTHY_RATE, nhealthy );
    // Clamp lifestyle between [-200, 200]
    lifestyle = std::max( lifestyle, -200 );
    lifestyle = std::min( lifestyle, 200 );
}
void Character::set_daily_health( int nhealthy_mod )
{
    daily_health = nhealthy_mod;
}
void Character::mod_daily_health( int nhealthy_mod, int cap )
{
    // TODO: This really should be a full morale-like system, with per-effect caps
    //       and durations.  This version prevents any single effect from exceeding its
    //       intended ceiling, but multiple effects will overlap instead of adding.

    // Cap indicates how far the mod is allowed to shift in this direction.
    // It can have a different sign to the mod, e.g. for items that treat
    // extremely low health, but can't make you healthy.
    if( nhealthy_mod == 0 || cap == 0 ) {
        return;
    }
    int low_cap;
    int high_cap;
    if( nhealthy_mod < 0 ) {
        low_cap = cap;
        high_cap = 200;
    } else {
        low_cap = -200;
        high_cap = cap;
    }

    // If we're already out-of-bounds, we don't need to do anything.
    if( ( daily_health <= low_cap && nhealthy_mod < 0 ) ||
        ( daily_health >= high_cap && nhealthy_mod > 0 ) ) {
        return;
    }

    daily_health += nhealthy_mod;

    // Since we already bailed out if we were out-of-bounds, we can
    // just clamp to the boundaries here.
    daily_health = std::min( daily_health, high_cap );
    daily_health = std::max( daily_health, low_cap );
}

void Character::mod_health_tally( int mod )
{
    health_tally += mod;
}

int Character::get_stored_kcal() const
{
    return stored_calories / 1000;
}

int Character::kcal_speed_penalty() const
{
    static const std::vector<std::pair<float, float>> starv_thresholds = { {
            std::make_pair( 0.0f, -90.0f ),
            std::make_pair( character_weight_category::emaciated, -50.f ),
            std::make_pair( character_weight_category::underweight, -25.0f ),
            std::make_pair( character_weight_category::normal, 0.0f )
        }
    };
    if( get_kcal_percent() > 0.95f ) {
        return 0;
    } else {
        return std::round( multi_lerp( starv_thresholds, get_bmi_fat() ) );
    }
}

std::string Character::activity_level_str( float level ) const
{
    for( const std::pair<const float, std::string> &member : activity_levels_str_map ) {
        if( level <= member.first ) {
            return member.second;
        }
    }
    return ( --activity_levels_str_map.end() )->second;
}

std::string Character::debug_weary_info() const
{
    int amt = activity_history.weariness();
    std::string max_act = activity_level_str( maximum_exertion_level() );
    float move_mult = exertion_adjusted_move_multiplier( EXTRA_EXERCISE );

    int bmr = base_bmr();
    std::string weary_internals = activity_history.debug_weary_info();
    int cardio_mult = get_cardiofit();
    int thresh = weary_threshold();
    int current = weariness_level();
    int morale = get_morale_level();
    int weight = units::to_gram<int>( bodyweight() );
    float bmi = get_bmi();

    return string_format( "Weariness: %s Max Full Exert: %s Mult: %g\n BMR: %d CARDIO FITNESS: %d\n %s Thresh: %d At: %d\n Kcal: %d/%d Sleepiness: %d Morale: %d Wgt: %d (BMI %.1f)",
                          amt, max_act, move_mult, bmr, cardio_mult, weary_internals, thresh, current,
                          get_stored_kcal(), get_healthy_kcal(), sleepiness, morale, weight, bmi );
}

void Character::mod_stored_kcal( int nkcal, const bool ignore_weariness )
{
    if( needs_food() ) {
        mod_stored_calories( nkcal * 1000, ignore_weariness );
    }
}

void Character::mod_stored_calories( int ncal, const bool ignore_weariness )
{
    int nkcal = ncal / 1000;
    if( nkcal > 0 ) {
        add_gained_calories( nkcal );
    } else {
        add_spent_calories( -nkcal );
    }

    if( !ignore_weariness ) {
        activity_history.calorie_adjust( ncal );
    }
    if( ncal < 0 || std::numeric_limits<int>::max() - ncal > stored_calories ) {
        set_stored_calories( stored_calories + ncal );
    }
}

void Character::set_stored_kcal( int kcal )
{
    set_stored_calories( kcal * 1000 );
}

void Character::set_stored_calories( int cal )
{
    if( stored_calories != cal ) {
        int cached_bmi = std::floor( get_bmi_fat() );
        stored_calories = cal;

        //some mutant change their max_hp according to their bmi
        recalc_hp();
        //need to check obesity penalties when this happens if BMI changed
        if( std::floor( get_bmi_fat() ) != cached_bmi ) {
            calc_encumbrance();
        }
    }
}

int Character::get_healthy_kcal() const
{
    float healthy_weight = get_cached_organic_size() * 5.0f * std::pow( height() / 100.0f, 2 );
    return std::floor( KCAL_PER_KG * healthy_weight );
}

float Character::get_kcal_percent() const
{
    return static_cast<float>( get_stored_kcal() ) / static_cast<float>( get_healthy_kcal() );
}

int Character::get_hunger() const
{
    return hunger;
}

void Character::mod_hunger( int nhunger )
{
    if( needs_food() ) {
        set_hunger( hunger + nhunger );
    }
}

void Character::set_hunger( int nhunger )
{
    if( hunger != nhunger ) {
        // cap hunger at 300, just below famished
        hunger = std::min( 300, nhunger );
        on_stat_change( "hunger", hunger );
    }
}

// this is a translation from a legacy value
int Character::get_starvation() const
{
    static const std::vector<std::pair<float, float>> starv_thresholds = { {
            std::make_pair( 0.0f, 6000.0f ),
            std::make_pair( 0.8f, 300.0f ),
            std::make_pair( 0.95f, 100.0f )
        }
    };
    if( get_kcal_percent() < 0.95f ) {
        return std::round( multi_lerp( starv_thresholds, get_kcal_percent() ) );
    }
    return 0;
}

int Character::get_thirst() const
{
    return thirst;
}

int Character::get_instant_thirst() const
{
    return thirst - std::max( units::to_milliliter<int>
                              ( stomach.get_water() / 10 ), 0 );
}

void Character::mod_thirst( int nthirst )
{
    if( has_flag( json_flag_NO_THIRST ) || !needs_food() ) {
        return;
    }
    set_thirst( std::max( -100, thirst + nthirst ) );
}

void Character::set_thirst( int nthirst )
{
    if( thirst != nthirst ) {
        thirst = nthirst;
        on_stat_change( "thirst", thirst );
    }
}

void Character::mod_sleepiness( int nsleepiness )
{
    set_sleepiness( sleepiness + nsleepiness );
}

void Character::mod_sleep_deprivation( int nsleep_deprivation )
{
    set_sleep_deprivation( sleep_deprivation + nsleep_deprivation );
}

void Character::set_sleepiness( int nsleepiness )
{
    nsleepiness = std::max( nsleepiness, -1000 );
    if( sleepiness != nsleepiness ) {
        sleepiness = nsleepiness;
        on_stat_change( "sleepiness", sleepiness );
    }
}

void Character::set_sleepiness( sleepiness_levels nsleepiness )
{
    set_sleepiness( static_cast<int>( nsleepiness ) );
}

void Character::set_sleep_deprivation( int nsleep_deprivation )
{
    sleep_deprivation = std::min( static_cast< int >( SLEEP_DEPRIVATION_MASSIVE ), std::max( 0,
                                  nsleep_deprivation ) );
}

time_duration Character::get_daily_sleep() const
{
    return daily_sleep;
}

void Character::mod_daily_sleep( time_duration mod )
{
    daily_sleep += mod;
}

void Character::reset_daily_sleep()
{
    daily_sleep = 0_turns;
}

time_duration Character::get_continuous_sleep() const
{
    return continuous_sleep;
}

void Character::mod_continuous_sleep( time_duration mod )
{
    continuous_sleep += mod;
}

void Character::reset_continuous_sleep()
{
    continuous_sleep = 0_turns;
}

int Character::get_sleepiness() const
{
    return sleepiness;
}

int Character::get_sleep_deprivation() const
{
    return sleep_deprivation;
}

bool Character::is_deaf() const
{
    return get_effect_int( effect_deaf ) > 2 || worn_with_flag( flag_DEAF ) ||
           has_flag( json_flag_DEAF ) || has_effect( effect_narcosis ) ||
           ( has_trait( trait_M_SKIN3 ) &&
             get_map().has_flag_ter_or_furn( ter_furn_flag::TFLAG_FUNGUS, pos_bub() )
             && in_sleep_state() );
}

bool Character::is_mute() const
{
    return has_flag( flag_MUTE ) || worn_with_flag( flag_MUTE ) ||
           ( has_trait( trait_PROF_FOODP ) && !( is_wearing( itype_foodperson_mask ) ||
                   is_wearing( itype_foodperson_mask_on ) ) );
}

void Character::on_damage_of_type( const effect_source &source, int adjusted_damage,
                                   const damage_type_id &dt, const bodypart_id &bp )
{
    // Handle bp onhit effects
    bodypart *body_part = get_part( bp );
    const bool is_minor = bp->main_part != bp->id;
    add_msg_debug( debugmode::DF_CHARACTER, "Targeting limb %s with %d %s type damage", bp->name,
                   adjusted_damage, dt->name.translated() );
    // Damage as a percent of the limb's max hp or raw dmg for minor parts
    float dmg_ratio = is_minor ? adjusted_damage : 100 * adjusted_damage / body_part->get_hp_max();
    add_msg_debug( debugmode::DF_CHARACTER, "Main BP max hp %d, damage ratio %.1f",
                   body_part->get_hp_max(), dmg_ratio );
    for( const bp_onhit_effect &eff : body_part->get_onhit_effects( dt ) ) {
        if( dmg_ratio >= eff.dmg_threshold ) {
            float scaling = ( dmg_ratio - eff.dmg_threshold ) / eff.scale_increment;
            int scaled_chance = roll_remainder( std::max( static_cast<float>( eff.chance ),
                                                eff.chance + eff.chance_dmg_scaling * scaling ) );
            add_msg_debug( debugmode::DF_CHARACTER, "Scaling factor %.1f, base chance %d%%, scaled chance %d%%",
                           scaling, eff.chance, scaled_chance );
            if( x_in_y( scaled_chance, 100 ) ) {

                // Scale based on damage done
                int scaled_dur = roll_remainder( std::max( static_cast<float>( eff.duration ),
                                                 eff.duration + eff.duration_dmg_scaling * scaling ) );
                int scaled_int = roll_remainder( std::max( static_cast<float>( eff.intensity ),
                                                 eff.intensity + eff.intensity_dmg_scaling * scaling ) );

                // go through all effects. if they modify effect duration, scale dur accordingly.
                // obviously inefficient, but probably fast enough since this only is calculated once per attack.
                float eff_scale = 1.0f;
                for( const effect &e : get_effects() ) {
                    for( const effect_dur_mod &eff_dur_mod : e.get_effect_dur_scaling() ) {
                        // debugmsg( "Checking effect %s, with same_bp, modifier %f", eff.id.c_str(), eff_dur_mod.modifier);
                        if( eff_dur_mod.effect_id == eff.id ) {
                            if( eff_dur_mod.same_bp ) {
                                if( e.get_bp() != body_part->get_id() ) {
                                    continue;
                                }
                            }
                            eff_scale *= eff_dur_mod.modifier;
                        }
                    }
                }

                scaled_dur = static_cast<int>( scaled_dur * eff_scale );

                add_msg_debug( debugmode::DF_CHARACTER,
                               "Atempting to add effect %s, scaled duration %d turns, scaled intensity %d, effect scaling %f",
                               eff.id.c_str(),
                               scaled_dur,
                               scaled_int,
                               eff_scale );

                // Handle intensity/duration clamping
                if( eff.max_intensity != INT_MAX ) {
                    int prev_int = get_effect( eff.id, bp ).get_intensity();
                    add_msg_debug( debugmode::DF_CHARACTER,
                                   "Max intensity %d, current intensity %d", eff.max_intensity, prev_int );
                    if( ( prev_int + scaled_int ) > eff.max_intensity ) {
                        scaled_int = eff.max_intensity - prev_int;
                        add_msg_debug( debugmode::DF_CHARACTER,
                                       "Limiting added intensity to %d", scaled_int );
                    }
                }
                if( eff.max_duration != INT_MAX ) {
                    int prev_dur = to_turns<int>( get_effect( eff.id, bp ).get_duration() );
                    add_msg_debug( debugmode::DF_CHARACTER,
                                   "Max duration %d, current duration %d", eff.max_duration, prev_dur );
                    if( ( prev_dur + scaled_dur ) > eff.max_duration ) {
                        scaled_dur = eff.max_duration - prev_dur;
                        add_msg_debug( debugmode::DF_CHARACTER,
                                       "Limiting added duration to %d seconds", scaled_dur );
                    }
                }
                // Add the effect globally if defined
                add_effect( source, eff.id, 1_turns * scaled_dur, eff.global ? bodypart_str_id::NULL_ID() : bp,
                            false, scaled_int );
            }
        }
    }
    // Electrical damage has a chance to temporarily incapacitate bionics in the damaged body_part.
    if( dt == damage_electric ) {
        const time_duration min_disable_time = 10_turns * adjusted_damage;
        for( bionic &i : *my_bionics ) {
            if( !i.powered ) {
                // Unpowered bionics are protected from power surges.
                continue;
            }
            const bionic_data &info = i.info();
            if( info.has_flag( STATIC( json_character_flag( "BIONIC_SHOCKPROOF" ) ) ) ||
                info.has_flag( STATIC( json_character_flag( "BIONIC_FAULTY" ) ) ) ) {
                continue;
            }
            const std::map<bodypart_str_id, size_t> &bodyparts = info.occupied_bodyparts;
            if( bodyparts.find( bp.id() ) != bodyparts.end() ) {
                const int bp_hp = get_part_hp_cur( bp );
                // The chance to incapacitate is as high as 50% if the attack deals damage equal to one third of the body part's current health.
                if( x_in_y( adjusted_damage * 3, bp_hp ) && one_in( 2 ) ) {
                    if( i.incapacitated_time == 0_turns ) {
                        add_msg_if_player( m_bad, _( "Your %s bionic shorts out!" ), info.name );
                    }
                    i.incapacitated_time += rng( min_disable_time, 10 * min_disable_time );
                }
            }
        }
    }
}

void Character::reset_bonuses()
{
    // Reset all bonuses to 0 and multipliers to 1.0
    str_bonus = 0;
    dex_bonus = 0;
    per_bonus = 0;
    int_bonus = 0;

    Creature::reset_bonuses();
}

int Character::get_max_healthy() const
{
    const float bmi = get_bmi_fat();
    int over_factor = std::round( std::max( 0.0f,
                                            25 * ( bmi - character_weight_category::overweight ) ) );
    int under_factor = std::round( std::max( 0.0f,
                                   200 * ( character_weight_category::normal - bmi ) ) );
    return std::max( 200 - over_factor - under_factor, -200 );
}

void Character::regen( int rate_multiplier )
{
    int pain_ticks = std::max( 1, roll_remainder( enchantment_cache->modify_value(
                                   enchant_vals::mod::PAIN_REMOVE, rate_multiplier ) ) );
    while( get_pain() > 0 && pain_ticks-- > 0 ) {
        mod_pain( -roll_remainder( 0.2f + get_pain() / 50.0f ) );
    }

    float rest = rest_quality();
    float heal_rate = healing_rate( rest ) * to_turns<int>( 5_minutes );
    if( heal_rate > 0.0f ) {
        healall( roll_remainder( rate_multiplier * heal_rate ) );
    } else if( heal_rate < 0.0f ) {
        int rot_rate = roll_remainder( rate_multiplier * -heal_rate );
        // Has to be in loop because some effects depend on rounding
        while( rot_rate-- > 0 ) {
            hurtall( 1, nullptr, false );
        }
    }

    // include healing effects
    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        float healing = healing_rate_medicine( rest, bp ) * to_turns<int>( 5_minutes );
        int healing_apply = roll_remainder( healing );
        mod_part_healed_total( bp, healing_apply );
        heal( bp, healing_apply );
        if( get_part_damage_bandaged( bp ) > 0 ) {
            mod_part_damage_bandaged( bp, -healing_apply );
            if( get_part_damage_bandaged( bp ) <= 0 ) {
                set_part_damage_bandaged( bp, 0 );
                remove_effect( effect_bandaged, bp );
                add_msg_if_player( _( "Bandaged wounds on your %s healed." ), body_part_name( bp ) );
            }
        }
        if( get_part_damage_disinfected( bp ) > 0 ) {
            mod_part_damage_disinfected( bp, -healing_apply );
            if( get_part_damage_disinfected( bp ) <= 0 ) {
                set_part_damage_disinfected( bp, 0 );
                remove_effect( effect_disinfected, bp );
                add_msg_if_player( _( "Disinfected wounds on your %s healed." ), body_part_name( bp ) );
            }
        }

        // remove effects if the limb was healed by other way
        if( has_effect( effect_bandaged, bp.id() ) && is_part_at_max_hp( bp ) ) {
            set_part_damage_bandaged( bp, 0 );
            remove_effect( effect_bandaged, bp );
            add_msg_if_player( _( "Bandaged wounds on your %s healed." ), body_part_name( bp ) );
        }
        if( has_effect( effect_disinfected, bp.id() ) && is_part_at_max_hp( bp ) ) {
            set_part_damage_disinfected( bp, 0 );
            remove_effect( effect_disinfected, bp );
            add_msg_if_player( _( "Disinfected wounds on your %s healed." ), body_part_name( bp ) );
        }
    }

    if( get_rad() > 0 ) {
        mod_rad( -roll_remainder( rate_multiplier / 50.0f ) );
    }
}

void Character::update_health()
{
    // Limit daily_health to [-200, 200].
    // This also sets approximate bounds for the character's health.
    if( get_daily_health() > 200 ) {
        set_daily_health( 200 );
    } else if( get_daily_health() < -200 ) {
        set_daily_health( -200 );
    }

    // Active leukocyte breeder will keep your mean health_tally near 100
    int effective_daily_health = enchantment_cache->modify_value(
                                     enchant_vals::mod::EFFECTIVE_HEALTH_MOD, 0 );
    if( effective_daily_health == 0 ) {
        effective_daily_health = get_daily_health();
    }

    if( calendar::once_every( 1_days ) ) {
        mod_health_tally( effective_daily_health );
        int mean_daily_health = get_health_tally() / 7;
        mod_livestyle( mean_daily_health );
        mod_health_tally( -mean_daily_health );
        set_daily_health( 0 );
    }
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "Lifestyle: %d, Daily health: %d", get_lifestyle(),
                   get_daily_health() );
}

int Character::weariness() const
{
    return activity_history.weariness();
}

int Character::weary_threshold() const
{
    const int bmr = base_bmr();
    int threshold = bmr * get_option<float>( "WEARY_BMR_MULT" );
    // reduce by 1% per 14 points of sleepiness after 150 points
    threshold *= 1.0f - ( ( std::max( sleepiness, -20 ) - 150 ) / 1400.0f );
    // Each 2 points of morale increase or decrease by 1%
    threshold *= 1.0f + ( get_morale_level() / 200.0f );
    // TODO: Hunger effects this

    return std::max( threshold, bmr / 10 );
}

std::pair<int, int> Character::weariness_transition_progress() const
{
    // Mostly a duplicate of the below function. No real way to clean this up
    int amount = weariness();
    int threshold = weary_threshold();
    amount -= threshold * get_option<float>( "WEARY_INITIAL_STEP" );
    // failsafe if threshold is zero; see #72242
    if( threshold == 0 ) {
        return { std::abs( amount ), threshold };
    } else {
        while( amount >= 0 ) {
            amount -= threshold;
            if( threshold > 20 ) {
                threshold *= get_option<float>( "WEARY_THRESH_SCALING" );
            }
        }
    }

    // If we return the absolute value of the amount, it will work better
    // Because as it decreases, we will approach a transition
    return {std::abs( amount ), threshold};
}

int Character::weariness_level() const
{
    int amount = weariness();
    int threshold = weary_threshold();
    int level = 0;
    amount -= threshold * get_option<float>( "WEARY_INITIAL_STEP" );
    // failsafe if threshold is zero; see #72242
    if( threshold == 0 ) {
        return level;
    } else {
        while( amount >= 0 ) {
            amount -= threshold;
            if( threshold > 20 ) {
                threshold *= get_option<float>( "WEARY_THRESH_SCALING" );
            }
            ++level;
        }
    }

    return level;
}


int Character::weariness_transition_level() const
{
    int amount = weariness();
    int threshold = weary_threshold();
    amount -= threshold * get_option<float>( "WEARY_INITIAL_STEP" );
    // failsafe if threshold is zero; see #72242
    if( threshold == 0 ) {
        return std::abs( amount );
    } else {
        while( amount >= 0 ) {
            amount -= threshold;
            if( threshold > 20 ) {
                threshold *= get_option<float>( "WEARY_THRESH_SCALING" );
            }
        }
    }

    return std::abs( amount );
}

float Character::maximum_exertion_level() const
{
    switch( weariness_level() ) {
        case 0:
            return EXTRA_EXERCISE;
        case 1:
            return ACTIVE_EXERCISE;
        case 2:
            return BRISK_EXERCISE;
        case 3:
            return MODERATE_EXERCISE;
        case 4:
            return LIGHT_EXERCISE;
        case 5:
        default:
            return NO_EXERCISE;
    }
}

float Character::exertion_adjusted_move_multiplier( float level ) const
{
    // The default value for level is -1.0
    // And any values we get that are negative or 0
    // will cause incorrect behavior
    if( level <= 0 ) {
        level = activity_history.activity( in_sleep_state() );
    }
    const float max = maximum_exertion_level();
    if( level < max ) {
        return 1.0f;
    }
    return max / level;
}

float Character::instantaneous_activity_level() const
{
    return activity_history.instantaneous_activity_level();
}

int Character::activity_level_index() const
{
    // Activity levels are 1, 2, 4, 6, 8, 10
    // So we can easily cut them in half and round down for an index
    return std::floor( instantaneous_activity_level() / 2 );
}

float Character::activity_level() const
{
    float max = maximum_exertion_level();
    float attempted_level = activity_history.activity( in_sleep_state() );
    return std::min( max, attempted_level );
}

bool Character::needs_food() const
{
    return !( is_npc() && get_option<bool>( "NO_NPC_FOOD" ) );
}

void Character::update_needs( int rate_multiplier )
{
    const int current_stim = get_stim();
    // Hunger, thirst, & sleepiness up every 5 minutes
    effect &sleep = get_effect( effect_sleep );
    // No food/thirst/sleepiness clock at all
    const bool debug_ls = has_trait( trait_DEBUG_LS );
    // No food/thirst, capped sleepiness clock (only up to tired)
    const bool asleep = !sleep.is_null();
    const bool lying = asleep || has_effect( effect_lying_down ) ||
                       activity.id() == ACT_TRY_SLEEP;

    needs_rates rates = calc_needs_rates();

    const bool wasnt_sleepinessd = get_sleepiness() <= sleepiness_levels::DEAD_TIRED;
    // Don't increase sleepiness if sleeping or trying to sleep or if we're at the cap.
    if( get_sleepiness() < 1050 && !asleep && !debug_ls ) {
        if( rates.sleepiness > 0.0f ) {
            int sleepiness_roll = roll_remainder( rates.sleepiness * rate_multiplier );
            mod_sleepiness( sleepiness_roll );

            // Synaptic regen bionic stops SD while awake and boosts it while sleeping
            if( !has_flag( json_flag_STOP_SLEEP_DEPRIVATION ) ) {
                // sleepiness_roll should be around 1 - so the counter increases by 1 every minute on average,
                // but characters who need less sleep will also get less sleep deprived, and vice-versa.

                // Note: Since needs are updated in 5-minute increments, we have to multiply the roll again by
                // 5. If rate_multiplier is > 1, sleepiness_roll will be higher and this will work out.
                mod_sleep_deprivation( sleepiness_roll * 5 );
            }

            if( !needs_food() && get_sleepiness() > sleepiness_levels::TIRED ) {
                set_sleepiness( sleepiness_levels::TIRED );
                set_sleep_deprivation( 0 );
            }
        }
    } else if( asleep ) {
        if( rates.recovery > 0.0f ) {
            int recovered = roll_remainder( rates.recovery * rate_multiplier );
            if( get_sleepiness() - recovered < -20 ) {
                // Should be wake up, but that could prevent some retroactive regeneration
                sleep.set_duration( 1_turns );
                mod_sleepiness( -25 );
            } else {
                if( has_effect( effect_disrupted_sleep ) || has_effect( effect_recently_coughed ) ) {
                    recovered *= .5;
                }
                mod_sleepiness( -recovered );

                // Sleeping on the ground, no bionic = 1x rest_modifier
                // Sleeping on a bed, no bionic      = 2x rest_modifier
                // Sleeping on a comfy bed, no bionic= 3x rest_modifier

                // Sleeping on the ground, bionic    = 3x rest_modifier
                // Sleeping on a bed, bionic         = 6x rest_modifier
                // Sleeping on a comfy bed, bionic   = 9x rest_modifier
                float rest_modifier = ( has_flag( json_flag_STOP_SLEEP_DEPRIVATION ) ? 3 : 1 );
                // Melatonin supplements also add a flat bonus to recovery speed
                if( has_effect( effect_melatonin ) ) {
                    rest_modifier += 1;
                }

                const int comfort = get_comfort_at( pos_bub() ).comfort;
                if( comfort >= comfort_data::COMFORT_VERY_COMFORTABLE ) {
                    rest_modifier *= 3;
                } else if( comfort >= comfort_data::COMFORT_COMFORTABLE ) {
                    rest_modifier *= 2.5;
                } else if( comfort >= comfort_data::COMFORT_SLIGHTLY_COMFORTABLE ) {
                    rest_modifier *= 2;
                }

                // If we're just tired, we'll get a decent boost to our sleep quality.
                // The opposite is true for very tired characters.
                if( get_sleepiness() < sleepiness_levels::DEAD_TIRED ) {
                    rest_modifier += 2;
                } else if( get_sleepiness() >= sleepiness_levels::EXHAUSTED ) {
                    rest_modifier = ( rest_modifier > 2 ) ? rest_modifier - 2 : 1;
                }

                // Recovered is multiplied by 2 as well, since we spend 1/3 of the day sleeping
                mod_sleep_deprivation( -rest_modifier * ( recovered * 2 ) );

            }
        }
        if( calendar::once_every( 10_minutes ) &&
            ( has_trait( trait_WATERSLEEP ) || has_trait( trait_UNDINE_SLEEP_WATER ) ) ) {
            mod_sleepiness( -3 ); // Fish sleep less in water
        }
    }
    if( is_avatar() && wasnt_sleepinessd && get_sleepiness() > sleepiness_levels::DEAD_TIRED &&
        !lying ) {
        if( !activity ) {
            add_msg_if_player( m_warning, _( "You're feeling tired.  %s to lie down for sleep." ),
                               press_x( ACTION_SLEEP ) );
        } else {
            g->cancel_activity_query( _( "You're feeling tired." ) );
        }
    }

    if( current_stim < 0 ) {
        set_stim( std::min( current_stim + rate_multiplier, 0 ) );
    } else if( current_stim > 0 ) {
        set_stim( std::max( current_stim - rate_multiplier, 0 ) );
    }

    if( get_painkiller() > 0 ) {
        mod_painkiller( -std::min( get_painkiller(), rate_multiplier ) );
    }

    if( get_bp_effect_mod() > 0 ) {
        modify_bp_effect_mod( -std::min( get_bp_effect_mod(), rate_multiplier ) );
    } else if( get_bp_effect_mod() < 0 ) {
        modify_bp_effect_mod( std::min( -get_bp_effect_mod(), rate_multiplier ) );
    }

    if( get_heartrate_effect_mod() > 0 ) {
        modify_heartrate_effect_mod( -std::min( get_heartrate_effect_mod(), rate_multiplier ) );
    } else if( get_heartrate_effect_mod() < 0 ) {
        modify_heartrate_effect_mod( std::min( -get_heartrate_effect_mod(), rate_multiplier ) );
    }

    if( get_respiration_effect_mod() > 0 ) {
        modify_respiration_effect_mod( -std::min( get_respiration_effect_mod(), rate_multiplier ) );
    } else if( get_respiration_effect_mod() < 0 ) {
        modify_respiration_effect_mod( std::min( -get_respiration_effect_mod(), rate_multiplier ) );
    }

}

needs_rates Character::calc_needs_rates() const
{
    const effect &sleep = get_effect( effect_sleep );
    const bool asleep = !sleep.is_null();

    needs_rates rates;
    rates.hunger = metabolic_rate();

    rates.kcal = get_bmr();

    add_msg_debug_if( is_avatar(), debugmode::DF_CHAR_CALORIES, "Metabolic rate: %.2f", rates.hunger );

    static const std::string player_thirst_rate( "PLAYER_THIRST_RATE" );
    rates.thirst = get_option< float >( player_thirst_rate );

    static const std::string player_sleepiness_rate( "PLAYER_SLEEPINESS_RATE" );
    rates.sleepiness = get_option< float >( player_sleepiness_rate );

    if( asleep ) {
        calc_sleep_recovery_rate( rates );
    } else {
        rates.recovery = 0;
    }

    if( has_activity( ACT_TREE_COMMUNION ) ) {
        // Much of the body's needs are taken care of by the trees.
        // Hair Roots don't provide any bodily needs.
        if( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) || has_trait( trait_CHLOROMORPH ) ) {
            rates.hunger *= 0.5f;
            rates.thirst *= 0.5f;
            rates.sleepiness *= 0.5f;
        }
    }

    if( has_trait( trait_TRANSPIRATION ) ) {
        // Transpiration, the act of moving nutrients with evaporating water, can take a very heavy toll on your thirst when it's really hot.
        rates.thirst *= ( ( units::to_fahrenheit( get_weather().get_temperature(
                                pos_bub() ) ) - 32.5f ) / 40.0f );
    }

    rates.hunger = enchantment_cache->modify_value( enchant_vals::mod::HUNGER, rates.hunger );
    rates.sleepiness = enchantment_cache->modify_value( enchant_vals::mod::SLEEPINESS,
                       rates.sleepiness );
    rates.thirst = enchantment_cache->modify_value( enchant_vals::mod::THIRST, rates.thirst );

    return rates;
}

void Character::calc_sleep_recovery_rate( needs_rates &rates ) const
{
    const effect &sleep = get_effect( effect_sleep );
    rates.recovery = enchantment_cache->modify_value( enchant_vals::mod::SLEEPINESS_REGEN, 1 );

    // -5% sleep recovery rate for every main part below cold
    float temp_mod = 0.0f;
    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        if( get_part_temp_cur( bp ) <= BODYTEMP_COLD ) {
            temp_mod -= 0.05f;
        }
    }
    rates.recovery += temp_mod;

    if( !is_hibernating() ) {
        // Hunger and thirst advance more slowly while we sleep. This is the standard rate.
        rates.hunger *= 0.5f;
        rates.thirst *= 0.5f;
        const int intense = sleep.is_null() ? 0 : sleep.get_intensity();
        // Accelerated recovery capped to 2x over 2 hours
        // After 16 hours of activity, equal to 7.25 hours of rest
        const int accelerated_recovery_chance = 24 - intense + 1;
        const float accelerated_recovery_rate = 1.0f / accelerated_recovery_chance;
        rates.recovery += accelerated_recovery_rate;
    } else {
        // Hunger and thirst advance *much* more slowly whilst we hibernate.
        rates.hunger *= ( 2.0f / 7.0f );
        rates.thirst *= ( 2.0f / 7.0f );
    }
    rates.recovery -= static_cast<float>( get_perceived_pain() ) / 60;
}

item Character::reduce_charges( item *it, int quantity )
{
    if( !has_item( *it ) ) {
        debugmsg( "invalid item (name %s) for reduce_charges", it->tname() );
        return item();
    }
    if( it->charges <= quantity ) {
        return i_rem( it );
    }
    it->mod_charges( -quantity );
    item result( *it );
    result.charges = quantity;
    return result;
}

const profession *Character::get_profession() const
{
    return prof;
}

std::set<const profession *> Character::get_hobbies() const
{
    return hobbies;
}

bool Character::has_mission_item( int mission_id ) const
{
    return mission_id != -1 && has_item_with( has_mission_item_filter{ mission_id } );
}

void Character::check_needs_extremes()
{
    // Check if we've overdosed... in any deadly way.
    if( get_stim() > 250 ) {
        add_msg_player_or_npc( m_bad,
                               _( "You have a sudden heart attack!" ),
                               _( "<npcname> has a sudden heart attack!" ) );
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), efftype_id() );
        set_part_hp_cur( body_part_torso, 0 );
    } else if( get_stim() < -200 || get_painkiller() > 240 ) {
        add_msg_player_or_npc( m_bad,
                               _( "Your breathing stops completely." ),
                               _( "<npcname>'s breathing stops completely." ) );
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), efftype_id() );
        set_part_hp_cur( body_part_torso, 0 );
    } else if( has_effect( effect_jetinjector ) && get_effect_dur( effect_jetinjector ) > 40_minutes ) {
        if( !has_flag( json_flag_PAIN_IMMUNE ) ) {
            add_msg_player_or_npc( m_bad,
                                   _( "Your heart spasms painfully and stops." ),
                                   _( "<npcname>'s heart spasms painfully and stops." ) );
        } else {
            add_msg_player_or_npc( _( "Your heart spasms and stops." ),
                                   _( "<npcname>'s heart spasms and stops." ) );
        }
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), effect_jetinjector );
        set_part_hp_cur( body_part_torso, 0 );
    } else if( get_effect_dur( effect_adrenaline ) > 50_minutes ) {
        add_msg_player_or_npc( m_bad,
                               _( "Your heart spasms and stops." ),
                               _( "<npcname>'s heart spasms and stops." ) );
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), effect_adrenaline );
        set_part_hp_cur( body_part_torso, 0 );
    } else if( get_effect_int( effect_drunk ) > 4 ) {
        add_msg_player_or_npc( m_bad,
                               _( "Your breathing slows down to a stop." ),
                               _( "<npcname>'s breathing slows down to a stop." ) );
        get_event_bus().send<event_type::dies_from_drug_overdose>( getID(), effect_drunk );
        set_part_hp_cur( body_part_torso, 0 );
    }

    // check if we've starved
    if( needs_food() ) {
        if( get_stored_kcal() <= 0 ) {
            add_msg_if_player( m_bad, _( "You have starved to death." ) );
            get_event_bus().send<event_type::dies_of_starvation>( getID() );
            set_part_hp_cur( body_part_torso, 0 );
        } else {
            if( calendar::once_every( 12_hours ) ) {
                std::string category;
                if( stomach.contains() <= stomach.capacity( *this ) / 4 ) {
                    if( get_kcal_percent() < 0.1f ) {
                        category = "starving";
                    } else if( get_kcal_percent() < 0.25f ) {
                        category = "emaciated";
                    } else if( get_kcal_percent() < 0.5f ) {
                        category = "malnutrition";
                    } else if( get_kcal_percent() < 0.8f ) {
                        category = "low_cal";
                    }
                } else {
                    if( get_kcal_percent() < 0.1f ) {
                        category = "empty_starving";
                    } else if( get_kcal_percent() < 0.25f ) {
                        category = "empty_emaciated";
                    } else if( get_kcal_percent() < 0.5f ) {
                        category = "empty_malnutrition";
                    } else if( get_kcal_percent() < 0.8f ) {
                        category = "empty_low_cal";
                    }
                }
                if( !category.empty() ) {
                    const translation message = SNIPPET.random_from_category( category ).value_or( translation() );
                    add_msg_if_player( m_warning, message );
                }

            }
        }
    }

    // Check if we're dying of thirst
    if( needs_food() && get_thirst() >= 600 && ( stomach.get_water() == 0_ml ||
            guts.get_water() == 0_ml ) ) {
        if( get_thirst() >= 1200 ) {
            add_msg_if_player( m_bad, _( "You have died of dehydration." ) );
            get_event_bus().send<event_type::dies_of_thirst>( getID() );
            set_part_hp_cur( body_part_torso, 0 );
        } else if( get_thirst() >= 1000 && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "Even your eyes feel dry…" ) );
        } else if( get_thirst() >= 800 && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "You are THIRSTY!" ) );
        } else if( calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "Your mouth feels so dry…" ) );
        }
    }

    // Check if we're falling asleep, unless we're sleeping
    if( get_sleepiness() >= sleepiness_levels::EXHAUSTED + 25 && !in_sleep_state() ) {
        if( get_sleepiness() >= sleepiness_levels::MASSIVE_SLEEPINESS ) {
            add_msg_if_player( m_bad, _( "Survivor sleep now." ) );
            get_event_bus().send<event_type::falls_asleep_from_exhaustion>( getID() );
            mod_sleepiness( -10 );
            fall_asleep();
        } else if( get_sleepiness() >= 800 && calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "Anywhere would be a good place to sleep…" ) );
        } else if( calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "You feel like you haven't slept in days." ) );
        }
    }

    // Even if we're not Exhausted, we really should be feeling lack/sleep earlier
    // Penalties start at Dead Tired and go from there
    if( get_sleepiness() >= sleepiness_levels::DEAD_TIRED && !in_sleep_state() ) {
        if( get_sleepiness() >= 700 ) {
            if( calendar::once_every( 30_minutes ) ) {
                add_msg_if_player( m_warning, _( "You're too physically tired to stop yawning." ) );
                add_effect( effect_lack_sleep, 30_minutes + 1_turns );
            }
            /** @EFFECT_INT slightly decreases occurrence of short naps when dead tired */
            if( one_in( 50 + int_cur ) ) {
                // Rivet's idea: look out for microsleeps!
                fall_asleep( 30_seconds );
            }
        } else if( get_sleepiness() >= sleepiness_levels::EXHAUSTED ) {
            if( calendar::once_every( 30_minutes ) ) {
                add_msg_if_player( m_warning, _( "How much longer until bedtime?" ) );
                add_effect( effect_lack_sleep, 30_minutes + 1_turns );
            }
            /** @EFFECT_INT slightly decreases occurrence of short naps when exhausted */
            if( one_in( 100 + int_cur ) ) {
                fall_asleep( 30_seconds );
            }
        } else if( get_sleepiness() >= sleepiness_levels::DEAD_TIRED &&
                   calendar::once_every( 30_minutes ) ) {
            add_msg_if_player( m_warning, _( "*yawn* You should really get some sleep." ) );
            add_effect( effect_lack_sleep, 30_minutes + 1_turns );
        }
    }

    // Sleep deprivation kicks in if lack of sleep is avoided with stimulants or otherwise for long periods of time
    int sleep_deprivation = get_sleep_deprivation();
    float sleep_deprivation_pct = sleep_deprivation / static_cast<float>( SLEEP_DEPRIVATION_MASSIVE );

    if( sleep_deprivation >= SLEEP_DEPRIVATION_HARMLESS && !in_sleep_state() ) {
        if( calendar::once_every( 60_minutes ) ) {
            if( sleep_deprivation < SLEEP_DEPRIVATION_MINOR ) {
                add_msg_if_player( m_warning,
                                   _( "Your mind feels tired.  It's been a while since you've slept well." ) );
                mod_sleepiness( 1 );
            } else if( sleep_deprivation < SLEEP_DEPRIVATION_SERIOUS ) {
                add_msg_if_player( m_bad,
                                   _( "Your mind feels foggy from a lack of good sleep, and your eyes keep trying to close against your will." ) );
                mod_sleepiness( 5 );

                if( one_in( 10 ) ) {
                    mod_daily_health( -1, -10 );
                }
            } else if( sleep_deprivation < SLEEP_DEPRIVATION_MAJOR ) {
                add_msg_if_player( m_bad,
                                   _( "Your mind feels weary, and you dread every wakeful minute that passes.  You crave sleep, and feel like you're about to collapse." ) );
                mod_sleepiness( 10 );

                if( one_in( 5 ) ) {
                    mod_daily_health( -2, -10 );
                }
            } else if( sleep_deprivation < SLEEP_DEPRIVATION_MASSIVE ) {
                add_msg_if_player( m_bad,
                                   _( "You haven't slept decently for so long that your whole body is screaming for mercy.  It's a miracle that you're still awake, but it feels more like a curse now." ) );
                mod_sleepiness( 40 );

                mod_daily_health( -5, -10 );
            }
            // else you pass out for 20 hours, guaranteed

            // Microsleeps are slightly worse if you're sleep deprived, but not by much. (chance: 1 in (75 + int_cur) at lethal sleep deprivation)
            // Note: these can coexist with sleepiness-related microsleeps
            /** @EFFECT_INT slightly decreases occurrence of short naps when sleep deprived */
            if( one_in( static_cast<int>( sleep_deprivation_pct * 75 ) + int_cur ) ) {
                fall_asleep( 30_seconds );
            }

            // Stimulants can be used to stay awake a while longer, but after a while you'll just collapse.
            bool can_pass_out = ( get_stim() < 30 && sleep_deprivation >= SLEEP_DEPRIVATION_MINOR ) ||
                                sleep_deprivation >= SLEEP_DEPRIVATION_MAJOR;

            if( can_pass_out && calendar::once_every( 10_minutes ) ) {
                /** @EFFECT_PER slightly increases resilience against passing out from sleep deprivation */
                if( one_in( static_cast<int>( ( 1 - sleep_deprivation_pct ) * 100 ) + per_cur ) ||
                    sleep_deprivation >= SLEEP_DEPRIVATION_MASSIVE ) {
                    add_msg_player_or_npc( m_bad,
                                           _( "Your body collapses due to sleep deprivation, your neglected fatigue rushing back all at once, and you pass out on the spot." )
                                           , _( "<npcname> collapses to the ground from exhaustion." ) );
                    if( get_sleepiness() < sleepiness_levels::EXHAUSTED ) {
                        set_sleepiness( sleepiness_levels::EXHAUSTED );
                    }

                    if( sleep_deprivation >= SLEEP_DEPRIVATION_MAJOR ) {
                        fall_asleep( 20_hours );
                    } else if( sleep_deprivation >= SLEEP_DEPRIVATION_SERIOUS ) {
                        fall_asleep( 16_hours );
                    } else {
                        fall_asleep( 12_hours );
                    }
                }
            }

        }
    }
}

void Character::get_sick( bool is_flu )
{
    // Health is in the range [-200,200].
    // Diseases are half as common for every 50 health you gain.
    const float health_factor = std::pow( 2.0f, get_lifestyle() / 50.0f );
    const float env_factor = 2.0f + std::pow( get_env_resist( body_part_mouth ), 0.3f );
    const int disease_rarity = static_cast<int>( health_factor + env_factor );
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "disease_rarity = %d", disease_rarity );

    if( one_in( disease_rarity ) ) {
        // Normal people get sick about 2-4 times/year. If they are disease resistant, it's 1 time per year.
        const int base_diseases_per_year = has_trait( trait_DISRESISTANT ) ? 1 : 3;
        const time_duration immunity_duration = calendar::season_length() * 4 / base_diseases_per_year;

        bool already_has_flu = has_effect( effect_flu ) ||
                               count_queued_effects( "pre_flu" ) ||
                               count_queued_effects( "flu" );
        bool already_has_cold = has_effect( effect_common_cold ) ||
                                count_queued_effects( "pre_common_cold" ) ||
                                count_queued_effects( "common_cold" );

        if( is_flu && !already_has_flu ) {
            // Total duration of symptoms
            time_duration total_duration = rng( 3_days, 10_days );
            // Time before first symptoms
            time_duration incubation_period = rng( 18_hours, 36_hours );
            // Time from first symptoms to full blown disease
            time_duration warning_period = std::min( rng( 18_hours, 36_hours ), total_duration );

            add_msg_debug( debugmode::DF_CHAR_HEALTH, "Queuing flu: incubation %d h, warning %d h, total %d h",
                           to_hours<int>( incubation_period ),
                           to_hours<int>( warning_period ),
                           to_hours<int>( total_duration ) );

            queue_effect( "pre_flu", incubation_period, warning_period );
            queue_effect( "flu", incubation_period + warning_period, total_duration - warning_period );
            queue_effect( "flushot", incubation_period + total_duration, immunity_duration );

        } else if( !already_has_cold ) {
            // Total duration of symptoms
            time_duration total_duration = rng( 1_days, 14_days );
            // Time before first symptoms
            time_duration incubation_period = rng( 18_hours, 36_hours );
            // Time from first symptoms to full blown disease
            time_duration warning_period = std::min( rng( 18_hours, 36_hours ), total_duration );

            add_msg_debug( debugmode::DF_CHAR_HEALTH, "Queuing cold: incubation %d h, warning %d h, total %d h",
                           to_hours<int>( incubation_period ),
                           to_hours<int>( warning_period ),
                           to_hours<int>( total_duration ) );

            queue_effect( "pre_common_cold", incubation_period, warning_period );
            queue_effect( "common_cold", incubation_period + warning_period, total_duration - warning_period );
            queue_effect( "common_cold_immunity", incubation_period + total_duration, immunity_duration );
        }
    }
}

bool Character::is_hibernating() const
{
    // Hibernating only kicks in whilst Engorged; separate tracking for hunger/thirst here
    // as a safety catch.  One test subject managed to get two Colds during hibernation;
    // since those add sleepiness and dry out the character, the subject went for the full 10 days plus
    // a little, and came out of it well into Parched.  Hibernating shouldn't endanger your
    // life like that--but since there's much less fluid reserve than food reserve,
    // simply using the same numbers won't work.
    return has_effect( effect_sleep ) && get_kcal_percent() > 0.8f &&
           get_thirst() <= 80 && has_active_mutation( trait_HIBERNATE );
}

void Character::toolmod_add( item_location tool, item_location mod )
{
    if( !tool && !mod ) {
        debugmsg( "Tried toolmod installation but mod/tool not available" );
        return;
    }
    // first check at least the minimum requirements are met
    if( !has_trait( trait_DEBUG_HS ) && !can_use( *mod, *tool ) ) {
        return;
    }

    if( !query_yn( _( "Permanently install your %1$s in your %2$s?" ),
                   colorize( mod->tname(), mod->color_in_inventory() ),
                   colorize( tool->tname(), tool->color_in_inventory() ) ) ) {
        add_msg_if_player( _( "Never mind." ) );
        return; // player canceled installation
    }

    assign_activity( ACT_TOOLMOD_ADD, 1, -1 );
    activity.targets.emplace_back( std::move( tool ) );
    activity.targets.emplace_back( std::move( mod ) );
}

void Character::temp_equalizer( const bodypart_id &bp1, const bodypart_id &bp2 )
{
    if( has_flag( json_flag_CANNOT_CHANGE_TEMPERATURE ) ) {
        return;
    }
    // Body heat is moved around.
    // Shift in one direction only, will be shifted in the other direction separately.
    const units::temperature_delta diff = ( get_part_temp_cur( bp2 ) - get_part_temp_cur( bp1 ) ) *
                                          0.0001; // If bp1 is warmer, it will lose heat
    mod_part_temp_cur( bp1, diff );
}

float Character::get_dodge_base() const
{
    /** @EFFECT_DEX increases dodge base */
    /** @EFFECT_DODGE increases dodge_base */
    return get_dex() / 2.0f + get_skill_level( skill_dodge );
}
float Character::get_hit_base() const
{
    /** @EFFECT_DEX increases hit base, slightly */
    return get_dex() / 4.0f;
}

std::string Character::get_name() const
{
    return play_name.value_or( name );
}

std::vector<std::string> Character::get_grammatical_genders() const
{
    if( male ) {
        return { "m" };
    } else {
        return { "f" };
    }
}

nc_color Character::symbol_color() const
{
    nc_color basic = basic_symbol_color();

    if( has_effect( effect_downed ) ) {
        return hilite( basic );
    } else if( has_flag( json_flag_GRAB ) ) {
        return cyan_background( basic );
    }

    const field &fields = get_map().field_at( pos_bub() );

    // Priority: electricity, fire, acid, gases
    bool has_elec = false;
    bool has_fire = false;
    bool has_acid = false;
    bool has_fume = false;
    for( const auto &field : fields ) {
        has_elec = field.first.obj().has_elec;
        if( has_elec ) {
            return hilite( basic );
        }
        has_fire = field.first.obj().has_fire;
        has_acid = field.first.obj().has_acid;
        has_fume = field.first.obj().has_fume;
    }
    if( has_fire ) {
        return red_background( basic );
    }
    if( has_acid ) {
        return green_background( basic );
    }
    if( has_fume ) {
        return white_background( basic );
    }
    if( in_sleep_state() ) {
        return hilite( basic );
    }
    return basic;
}

bool Character::check_immunity_data( const field_immunity_data &ft ) const
{
    for( const json_character_flag &flag : ft.immunity_data_flags ) {
        if( has_flag( flag ) ) {
            return true;
        }
    }
    bool immune_by_body_part_resistance = !ft.immunity_data_body_part_env_resistance.empty();
    for( const std::pair<body_part_type::type, int> &fide :
         ft.immunity_data_body_part_env_resistance ) {
        for( const bodypart_id &bp : get_all_body_parts_of_type( fide.first ) ) {
            if( get_env_resist( bp ) < fide.second ) {
                // If any one of a bodypart type is unprotected disregard this immunity type
                // TODO: mitigate effect strength based on protected:unprotected ratio?
                immune_by_body_part_resistance = false;
                break;
            }
        }
    }
    if( immune_by_body_part_resistance ) {
        return true;
    }

    bool immune_by_worn_flags = !ft.immunity_data_part_item_flags.empty();
    // Check if all worn flags are fulfilled
    for( const std::pair<body_part_type::type, flag_id> &fide :
         ft.immunity_data_part_item_flags ) {
        for( const bodypart_id &bp : get_all_body_parts_of_type( fide.first ) ) {
            if( !worn_with_flag( fide.second, bp ) ) {
                // If any one of a bodypart type is unprotected disregard this immunity type
                // TODO: mitigate effect strength based on protected:unprotected ratio?
                immune_by_worn_flags = false;
                break;
            }
        }
    }

    if( immune_by_worn_flags ) {
        return true;
    }

    // Check if the optional worn flags are fulfilled
    for( const std::pair<body_part_type::type, flag_id> &fide :
         ft.immunity_data_part_item_flags_any ) {
        for( const bodypart_id &bp : get_all_body_parts_of_type( fide.first ) ) {
            if( worn_with_flag( fide.second, bp ) ) {
                // For now a single flag will protect all limbs, TODO: handle optional flags for multiples of the same limb type
                return true;
            }
        }
    }

    return false;
}

bool Character::is_immune_field( const field_type_id &fid ) const
{
    // Obviously this makes us invincible
    if( has_trait( trait_DEBUG_NODMG ) || has_effect( effect_incorporeal ) ) {
        return true;
    }

    // Check to see if we are immune
    const field_type &ft = fid.obj();

    if( check_immunity_data( ft.immunity_data ) ) {
        return true;
    }

    if( ft.has_elec ) {
        return is_elec_immune();
    }
    if( ft.has_fire ) {
        return has_flag( json_flag_HEATSINK ) || is_wearing( itype_rm13_armor_on );
    }
    if( ft.has_acid ) {
        return !is_on_ground() && get_env_resist( body_part_foot_l ) >= 15 &&
               get_env_resist( body_part_foot_r ) >= 15 &&
               get_env_resist( body_part_leg_l ) >= 15 &&
               get_env_resist( body_part_leg_r ) >= 15 &&
               // FIXME: Hardcoded damage type
               get_armor_type( damage_acid, body_part_foot_l ) >= 5 &&
               get_armor_type( damage_acid, body_part_foot_r ) >= 5 &&
               get_armor_type( damage_acid, body_part_leg_l ) >= 5 &&
               get_armor_type( damage_acid, body_part_leg_r ) >= 5;
    }
    // If we haven't found immunity yet fall up to the next level
    return Creature::is_immune_field( fid );
}

// FIXME: Relies on hardcoded damage type
bool Character::is_elec_immune() const
{
    return is_immune_damage( damage_electric );
}

bool Character::is_immune_effect( const efftype_id &eff ) const
{
    // FIXME: Hardcoded damage types
    if( eff == effect_downed ) {
        return is_knockdown_immune();
    } else if( eff == effect_onfire ) {
        return is_immune_damage( damage_heat );
    } else if( eff == effect_deaf ) {
        return worn_with_flag( flag_DEAF ) || has_flag( json_flag_DEAF ) ||
               worn_with_flag( flag_PARTIAL_DEAF ) ||
               has_flag( json_flag_IMMUNE_HEARING_DAMAGE ) ||
               is_wearing( itype_rm13_armor_on ) || is_deaf();
    } else if( eff->has_flag( flag_MUTE ) ) {
        return has_bionic( bio_voice );
    } else if( eff == effect_corroding ) {
        return is_immune_damage( damage_acid ) || has_trait( trait_SLIMY ) ||
               has_trait( trait_VISCOUS );
    }
    for( const json_character_flag &flag : eff->immune_flags ) {
        if( has_flag( flag ) ) {
            return true;
        }
    }
    return false;
}

bool Character::is_immune_damage( const damage_type_id &dt ) const
{
    if( dt.is_null() ) {
        return true;
    }
    for( const std::string &fl : dt->immune_flags ) {
        if( has_flag( json_character_flag( fl ) ) || worn_with_flag( flag_id( fl ) ) ) {
            return true;
        }
    }
    return false;
}

bool Character::is_rad_immune() const
{
    bool has_helmet = false;
    return ( is_wearing_power_armor( &has_helmet ) && has_helmet ) || worn_with_flag( flag_RAD_PROOF );
}

bool Character::is_knockdown_immune() const
{
    // hard code for old tentacle mutation
    bool knockdown_immune = has_trait( trait_LEG_TENT_BRACE ) && is_barefoot();

    // if we have 1.0 or greater knockdown resist
    knockdown_immune |= calculate_by_enchantment( 0.0, enchant_vals::mod::KNOCKDOWN_RESIST ) >= 1;
    return knockdown_immune;
}

int Character::throw_range( const item &it ) const
{
    if( it.is_null() ) {
        return -1;
    }

    item tmp = it;

    if( tmp.count_by_charges() && tmp.charges > 1 ) {
        tmp.charges = 1;
    }

    int ench_bonus = enchantment_cache->get_value_add( enchant_vals::mod::THROW_STR );
    int str = get_arm_str() + ench_bonus;

    /** @ARM_STR determines maximum weight that can be thrown */
    if( ( tmp.weight() / 113_gram ) > str * 15 )  {
        return 0;
    }
    // Increases as weight decreases until 150 g, then decreases again
    /** @ARM_STR increases throwing range, vs item weight (high or low) */
    if( is_mounted() ) {
        auto *mons = mounted_creature.get();
        str = mons->mech_str_addition() != 0 ? mons->mech_str_addition() : str;
    }
    int ret = ( str * 10 ) / ( tmp.weight() >= 150_gram ? tmp.weight() / 113_gram : 10 -
                               static_cast<int>(
                                   tmp.weight() / 15_gram ) );
    ret -= tmp.volume() / 1_liter;
    if( has_active_bionic( bio_railgun ) && tmp.made_of_any( ferric ) ) {
        ret *= 2;
    }
    if( ret < 1 ) {
        return 1;
    }

    // Cap at triple of our strength + skill
    if( ret > round( str * 3 + get_skill_level( skill_throw ) + ench_bonus ) ) {
        return round( str * 3 + get_skill_level( skill_throw ) + ench_bonus );
    }

    return ret;
}

const std::vector<material_id> Character::fleshy = { material_flesh, material_hflesh };
bool Character::made_of( const material_id &m ) const
{
    // TODO: check for mutations that change this.
    return std::find( fleshy.begin(), fleshy.end(), m ) != fleshy.end();
}
bool Character::made_of_any( const std::set<material_id> &ms ) const
{
    // TODO: check for mutations that change this.
    return std::any_of( fleshy.begin(), fleshy.end(), [&ms]( const material_id & e ) {
        return ms.count( e );
    } );
}

bool Character::is_blind() const
{
    return worn_with_flag( flag_BLIND ) ||
           has_flag( json_flag_BLIND ) || get_limb_score( limb_score_vision ) <= 0;
}

bool Character::is_invisible() const
{
    return has_flag( json_flag_INVISIBLE ) ||
           is_wearing_active_optcloak() ||
           has_trait( trait_DEBUG_CLOAK );
}

int Character::visibility( bool, int ) const
{
    // 0-100 %
    if( is_invisible() ) {
        return 0;
    }
    // TODO:
    // if ( dark_clothing() && light check ...
    // or add STEALTH_MODIFIER enchantment to dark clothes
    int stealth_modifier = enchantment_cache->modify_value( enchant_vals::mod::STEALTH_MODIFIER, 1 );
    return clamp( 100 - stealth_modifier, 40, 160 );
}

/*
 * Calculate player brightness based on the brightest active item, as
 * per itype tag LIGHT_* and optional CHARGEDIM ( fade starting at 20% charge )
 * item.light.* is -unimplemented- for the moment, as it is a custom override for
 * applying light sources/arcs with specific angle and direction.
 */
float Character::active_light() const
{
    float lumination = 0.0f;

    int maxlum = 0;
    cache_visit_items_with( "is_emissive", &item::is_emissive, [&maxlum]( const item & it ) {
        const int lumit = it.getlight_emit();
        if( maxlum < lumit ) {
            maxlum = lumit;
        }
    } );

    lumination = static_cast<float>( maxlum );

    float mut_lum = 0.0f;
    for( const trait_id &mut : get_functioning_mutations() ) {
        float curr_lum = 0.0f;
        for( const std::pair<const bodypart_str_id, float> &elem : mut->lumination ) {
            const float lum_coverage = worn.coverage_with_flags_exclude( elem.first.id(),
            { flag_ALLOWS_NATURAL_ATTACKS, flag_SEMITANGIBLE, flag_PERSONAL } ) / 100.0f;
            curr_lum += elem.second * ( 1 - lum_coverage );
        }
        mut_lum += curr_lum;
    }

    lumination = std::max( lumination, mut_lum );

    lumination = std::max( lumination,
                           static_cast<float>( enchantment_cache->modify_value( enchant_vals::mod::LUMINATION, 0 ) ) );

    if( lumination < 5 && ( has_effect( effect_glowing ) || has_effect( effect_glowy_led ) ) ) {
        lumination = 5;
    }
    return lumination;
}

bool Character::sees_with_specials( const Creature &critter ) const
{
    const const_dialogue d( get_const_talker_for( *this ), get_const_talker_for( critter ) );
    const enchant_cache::special_vision foo = enchantment_cache->get_vision( d );

    if( enchantment_cache->get_vision_can_see( foo ) ) {
        return true;
    }
    return false;
}

bool Character::pour_into( item_location &container, item &liquid, bool ignore_settings,
                           bool silent )
{
    std::string err;
    int max_remaining_capacity = container->get_remaining_capacity_for_liquid( liquid, *this, &err );
    int amount = container->all_pockets_rigid() ? max_remaining_capacity :
                 std::min( max_remaining_capacity, container.max_charges_by_parent_recursive( liquid ).value() );

    if( !err.empty() ) {
        if( !container->has_item_with( [&liquid]( const item & it ) {
        return it.typeId() == liquid.typeId();
        } ) ) {
            add_msg_if_player( m_bad, err );
        } else {
            //~ you filled <container> to the brim with <liquid>
            add_msg_if_player( _( "You filled %1$s to the brim with %2$s." ), container->tname(),
                               liquid.tname() );
        }
        return false;
    }

    if( amount == 0 ) {
        add_msg_if_player( _( "The %1$s can't expand to fit any more %2$s." ), container->tname(),
                           liquid.tname() );
        return false;
    }

    // get_remaining_capacity_for_liquid doesn't consider the current amount of liquid
    if( liquid.count_by_charges() ) {
        amount = std::min( amount, liquid.charges );
    }

    if( !silent ) {
        add_msg_if_player( _( "You pour %1$s into the %2$s." ), liquid.tname(), container->tname() );
    }

    liquid.charges -= container->fill_with( liquid, amount, false, false, ignore_settings );
    inv->unsort();

    if( liquid.charges > 0 && !silent ) {
        add_msg_if_player( _( "There's some left over!" ) );
    }

    get_avatar().invalidate_weight_carried_cache();

    return true;
}

bool Character::pour_into( const vpart_reference &vp, item &liquid ) const
{
    if( !vp.part().fill_with( liquid ) ) {
        return false;
    }

    //~ $1 - vehicle name, $2 - part name, $3 - liquid type
    add_msg_if_player( _( "You refill the %1$s's %2$s with %3$s." ),
                       vp.vehicle().name, vp.part().name(), liquid.type_name() );

    if( liquid.charges > 0 ) {
        add_msg_if_player( _( "There's some left over!" ) );
    }
    return true;
}

float Character::rest_quality() const
{
    map &here = get_map();
    const tripoint_bub_ms your_pos = pos_bub();
    float rest = 0.0f;
    const float ur_act_level = instantaneous_activity_level();
    // Negative morales are penalties
    int cold_penalty = -has_morale( morale_cold );
    int heat_penalty = -has_morale( morale_hot );
    if( cold_penalty > 0 || heat_penalty > 0 ) {
        // Extreme temperatures have the body working harder to fight them, with less energy dedicated to healing, even just laying around will have you shivering or sweating.
        rest -= cold_penalty / 10.0f;
        rest -= heat_penalty / 10.0f;
    }

    if( has_effect( effect_sleep ) ) {
        // Beds should normally have a maximum of 2C warmth, rest == 1.0 on a pristine bed. Less comfortable beds provide less rest quality.
        // Cannot be lower than 0, even though some beds(like *the bare ground*) have negative floor_bedding_warmth. Maybe this should change?
        rest += ( std::max( units::to_kelvin_delta( floor_bedding_warmth( your_pos ) ),
                            0.f ) / 2.0f ) ;
        return std::max( rest, 0.0f );
    }
    const optional_vpart_position veh_part = here.veh_at( your_pos );
    bool has_vehicle_seat = !!veh_part.part_with_feature( "SEAT", true );
    if( ur_act_level <= LIGHT_EXERCISE ) {
        rest += 0.1f;
        if( here.has_flag_ter_or_furn( "CAN_SIT", your_pos.xy() ) || has_vehicle_seat ) {
            // If not performing any real exercise (not even moving around), chairs allow you to rest a little bit.
            rest += 0.2f;
        } else if( floor_bedding_warmth( your_pos ) > 0_C_delta ) {
            // Any comfortable bed can substitute for a chair, but only if you don't have one.
            rest += 0.2f * ( units::to_celsius_delta( floor_bedding_warmth( your_pos ) ) / 2.0f );
        }
        if( ur_act_level <= NO_EXERCISE ) {
            rest += 0.2f;
        }
    }
    // These stack!
    if( ur_act_level >= BRISK_EXERCISE ) {
        rest -= 0.1f;
    }
    if( ur_act_level >= ACTIVE_EXERCISE ) {
        rest -= 0.2f;
    }
    if( ur_act_level >= EXTRA_EXERCISE ) {
        rest -= 0.3f;
    }
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "%s resting quality: %.6f", get_name(), rest );
    return rest;
}

std::vector<std::string> Character::extended_description() const
{
    std::vector<std::string> tmp;
    std::string name_str = colorize( get_name(), basic_symbol_color() );
    if( is_avatar() ) {
        // <bad>This is me, <player_name>.</bad>
        tmp.emplace_back( string_format( _( "This is you - %s." ), name_str ) );
    } else {
        tmp.emplace_back( string_format( _( "This is %s." ), name_str ) );
    }

    tmp.emplace_back( "--" );

    const std::vector<bodypart_id> &bps = get_all_body_parts( get_body_part_flags::only_main );
    // Find length of bp names, to align
    // accumulate looks weird here, any better function?
    int longest = std::accumulate( bps.begin(), bps.end(), 0,
    []( int m, bodypart_id bp ) {
        return std::max( m, utf8_width( body_part_name_as_heading( bp, 1 ) ) );
    } );

    // This is a stripped-down version of the body_window function
    // This should be extracted into a separate function later on
    for( const bodypart_id &bp : bps ) {
        const std::string &bp_heading = body_part_name_as_heading( bp, 1 );

        const nc_color state_col = display::limb_color( *this, bp, true, true, true );
        nc_color name_color = state_col;
        std::pair<std::string, nc_color> hp_bar = get_hp_bar( get_part_hp_cur( bp ), get_part_hp_max( bp ),
                false );
        std::pair<std::string, nc_color> hp_numbers =
        { " " + std::to_string( get_part_hp_cur( bp ) ) + "/" + std::to_string( get_part_hp_max( bp ) ), c_white };

        std::string bp_stat = colorize( left_justify( bp_heading, longest ), name_color );
        if( debug_mode ) {
            bp_stat += colorize( hp_numbers.first, hp_numbers.second );
        } else {
            // Trailing bars. UGLY!
            // TODO: Integrate into get_hp_bar somehow
            bp_stat += colorize( hp_bar.first, hp_bar.second );
            bp_stat += colorize( std::string( 5 - utf8_width( hp_bar.first ), '.' ), c_white );
        }
        tmp.emplace_back( bp_stat );
    }

    tmp.emplace_back( "--" );
    std::string wielding;
    if( weapon.is_null() ) {
        wielding = _( "Nothing" );
    } else {
        wielding = weapon.tname();
    }

    tmp.emplace_back( string_format( _( "Wielding: %s" ), colorize( wielding, c_red ) ) );
    std::string wearing = _( "Wearing:" ) + std::string( " " );

    const std::list<item_location> visible_worn_items = get_visible_worn_items();
    std::string worn_string = enumerate_as_string( visible_worn_items.begin(), visible_worn_items.end(),
    []( const item_location & it ) {
        return it.get_item()->tname();
    } );
    wearing += !worn_string.empty() ? worn_string : _( "Nothing" );
    tmp.emplace_back( wearing );

    int visibility_cap = get_player_character().get_mutation_visibility_cap( this );
    const std::string trait_str = visible_mutations( visibility_cap );
    if( !trait_str.empty() ) {
        tmp.emplace_back( string_format( _( "Traits: %s" ), trait_str ) );
    }

    std::vector<std::string> ret;
    ret.reserve( tmp.size() );
    for( const std::string &s : tmp ) {
        ret.emplace_back( replace_colors( s ) );
    }

    return ret;
}

social_modifiers Character::get_mutation_bionic_social_mods() const
{
    social_modifiers mods;
    for( const trait_id &mut : get_functioning_mutations() ) {
        mods += mut->social_mods;
    }
    for( const bionic &bio : *my_bionics ) {
        mods += bio.info().social_mods;
    }
    return mods;
}


namespace
{
float _hp_modified_rate( Character const &who, float rate )
{
    float const primary_hp_mod = who.enchantment_cache->get_value_multiply( enchant_vals::mod::MAX_HP );
    if( primary_hp_mod < 0.0f ) {
        cata_assert( primary_hp_mod >= -1.0f );
        return rate * ( 1.0f + primary_hp_mod );
    }

    return rate;
}
} // namespace

float Character::healing_rate( float at_rest_quality ) const
{
    float const rest = clamp( at_rest_quality, 0.0f, 1.0f );
    // TODO: Cache
    float const base_heal_rate = is_avatar() ? get_option<float>( "PLAYER_HEALING_RATE" )
                                 : get_option<float>( "NPC_HEALING_RATE" );
    float const heal_rate = enchantment_cache->modify_value( enchant_vals::mod::REGEN_HP,
                            base_heal_rate );
    float awake_rate = ( 1.0f - rest ) * heal_rate;
    awake_rate *= enchantment_cache->modify_value( enchant_vals::mod::REGEN_HP_AWAKE,
                  1 ) - 1;
    float const asleep_rate = rest * heal_rate * ( 1.0f + get_lifestyle() / 200.0f );
    float final_rate = awake_rate + asleep_rate;
    // Most common case: awake player with no regenerative abilities
    // ~7e-5 is 1 hp per day, anything less than that is totally negligible
    static constexpr float eps = 0.000007f;
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "%s healing: %.6f", get_name(), final_rate );
    if( std::abs( final_rate ) < eps ) {
        return 0.0f;
    }

    return final_rate;
}

float Character::healing_rate_medicine( float at_rest_quality, const bodypart_id &bp ) const
{
    float rate_medicine = 0.0f;

    for( const auto &elem : *effects ) {
        for( const std::pair<const bodypart_id, effect> &i : elem.second ) {
            if( i.first != bp ) {
                continue;
            }
            const effect &eff = i.second;
            float tmp_rate = static_cast<float>( eff.get_amount( "HEAL_RATE" ) ) / to_turns<int>
                             ( 24_hours );

            if( bp == body_part_head ) {
                tmp_rate *= eff.get_amount( "HEAL_HEAD" ) / 100.0f;
            }
            if( bp == body_part_torso ) {
                tmp_rate *= eff.get_amount( "HEAL_TORSO" ) / 100.0f;
            }
            rate_medicine += tmp_rate;
        }
    }

    rate_medicine = enchantment_cache->modify_value( enchant_vals::mod::REGEN_HP,
                    rate_medicine );
    // Sufficiently negative rest quality can completely eliminate your healing, but never turn it negative.
    rate_medicine *= 1.0f + std::max( at_rest_quality, -1.0f );

    // increase healing if character has both effects
    if( has_effect( effect_bandaged ) && has_effect( effect_disinfected ) ) {
        rate_medicine *= 1.25;
    }

    if( get_lifestyle() > 0.0f ) {
        rate_medicine *= 1.0f + get_lifestyle() / 200.0f;
    } else {
        rate_medicine *= 1.0f + get_lifestyle() / 400.0f;
    }
    return _hp_modified_rate( *this, rate_medicine );
}

float Character::get_bmi() const
{
    return get_bmi_lean() + get_bmi_fat();
}

float Character::get_bmi_lean() const
{
    //strength BMIs decrease to zero as you starve (muscle atrophy)
    if( get_bmi_fat() < character_weight_category::normal ) {
        const int str_penalty = std::floor( ( 1.0f - ( get_bmi_fat() /
                                              character_weight_category::normal ) ) * str_max );
        return 12.0f + get_str_base() - str_penalty;
    }
    return 12.0f + get_str_base();
}

float Character::get_bmi_fat() const
{
    return ( get_stored_kcal() / KCAL_PER_KG ) / ( std::pow( height() / 100.0f,
            2 ) * get_cached_organic_size() );
}

bool Character::has_calorie_deficit() const
{
    return get_bmi_fat() < character_weight_category::normal;
}

units::mass Character::bodyweight() const
{
    return bodyweight_fat() + bodyweight_lean();
}

units::mass Character::bodyweight_fat() const
{
    return units::from_kilogram( get_stored_kcal() / KCAL_PER_KG );
}

units::mass Character::bodyweight_lean() const
{
    //12 plus base strength gives non fat bmi, adjusted by starvation in get_bmi_lean()
    //this is multiplied by our total hit size from mutated body parts (or lack of parts thereof)
    //for example a tail with a hit size of 10 means our lean mass is 10% greater
    //or if we chop off our arms and legs to get bionic replacements, we're down to about 42% of our original lean mass
    return get_cached_organic_size() * units::from_kilogram( get_bmi_lean() * std::pow(
                height() / 100.0f,
                2 ) );
}

float Character::fat_ratio() const
{
    return bodyweight_fat() / bodyweight();
}

units::mass Character::bionics_weight() const
{
    units::mass bio_weight = 0_gram;
    for( const bionic_id &bid : get_bionics() ) {
        if( !bid->included ) {
            bio_weight += item::find_type( bid->itype() )->weight;
        }
    }
    return bio_weight;
}

void Character::reset_chargen_attributes()
{
    init_age = 25;
    init_height = 175;
}

int Character::base_age() const
{
    return init_age;
}

void Character::set_base_age( int age )
{
    init_age = age;
}

void Character::mod_base_age( int mod )
{
    init_age += mod;
}

int Character::age( time_point when ) const
{
    int years_since_cataclysm = to_turns<int>( when - calendar::turn_zero ) /
                                to_turns<int>( calendar::year_length() );
    return init_age + years_since_cataclysm;
}

std::string Character::age_string( time_point when ) const
{
    //~ how old the character is in years. try to limit number of characters to fit on the screen
    std::string unformatted = _( "%d years" );
    return string_format( unformatted, age( when ) );
}

int Character::ugliness() const
{
    int ugliness = 0;
    for( trait_id &mut : get_functioning_mutations() ) {
        ugliness += mut.obj().ugliness;
    }
    for( const bodypart_id &bp : get_all_body_parts() ) {
        if( bp->ugliness == 0 && bp->ugliness_mandatory == 0 ) {
            continue;
        }
        ugliness += bp->ugliness_mandatory;
        ugliness += bp->ugliness - ( bp->ugliness * worn.get_coverage( bp ) / 100 );
    }
    ugliness = enchantment_cache->modify_value( enchant_vals::mod::UGLINESS, ugliness );
    return ugliness;
}

struct HeightLimits {
    int min_height = 0;
    int base_height = 0;
    int max_height = 0;
};

/** Min and max heights in cm for each size category */
static const std::map<creature_size, HeightLimits> size_category_height_limits {
    { creature_size::tiny, { 58, 70, 87 } },
    { creature_size::small, { 88, 122, 144 } },
    { creature_size::medium, { 145, 175, 200 } }, // minimum is 2 std. deviations below average female height
    { creature_size::large, { 201, 227, 250 } },
    { creature_size::huge, { 251, 280, 320 } },
};

int Character::min_height( creature_size size_category )
{
    return size_category_height_limits.at( size_category ).min_height;
}

int Character::default_height( creature_size size_category )
{
    return size_category_height_limits.at( size_category ).base_height;
}

int Character::max_height( creature_size size_category )
{
    return size_category_height_limits.at( size_category ).max_height;
}

int Character::base_height() const
{
    return init_height;
}

void Character::set_base_height( int height )
{
    init_height = height;
}

void Character::mod_base_height( int mod )
{
    init_height += mod;
}

std::string Character::height_string() const
{
    const bool metric = get_option<std::string>( "DISTANCE_UNITS" ) == "metric";

    if( metric ) {
        std::string metric_string = _( "%d cm" );
        return string_format( metric_string, height() );
    }

    int total_inches = std::round( height() / 2.54 );
    int feet = std::floor( total_inches / 12 );
    int remainder_inches = total_inches % 12;
    return string_format( "%d\'%d\"", feet, remainder_inches );
}

int Character::height() const
{
    const double base_height_deviation = base_height() / static_cast< double >
                                         ( Character::default_height() );
    const HeightLimits &limits = size_category_height_limits.at( get_size() );
    return clamp<int>( std::round( base_height_deviation * limits.base_height ),
                       limits.min_height, limits.max_height );
}

int Character::base_bmr() const
{
    /**
    Values are for males, and average!
    */
    const int equation_constant = 5;
    const int weight_factor = units::to_gram<int>( bodyweight() / 100.0 );
    const int height_factor = 6.25 * height();
    const int age_factor = 5 * age();
    return metabolic_rate_base() * ( weight_factor + height_factor - age_factor + equation_constant );
}

int Character::get_bmr() const
{
    return base_bmr() * std::ceil( clamp( activity_history.average_activity(), NO_EXERCISE,
                                          maximum_exertion_level() ) );
}


void Character::set_activity_level( float new_level )
{
    if( new_level <= NO_EXERCISE && in_sleep_state() ) {
        new_level = std::min( new_level, SLEEP_EXERCISE );
    }

    activity_history.log_activity( new_level );
}

void Character::reset_activity_level()
{
    activity_history.reset_activity_level();
}

std::string Character::activity_level_str() const
{
    return activity_history.activity_level_str();
}

void Character::mend_item( item_location &&obj, bool interactive )
{
    if( has_trait( trait_DEBUG_HS ) ) {
        uilist menu;
        menu.text = _( "Toggle which fault?" );
        std::vector<std::pair<fault_id, bool>> opts;
        for( const auto &f : obj->faults_potential() ) {
            opts.emplace_back( f, obj->has_fault( f ) );
            menu.addentry( -1, true, -1, string_format(
                               opts.back().second ? pgettext( "fault", "Mend: %s" ) : pgettext( "fault", "Set: %s" ),
                               f.obj().name() ) );
        }
        if( opts.empty() ) {
            add_msg( m_info, _( "The %s doesn't have any faults to toggle." ), obj->tname() );
            return;
        }
        menu.query();
        if( menu.ret >= 0 ) {
            if( opts[menu.ret].second ) {
                obj->remove_fault( opts[menu.ret].first );
            } else {
                obj->set_fault( opts[menu.ret].first, true, false );
            }
        }
        return;
    }

    const inventory &inv = crafting_inventory();

    struct mending_option {
        fault_id fault;
        std::reference_wrapper<const fault_fix> fix;
        bool doable;
        time_duration time_to_fix = 1_hours;
    };

    std::vector<mending_option> mending_options;
    for( const fault_id &f : obj->faults ) {
        for( const fault_fix_id &fix_id : f->get_fixes() ) {
            const fault_fix &fix = *fix_id;
            mending_option opt{ f, fix, true };
            for( const auto &[skill_id, level] : fix.skills ) {
                if( get_greater_skill_or_knowledge_level( skill_id ) < level ) {
                    opt.doable = false;
                    break;
                }
            }
            opt.doable &= fix.get_requirements().can_make_with_inventory( inv, is_crafting_component );
            mending_options.emplace_back( opt );
        }
    }

    if( mending_options.empty() ) {
        if( interactive ) {
            add_msg( m_info, _( "The %s doesn't have any faults to mend." ), obj->tname() );
            if( obj->damage() > obj->degradation() ) {
                const std::set<itype_id> &rep = obj->repaired_with();
                if( rep.empty() ) {
                    add_msg( m_info, _( "It is damaged, but cannot be repaired." ) );
                } else {
                    const std::string repair_options =
                    enumerate_as_string( rep.begin(), rep.end(), []( const itype_id & e ) {
                        return item::nname( e );
                    }, enumeration_conjunction::or_ );

                    add_msg( m_info, _( "It is damaged, and could be repaired with %s.  "
                                        "%s to use one of those items." ),
                             repair_options, press_x( ACTION_USE ) );
                }
            }
        }
        return;
    }

    int sel = 0;
    if( interactive ) {
        uilist menu;
        menu.title = _( "Mend which fault?" );
        menu.desc_enabled = true;

        constexpr int fold_width = 80;

        for( mending_option &opt : mending_options ) {
            const fault_fix &fix = opt.fix;
            const nc_color col = opt.doable ? c_white : c_light_gray;

            const requirement_data &reqs = fix.get_requirements();
            auto tools = reqs.get_folded_tools_list( fold_width, col, inv );
            auto comps = reqs.get_folded_components_list( fold_width, col, inv, is_crafting_component );

            std::string descr = word_rewrap( obj.get_item()->get_fault_description( opt.fault ), 80 ) + "\n\n";
            for( const fault_id &fid : fix.faults_removed ) {
                if( obj->has_fault( fid ) ) {
                    descr += string_format( _( "Removes fault: <color_green>%s</color>\n" ), fid->name() );
                } else {
                    descr += string_format( _( "Removes fault: <color_light_gray>%s</color>\n" ), fid->name() );
                }
            }
            for( const fault_id &fid : fix.faults_added ) {
                descr += string_format( _( "Adds fault: <color_yellow>%s</color>\n" ), fid->name() );
            }
            if( fix.mod_damage < 0 ) {
                descr += string_format( _( "<color_green>Repairs</color> %d damage.\n" ),
                                        -fix.mod_damage / itype::damage_scale );
            } else if( fix.mod_damage > 0 ) {
                descr += string_format( _( "<color_red>Applies</color> %d damage.\n" ),
                                        fix.mod_damage / itype::damage_scale );
            }

            opt.time_to_fix = fix.time;
            // if an item has a time saver flag multiply total time by that flag's time factor
            for( const auto &[flag_id, mult] : fix.time_save_flags ) {
                if( obj->has_flag( flag_id ) ) {
                    opt.time_to_fix *= mult;
                }
            }
            // if you have a time saver prof multiply total time by that prof's time factor
            for( const auto &[proficiency_id, mult] : fix.time_save_profs ) {
                if( has_proficiency( proficiency_id ) ) {
                    opt.time_to_fix *= mult;
                }
            }
            descr += string_format( _( "Time required: <color_cyan>%s</color>\n" ),
                                    to_string_approx( opt.time_to_fix ) );
            if( fix.skills.empty() ) {
                descr += string_format( _( "Skills: <color_cyan>none</color>\n" ) );
            } else {
                descr += string_format( _( "Skills: %s\n" ), enumerate_as_string(
                fix.skills.begin(), fix.skills.end(), [this]( const std::pair<skill_id, int> &sk ) {
                    if( get_greater_skill_or_knowledge_level( sk.first ) >= sk.second ) {
                        return string_format( pgettext( "skill requirement",
                                                        //~ %1$s: skill name, %2$s: current skill level, %3$s: required skill level
                                                        "<color_cyan>%1$s</color> <color_green>(%2$d/%3$d)</color>" ),
                                              sk.first->name(), static_cast<int>( get_greater_skill_or_knowledge_level( sk.first ) ), sk.second );
                    } else {
                        return string_format( pgettext( "skill requirement",
                                                        //~ %1$s: skill name, %2$s: current skill level, %3$s: required skill level
                                                        "<color_cyan>%1$s</color> <color_red>(%2$d/%3$d)</color>" ),
                                              sk.first->name(), static_cast<int>( get_greater_skill_or_knowledge_level( sk.first ) ), sk.second );
                    }
                } ) );
            }

            for( const std::string &line : tools ) {
                descr += line + "\n";
            }
            for( const std::string &line : comps ) {
                descr += line + "\n";
            }

            menu.addentry_desc( -1, true, -1, fix.name.translated(), colorize( descr, col ) );
        }
        menu.query();
        if( menu.ret < 0 ) {
            add_msg( _( "Never mind." ) );
            return;
        }
        sel = menu.ret;
    }

    if( sel >= 0 ) {
        const mending_option &opt = mending_options[sel];
        if( !opt.doable ) {
            if( interactive ) {
                add_msg( m_info, _( "You are currently unable to mend the %s this way." ), obj->tname() );
            }
            return;
        }

        const fault_fix &fix = opt.fix;
        assign_activity( ACT_MEND_ITEM, to_moves<int>( opt.time_to_fix ) );
        activity.name = opt.fault.str();
        activity.str_values.emplace_back( fix.id.str() );
        activity.targets.push_back( std::move( obj ) );
    }
}

int Character::get_stim() const
{
    return stim;
}

void Character::set_stim( int new_stim )
{
    stim = new_stim;
}

void Character::mod_stim( int mod )
{
    stim += mod;
}

int Character::get_rad() const
{
    return radiation;
}

void Character::set_rad( int new_rad )
{
    if( radiation != new_rad ) {
        radiation = new_rad;
        on_stat_change( "radiation", radiation );
    }
}

void Character::mod_rad( int mod )
{
    if( has_flag( json_flag_NO_RADIATION ) ) {
        return;
    }
    set_rad( std::max( 0, get_rad() + mod ) );
}

void Character::calculate_leak_level()
{
    float ret = 0.0f;
    // This is bad way to calculate radiation and should be rewritten some day.
    for( const item_location &item_loc : const_cast<Character *>( this )->all_items_loc() ) {
        const item *it = item_loc.get_item();
        if( it->has_flag( flag_RADIOACTIVE ) ) {
            if( it->has_flag( flag_LEAK_ALWAYS ) ) {
                ret += to_gram( it->weight() ) / 250.f;
            } else if( it->has_flag( flag_LEAK_DAM ) ) {
                ret += it->damage_level();
            }
        }
    }
    leak_level_dirty = false;
    leak_level = ret;
}

float Character::get_leak_level() const
{
    return leak_level;
}

void Character::invalidate_leak_level_cache()
{
    leak_level_dirty = true;
}

int Character::get_stamina() const
{
    if( is_npc() ) {
        // No point in doing a bunch of checks on NPCs for now since they can't use stamina.
        return get_stamina_max();
    }
    return stamina;
}

int Character::get_stamina_max() const
{
    if( is_npc() ) {
        // No point in doing a bunch of checks on NPCs for now since they can't use stamina.
        return 10000;
    }
    // Since adding cardio, 'player_max_stamina' is really 'base max stamina' and gets further modified
    // by your CV fitness.  Name has been kept this way to avoid needing to change the code.
    // Default base maximum stamina and cardio scaling are defined in data/core/game_balance.json
    static const std::string player_max_stamina( "PLAYER_MAX_STAMINA_BASE" );
    static const std::string player_cardiofit_stamina_scale( "PLAYER_CARDIOFIT_STAMINA_SCALING" );

    // Cardiofit stamina mod defaults to 5, and get_cardiofit() should return a value in the vicinity
    // of 1000-3000, so this should add somewhere between 3000 to 15000 stamina.
    int max_stamina = get_option<int>( player_max_stamina ) +
                      get_option<int>( player_cardiofit_stamina_scale ) * get_cardiofit();
    max_stamina = enchantment_cache->modify_value( enchant_vals::mod::MAX_STAMINA, max_stamina );

    return max_stamina;
}

void Character::set_stamina( int new_stamina )
{
    stamina = new_stamina;
}

int Character::get_arms_power_use() const
{
    // millijoules
    return arms_power_use;
}

int Character::get_legs_power_use() const
{
    // millijoules
    return legs_power_use;
}

float Character::get_arms_stam_mult() const
{
    return arms_stam_mult;
}

float Character::get_legs_stam_mult() const
{
    return legs_stam_mult;
}

void Character::recalc_limb_energy_usage()
{
    // calculate energy usage of arms
    float total_limb_count = 0.0f;
    float bionic_limb_count = 0.0f;
    float bionic_powercost = 0;
    body_part_type::type arm_type = body_part_type::type::arm;
    for( const bodypart_id &bp : get_all_body_parts_of_type( arm_type ) ) {
        total_limb_count++;
        if( bp->has_flag( json_flag_BIONIC_LIMB ) ) {
            bionic_powercost += bp->power_efficiency;
            bionic_limb_count++;
        }
    }
    arms_power_use = bionic_powercost;
    if( bionic_limb_count > 0 ) {
        arms_stam_mult = 1 - ( bionic_limb_count / total_limb_count );
    } else {
        arms_stam_mult = 1.0f;
    }
    //sanity check ourselves in debug
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "Total arms in use: %.1f, Bionic arms: %.1f",
                   total_limb_count,
                   bionic_limb_count );
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "bionic power per arms stamina: %d", arms_power_use );
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "arms stam usage mult by %.1f", arms_stam_mult );

    // calculate energy usage of legs
    total_limb_count = 0.0f;
    bionic_limb_count = 0.0f;
    bionic_powercost = 0;
    body_part_type::type leg_type = body_part_type::type::leg;
    for( const bodypart_id &bp : get_all_body_parts_of_type( leg_type ) ) {
        total_limb_count++;
        if( bp->has_flag( json_flag_BIONIC_LIMB ) ) {
            bionic_powercost += bp->power_efficiency;
            bionic_limb_count++;
        }
    }
    legs_power_use = bionic_powercost;
    if( bionic_limb_count > 0 ) {
        legs_stam_mult = 1 - ( bionic_limb_count / total_limb_count );
    } else {
        legs_stam_mult = 1.0f;
    }
    //sanity check ourselves in debug
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "Total legs in use: %.1f, Bionic legs: %.1f",
                   total_limb_count,
                   bionic_limb_count );
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "bionic power per legs stamina: %d", legs_power_use );
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "legs stam usage mult by %.1f", legs_stam_mult );
}

void Character::burn_energy_arms( int mod )
{
    mod_stamina( mod * get_arms_stam_mult() );
    if( get_arms_power_use() > 0 ) {
        mod_power_level( units::from_millijoule( mod * get_arms_power_use() ) );
    }
}

void Character::burn_energy_legs( int mod )
{
    mod_stamina( mod * get_legs_stam_mult() );
    if( get_legs_power_use() > 0 ) {
        mod_power_level( units::from_millijoule( mod * get_legs_power_use() ) );
    }
}

void Character::burn_energy_all( int mod )
{
    mod_stamina( mod * ( get_arms_stam_mult() + get_legs_stam_mult() ) * 0.5f );
    if( ( get_arms_power_use() + get_legs_power_use() ) > 0 ) {
        mod_power_level( units::from_millijoule( mod * ( get_arms_power_use() + get_legs_power_use() ) ) );
    }
}

void Character::mod_stamina( int mod )
{
    // TODO: Make NPCs smart enough to use stamina
    if( is_npc() || has_trait( trait_DEBUG_STAMINA ) ) {
        return;
    }

    float quarter_thresh = 0.25 * get_stamina_max();
    float half_thresh = 0.5 * get_stamina_max();

    int quarter_stam_counter = get_value( "quarter_stam_counter" ).dbl();

    if( stamina > half_thresh && stamina + mod < half_thresh ) {
        set_value( "got_to_half_stam", "true" );
    }

    if( stamina > quarter_thresh && stamina + mod < quarter_thresh && quarter_stam_counter < 5 ) {
        quarter_stam_counter++;
        set_value( "quarter_stam_counter", quarter_stam_counter );
        mod_daily_health( 1, 5 );
    }

    stamina += mod;
    if( stamina < 0 ) {
        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( !bp->windage_effect.is_null() ) {
                add_effect( bp->windage_effect, 10_turns );
            }
        }
    }
    stamina = clamp( stamina, 0, get_stamina_max() );
}

void Character::burn_move_stamina( int moves )
{
    map &here = get_map();
    int overburden_percentage = 0;
    //add half the difference between current stored kcal weight and healthy stored kcal weight to weight of carried gear
    units::mass fat_penalty = units::from_kilogram( 0.5f * std::max( 0.0f,
                              ( get_healthy_kcal() - get_stored_kcal() ) / KCAL_PER_KG ) );
    units::mass current_weight = weight_carried() + fat_penalty;
    // Make it at least 1 gram to avoid divide-by-zero warning
    units::mass max_weight = std::max( weight_capacity(), 1_gram );
    if( current_weight > max_weight ) {
        overburden_percentage = ( current_weight - max_weight ) * 100 / max_weight;
    }

    int burn_ratio = get_option<int>( "PLAYER_BASE_STAMINA_BURN_RATE" );
    for( const bionic_id &bid : get_bionic_fueled_with_muscle() ) {
        if( has_active_bionic( bid ) ) {
            burn_ratio = burn_ratio * 2 - 3;
        }
    }
    burn_ratio += overburden_percentage;

    ///\EFFECT_SWIMMING decreases stamina burn when swimming
    //Appropriate traits let you walk along the bottom without getting as tired
    if( here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, pos_bub( here ) ) &&
        !has_flag( json_flag_WATERWALKING ) &&
        ( !has_flag( json_flag_WALK_UNDERWATER ) ||
          here.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, pos_bub( here ) ) ) &&
        !here.has_flag_furn( "BRIDGE", pos_bub( here ) ) &&
        !( in_vehicle && here.veh_at( pos_bub( here ) )->vehicle().can_float( here ) ) ) {
        burn_ratio += 100 / std::pow( 1.1, get_skill_level( skill_swimming ) );
    }

    burn_ratio *= move_mode->stamina_mult();
    burn_energy_legs( -( ( moves * burn_ratio ) / 100.0 ) * get_modifier(
                          character_modifier_stamina_move_cost_mod ) * get_modifier(
                          character_modifier_move_mode_move_cost_mod ) );
    add_msg_debug( debugmode::DF_CHARACTER, "Stamina burn: %d", -( ( moves * burn_ratio ) / 100 ) );
    // Chance to suffer pain if overburden and stamina runs out or has trait BADBACK
    // Starts at 1 in 25, goes down by 5 for every 50% more carried
    if( ( current_weight > max_weight ) && ( has_trait( trait_BADBACK ) || get_stamina() == 0 ) &&
        one_in( 35 - 5 * current_weight / ( max_weight / 2 ) ) ) {
        add_msg_if_player( m_bad, _( "Your body strains under the weight!" ) );
        // 1 more pain for every 800 grams more (5 per extra STR needed)
        if( ( ( current_weight - max_weight ) / 800_gram > get_pain() && get_pain() < 100 ) ) {
            mod_pain( 1 );
        }
    }
}

void Character::update_stamina( int turns )
{
    static const std::string player_base_stamina_regen_rate( "PLAYER_BASE_STAMINA_REGEN_RATE" );
    const float base_regen_rate = get_option<float>( player_base_stamina_regen_rate );
    // Your stamina regen rate works as a function of how fit you are compared to your body size.
    // This allows it to scale more quickly than your stamina, so that at higher fitness levels you
    // recover stamina faster.
    const float effective_regen_rate = base_regen_rate * get_cardiofit() / get_cardio_acc_base();
    const int current_stim = get_stim();
    // Values above or below normal will increase or decrease stamina regen
    const float mod_regen = enchantment_cache->modify_value( enchant_vals::mod::STAMINA_REGEN_MOD, 0 );
    // Mutated stamina works even when winded
    const float base_multiplier = mod_regen + ( has_effect( effect_winded ) ? 0.1f : 1.0f );
    // Ensure multiplier is at least 0.1
    const float stamina_multiplier = std::max<float>( 0.1f, base_multiplier );

    // Recover some stamina every turn. Start with zero, then increase recovery factor based on
    // mutations, stimulants, and bionics before rolling random recovery based on this factor.
    float stamina_recovery = 0.0f;
    // But mouth encumbrance interferes, even with mutated stamina.
    stamina_recovery += stamina_multiplier * std::max( 1.0f,
                        effective_regen_rate * get_modifier( character_modifier_stamina_recovery_breathing_mod ) );
    // only apply stim-related and mutant stamina boosts if you don't have bionic lungs
    if( !has_bionic( bio_synlungs ) ) {
        stamina_recovery = enchantment_cache->modify_value( enchant_vals::mod::REGEN_STAMINA,
                           stamina_recovery );
        // TODO: recovering stamina causes hunger/thirst/sleepiness.
        // TODO: Tiredness slowing recovery

        // stim recovers stamina (or impairs recovery)
        if( current_stim > 0 ) {
            // TODO: Make stamina recovery with stims cost health
            stamina_recovery += std::min( 5.0f, current_stim / 15.0f );
        } else if( current_stim < 0 ) {
            // Affect it less near 0 and more near full
            // Negative stim kill at -200
            // At -100 stim it inflicts -20 malus to regen at 100%  stamina,
            // effectivly countering stamina gain of default 20,
            // at 50% stamina its -10 (50%), cuts by 25% at 25% stamina
            stamina_recovery += current_stim / 5.0f * get_stamina() / get_stamina_max();
        }
    }
    const int max_stam = get_stamina_max();
    if( get_power_level() >= 3_kJ && has_active_bionic( bio_gills ) ) {
        int bonus = std::min<int>( units::to_kilojoule( get_power_level() ) / 3,
                                   max_stam - get_stamina() - stamina_recovery * turns );
        // so the effective recovery is up to 5x default
        bonus = std::min( bonus, 4 * static_cast<int>( effective_regen_rate ) );
        if( bonus > 0 ) {
            stamina_recovery += bonus;
            bonus /= 10;
            bonus = std::max( bonus, 1 );
            mod_power_level( units::from_kilojoule( static_cast<std::int64_t>( -bonus ) ) );
        }
    }

    // Roll to determine actual stamina recovery over this period
    int recover_amount = std::ceil( stamina_recovery * turns );
    mod_stamina( recover_amount );
    add_msg_debug( debugmode::DF_CHARACTER, "Stamina recovery: %d", recover_amount );

    // Cap at max
    set_stamina( std::min( std::max( get_stamina(), 0 ), max_stam ) );
}

int Character::get_cardiofit() const
{
    if( is_npc() ) {
        // No point in doing a bunch of checks on NPCs for now since they can't use cardio.
        return 2 * get_cardio_acc_base();
    }

    if( has_bionic( bio_synlungs ) ) {
        // If you have the synthetic lungs bionic your cardioacc is forced to a specific value
        return 3 * get_cardio_acc_base();
    }

    const int cardio_base = get_cardio_acc();

    // Mut mod contains the base 1.0f for all modifiers
    const float mut_mod = enchantment_cache->modify_value( enchant_vals::mod::CARDIO_MULTIPLIER, 1 );
    // 1 point of athletics skill = 1% more cardio, up to 10% cardio
    const float athletics_mod = get_skill_level( skill_swimming ) / 100.0f;
    // At some point we might have proficiencies that affect this.
    const float prof_mod = 0.0f;
    // 10 points of health = 1% cardio, up to 20% cardio
    float health_mod = get_lifestyle() / 1000.0f;

    // Negative effects of health are doubled, up to 40% cardio
    if( health_mod < 0.0f ) {
        health_mod *= 2.0f;
    }

    // Add up all of our cardio mods.
    float cardio_modifier = mut_mod + athletics_mod + prof_mod + health_mod;
    // Modify cardio accumulator by our cardio mods.
    const int cardio_fitness = static_cast<int>( cardio_base * cardio_modifier );

    return cardio_fitness;
}

int Character::get_cardio_acc() const
{
    return cardio_acc;
}

void Character::set_cardio_acc( int ncardio_acc )
{
    cardio_acc = ncardio_acc;
}

void Character::reset_cardio_acc()
{
    set_cardio_acc( get_cardio_acc_base() );
}

int Character::get_cardio_acc_base() const
{
    return base_cardio_acc;
}

bool Character::invoke_item( item *used )
{
    return invoke_item( used, pos_bub() );
}

bool Character::invoke_item( item *, const tripoint_bub_ms &, int )
{
    return false;
}

bool Character::invoke_item( item *used, const std::string &method )
{
    return invoke_item( used, method, pos_bub() );
}

bool Character::invoke_item( item *used, const std::string &method, const tripoint_bub_ms &pt,
                             int pre_obtain_moves )
{
    if( method.empty() ) {
        return invoke_item( used, pt, pre_obtain_moves );
    }

    if( used->is_broken() ) {
        add_msg_if_player( m_bad, _( "Your %s was broken and won't turn on." ), used->tname() );
        return false;
    }
    if( !used->ammo_sufficient( this, method ) ) {
        int ammo_req = used->ammo_required();
        std::string it_name = used->tname();
        if( used->has_flag( flag_USE_UPS ) ) {
            add_msg_if_player( m_info,
                               n_gettext( "Your %s needs %d charge from some UPS.",
                                          "Your %s needs %d charges from some UPS.",
                                          ammo_req ),
                               it_name, ammo_req );
        } else if( used->has_flag( flag_USES_BIONIC_POWER ) ) {
            add_msg_if_player( m_info,
                               n_gettext( "Your %s needs %d kJ of bionic power.",
                                          "Your %s needs %d kJ of bionic power.",
                                          ammo_req ),
                               it_name, ammo_req );
        } else {
            int ammo_rem = used->ammo_remaining( );
            add_msg_if_player( m_info,
                               n_gettext( "Your %s has %d charge, but needs %d.",
                                          "Your %s has %d charges, but needs %d.",
                                          ammo_rem ),
                               it_name, ammo_rem, ammo_req );
        }
        set_moves( pre_obtain_moves );
        return false;
    }

    item *actually_used = used->get_usable_item( method );
    if( actually_used == nullptr ) {
        debugmsg( "Tried to invoke a method %s on item %s, which doesn't have this method",
                  method.c_str(), used->tname() );
        set_moves( pre_obtain_moves );
        return false;
    }

    std::optional<int> charges_used = actually_used->type->invoke( this, *actually_used,
                                      pt, method );
    if( !charges_used.has_value() ) {
        set_moves( pre_obtain_moves );
        return false;
    }

    if( charges_used.value() == 0 ) {
        // Not really used.
        // The item may also have been deleted
        return false;
    }

    if( actually_used->is_comestible() ) {
        const bool ret = consume_effects( *used );
        const int consumed = used->activation_consume( charges_used.value(), pt, this );
        if( consumed == 0 ) {
            // Nothing was consumed from within the item. "Eat" the item itself away.
            i_rem( actually_used );
        }
        return ret;
    }

    actually_used->activation_consume( charges_used.value(), pt, this );

    if( actually_used->has_flag( flag_SINGLE_USE ) || actually_used->is_bionic() ||
        actually_used->is_deployable() ) {
        i_rem( actually_used );
        return true;
    }

    return false;
}

bool Character::consume_charges( item &used, int qty )
{
    if( qty < 0 ) {
        debugmsg( "Tried to consume negative charges" );
        return false;
    }

    if( qty == 0 ) {
        return false;
    }

    if( !used.is_tool() && !used.is_food() && !used.is_medication() ) {
        debugmsg( "Tried to consume charges for non-tool, non-food, non-med item" );
        return false;
    }

    // Consume comestibles destroying them if no charges remain
    if( used.is_food() || used.is_medication() ) {
        used.charges -= qty;
        if( used.charges <= 0 ) {
            i_rem( &used );
            return true;
        }
        return false;
    }

    // Tools which don't require ammo are instead destroyed
    if( used.is_tool() && !used.ammo_required() ) {
        if( has_item( used ) ) {
            i_rem( &used );
        } else {
            map_stack items = get_map().i_at( pos_bub() );
            for( item_stack::iterator iter = items.begin(); iter != items.end(); iter++ ) {
                if( &( *iter ) == &used ) {
                    iter = items.erase( iter );
                    break;
                }
            }
        }
        return true;
    }

    used.ammo_consume( qty, pos_bub(), this );
    return false;
}

int Character::item_handling_cost( const item &it, bool penalties, int base_cost,
                                   int charges_in_it, bool bulk_cost ) const
{
    int mv = base_cost;
    if( penalties ) {
        // 50 moves per liter, up to 200 at 4 liters
        mv += std::min( 200, it.volume( false, false, charges_in_it ) / 20_ml );
    }

    if( weapon.typeId() == itype_e_handcuffs ) {
        mv *= 4;
    } else if( penalties && has_flag( json_flag_GRAB ) ) {
        // Grabbed penalty scales for grabbed arms/hands
        int pen = 2;
        for( const effect &eff : get_effects_with_flag( json_flag_GRAB ) ) {
            if( eff.get_bp()->primary_limb_type() == body_part_type::type::arm ||
                eff.get_bp()->primary_limb_type() == body_part_type::type::hand ) {
                pen++;
            }
        }
        mv *= pen;
    }

    // The following is considered as overhead and is not included in the cost when handling multiple items at once.
    if( !bulk_cost ) {
        // For single handed items use the least encumbered hand
        if( it.is_two_handed( *this ) ) {
            for( const bodypart_id &part : get_all_body_parts_of_type( body_part_type::type::hand ) ) {
                mv += encumb( part );
            }
        } else {
            int min_encumb = INT_MAX;
            for( const bodypart_id &part : get_all_body_parts_of_type( body_part_type::type::hand ) ) {
                min_encumb = std::min( min_encumb, encumb( part ) );
            }
            mv += min_encumb;
        }
    }

    return std::max( mv, 0 );
}

int Character::item_store_cost( const item &it, const item & /* container */, bool penalties,
                                int base_cost ) const
{
    /** @EFFECT_PISTOL decreases time taken to store a pistol */
    /** @EFFECT_SMG decreases time taken to store an SMG */
    /** @EFFECT_RIFLE decreases time taken to store a rifle */
    /** @EFFECT_SHOTGUN decreases time taken to store a shotgun */
    /** @EFFECT_LAUNCHER decreases time taken to store a launcher */
    /** @EFFECT_STABBING decreases time taken to store a stabbing weapon */
    /** @EFFECT_CUTTING decreases time taken to store a cutting weapon */
    /** @EFFECT_BASHING decreases time taken to store a bashing weapon */
    float lvl = get_skill_level( it.is_gun() ? it.gun_skill() : it.melee_skill() );
    return item_handling_cost( it, penalties, base_cost ) / ( ( lvl + 10.0f ) / 10.0f );
}

int Character::item_retrieve_cost( const item &it, const item &container, bool penalties,
                                   int base_cost ) const
{
    // Drawing from an holster use the same formula as storing an item for now
    /**
         * @EFFECT_PISTOL decreases time taken to draw pistols from holsters
         * @EFFECT_SMG decreases time taken to draw smgs from holsters
         * @EFFECT_RIFLE decreases time taken to draw rifles from holsters
         * @EFFECT_SHOTGUN decreases time taken to draw shotguns from holsters
         * @EFFECT_LAUNCHER decreases time taken to draw launchers from holsters
         * @EFFECT_STABBING decreases time taken to draw stabbing weapons from sheathes
         * @EFFECT_CUTTING decreases time taken to draw cutting weapons from scabbards
         * @EFFECT_BASHING decreases time taken to draw bashing weapons from holsters
         */
    return item_store_cost( it, container, penalties, base_cost );
}

void Character::cough( bool harmful, int loudness )
{
    if( has_effect( effect_cough_suppress ) ) {
        return;
    }

    if( harmful ) {
        const int stam = get_stamina();
        const int malus = get_stamina_max() * 0.05; // 5% max stamina
        mod_stamina( -malus );
        if( stam < malus && x_in_y( malus - stam, malus ) && one_in( 6 ) ) {
            apply_damage( nullptr, body_part_torso, 1 );
        }
    }

    if( !is_npc() ) {
        add_msg( m_bad, _( "You cough heavily." ) );
    }
    sounds::sound( pos_bub(), loudness, sounds::sound_t::speech, _( "a hacking cough." ), false, "misc",
                   "cough" );

    mod_moves( -get_speed() * 0.8 );

    add_effect( effect_disrupted_sleep, 5_minutes );
}

void Character::wake_up()
{
    //Can't wake up if under anesthesia
    if( has_effect( effect_narcosis ) ) {
        return;
    }

    // Do not remove effect_sleep or effect_alarm_clock now otherwise it invalidates an effect
    // iterator in player::process_effects().
    // We just set it for later removal (also happening in player::process_effects(), so no side
    // effects) with a duration of 0 turns.

    if( has_effect( effect_sleep ) ) {
        get_effect( effect_sleep ).set_duration( 0_turns );
        get_event_bus().send<event_type::character_wakes_up>( getID() );
    }
    remove_effect( effect_slept_through_alarm );
    remove_effect( effect_lying_down );
    if( has_effect( effect_alarm_clock ) ) {
        get_effect( effect_alarm_clock ).set_duration( 0_turns );
    }
    recalc_sight_limits();

    if( movement_mode_is( move_mode_prone ) ) {
        set_movement_mode( move_mode_walk );
    }
}

int Character::get_shout_volume() const
{
    int base = 10;
    int shout_multiplier = 2;

    base = enchantment_cache->modify_value( enchant_vals::mod::SHOUT_NOISE, base );
    shout_multiplier = enchantment_cache->modify_value( enchant_vals::mod::SHOUT_NOISE_STR_MULT,
                       shout_multiplier );

    // You can't shout without your face
    if( has_trait( trait_PROF_FOODP ) && !( is_wearing( itype_foodperson_mask ) ||
                                            is_wearing( itype_foodperson_mask_on ) ) ) {
        base = 0;
        shout_multiplier = 0;
    }

    // Masks and such dampen the sound
    // Balanced around whisper for wearing bondage mask
    // and noise ~= 10 (door smashing) for wearing dust mask for character with strength = 8
    /** @EFFECT_STR increases shouting volume */
    int noise = ( base + str_cur * shout_multiplier ) * get_limb_score( limb_score_breathing );

    // Minimum noise volume possible after all reductions.
    // Volume 1 can't be heard even by player
    constexpr int minimum_noise = 2;

    if( noise <= base ) {
        noise = std::max( minimum_noise, noise );
    }

    // Screaming underwater is not good for oxygen and harder to do overall
    if( underwater ) {
        noise = std::max( minimum_noise, noise / 2 );
    }
    return noise;
}

void Character::shout( std::string msg, bool order )
{
    int base = 10;
    std::string shout;

    // You can't shout without your face
    if( has_trait( trait_PROF_FOODP ) && !( is_wearing( itype_foodperson_mask ) ||
                                            is_wearing( itype_foodperson_mask_on ) ) ) {
        add_msg_if_player( m_warning, _( "You try to shout, but you have no face!" ) );
        return;
    }

    // Mutations make shouting louder, they also define the default message
    if( has_trait( trait_SHOUT3 ) ) {
        base = 20;
        if( msg.empty() ) {
            msg = is_avatar() ? _( "yourself let out a piercing howl!" ) : _( "a piercing howl!" );
            shout = "howl";
        }
    } else if( has_trait( trait_SHOUT2 ) ) {
        base = 15;
        if( msg.empty() ) {
            msg = is_avatar() ? _( "yourself scream loudly!" ) : _( "a loud scream!" );
            shout = "scream";
        }
    }

    if( msg.empty() ) {
        msg = is_avatar() ? _( "yourself shout loudly!" ) : _( "a loud shout!" );
        shout = "default";
    }
    int noise = get_shout_volume();

    // Minimum noise volume possible after all reductions.
    // Volume 1 can't be heard even by player
    constexpr int minimum_noise = 2;

    if( noise <= base ) {
        std::wstring wstr( utf8_to_wstr( msg ) );
        std::transform( wstr.begin(), wstr.end(), wstr.begin(), towlower );
        msg = wstr_to_utf8( wstr );
    }

    // Screaming underwater is not good for oxygen and harder to do overall
    if( underwater ) {
        if( !has_trait( trait_GILLS ) && !has_trait( trait_GILLS_CEPH ) ) {
            mod_stat( "oxygen", -noise );
        }
    }

    // TODO: indistinct noise descriptions should be handled in the sounds code
    if( noise <= minimum_noise ) {
        add_msg_if_player( m_warning,
                           _( "The sound of your voice is almost completely muffled!" ) );
        msg = is_avatar() ? _( "your muffled shout" ) : _( "an indistinct voice" );
    } else if( get_limb_score( limb_score_breathing ) < 0.5f ) {
        // The shout's volume is 1/2 or lower of what it would be without the penalty
        add_msg_if_player( m_warning, _( "The sound of your voice is significantly muffled!" ) );
    }

    sounds::sound( pos_bub(), noise, order ? sounds::sound_t::order : sounds::sound_t::alert, msg,
                   false,
                   "shout", shout );
}

void Character::signal_nemesis()
{
    const tripoint_abs_omt ompos = pos_abs_omt();
    const tripoint_abs_sm smpos = project_to<coords::sm>( ompos );
    overmap_buffer.signal_nemesis( smpos );
}

void Character::vomit()
{
    get_event_bus().send<event_type::throws_up>( getID() );

    if( stomach.contains() != 0_ml ) {
        get_map().add_field( adjacent_tile(), fd_bile, 1 );
        add_msg_player_or_npc( m_bad, _( "You throw up heavily!" ), _( "<npcname> throws up heavily!" ) );
    }
    // needed to ensure digesting vitamin_type::DRUGs are also emptied, even on an empty stomach.
    stomach.empty();


    if( !has_effect( effect_nausea ) ) {  // Prevents never-ending nausea
        const effect dummy_nausea( effect_source( this ), &effect_nausea.obj(), 0_turns,
                                   bodypart_str_id::NULL_ID(), false, 1, calendar::turn );
        add_effect( effect_nausea, std::max( dummy_nausea.get_max_duration() * units::to_milliliter(
                stomach.contains() ) / 21, dummy_nausea.get_int_dur_factor() ) );
    }

    mod_moves( -100 );
    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            effect &it = _effect_it.second;
            if( it.get_id() == effect_foodpoison ) {
                it.mod_duration( -30_minutes );
            } else if( it.get_id() == effect_drunk ) {
                it.mod_duration( rng( -10_minutes, -50_minutes ) );
            }
        }
    }
    remove_effect( effect_pkill1_generic );
    remove_effect( effect_pkill1_acetaminophen );
    remove_effect( effect_pkill1_nsaid );
    remove_effect( effect_pkill2 );
    remove_effect( effect_pkill3 );
    // Don't wake up when just retching
    if( stomach.contains() > 0_ml ) {
        wake_up();
    }
}

// adjacent_tile() returns a safe, unoccupied adjacent tile. If there are no such tiles, returns player position instead.
tripoint_bub_ms Character::adjacent_tile() const
{
    std::vector<tripoint_bub_ms> ret;
    ret.reserve( 4 );
    int dangerous_fields = 0;
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_bub_ms &p : here.points_in_radius( pos_bub(), 1 ) ) {
        if( p == pos_bub() ) {
            // Don't consider player position
            continue;
        }
        if( creatures.creature_at( p ) != nullptr ) {
            continue;
        }
        if( here.impassable( p ) ) {
            continue;
        }
        const trap &curtrap = here.tr_at( p );
        // If we don't known a trap here, the spot "appears" to be good, so consider it.
        // Same if we know a benign trap (as it's not dangerous).
        if( curtrap.can_see( p, *this ) && !curtrap.is_benign() ) {
            continue;
        }
        // Only consider tile if unoccupied, passable and has no traps
        dangerous_fields = 0;
        field &tmpfld = here.field_at( p );
        for( auto &fld : tmpfld ) {
            const field_entry &cur = fld.second;
            if( cur.is_dangerous() ) {
                dangerous_fields++;
            }
        }

        if( dangerous_fields == 0 ) {
            ret.push_back( p );
        }
    }

    return random_entry( ret, pos_bub() ); // player position if no valid adjacent tiles
}

void Character::set_fac_id( const std::string &my_fac_id )
{
    fac_id = faction_id( my_fac_id );
}

std::string get_stat_name( character_stat Stat )
{
    switch( Stat ) {
        // *INDENT-OFF*
    case character_stat::STRENGTH:     return pgettext( "strength stat", "STR" );
    case character_stat::DEXTERITY:    return pgettext( "dexterity stat", "DEX" );
    case character_stat::INTELLIGENCE: return pgettext( "intelligence stat", "INT" );
    case character_stat::PERCEPTION:   return pgettext( "perception stat", "PER" );
        // *INDENT-ON*
        default:
            break;
    }
    return pgettext( "fake stat there's an error", "ERR" );
}

int Character::mutation_height( const trait_id &mut ) const
{
    const mutation_branch &mdata = mut.obj();
    int height_prereqs = 0;
    int height_prereqs2 = 0;
    for( const trait_id &prereq : mdata.prereqs ) {
        height_prereqs = std::max( mutation_height( prereq ), height_prereqs );
    }
    for( const trait_id &prereq : mdata.prereqs2 ) {
        height_prereqs2 = std::max( mutation_height( prereq ), height_prereqs2 );
    }
    return std::max( height_prereqs, height_prereqs2 ) + 1;
}

void Character::calc_mutation_levels()
{
    mutation_category_level.clear();
    // This intentionally counts multiple leads_to mutations
    // from the same root mutation as having full height

    // For each of our mutations...
    for( const trait_id &mut : get_mutations( true, true ) ) {
        const mutation_branch &mdata = mut.obj();
        if( mdata.threshold ) {
            // Thresholds are worth 10, and technically support multiple categories
            for( const mutation_category_id &cat : mdata.category ) {
                mutation_category_level[cat] += 10;
            }
            continue;
        }
        // ... if we don't have a valid addititive version...
        bool have_upgrade = false;
        for( const trait_id &upgrade : mdata.additions ) {
            const mutation_branch &upgradedata = upgrade.obj();
            if( has_trait( upgrade ) && !upgradedata.flags.count( json_flag_NON_THRESH ) ) {
                have_upgrade = true;
                break;
            }
        }
        if( !have_upgrade && !mdata.flags.count( json_flag_NON_THRESH ) ) {
            // ... find the mutations distance from normalcy...
            int mut_height = mutation_height( mut );
            // ... and for each category it falls in...
            for( const mutation_category_id &cat : mdata.category ) {
                // ... add the height and a constant value
                mutation_category_level[cat] += mut_height + 2;
            }
        }
    }
}

void Character::drench_mut_calc()
{
    for( const bodypart_id &bp : get_all_body_parts() ) {
        int ignored = 0;
        int neutral = 0;
        int good = 0;

        for( const trait_id &iter : get_functioning_mutations() ) {
            const mutation_branch &mdata = iter.obj();
            const auto wp_iter = mdata.protection.find( bp.id() );
            if( wp_iter != mdata.protection.end() ) {
                ignored += wp_iter->second.x;
                neutral += wp_iter->second.y;
                good += wp_iter->second.z;
            }
        }
        set_part_mut_drench( bp, { WT_GOOD, good } );
        set_part_mut_drench( bp, { WT_NEUTRAL, neutral } );
        set_part_mut_drench( bp, { WT_IGNORED, ignored } );
    }
}

/// Returns a weighted list of all mutation categories with current blood vitamin levels
weighted_int_list<mutation_category_id> Character::get_vitamin_weighted_categories() const
{
    weighted_int_list<mutation_category_id> weighted_output;
    for( const auto &elem : mutation_category_trait::get_all() ) {
        add_msg_debug( debugmode::DF_MUTATION, "get_vitamin_weighted_categories: category %s weight %d",
                       elem.second.id.c_str(), vitamin_get( elem.second.vitamin ) );
        weighted_output.add( elem.first, vitamin_get( elem.second.vitamin ) );
    }
    return weighted_output;
}

int Character::vitamin_RDA( const vitamin_id &vitamin, int ammount ) const
{
    const double multiplier = vitamin_rate( vitamin ) * 100 / 1_days;
    return std::lround( ammount * multiplier );
}

void Character::recalculate_bodyparts()
{
    body_part_set body_set;
    for( const bodypart_id &bp : creature_anatomy->get_bodyparts() ) {
        body_set.set( bp.id() );
    }
    body_set = enchantment_cache->modify_bodyparts( body_set );
    // first come up with the bodyparts that need to be removed from body
    for( auto bp_iter = body.begin(); bp_iter != body.end(); ) {
        if( !body_set.test( bp_iter->first ) ) {
            bp_iter = body.erase( bp_iter );
        } else {
            ++bp_iter;
        }
    }
    // then add the parts in bodyset that are missing from body
    for( const bodypart_str_id &bp : body_set ) {
        if( body.find( bp ) == body.end() ) {
            body[bp] = bodypart( bp );
        }
    }
    tally_organic_size();
    recalc_limb_energy_usage();

    add_msg_debug( debugmode::DF_ANATOMY_BP, "New healthy kcal %d",
                   get_healthy_kcal() );
    calc_encumbrance();
    add_msg_debug( debugmode::DF_ANATOMY_BP, "New stored kcal %d",
                   get_stored_kcal() );
    add_msg_debug( debugmode::DF_ANATOMY_BP, "New organic size %.1f",
                   get_cached_organic_size() );
}

void Character::recalculate_enchantment_cache()
{
    enchantment_cache->clear();

    cache_visit_items_with( "is_relic", &item::is_relic, [this]( const item & it ) {
        for( const enchant_cache &ench : it.get_proc_enchantments() ) {
            if( ench.is_active( *this, it ) ) {
                enchantment_cache->force_add( ench );
            }
        }
        for( const enchantment &ench : it.get_defined_enchantments() ) {
            if( ench.is_active( *this, it ) ) {
                enchantment_cache->force_add( ench, *this );
            }
        }
    } );


    for( const bionic &bio : *my_bionics ) {
        const bionic_id &bid = bio.id;

        for( const enchantment_id &ench_id : bid->enchantments ) {
            const enchantment &ench = ench_id.obj();
            if( ench.is_active( *this, bio.powered &&
                                bid->has_flag( STATIC( json_character_flag( "BIONIC_TOGGLED" ) ) ) ) ) {
                enchantment_cache->force_add( ench, *this );
            }
        }
    }

    for( const auto &elem : *effects ) {
        for( const enchantment_id &ench_id : elem.first->enchantments ) {
            const enchantment &ench = ench_id.obj();
            if( ench.is_active( *this, true ) ) {
                enchantment_cache->force_add( ench, *this );
            }
        }
    }

    for( const std::pair<const trait_id, trait_data> &mut_map : my_mutations ) {
        if( mut_map.second.corrupted == 0 ) {
            const mutation_branch &mut = mut_map.first.obj();
            for( const enchantment_id &ench_id : mut.enchantments ) {
                const enchantment &ench = ench_id.obj();
                if( ench.is_active( *this, mut.activated && mut_map.second.powered ) ) {
                    enchantment_cache->force_add_mutation( ench );
                }
            }
        }
    }
    new_mutation_cache->mutations = enchantment_cache->mutations;
    update_cached_mutations();
    for( const std::pair<const trait_id, trait_data> &mut_map : cached_mutations ) {
        if( mut_map.second.corrupted == 0 ) {
            const mutation_branch &mut = mut_map.first.obj();
            for( const enchantment_id &ench_id : mut.enchantments ) {
                const enchantment &ench = ench_id.obj();
                if( ench.is_active( *this, mut.activated && mut_map.second.powered ) ) {
                    enchantment_cache->force_add( ench, *this );
                }
            }
        }
    }

    if( enchantment_cache->modifies_bodyparts() ) {
        recalculate_bodyparts();
    }

    // do final statistic recalculations
    if( get_stamina() > get_stamina_max() ) {
        set_stamina( get_stamina_max() );
    }
    recalc_hp();
}

void Character::update_cached_mutations()
{
    const std::vector<trait_id> &new_muts = new_mutation_cache->get_mutations();
    const std::vector<trait_id> &old_muts = old_mutation_cache->get_mutations();
    if( my_mutations_dirty.empty() ) {
        if( cached_mutations.empty() ) {
            // migration from old save
            cached_mutations = my_mutations;
        }
        if( new_muts.empty() && old_muts.empty() ) {
            // Quick bail out as no changes happened.
            return;
        }
    }

    if( !old_muts.empty() ) {
        for( const trait_id &it : old_muts ) {
            auto remains = find( new_muts.begin(), new_muts.end(), it );
            if( remains == new_muts.end() ) {
                mutations_to_remove.insert( it );
            } else {
                for( auto &iter : my_mutations_dirty ) {
                    if( are_opposite_traits( it, iter.first ) || b_is_lower_trait_of_a( it, iter.first )
                        || are_same_type_traits( it, iter.first ) ) {
                        ++iter.second.corrupted;
                        ++my_mutations[it].corrupted;
                    }
                }
            }
        }
        for( auto &iter : my_mutations_dirty ) {
            cached_mutations.emplace( iter );
            if( iter.second.corrupted == 0 ) {
                mutation_effect( iter.first, false );
                do_mutation_updates();
            }
        }
        for( const trait_id &it : new_muts ) {
            auto exists = find( old_muts.begin(), old_muts.end(), it );
            if( exists == old_muts.end() ) {
                mutations_to_add.insert( it );
            }
        }
    } else {
        for( auto &it : my_mutations_dirty ) {
            cached_mutations.emplace( it );
            mutation_effect( it.first, false );
            do_mutation_updates();
        }
        for( const trait_id &it : new_muts ) {
            if( !cached_mutations.count( it ) ) {
                mutations_to_add.insert( it );
            }
        }
    }
    my_mutations_dirty.clear();
    old_mutation_cache = new_mutation_cache;
    new_mutation_cache->clear();
    if( mutations_to_add.empty() && mutations_to_remove.empty() ) {
        // Quick bail out as no changes happened.
        return;
    }

    const std::vector<trait_id> &current_traits = get_mutations();
    for( const trait_id &mut : mutations_to_remove ) {
        // check if the player still has a mutation
        // since a trait from an item might be provided by another item as well
        auto it = std::find( current_traits.begin(), current_traits.end(), mut );
        if( it == current_traits.end() ) {
            for( auto &iter : cached_mutations ) {
                if( ( are_opposite_traits( iter.first, mut ) || b_is_higher_trait_of_a( iter.first, mut )
                      || are_same_type_traits( iter.first, mut ) ) && iter.second.corrupted > 0 ) {
                    --iter.second.corrupted;
                    if( my_mutations.count( iter.first ) ) {
                        --my_mutations[iter.first].corrupted;
                    }
                    if( iter.second.corrupted == 0 ) {
                        mutation_effect( iter.first, false );
                        do_mutation_updates();
                    }
                }
            }
            cached_mutations.erase( mut );
            mutation_loss_effect( mut );
            do_mutation_updates();
        }
    }
    for( const trait_id &mut : mutations_to_add ) {
        for( auto &iter : cached_mutations ) {
            if( are_opposite_traits( iter.first, mut ) || b_is_higher_trait_of_a( iter.first, mut )
                || are_same_type_traits( iter.first, mut ) ) {
                ++iter.second.corrupted;
                if( my_mutations.count( iter.first ) ) {
                    ++my_mutations[iter.first].corrupted;
                }
                if( iter.second.corrupted == 1 ) {
                    mutation_loss_effect( iter.first );
                    do_mutation_updates();
                }
            }
        }
        cached_mutations.emplace( mut, trait_data() );
        mutation_effect( mut, true );
        do_mutation_updates();
    }
    mutations_to_add.clear();
    mutations_to_remove.clear();
    trait_flag_cache.clear();
}

void Character::passive_absorb_hit( const bodypart_id &bp, damage_unit &du ) const
{
    // >0 check because some mutations provide negative armor
    // Thin skin check goes before subdermal armor plates because SUBdermal
    if( du.amount > 0.0f ) {
        du.amount -= mutation_armor( bp, du );
        du.amount -= bp->damage_resistance( du );
    }
    du.amount -= bionic_armor_bonus( bp, du.type ); //Check for passive armor bionics
    du.amount -= mabuff_armor_bonus( du.type );
    du.amount = std::max( 0.0f, du.amount );
}

void Character::did_hit( Creature &target )
{
    enchantment_cache->cast_hit_you( *this, target );
}

ret_val<void> Character::can_wield( const item &it ) const
{
    if( it.has_flag( flag_INTEGRATED ) ) {
        return ret_val<void>::make_failure( _( "You can't wield a part of your body." ) );
    }
    if( has_effect( effect_incorporeal ) ) {
        return ret_val<void>::make_failure( _( "You can't wield anything while incorporeal." ) );
    }
    if( !has_min_manipulators() ) {
        return ret_val<void>::make_failure(
                   _( "You need at least one arm available to even consider wielding something." ) );
    }
    if( it.made_of( phase_id::LIQUID ) ) {
        return ret_val<void>::make_failure( _( "You can't wield spilt liquids." ) );
    }
    if( it.made_of( phase_id::GAS ) ) {
        return ret_val<void>::make_failure( _( "You can't wield loose gases." ) );
    }
    if( it.is_frozen_liquid() && !it.has_flag( flag_SHREDDED ) ) {
        return ret_val<void>::make_failure( _( "You can't wield unbroken frozen liquids." ) );
    }
    if( it.has_flag( flag_NO_UNWIELD ) ) {
        if( get_wielded_item() && get_wielded_item().get_item() == &it ) {
            return ret_val<void>::make_failure( _( "You can't unwield this." ) );
        }
        return ret_val<void>::make_failure(
                   _( "You can't wield this.  Wielding it would make it impossible to unwield it." ) );
    }
    if( it.has_flag( flag_BIONIC_WEAPON ) ) {
        return ret_val<void>::make_failure(
                   _( "You can't wield this.  It looks like it has to be attached to a bionic." ) );
    }

    if( is_armed() && !can_unwield( weapon ).success() ) {
        return ret_val<void>::make_failure( _( "The %s prevents you from wielding the %s." ),
                                            weapname(), it.tname() );
    }
    monster *mount = mounted_creature.get();
    if( it.is_two_handed( *this ) && ( !has_two_arms_lifting() ||
                                       worn_with_flag( flag_RESTRICT_HANDS ) ) &&
        !( is_mounted() && mount->has_flag( mon_flag_RIDEABLE_MECH ) &&
           mount->type->mech_weapon && it.typeId() == mount->type->mech_weapon ) ) {
        if( worn_with_flag( flag_RESTRICT_HANDS ) ) {
            return ret_val<void>::make_failure(
                       _( "Something you are wearing hinders the use of both hands." ) );
        } else if( it.has_flag( flag_ALWAYS_TWOHAND ) ) {
            return ret_val<void>::make_failure( _( "You can't wield the %s with only one arm." ),
                                                it.tname() );
        } else {
            return ret_val<void>::make_failure( _( "You are too weak to wield the %s with only one arm." ),
                                                it.tname() );
        }
    }
    if( is_mounted() && mount->has_flag( mon_flag_RIDEABLE_MECH ) &&
        mount->type->mech_weapon && it.typeId() != mount->type->mech_weapon ) {
        return ret_val<void>::make_failure( _( "You cannot wield anything while piloting a mech." ) );
    }
    if( controlling_vehicle ) {
        if( worn_with_flag( flag_RESTRICT_HANDS ) ) {
            return ret_val<void>::make_failure(
                       _( "Something you are wearing hinders the use of both hands." ) );
        }
        if( !has_two_arms_lifting() || it.has_flag( flag_ALWAYS_TWOHAND ) ) {
            return ret_val<void>::make_failure( _( "You can't wield your %s while driving." ),
                                                it.tname() );
        }
    }

    return ret_val<void>::make_success();
}

bool Character::has_wield_conflicts( const item &it ) const
{
    return is_wielding( it ) || ( is_armed() && !it.can_combine( weapon ) );
}

bool Character::unwield()
{
    if( weapon.is_null() ) {
        return true;
    }

    if( !can_unwield( weapon ).success() ) {
        return false;
    }

    // currently the only way to unwield NO_UNWIELD weapon is if it's a bionic that can be deactivated
    if( weapon.has_flag( flag_NO_UNWIELD ) ) {
        std::optional<bionic *> bio_opt = find_bionic_by_uid( get_weapon_bionic_uid() );
        return bio_opt ? deactivate_bionic( **bio_opt ) : false;
    }

    const std::string query = string_format( _( "Stop wielding %s?" ), weapon.tname() );

    if( !dispose_item( item_location( *this, &weapon ), query ) ) {
        return false;
    }

    inv->unsort();

    return true;
}

std::string Character::weapname() const
{
    std::string name = weapname_simple();
    const std::string mode = weapname_mode();
    const std::string ammo = weapname_ammo();

    if( !mode.empty() ) {
        name = string_format( "%s %s", mode, name );
    }
    if( !ammo.empty() ) {
        name = string_format( "%s %s", name, ammo );
    }

    return name;
}

std::string Character::weapname_simple() const
{
    //To make wield state consistent, gun_nam; when calling tname, is disabling 'with_collapsed' flag
    if( weapon.is_gun() ) {
        gun_mode current_mode = weapon.gun_current_mode();
        const bool no_mode = !current_mode.target;
        tname::segment_bitset segs( tname::default_tname );
        segs.set( tname::segments::TAGS, false );
        std::string gun_name = no_mode ? weapon.display_name() : current_mode->tname( 1, segs );
        return gun_name;

    } else if( !is_armed() ) {
        return _( "fists" );
    } else {
        return weapon.tname();
    }
}

std::string Character::weapname_mode() const
{
    if( weapon.is_gun() ) {
        gun_mode current_mode = weapon.gun_current_mode();
        const bool no_mode = !current_mode.target;
        std::string gunmode;
        if( !no_mode && current_mode->gun_all_modes().size() > 1 ) {
            gunmode = current_mode.tname();
        }
        return gunmode;
    } else {
        return "";
    }
}

std::string Character::weapname_ammo() const
{
    if( weapon.is_gun() ) {
        gun_mode current_mode = weapon.gun_current_mode();
        const bool no_mode = !current_mode.target;
        // only required for empty mags and empty guns
        std::string mag_ammo;

        if( !no_mode && ( current_mode->uses_magazine() || current_mode->magazine_integral() ) ) {
            if( current_mode->uses_magazine() && !current_mode->magazine_current() ) {
                mag_ammo = _( "(empty)" );
            } else {
                int cur_ammo = current_mode->ammo_remaining( );
                int max_ammo;
                if( cur_ammo == 0 ) {
                    max_ammo = current_mode->ammo_capacity( item( current_mode->ammo_default() ).ammo_type() );
                } else {
                    max_ammo = current_mode->ammo_capacity( current_mode->loaded_ammo().ammo_type() );
                }

                const double ratio = static_cast<double>( cur_ammo ) / static_cast<double>( max_ammo );
                nc_color charges_color;
                if( cur_ammo == 0 ) {
                    charges_color = c_light_red;
                } else if( cur_ammo == max_ammo ) {
                    charges_color = c_white;
                } else if( ratio < 1.0 / 3.0 ) {
                    charges_color = c_red;
                } else if( ratio < 2.0 / 3.0 ) {
                    charges_color = c_yellow;
                } else {
                    charges_color = c_light_green;
                }
                mag_ammo = string_format( "(%s)", colorize( string_format( "%i/%i", cur_ammo, max_ammo ),
                                          charges_color ) );
            }
        }

        return mag_ammo;

    } else {
        return "";
    }
}

void Character::on_hit( map *here, Creature *source, bodypart_id bp_hit,
                        float /*difficulty*/, dealt_projectile_attack const *const proj )
{
    check_dead_state( here );
    if( source == nullptr || proj != nullptr ) {
        return;
    }

    // Creature attacking an invisible player will remain aware of their location as long as they keep hitting something
    if( is_avatar() && source->is_monster() && source->has_effect( effect_stumbled_into_invisible ) ) {
        source->as_monster()->stumble_invis( *this, false );
    }

    if( is_npc() ) {
        as_npc()->on_attacked( *source );
    }

    bool u_see = get_player_view().sees( *here, *this );
    const units::energy trigger_cost_base = bio_ods->power_trigger;
    if( has_active_bionic( bio_ods ) && get_power_level() > ( 5 * trigger_cost_base ) ) {
        if( is_avatar() ) {
            add_msg( m_good, _( "Your offensive defense system shocks %s in mid-attack!" ),
                     source->disp_name() );
        } else if( u_see ) {
            add_msg( _( "%1$s's offensive defense system shocks %2$s in mid-attack!" ),
                     disp_name(),
                     source->disp_name() );
        }
        int shock = rng( 1, 4 );
        mod_power_level( -shock * trigger_cost_base );
        damage_instance ods_shock_damage;
        // FIXME: Hardcoded damage type
        ods_shock_damage.add_damage( damage_electric, shock * 5 );
        // Should hit body part used for attack
        source->deal_damage( this, body_part_torso, ods_shock_damage );
    }
    if( !wearing_something_on( bp_hit ) &&
        ( has_trait( trait_SPINES ) || has_trait( trait_QUILLS ) ) ) {
        int spine = rng( 1, has_trait( trait_QUILLS ) ? 20 : 8 );
        if( !is_avatar() ) {
            if( u_see ) {
                add_msg( _( "%1$s's %2$s puncture %3$s in mid-attack!" ), get_name(),
                         ( has_trait( trait_QUILLS ) ? _( "quills" ) : _( "spines" ) ),
                         source->disp_name() );
            }
        } else {
            add_msg( m_good, _( "Your %1$s puncture %2$s in mid-attack!" ),
                     ( has_trait( trait_QUILLS ) ? _( "quills" ) : _( "spines" ) ),
                     source->disp_name() );
        }
        damage_instance spine_damage;
        // FIXME: Hardcoded damage type
        spine_damage.add_damage( damage_stab, spine );
        source->deal_damage( this, body_part_torso, spine_damage );
    }
    if( ( !wearing_something_on( bp_hit ) ) && has_trait( trait_THORNS ) &&
        ( !source->has_weapon() ) ) {
        if( !is_avatar() ) {
            if( u_see ) {
                add_msg( _( "%1$s's %2$s scrape %3$s in mid-attack!" ), get_name(),
                         _( "thorns" ), source->disp_name() );
            }
        } else {
            add_msg( m_good, _( "Your thorns scrape %s in mid-attack!" ), source->disp_name() );
        }
        int thorn = rng( 1, 4 );
        damage_instance thorn_damage;
        // FIXME: Hardcoded damage type
        thorn_damage.add_damage( damage_cut, thorn );
        // In general, critters don't have separate limbs
        // so safer to target the torso
        source->deal_damage( this, body_part_torso, thorn_damage );
    }
    if( ( !wearing_something_on( bp_hit ) ) && has_trait( trait_CF_HAIR ) ) {
        if( !is_avatar() ) {
            if( u_see ) {
                add_msg( _( "%1$s gets a load of %2$s's %3$s stuck in!" ), source->disp_name(),
                         get_name(), _( "hair" ) );
            }
        } else {
            add_msg( m_good, _( "Your hairs detach into %s!" ), source->disp_name() );
        }
        source->add_effect( effect_stunned, 2_turns );
        if( one_in( 3 ) ) { // In the eyes!
            source->add_effect( effect_blind, 2_turns );
        }
    }

    const optional_vpart_position veh_part = here->veh_at( pos_abs() );
    bool in_skater_vehicle = in_vehicle && veh_part.part_with_feature( "SEAT_REQUIRES_BALANCE", false );

    if( ( worn_with_flag( flag_REQUIRES_BALANCE ) || in_skater_vehicle ) && !is_on_ground() )  {
        int rolls = 4;
        if( worn_with_flag( flag_ROLLER_ONE ) && !in_skater_vehicle ) {
            rolls += 2;
        }
        if( has_trait( trait_PROF_SKATER ) ) {
            rolls--;
        }
        if( has_trait( trait_DEFT ) ) {
            rolls--;
        }
        if( has_trait( trait_CLUMSY ) ) {
            rolls++;
        }

        if( stability_roll() < dice( rolls, 10 ) ) {
            if( !is_avatar() ) {
                if( u_see ) {
                    add_msg( _( "%1$s loses their balance while being hit!" ), get_name() );
                }
            } else {
                add_msg( m_bad, _( "You lose your balance while being hit!" ) );
            }
            if( in_skater_vehicle ) {
                g->fling_creature( this, rng_float( 0_degrees, 360_degrees ), 10 );
            }
            // This kind of downing is not subject to immunity.
            add_effect( effect_downed, 2_turns, false, 0, true );
        }
    }

    enchantment_cache->cast_hit_me( *this, source );
}

/*
    Where damage to character is actually applied to hit body parts
    Might be where to put bleed stuff rather than in player::deal_damage()
 */
void Character::apply_damage( Creature *source, bodypart_id hurt, int dam,
                              const bool bypass_med )
{
    if( is_dead_state() || has_effect( effect_incorporeal ) ||
        has_flag( json_flag_CANNOT_TAKE_DAMAGE ) ) {
        // don't do any more damage if we're already dead
        // Or if we're debugging and don't want to die
        // Or we are intangible
        return;
    }

    if( hurt == bodypart_str_id::NULL_ID() ) {
        debugmsg( "Wacky body part hurt!" );
        hurt = body_part_torso;
    }

    int pain = 0;
    if( !hurt->has_flag( json_flag_BIONIC_LIMB ) ) {
        pain = mod_pain( dam / 2 );
    }

    const bodypart_id &part_to_damage = hurt->main_part;

    const int dam_to_bodypart = std::min( dam, get_part_hp_cur( part_to_damage ) );

    mod_part_hp_cur( part_to_damage, - dam_to_bodypart );
    if( source ) {
        cata::event e = cata::event::make<event_type::character_takes_damage>( getID(), dam_to_bodypart,
                        part_to_damage.id(), pain );
        get_event_bus().send_with_talker( this, source, e );
    } else {
        get_event_bus().send<event_type::character_takes_damage>( getID(), dam_to_bodypart,
                part_to_damage.id(), pain );
    }

    if( !weapon.is_null() && !can_wield( weapon ).success() &&
        can_drop( weapon ).success() ) {
        add_msg_if_player( _( "You are no longer able to wield your %s and drop it!" ),
                           weapon.display_name() );
        put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { weapon } );
        i_rem( &weapon );
    }
    if( has_effect( effect_mending, part_to_damage.id() ) && ( source == nullptr ||
            !source->is_hallucination() ) ) {
        effect &e = get_effect( effect_mending, part_to_damage );
        float remove_mend = dam / 20.0f;
        e.mod_duration( -e.get_max_duration() * remove_mend );
    }

    if( dam > get_painkiller() ) {
        on_hurt( source );
    }

    if( !bypass_med ) {
        // remove healing effects if damaged
        int remove_med = roll_remainder( dam / 5.0f );
        if( remove_med > 0 && has_effect( effect_bandaged, part_to_damage.id() ) ) {
            remove_med -= reduce_healing_effect( effect_bandaged, remove_med, part_to_damage );
        }
        if( remove_med > 0 && has_effect( effect_disinfected, part_to_damage.id() ) ) {
            reduce_healing_effect( effect_disinfected, remove_med, part_to_damage );
        }
    }
}

dealt_damage_instance Character::deal_damage( Creature *source, bodypart_id bp,
        const damage_instance &d, const weakpoint_attack &attack, const weakpoint & )
{
    const map &here = get_map();

    if( has_effect( effect_incorporeal ) || has_flag( json_flag_CANNOT_TAKE_DAMAGE ) ) {
        return dealt_damage_instance();
    }

    //damage applied here
    dealt_damage_instance dealt_dams = Creature::deal_damage( source, bp, d, attack );
    //block reduction should be by applied this point
    int dam = dealt_dams.total_damage();

    const tripoint_bub_ms pos = pos_bub( here );

    // TODO: Pre or post blit hit tile onto "this"'s location here
    if( dam > 0 && get_player_view().sees( here, pos ) ) {
        g->draw_hit_player( *this, dam );

        if( is_avatar() && source ) {
            const tripoint_bub_ms source_pos = source->pos_bub( here );

            //monster hits player melee
            SCT.add( pos.xy().raw(),
                     direction_from( point::zero, point( pos.x() - source_pos.x(), pos.y() - source_pos.y() ) ),
                     get_hp_bar( dam, get_hp_max( bp ) ).first, m_bad, body_part_name( bp ), m_neutral );
        }
    }

    // And slimespawners too
    if( has_trait( trait_SLIMESPAWNER ) && ( dam >= 10 ) && one_in( 20 - dam ) ) {
        if( monster *const slime = g->place_critter_around( mon_player_blob, pos, 1 ) ) {
            slime->friendly = -1;
            add_msg_if_player( m_warning, _( "A mass of slime is torn from you, and moves on its own!" ) );
        }
    }

    Character &player_character = get_player_character();
    //Acid blood effects.
    bool u_see = player_character.sees( here, *this );
    // FIXME: Hardcoded damage type
    int cut_dam = dealt_dams.type_damage( damage_cut );
    if( source && has_flag( json_flag_ACIDBLOOD ) && !bp->has_flag( json_flag_BIONIC_LIMB ) &&
        !one_in( 3 ) &&
        ( dam >= 4 || cut_dam > 0 ) && ( rl_dist( player_character.pos_abs(), source->pos_abs() ) <= 1 ) ) {
        if( is_avatar() ) {
            add_msg( m_good, _( "Your acidic blood splashes %s in mid-attack!" ),
                     source->disp_name() );
        } else if( u_see ) {
            add_msg( _( "%1$s's acidic blood splashes on %2$s in mid-attack!" ),
                     disp_name(), source->disp_name() );
        }
        damage_instance acidblood_damage;
        // FIXME: Hardcoded damage type
        acidblood_damage.add_damage( damage_acid, rng( 4, 16 ) );
        if( !one_in( 4 ) ) {
            source->deal_damage( this, body_part_arm_l, acidblood_damage );
            source->deal_damage( this, body_part_arm_r, acidblood_damage );
        } else {
            source->deal_damage( this, body_part_torso, acidblood_damage );
            source->deal_damage( this, body_part_head, acidblood_damage );
        }
    }

    int recoil_mul = 100;

    if( bp == body_part_hand_l || bp == body_part_arm_l ||
        bp == body_part_hand_r || bp == body_part_arm_r ) {
        recoil_mul = 200;
    } else if( bp == bodypart_str_id::NULL_ID() ) {
        debugmsg( "Wacky body part hit!" );
    }

    // TODO: Scale with damage in a way that makes sense for power armors, plate armor and naked skin.
    recoil += recoil_mul * weapon.volume() / 250_ml;
    recoil = std::min( MAX_RECOIL, recoil );

    int sum_cover = 0;
    bool dealt_melee = false;
    bool dealt_ranged = false;
    for( const damage_unit &du : d ) {
        if( !du.type->physical ) {
            continue;
        }
        // Assume that ranged == getting shot
        if( !du.type->melee_only ) {
            dealt_ranged = true;
        } else {
            dealt_melee = true;
        }
    }
    sum_cover += worn.sum_filthy_cover( dealt_ranged, dealt_melee, bp );

    // Chance of infection is damage (with cut and stab x4) * sum of coverage on affected body part, in percent.
    // i.e. if the body part has a sum of 100 coverage from filthy clothing,
    // each point of damage has a 1% change of causing infection.
    // if the bodypart doesn't bleed, it doesn't have blood, and cannot get infected
    if( sum_cover > 0 && !bp->has_flag( json_flag_BIONIC_LIMB ) ) {
        // FIXME: Hardcoded damage types
        const int cut_type_dam = dealt_dams.type_damage( damage_cut ) +
                                 dealt_dams.type_damage( damage_stab );
        const int combined_dam = dealt_dams.type_damage( damage_bash ) + ( cut_type_dam * 4 );
        const int infection_chance = ( combined_dam * sum_cover ) / 100;
        if( x_in_y( infection_chance, 100 ) ) {
            if( has_effect( effect_bite, bp.id() ) ) {
                add_effect( effect_bite, 40_minutes, bp, true );
            } else if( has_effect( effect_infected, bp.id() ) ) {
                add_effect( effect_infected, 25_minutes, bp, true );
            } else {
                add_effect( effect_bite, 1_turns, bp, true );
            }
            add_msg_if_player( _( "Filth from your clothing has been embedded deep in the wound." ) );
        }
    }

    on_hurt( source );
    return dealt_dams;
}

int Character::reduce_healing_effect( const efftype_id &eff_id, int remove_med,
                                      const bodypart_id &hurt )
{
    effect &e = get_effect( eff_id, hurt );
    int intensity = e.get_intensity();
    if( remove_med < intensity ) {
        if( eff_id == effect_bandaged ) {
            add_msg_if_player( m_bad, _( "Bandages on your %s were damaged!" ), body_part_name( hurt ) );
        } else  if( eff_id == effect_disinfected ) {
            add_msg_if_player( m_bad, _( "You got some filth on your disinfected %s!" ),
                               body_part_name( hurt ) );
        }
    } else {
        if( eff_id == effect_bandaged ) {
            add_msg_if_player( m_bad, _( "Bandages on your %s were destroyed!" ),
                               body_part_name( hurt ) );
        } else  if( eff_id == effect_disinfected ) {
            add_msg_if_player( m_bad, _( "Your %s is no longer disinfected!" ), body_part_name( hurt ) );
        }
    }
    e.mod_duration( -6_hours * remove_med );
    return intensity;
}

void Character::heal_bp( bodypart_id bp, int dam )
{
    heal( bp, dam );
}

void Character::heal( const bodypart_id &healed, int dam )
{
    if( !is_limb_broken( healed ) && ( dam != 0 || healed->has_flag( json_flag_ALWAYS_HEAL ) ) ) {
        add_msg_debug( debugmode::DF_CHAR_HEALTH, "Base healing of %s = %d", body_part_name( healed ),
                       dam );
        if( healed->has_flag( json_flag_HEAL_OVERRIDE ) ) {
            dam = healed->heal_bonus;
            add_msg_debug( debugmode::DF_CHAR_HEALTH, "Heal override, new healing %d", dam );
        } else {
            dam += healed->heal_bonus;
            add_msg_debug( debugmode::DF_CHAR_HEALTH, "Healing after bodypart heal bonus %d", dam );
        }
        int effective_heal = std::min( dam,
                                       get_part_hp_max( healed ) - get_part_hp_cur( healed ) ) ;
        mod_part_hp_cur( healed, effective_heal );
        add_msg_debug( debugmode::DF_CHAR_HEALTH, "Final healing of %s = %d", body_part_name( healed ),
                       dam );
        get_event_bus().send<event_type::character_heals_damage>( getID(), effective_heal );
    }
}

void Character::healall( int dam )
{
    for( const bodypart_id &bp : get_all_body_parts() ) {
        heal( bp, dam );
        mod_part_healed_total( bp, dam );
    }
}

void Character::hurtall( int dam, Creature *source, bool disturb /*= true*/ )
{
    if( is_dead_state() || has_effect( effect_incorporeal ) ||
        dam <= 0 || has_flag( json_flag_CANNOT_TAKE_DAMAGE ) ) {
        return;
    }

    // Low pain: damage is spread all over the body, so not as painful as 6 hits in one part
    int pain = mod_pain( dam );

    std::vector<bodypart_id> body_parts = get_all_body_parts( get_body_part_flags::only_main );
    for( const bodypart_id &bp : body_parts ) {
        // Don't use apply_damage here or it will annoy the player with 6 queries
        const int dam_to_bodypart = std::min( dam, get_part_hp_cur( bp ) );
        mod_part_hp_cur( bp, - dam_to_bodypart );

        if( source ) {
            cata::event e = cata::event::make<event_type::character_takes_damage>( getID(), dam_to_bodypart,
                            bp.id(), pain / body_parts.size() );
            get_event_bus().send_with_talker( this, source == nullptr ? nullptr : source, e );
        } else {
            get_event_bus().send<event_type::character_takes_damage>( getID(), dam_to_bodypart, bp.id(),
                    pain / body_parts.size() );
        }

    }

    on_hurt( source, disturb );
}

int Character::hitall( int dam, int vary, Creature *source )
{
    int damage_taken = 0;
    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        int ddam = vary ? dam * rng( 100 - vary, 100 ) / 100 : dam;
        // FIXME: Hardcoded damage type
        damage_instance damage( damage_bash, ddam );
        damage_taken += deal_damage( source, bp, damage ).total_damage();
    }
    return damage_taken;
}

void Character::on_hurt( Creature *source, bool disturb /*= true*/ )
{
    const map &here = get_map();

    if( has_trait( trait_ADRENALINE ) && !has_effect( effect_adrenaline ) &&
        ( get_part_hp_cur( body_part_head ) < 25 ||
          get_part_hp_cur( body_part_torso ) < 15 ) ) {
        add_effect( effect_adrenaline, 20_minutes );
    }

    if( disturb ) {
        if( has_effect( effect_sleep ) && !has_bionic( bio_sleep_shutdown ) ) {
            wake_up();
        }
        if( uistate.distraction_attack && !is_npc() && !has_effect( effect_narcosis ) ) {
            if( source != nullptr ) {
                if( sees( here, *source ) ) {
                    g->cancel_activity_or_ignore_query( distraction_type::attacked,
                                                        string_format( _( "You were attacked by %s!" ), source->disp_name() ) );
                } else {
                    g->cancel_activity_or_ignore_query( distraction_type::attacked,
                                                        _( "You were attacked by something you can't see!" ) );
                }
            } else {
                g->cancel_activity_or_ignore_query( distraction_type::attacked, _( "You were hurt!" ) );
            }
        }
    }

    if( is_dead_state() ) {
        set_killer( source );
    }
}

bool Character::crossed_threshold() const
{
    for( const trait_id &mut : get_mutations() ) {
        if( mut->threshold ) {
            return true;
        }
    }
    return false;
}

mutation_category_id Character::get_threshold_category() const
{
    for( const trait_id &mut : get_functioning_mutations() ) {
        if( mut->threshold ) {
            const std::map<mutation_category_id, mutation_category_trait> &mutation_categories =
                mutation_category_trait::get_all();
            for( const auto &cat : mutation_categories ) {
                if( cat.second.threshold_mut == mut ) {
                    return cat.first;
                }
            }
        }
    }
    return mutation_category_id::NULL_ID();
}

void Character::update_type_of_scent( bool init )
{
    scenttype_id new_scent = scent_sc_human;
    for( const trait_id &mut : get_functioning_mutations() ) {
        if( mut.obj().scent_typeid ) {
            new_scent = mut.obj().scent_typeid.value();
        }
    }

    if( !init && new_scent != get_type_of_scent() ) {
        get_scent().reset();
    }
    set_type_of_scent( new_scent );
}

void Character::update_type_of_scent( const trait_id &mut, bool gain )
{
    const std::optional<scenttype_id> &mut_scent = mut->scent_typeid;
    if( mut_scent ) {
        if( gain && mut_scent.value() != get_type_of_scent() ) {
            set_type_of_scent( mut_scent.value() );
            get_scent().reset();
        } else {
            update_type_of_scent();
        }
    }
}

void Character::set_type_of_scent( const scenttype_id &id )
{
    type_of_scent = id;
}

scenttype_id Character::get_type_of_scent() const
{
    return type_of_scent;
}

void Character::restore_scent()
{
    const std::string prev_scent = get_value( "prev_scent" ).str();
    if( !prev_scent.empty() ) {
        remove_effect( effect_masked_scent );
        set_type_of_scent( scenttype_id( prev_scent ) );
        remove_value( "prev_scent" );
        remove_value( "waterproof_scent" );
        add_msg_if_player( m_info, _( "You smell like yourself again." ) );
    }
}

void Character::spores()
{
    map &here = get_map();
    fungal_effects fe;
    //~spore-release sound
    sounds::sound( pos_bub(), 10, sounds::sound_t::combat, _( "Pouf!" ), false, "misc", "puff" );
    for( const tripoint_bub_ms &sporep : here.points_in_radius( pos_bub(), 1 ) ) {
        if( sporep == pos_bub() ) {
            continue;
        }
        fe.fungalize( sporep, this, 0.25 );
    }
}

void Character::blossoms()
{
    // Player blossoms are shorter-ranged, but you can fire much more frequently if you like.
    sounds::sound( pos_bub(), 10, sounds::sound_t::combat, _( "Pouf!" ), false, "misc", "puff" );
    map &here = get_map();
    for( const tripoint_bub_ms &tmp : here.points_in_radius( pos_bub(), 2 ) ) {
        here.add_field( tmp, fd_fungal_haze, rng( 1, 2 ) );
    }
}

void Character::update_vitamins( const vitamin_id &vit )
{
    if( !needs_food() ) {
        return; // NPCs can only develop vitamin diseases if their needs are enabled
    }

    efftype_id def = vit.obj().deficiency();
    efftype_id exc = vit.obj().excess();

    int lvl = vit.obj().severity( vitamin_get( vit ) );
    if( lvl <= 0 ) {
        remove_effect( def );
    }
    if( lvl >= 0 ) {
        remove_effect( exc );
    }
    if( lvl > 0 ) {
        if( has_effect( def ) ) {
            get_effect( def ).set_intensity( lvl, is_avatar() );
        } else {
            add_effect( def, 1_turns, true, lvl );
        }
    }
    if( lvl < 0 ) {
        if( has_effect( exc ) ) {
            get_effect( exc ).set_intensity( -lvl, is_avatar() );
        } else {
            add_effect( exc, 1_turns, true, -lvl );
        }
    }
}

void Character::rooted_message() const
{
    if( ( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) || has_trait( trait_CHLOROMORPH ) ) &&
        get_map().has_flag( ter_furn_flag::TFLAG_PLOWABLE, pos_bub() ) &&
        is_barefoot() ) {
        add_msg( m_info, _( "You sink your roots into the soil." ) );
    }
}

void Character::rooted()
// This assumes that daily Iron, Calcium, and Thirst needs should be filled at the same rate.
// Baseline humans need 96 Iron and Calcium, and 288 Thirst per day.
// Thirst level -40 puts it right in the middle of the 'Hydrated' zone.
// TODO: The rates for iron, calcium, and thirst should probably be pulled from the nutritional data rather than being hardcoded here, so that future balance changes don't break this.
{
    if( ( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) || has_trait( trait_CHLOROMORPH ) ) &&
        get_map().has_flag( ter_furn_flag::TFLAG_PLOWABLE, pos_bub() ) && is_barefoot() ) {
        int time_to_full = 43200; // 12 hours
        if( has_trait( trait_ROOTS3 ) || has_trait( trait_CHLOROMORPH ) ) {
            time_to_full += -14400;    // -4 hours
        }
        if( x_in_y( 96, time_to_full ) ) {
            vitamin_mod( vitamin_iron, 1 );
            vitamin_mod( vitamin_calcium, 1 );
            mod_daily_health( 5, 50 );
        }
        if( get_thirst() > -40 && x_in_y( 288, time_to_full ) ) {
            mod_thirst( -1 );
        }
    }
}

std::vector<item *> Character::inv_dump()
{
    std::vector<item *> ret;
    if( is_armed() && can_drop( weapon ).success() ) {
        ret.push_back( &weapon );
    }
    worn.inv_dump( ret );
    inv->dump( ret );
    return ret;
}

std::vector<const item *> Character::inv_dump() const
{
    std::vector<const item *> ret;
    if( is_armed() && can_drop( weapon ).success() ) {
        ret.push_back( &weapon );
    }
    worn.inv_dump( ret );
    inv->dump( ret );
    return ret;
}

bool Character::covered_with_flag( const flag_id &f, const body_part_set &parts ) const
{
    if( parts.none() ) {
        return true;
    }

    return worn.covered_with_flag( f, parts );
}

bool Character::is_waterproof( const body_part_set &parts ) const
{
    return covered_with_flag( flag_WATERPROOF, parts );
}

units::volume Character::free_space() const
{
    units::volume volume_capacity = 0_ml;
    volume_capacity += weapon.get_total_capacity();
    for( const item_pocket *pocket : weapon.get_all_contained_pockets() ) {
        if( pocket->contains_phase( phase_id::SOLID ) ) {
            for( const item *it : pocket->all_items_top() ) {
                volume_capacity -= it->volume();
            }
        } else if( !pocket->empty() ) {
            volume_capacity -= pocket->volume_capacity();
        }
    }
    volume_capacity += weapon.check_for_free_space();
    volume_capacity += worn.free_space();
    return volume_capacity;
}

units::mass Character::free_weight_capacity() const
{
    units::mass weight_capacity = 0_gram;
    weight_capacity += weapon.get_remaining_weight_capacity();
    weight_capacity += worn.free_weight_capacity();
    return weight_capacity;
}

units::volume Character::holster_volume() const
{
    units::volume holster_volume = 0_ml;
    if( weapon.is_holster() ) {
        holster_volume += weapon.get_total_capacity();
    }
    holster_volume += worn.holster_volume();
    return holster_volume;
}

int Character::empty_holsters() const
{
    int e_holsters = 0;
    if( weapon.is_holster() ) {
        e_holsters += 1;
    }
    e_holsters += worn.empty_holsters();
    return e_holsters;
}

int Character::used_holsters() const
{
    int e_holsters = 0;
    if( weapon.is_holster() ) {
        e_holsters += weapon.get_used_holsters();
    }
    e_holsters += worn.used_holsters();
    return e_holsters;
}

int Character::total_holsters() const
{
    int e_holsters = 0;
    if( weapon.is_holster() ) {
        e_holsters += weapon.get_total_holsters();
    }
    e_holsters += worn.total_holsters();
    return e_holsters;
}

units::volume Character::free_holster_volume() const
{
    units::volume holster_volume = 0_ml;
    if( weapon.is_holster() ) {
        holster_volume += weapon.get_total_holster_volume() - weapon.get_used_holster_volume();
    }
    holster_volume += worn.free_holster_volume();
    return holster_volume;
}

units::volume Character::small_pocket_volume( const units::volume &threshold ) const
{
    units::volume small_spaces = 0_ml;
    if( weapon.get_total_capacity() <= threshold ) {
        small_spaces += weapon.get_total_capacity();
    }
    small_spaces += worn.small_pocket_volume( threshold );
    return small_spaces;
}

units::volume Character::volume_capacity() const
{
    units::volume volume_capacity = 0_ml;
    volume_capacity += weapon.get_total_capacity();
    volume_capacity += worn.volume_capacity();
    return volume_capacity;
}

units::volume Character::volume_capacity_with_tweaks( const
        std::vector<std::pair<item_location, int>>
        &locations ) const
{
    std::map<const item *, int> dropping;
    for( const std::pair<item_location, int> &location_pair : locations ) {
        dropping.emplace( location_pair.first.get_item(), location_pair.second );
    }
    return volume_capacity_with_tweaks( item_tweaks( { dropping } ) );
}

units::volume Character::volume_capacity_with_tweaks( const item_tweaks &tweaks ) const
{
    const std::map<const item *, int> empty;
    const std::map<const item *, int> &without = tweaks.without_items ? tweaks.without_items->get() :
            empty;

    units::volume volume_capacity = 0_ml;

    if( !without.count( &weapon ) ) {
        volume_capacity += weapon.get_total_capacity();
    }

    volume_capacity += worn.volume_capacity_with_tweaks( without );

    return volume_capacity;
}

units::volume Character::volume_carried() const
{
    return volume_capacity() - free_space();
}

void Character::toggle_hauling()
{
    map &here = get_map();

    if( hauling ) {
        stop_hauling();
    } else {
        std::vector<item_location> items = here.get_haulable_items( pos_bub() );
        if( items.empty() ) {
            add_msg( m_info, _( "There are no items to haul here." ) );
            return;
        }
        start_hauling( items );
        start_autohaul();
    }
}

void Character::start_hauling( const std::vector<item_location> &items_to_haul = {} )
{
    map &here = get_map();

    if( here.veh_at( pos_bub() ) ) {
        add_msg( m_info, _( "You cannot haul inside vehicles." ) );
        return;
    } else if( here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, pos_bub() ) ) {
        add_msg( m_info, _( "You cannot haul while in deep water." ) );
        return;
    } else if( !here.can_put_items( pos_bub() ) || here.impassable_field_at( pos_bub() ) ) {
        add_msg( m_info, _( "You cannot haul items here." ) );
        return;
    }

    add_msg( _( "You start hauling items along the ground." ) );
    if( is_armed() ) {
        add_msg( m_warning, _( "Your hands are not free, which makes hauling slower." ) );
    }

    hauling = true;
    haul_list = items_to_haul;
}

void Character::stop_hauling()
{
    if( hauling ) {
        add_msg( _( "You stop hauling items." ) );
        hauling = false;
        autohaul = false;
        haul_list.clear();
    }
    if( has_activity( ACT_MOVE_ITEMS ) ) {
        cancel_activity();
    }
}

bool Character::is_hauling() const
{
    return hauling;
}

void Character::start_autohaul()
{
    autohaul = true;
    if( !is_hauling() ) {
        start_hauling();
    }
}

void Character::stop_autohaul()
{
    autohaul = false;
    if( haul_list.empty() ) {
        stop_hauling();
    }
}

bool Character::is_autohauling() const
{
    return autohaul;
}

bool Character::trim_haul_list( const std::vector<item_location> &valid_items )
{
    size_t qty_before = haul_list.size();
    haul_list.erase( std::remove_if( haul_list.begin(),
    haul_list.end(), [&valid_items]( const item_location & it ) {
        return std::count( valid_items.begin(), valid_items.end(), it ) == 0;
    } ), haul_list.end() );

    return qty_before != haul_list.size();
}

bool Character::knows_creature_type( const Creature *c ) const
{
    if( const monster *mon = dynamic_cast<const monster *>( c ) ) {
        return knows_creature_type( mon->type->id );
    }
    return false;
}

bool Character::knows_creature_type( const mtype_id &c ) const
{
    return known_monsters.count( c ) > 0;
}

void Character::set_knows_creature_type( const Creature *c )
{
    if( const monster *mon = dynamic_cast<const monster *>( c ) ) {
        set_knows_creature_type( mon->type->id );
    }
}

void Character::set_knows_creature_type( const mtype_id &c )
{
    known_monsters.emplace( c );
}

void Character::assign_activity( const activity_id &type, int moves, int index, int pos,
                                 const std::string &name )
{
    assign_activity( player_activity( type, moves, index, pos, name ) );
}

void Character::assign_activity( const activity_actor &actor )
{
    assign_activity( player_activity( actor ) );
}

void Character::assign_activity( const player_activity &act )
{
    bool resuming = false;
    if( !backlog.empty() && backlog.front().can_resume_with( act, *this ) ) {
        resuming = true;
        add_msg_if_player( _( "You resume your task." ) );
        activity = backlog.front();
        backlog.pop_front();
    } else {
        if( activity ) {
            backlog.push_front( activity );
        }

        activity = act;
    }

    activity.start_or_resume( *this, resuming );

    if( is_npc() ) {
        cancel_stashed_activity();
        npc *guy = dynamic_cast<npc *>( this );
        guy->set_attitude( NPCATT_ACTIVITY );
        guy->set_mission( NPC_MISSION_ACTIVITY );
        guy->current_activity_id = activity.id();
    }
}

bool Character::has_activity( const activity_id &type ) const
{
    return activity.id() == type;
}

bool Character::has_activity( const std::vector<activity_id> &types ) const
{
    return std::find( types.begin(), types.end(), activity.id() ) != types.end();
}

void Character::cancel_activity()
{
    activity.canceled( *this );
    if( has_activity( ACT_MOVE_ITEMS ) && is_hauling() ) {
        stop_hauling();
    }
    // Clear any backlog items that aren't auto-resume.
    for( auto backlog_item = backlog.begin(); backlog_item != backlog.end(); ) {
        if( backlog_item->auto_resume ) {
            backlog_item++;
        } else {
            backlog_item = backlog.erase( backlog_item );
        }
    }

    // act wait stamina interrupts an ongoing activity.
    // and automatically puts auto_resume = true on it
    // we don't want that to persist if there is another interruption.
    // and player moves elsewhere.
    if( has_activity( ACT_WAIT_STAMINA ) && !backlog.empty() &&
        backlog.front().auto_resume ) {
        backlog.front().auto_resume = false;
    }
    if( activity && activity.can_resume() ) {
        activity.allow_distractions();
        backlog.push_front( activity );
    }
    sfx::end_activity_sounds(); // kill activity sounds when canceled
    activity = player_activity();
}

void Character::resume_backlog_activity()
{
    if( !backlog.empty() && backlog.front().auto_resume ) {
        activity = backlog.front();
        activity.auto_resume = false;
        activity.allow_distractions();
        backlog.pop_front();
    }
}

void Character::fall_asleep()
{
    // Communicate to the player that he is using items on the floor
    std::string item_name = is_snuggling();
    if( item_name == "many" ) {
        if( one_in( 15 ) ) {
            add_msg_if_player( _( "You nestle into your pile of clothes for warmth." ) );
        } else {
            add_msg_if_player( _( "You use your pile of clothes for warmth." ) );
        }
    } else if( item_name != "nothing" ) {
        if( one_in( 15 ) ) {
            add_msg_if_player( _( "You snuggle your %s to keep warm." ), item_name );
        } else {
            add_msg_if_player( _( "You use your %s to keep warm." ), item_name );
        }
    }

    get_comfort_at( pos_bub() ).add_sleep_msgs( *this );

    if( has_bionic( bio_sleep_shutdown ) ) {
        add_msg_if_player( _( "Sleep Mode activated.  Disabling sensory response." ) );
    }
    if( has_active_mutation( trait_HIBERNATE ) &&
        get_kcal_percent() > 0.8f ) {
        if( is_avatar() ) {
            get_memorial().add( pgettext( "memorial_male", "Entered hibernation." ),
                                pgettext( "memorial_female", "Entered hibernation." ) );
        }
        // some days worth of round-the-clock Snooze.  Cata seasons default to 91 days.
        fall_asleep( 10_days );
        // If you're not sleepinessd enough for 10 days, you won't sleep the whole thing.
        // In practice, the sleepiness from filling the tank from (no msg) to Time For Bed
        // will last about 8 days.
    } else {
        fall_asleep( 10_hours );    // default max sleep time.
    }
}

void Character::fall_asleep( const time_duration &duration )
{
    if( activity ) {
        if( activity.id() == ACT_TRY_SLEEP ) {
            activity.set_to_null();
        } else {
            cancel_activity();
        }
    }
    add_effect( effect_sleep, duration );
    get_event_bus().send<event_type::character_falls_asleep>( getID(), to_seconds<int>( duration ) );
}

void Character::migrate_items_to_storage( bool disintegrate )
{
    inv->visit_items( [&]( const item * it, item * ) {
        if( disintegrate ) {
            if( try_add( *it, /*avoid=*/nullptr, it ) == item_location::nowhere ) {
                std::string profession_id = prof->ident().str();
                debugmsg( "ERROR: Could not put %s (%s) into inventory.  Check if the "
                          "profession (%s) has enough space.",
                          it->tname(), it->typeId().str(), profession_id );
                return VisitResponse::ABORT;
            }
        } else {
            item_location added = i_add( *it, true, /*avoid=*/nullptr,
                                         it, /*allow_drop=*/false, /*allow_wield=*/!has_wield_conflicts( *it ) );
            if( added == item_location::nowhere ) {
                put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { *it } );
            }
        }
        return VisitResponse::SKIP;
    } );
    inv->clear();
}

std::string Character::is_snuggling() const
{
    map &here = get_map();
    map_stack items_here = here.i_at( pos_bub() );
    item_stack::const_iterator begin = items_here.begin();
    item_stack::const_iterator end = items_here.end();

    if( in_vehicle ) {
        if( const std::optional<vpart_reference> ovp = here.veh_at( pos_bub() ).cargo() ) {
            const vehicle_stack vs = ovp->items();
            begin = vs.begin();
            end = vs.end();
        }
    }
    const item *floor_armor = nullptr;
    int ticker = 0;

    // If there are no items on the floor, return nothing
    if( begin == end ) {
        return "nothing";
    }

    for( auto candidate = begin; candidate != end; ++candidate ) {
        if( !candidate->is_armor() ) {
            continue;
        } else if( candidate->volume() > 250_ml && candidate->get_warmth() > 0 &&
                   ( candidate->covers( body_part_torso ) || candidate->covers( body_part_leg_l ) ||
                     candidate->covers( body_part_leg_r ) ) ) {
            floor_armor = &*candidate;
            ticker++;
        }
    }

    if( ticker == 0 ) {
        return "nothing";
    } else if( ticker == 1 ) {
        return floor_armor->type_name();
    } else if( ticker > 1 ) {
        return "many";
    }

    return "nothing";
}

// If the player is not wielding anything big, check if hands can be put in pockets
bool Character::can_use_pockets() const
{
    // TODO Check that the pocket actually has enough space for the wielded item?
    return weapon.volume() < 500_ml;
}

// If the player's head is not encumbered, check if hood can be put up
bool Character::can_use_hood() const
{
    return encumb( body_part_head ) < 10;
}

// If the player's mouth is not encumbered, check if collar can be put up
bool Character::can_use_collar() const
{
    return encumb( body_part_mouth ) < 10;
}

std::map<bodypart_id, int> Character::bonus_item_warmth() const
{
    const int pocket_warmth = worn.pocket_warmth();
    const int hood_warmth = worn.hood_warmth();
    const int collar_warmth = worn.collar_warmth();

    std::map<bodypart_id, int> ret;
    for( const bodypart_id &bp : get_all_body_parts() ) {
        ret.emplace( bp, 0 );

        if( ( bp == body_part_hand_l || bp == body_part_hand_r ) && can_use_pockets() ) {
            ret[bp] += pocket_warmth;
        }

        if( bp == body_part_head && can_use_hood() ) {
            ret[bp] += hood_warmth;
        }

        if( bp == body_part_mouth && can_use_collar() ) {
            ret[bp] += collar_warmth;
        }
    }

    return ret;
}

bool Character::can_use_floor_warmth() const
{
    return in_sleep_state() ||
           has_activity( ACT_WAIT ) ||
           has_activity( ACT_WAIT_NPC ) ||
           has_activity( ACT_WAIT_STAMINA ) ||
           has_activity( ACT_AUTODRIVE ) ||
           has_activity( ACT_READ ) ||
           has_activity( ACT_SOCIALIZE ) ||
           has_activity( ACT_MEDITATE ) ||
           has_activity( ACT_FISH ) ||
           has_activity( ACT_GAME ) ||
           has_activity( ACT_HAND_CRANK ) ||
           has_activity( ACT_HEATING ) ||
           has_activity( ACT_VIBE ) ||
           has_activity( ACT_TRY_SLEEP ) ||
           has_activity( ACT_OPERATION ) ||
           has_activity( ACT_TREE_COMMUNION ) ||
           has_activity( ACT_EAT_MENU ) ||
           has_activity( ACT_CONSUME_FOOD_MENU ) ||
           has_activity( ACT_CONSUME_DRINK_MENU ) ||
           has_activity( ACT_CONSUME_MEDS_MENU ) ||
           has_activity( ACT_STUDY_SPELL );
}

units::temperature_delta Character::floor_bedding_warmth( const tripoint_bub_ms &pos )
{
    map &here = get_map();
    const trap &trap_at_pos = here.tr_at( pos );
    const ter_id &ter_at_pos = here.ter( pos );
    const furn_id &furn_at_pos = here.furn( pos );

    const optional_vpart_position vp = here.veh_at( pos );
    const std::optional<vpart_reference> boardable = vp.part_with_feature( "BOARDABLE", true );
    // Search the floor for bedding
    if( furn_at_pos != furn_str_id::NULL_ID() ) {
        return furn_at_pos.obj().floor_bedding_warmth;
    } else if( !trap_at_pos.is_null() ) {
        return trap_at_pos.floor_bedding_warmth;
    } else if( boardable ) {
        return boardable->info().floor_bedding_warmth;
    } else {
        return ter_at_pos.obj().floor_bedding_warmth - 4_C_delta;
    }
}

units::temperature_delta Character::floor_item_warmth( const tripoint_bub_ms &pos )
{
    units::temperature_delta item_warmth = 0_C_delta;

    const auto warm = [&item_warmth]( const auto & stack ) {
        for( const item &elem : stack ) {
            if( !elem.is_armor() ) {
                continue;
            }
            // Items that are big enough and covers the torso are used to keep warm.
            // Smaller items don't do as good a job
            if( elem.volume() > 250_ml &&
                ( elem.covers( body_part_torso ) || elem.covers( body_part_leg_l ) ||
                  elem.covers( body_part_leg_r ) ) ) {
                item_warmth += 0.12_C_delta * elem.get_warmth() * ( elem.volume() / 2500_ml );
            }
        }
    };

    map &here = get_map();

    if( const optional_vpart_position veh_here = here.veh_at( pos ) ) {
        if( const std::optional<vpart_reference> ovp = veh_here.cargo() ) {
            warm( ovp->items() );
        }
    } else {
        warm( here.i_at( pos ) );
    }
    return item_warmth;
}

units::temperature_delta Character::floor_warmth( const tripoint_bub_ms &pos ) const
{
    const units::temperature_delta item_warmth = floor_item_warmth( pos );
    units::temperature_delta bedding_warmth = floor_bedding_warmth( pos );

    // If the PC has fur, etc, that will apply too
    const units::temperature_delta floor_mut_warmth = enchantment_cache->modify_value(
                enchant_vals::mod::BODYTEMP_SLEEP, units::from_kelvin_delta( 0.0f ) );
    // DOWN does not provide floor insulation, though.
    // Better-than-light fur or being in one's shell does.
    if( ( !has_trait( trait_DOWN ) ) && ( floor_mut_warmth >= 2_C_delta ) ) {
        bedding_warmth = std::max( 0_C_delta, bedding_warmth );
    }
    return item_warmth + bedding_warmth + floor_mut_warmth;
}

units::temperature_delta Character::bodytemp_modifier_traits( bool overheated ) const
{
    units::temperature_delta mod = 0_C_delta;
    for( const trait_id &iter : get_functioning_mutations() ) {
        mod += overheated ? iter->bodytemp_min : iter->bodytemp_max;
    }
    return mod;
}

units::temperature Character::temp_corrected_by_climate_control( units::temperature temperature,
        int heat_strength, int chill_strength ) const
{
    const units::temperature_delta base_variation = BODYTEMP_NORM - 27_C;
    const units::temperature_delta variation_heat = base_variation * ( heat_strength / 100.0f );
    const units::temperature_delta variation_chill = -base_variation * ( chill_strength / 100.0f );

    if( temperature > BODYTEMP_NORM ) {
        return std::max( BODYTEMP_NORM, temperature + variation_chill );
    } else {
        return std::min( BODYTEMP_NORM, temperature + variation_heat );
    }
}

bool Character::in_sleep_state() const
{
    return Creature::in_sleep_state() || activity.id() == ACT_TRY_SLEEP;
}

void Character::cache_visit_items_with( const itype_id &type,
                                        const std::function<void( item & )> &do_func )
{
    cache_visit_items_with( "HAS TYPE " + type.str(), type, {}, nullptr, do_func );
}

void Character::cache_visit_items_with( const flag_id &type_flag,
                                        const std::function<void( item & )> &do_func )
{
    cache_visit_items_with( "HAS FLAG " + type_flag.str(), {}, type_flag, nullptr, do_func );
}

void Character::cache_visit_items_with( const std::string &key, bool( item::*filter_func )() const,
                                        const std::function<void( item & )> &do_func )
{
    cache_visit_items_with( key, {}, {}, filter_func, do_func );
}

void Character::cache_visit_items_with( const std::string &key, const itype_id &type,
                                        const flag_id &type_flag, bool( item::*filter_func )() const,
                                        const std::function<void( item & )> &do_func )
{
    // If the cache already exists, use it. Remove all invalid item references.
    auto found_cache = inv_search_caches.find( key );
    if( found_cache != inv_search_caches.end() ) {
        inv_search_caches[key].items.erase( std::remove_if( inv_search_caches[key].items.begin(),
        inv_search_caches[key].items.end(), [&do_func]( const safe_reference<item> &it ) {
            if( it ) {
                do_func( *it );
                return false;
            }
            return true;
        } ), inv_search_caches[key].items.end() );
        return;
    } else {
        // Otherwise, add a new cache and populate with all appropriate items in the inventory. Empty lists are still created.
        inv_search_caches[key].type = type;
        inv_search_caches[key].type_flag = type_flag;
        inv_search_caches[key].filter_func = filter_func;
        visit_items( [&]( item * it, item * ) {
            if( ( !type.is_valid() || it->typeId() == type ) &&
                ( !type_flag.is_valid() || it->type->has_flag( type_flag ) ) &&
                ( filter_func == nullptr || ( it->*filter_func )() ) ) {

                inv_search_caches[key].items.push_back( it->get_safe_reference() );
                do_func( *it );
            }
            return VisitResponse::NEXT;
        } );
    }
}

void Character::cache_visit_items_with( const itype_id &type,
                                        const std::function<void( const item & )> &do_func ) const
{
    cache_visit_items_with( "HAS TYPE " + type.str(), type, {}, nullptr, do_func );
}

void Character::cache_visit_items_with( const flag_id &type_flag,
                                        const std::function<void( const item & )> &do_func ) const
{
    cache_visit_items_with( "HAS FLAG " + type_flag.str(), {}, type_flag, nullptr, do_func );
}

void Character::cache_visit_items_with( const std::string &key, bool( item::*filter_func )() const,
                                        const std::function<void( const item & )> &do_func ) const
{
    cache_visit_items_with( key, {}, {}, filter_func, do_func );
}

void Character::cache_visit_items_with( const std::string &key, const itype_id &type,
                                        const flag_id &type_flag, bool( item::*filter_func )() const,
                                        const std::function<void( const item & )> &do_func ) const
{
    // If the cache already exists, use it. Remove all invalid item references.
    auto found_cache = inv_search_caches.find( key );
    if( found_cache != inv_search_caches.end() ) {
        inv_search_caches[key].items.erase( std::remove_if( inv_search_caches[key].items.begin(),
        inv_search_caches[key].items.end(), [&do_func]( const safe_reference<item> &it ) {
            if( it ) {
                do_func( *it );
                return false;
            }
            return true;
        } ), inv_search_caches[key].items.end() );
        return;
    } else {
        // Otherwise, add a new cache and populate with all appropriate items in the inventory. Empty lists are still created.
        inv_search_caches[key].type = type;
        inv_search_caches[key].type_flag = type_flag;
        inv_search_caches[key].filter_func = filter_func;
        visit_items( [&]( item * it, item * ) {
            if( ( !type.is_valid() || it->typeId() == type ) &&
                ( !type_flag.is_valid() || it->type->has_flag( type_flag ) ) &&
                ( filter_func == nullptr || ( it->*filter_func )() ) ) {

                inv_search_caches[key].items.push_back( it->get_safe_reference() );
                do_func( *it );
            }
            return VisitResponse::NEXT;
        } );
    }
}

bool Character::cache_has_item_with( const itype_id &type,
                                     const std::function<bool( const item & )> &check_func ) const
{
    return cache_has_item_with( "HAS TYPE " + type.str(), type, {}, nullptr, check_func );
}

bool Character::cache_has_item_with( const flag_id &type_flag,
                                     const std::function<bool( const item & )> &check_func ) const
{
    return cache_has_item_with( "HAS FLAG " + type_flag.str(), {}, type_flag, nullptr, check_func );
}

bool Character::cache_has_item_with( const std::string &key, bool( item::*filter_func )() const,
                                     const std::function<bool( const item & )> &check_func ) const
{
    return cache_has_item_with( key, {}, {}, filter_func, check_func );
}

bool Character::cache_has_item_with( const std::string &key, const itype_id &type,
                                     const flag_id &type_flag, bool( item::*filter_func )() const,
                                     const std::function<bool( const item & )> &check_func ) const
{
    bool aborted = false;

    // If the cache already exists, use it. Stop iterating if the check_func ever returns true. Remove any invalid item references encountered.
    auto found_cache = inv_search_caches.find( key );
    if( found_cache != inv_search_caches.end() ) {
        for( auto iter = found_cache->second.items.begin();
             iter != found_cache->second.items.end(); ) {
            if( *iter ) {
                if( check_func( **iter ) ) {
                    aborted = true;
                    break;
                }
                ++iter;
            } else {
                iter = inv_search_caches[found_cache->first].items.erase( iter );
            }
        }
    } else {
        // Otherwise, add a new cache and populate with all appropriate items in the inventory. Empty lists are still created.
        inv_search_caches[key].type = type;
        inv_search_caches[key].type_flag = type_flag;
        inv_search_caches[key].filter_func = filter_func;
        visit_items( [&]( item * it, item * ) {
            if( ( !type.is_valid() || it->typeId() == type ) &&
                ( !type_flag.is_valid() || it->type->has_flag( type_flag ) ) &&
                ( filter_func == nullptr || ( it->*filter_func )() ) ) {

                inv_search_caches[key].items.push_back( it->get_safe_reference() );
                // If check_func returns true, stop running it but keep populating the cache.
                if( !aborted && check_func( *it ) ) {
                    aborted = true;
                }
            }
            return VisitResponse::NEXT;
        } );
    }
    return aborted;
}

bool Character::has_item_with_flag( const flag_id &flag, bool need_charges ) const
{
    return has_item_with( [&flag, &need_charges, this]( const item & it ) {
        return it.has_flag( flag ) && ( !need_charges || !it.is_tool() ||
                                        it.type->tool->max_charges == 0 || it.ammo_remaining( this ) > 0 );
    } );
}

bool Character::cache_has_item_with_flag( const flag_id &type_flag, bool need_charges ) const
{
    return cache_has_item_with( "HAS FLAG " + type_flag.str(), {}, type_flag, nullptr,
    [this, &need_charges]( const item & it ) {
        return !need_charges || !it.is_tool() || it.type->tool->max_charges == 0 ||
               it.ammo_remaining( this ) > 0;
    } );
}

std::vector<item *> Character::cache_get_items_with( const itype_id &type,
        const std::function<bool( item & )> &do_and_check_func )
{
    return cache_get_items_with( "HAS TYPE " + type.str(), type, {}, nullptr, do_and_check_func );
}

std::vector<item *> Character::cache_get_items_with( const flag_id &type_flag,
        const std::function<bool( item & )> &do_and_check_func )
{
    return cache_get_items_with( "HAS FLAG " + type_flag.str(), {}, type_flag, nullptr,
                                 do_and_check_func );
}

std::vector<item *> Character::cache_get_items_with( const std::string &key,
        bool( item::*filter_func )() const,
        const std::function<bool( item & )> &do_and_check_func )
{
    return cache_get_items_with( key, {}, {}, filter_func, do_and_check_func );
}

std::vector<item *> Character::cache_get_items_with( const std::string &key, const itype_id &type,
        const flag_id &type_flag, bool( item::*filter_func )() const,
        const std::function<bool( item & )> &do_and_check_func )
{
    std::vector<item *> ret;
    cache_visit_items_with( key, type, type_flag, filter_func, [&ret, &do_and_check_func]( item & it ) {
        if( do_and_check_func( it ) ) {
            ret.push_back( &it );
        }
    } );
    return ret;
}

std::vector<const item *> Character::cache_get_items_with( const itype_id &type,
        const std::function<bool( const item & )> &check_func ) const
{
    return cache_get_items_with( "HAS TYPE " + type.str(), type, {}, nullptr, check_func );
}

std::vector<const item *> Character::cache_get_items_with( const flag_id &type_flag,
        const std::function<bool( const item & )> &check_func ) const
{
    return cache_get_items_with( "HAS FLAG " + type_flag.str(), {}, type_flag, nullptr, check_func );
}

std::vector<const item *> Character::cache_get_items_with( const std::string &key,
        bool( item::*filter_func )() const,
        const std::function<bool( const item & )> &check_func ) const
{
    return cache_get_items_with( key, {}, {}, filter_func, check_func );
}

std::vector<const item *> Character::cache_get_items_with( const std::string &key,
        const itype_id &type, const flag_id &type_flag, bool( item::*filter_func )() const,
        const std::function<bool( const item & )> &check_func ) const
{
    std::vector<const item *> ret;
    cache_visit_items_with( key, type, type_flag, filter_func, [&ret, &check_func]( const item & it ) {
        if( check_func( it ) ) {
            ret.push_back( &it );
        }
    } );
    return ret;
}

void Character::add_to_inv_search_caches( item &it ) const
{
    for( auto &cache : inv_search_caches ) {
        if( ( cache.second.type.is_valid() && it.typeId() != cache.second.type ) ||
            ( cache.second.type_flag.is_valid() && !it.type->has_flag( cache.second.type_flag ) ) ||
            ( cache.second.filter_func && !( it.*cache.second.filter_func )() ) ) {
            continue;
        }

        // If item is already in the cache, remove it so it can be re-added in its current state.
        for( auto iter = cache.second.items.begin(); iter != cache.second.items.end(); ) {
            if( *iter && iter->get() == &it ) {
                iter = inv_search_caches[cache.first].items.erase( iter );
            } else {
                ++iter;
            }
        }

        cache.second.items.push_back( it.get_safe_reference() );
    }
}

void Character::clear_inventory_search_cache()
{
    inv_search_caches.clear();
}

bool Character::has_charges( const itype_id &it, int quantity,
                             const std::function<bool( const item & )> &filter ) const
{
    if( it == itype_fire || it == itype_apparatus ) {
        return has_fire( quantity );
    }
    return charges_of( it, quantity, filter ) == quantity;
}

std::list<item> Character::use_amount( const itype_id &it, int quantity,
                                       const std::function<bool( const item & )> &filter, bool select_ind )
{
    std::list<item> ret;
    if( select_ind && !it->count_by_charges() ) {
        std::vector<item *> tmp = items_with( [&it, &filter]( const item & itm ) -> bool {
            return filter( itm ) && itm.typeId() == it;
        } );
        while( quantity != static_cast<int>( tmp.size() ) && quantity > 0 && !tmp.empty() ) {
            uilist imenu;
            //~ Select components from inventory to consume. %d = number of components left to consume.
            imenu.title = string_format( _( "Select which component to use (%d left)" ), quantity );
            for( const item *itm : tmp ) {
                imenu.addentry( itm->display_name() );
            }
            imenu.query();
            if( imenu.ret < 0 || static_cast<size_t>( imenu.ret ) >= tmp.size() ) {
                break;
            }
            if( tmp[imenu.ret]->use_amount( it, quantity, ret, filter ) ) {
                remove_item( *tmp[imenu.ret] );
            }
            tmp.erase( tmp.begin() + imenu.ret );
        }
    }
    if( quantity > 0 && weapon.use_amount( it, quantity, ret ) ) {
        remove_weapon();
    }
    ret = worn.use_amount( it, quantity, ret, filter, *this );

    if( quantity <= 0 ) {
        return ret;
    }
    std::list<item> tmp = inv->use_amount( it, quantity, filter );
    ret.splice( ret.end(), tmp );
    return ret;
}

bool Character::use_charges_if_avail( const itype_id &it, int quantity )
{
    if( has_charges( it, quantity ) ) {
        use_charges( it, quantity );
        return true;
    }
    return false;
}

std::pair<float, item> Character::get_best_weapon_by_damage_type( const damage_type_id dmg_type )
const
{
    std::pair<float, item> best_weapon = std::make_pair( 0.0f, item() );

    cache_visit_items_with( "best_damage_" + dmg_type.str(), &item::is_melee, [&best_weapon, this,
                  dmg_type]( const item & it ) {
        damage_instance di;
        roll_damage( dmg_type, false, di, true, it, attack_vector_id::NULL_ID(),
                     sub_bodypart_str_id::NULL_ID(), 1.f );
        for( damage_unit &du : di.damage_units ) {
            if( du.type == dmg_type && best_weapon.first < du.amount ) {
                best_weapon = std::make_pair( du.amount, it );
            }
        }
    } );

    return best_weapon;
}

units::energy Character::available_ups() const
{
    units::energy available_charges = 0_kJ;

    if( is_mounted() && mounted_creature.get()->has_flag( mon_flag_RIDEABLE_MECH ) ) {
        auto *mons = mounted_creature.get();
        available_charges += units::from_kilojoule( static_cast<std::int64_t>
                             ( mons->battery_item->ammo_remaining( ) ) );
    }

    bool has_bio_powered_ups = false;
    cache_visit_items_with( flag_IS_UPS, [&has_bio_powered_ups]( const item & it ) {
        if( it.has_flag( flag_USES_BIONIC_POWER ) && !has_bio_powered_ups ) {
            has_bio_powered_ups = true;
        }
    } );
    if( has_active_bionic( bio_ups ) || has_bio_powered_ups ) {
        available_charges += get_power_level();
    }

    cache_visit_items_with( flag_IS_UPS, [&available_charges]( const item & it ) {
        available_charges += units::from_kilojoule( static_cast<std::int64_t>( it.ammo_remaining(
                             ) ) );
    } );

    return available_charges;
}

units::energy Character::consume_ups( units::energy qty, const int radius )
{
    const units::energy wanted_qty = qty;

    // UPS from mounted mech
    if( qty != 0_kJ && is_mounted() && mounted_creature.get()->has_flag( mon_flag_RIDEABLE_MECH ) &&
        mounted_creature.get()->battery_item ) {
        auto *mons = mounted_creature.get();
        qty -= mons->use_mech_power( qty );
    }

    // UPS from bionic
    bool has_bio_powered_ups = false;
    cache_visit_items_with( flag_IS_UPS, [&has_bio_powered_ups]( const item & it ) {
        if( it.has_flag( flag_USES_BIONIC_POWER ) && !has_bio_powered_ups ) {
            has_bio_powered_ups = true;
        }
    } );
    if( qty != 0_kJ && has_power() && ( has_active_bionic( bio_ups ) || has_bio_powered_ups ) ) {
        units::energy bio = std::min( get_power_level(), qty );
        mod_power_level( -bio );
        qty -= std::min( qty, bio );
    }

    // UPS from inventory
    if( qty != 0_kJ ) {
        cache_visit_items_with( flag_IS_UPS, [&qty]( item & it ) {
            if( it.is_tool() && it.type->tool->fuel_efficiency >= 0 ) {
                qty -= it.energy_consume( qty, tripoint_bub_ms::zero, nullptr,
                                          it.type->tool->fuel_efficiency );
            } else {
                qty -= it.energy_consume( qty, tripoint_bub_ms::zero, nullptr );
            }
        } );
    }

    // UPS from nearby map
    if( qty != 0_kJ && radius > 0 ) {
        qty -= get_map().consume_ups( pos_bub(), radius, qty );
    }

    return wanted_qty - qty;
}

std::list<item> Character::use_charges( const itype_id &what, int qty, const int radius,
                                        const std::function<bool( const item & )> &filter, bool in_tools )
{
    std::list<item> res;
    inventory inv = crafting_inventory( pos_bub(), radius, true );

    if( qty <= 0 ) {
        return res;
    }

    if( what == itype_fire ) {
        use_fire( qty );
        return res;

    } else if( what == itype_UPS ) {
        // Fairly sure that nothing comes here. But handle it anyways.
        debugmsg( _( "This UPS use needs updating.  Create issue on github." ) );
        consume_ups( units::from_kilojoule( static_cast<std::int64_t>( qty ) ), radius );
        return res;
    }

    std::vector<item *> del;

    bool has_tool_with_UPS = false;
    // Detection of UPS tool
    inv.visit_items( [ &what, &qty, &has_tool_with_UPS, &filter]( item * e, item * ) {
        if( filter( *e ) && e->typeId() == what && e->has_flag( flag_USE_UPS ) ) {
            has_tool_with_UPS = true;
            return VisitResponse::ABORT;
        }
        return qty > 0 ? VisitResponse::NEXT : VisitResponse::ABORT;
    } );

    if( radius >= 0 ) {
        get_map().use_charges( pos_bub(), radius, what, qty, return_true<item>, nullptr, in_tools );
    }
    if( qty > 0 ) {
        visit_items( [this, &what, &qty, &res, &del, &filter, &in_tools]( item * e, item * ) {
            if( e->use_charges( what, qty, res, pos_bub(), filter, this, in_tools ) ) {
                del.push_back( e );
            }
            return qty > 0 ? VisitResponse::NEXT : VisitResponse::ABORT;
        } );
    }

    for( item *e : del ) {
        remove_item( *e );
    }

    if( has_tool_with_UPS ) {
        consume_ups( units::from_kilojoule( static_cast<std::int64_t>( qty ) ), radius );
    }

    return res;
}

std::list<item> Character::use_charges( const itype_id &what, int qty,
                                        const std::function<bool( const item & )> &filter )
{
    return use_charges( what, qty, -1, filter );
}

item Character::find_firestarter_with_charges( const int quantity ) const
{
    item ret;
    cache_has_item_with( flag_FIRESTARTER, [&]( const item & it ) {
        if( !it.typeId()->can_have_charges() ) {
            const use_function *usef = it.type->get_use( "firestarter" );
            if( usef != nullptr && usef->get_actor_ptr() != nullptr ) {
                const firestarter_actor *actor = dynamic_cast<const firestarter_actor *>( usef->get_actor_ptr() );
                if( actor->can_use( *this->as_character(), it, tripoint_bub_ms::zero ).success() ) {
                    ret = it;
                    return true;
                }
            }
        } else if( it.ammo_sufficient( this, quantity ) ) {
            ret = it;
            return true;
        }
        return false;
    } );
    return ret;
}

bool Character::has_fire( const int quantity ) const
{
    // TODO: Replace this with a "tool produces fire" flag.

    if( get_map().has_nearby_fire( pos_bub() ) ) {
        return true;
    }
    if( cache_has_item_with( flag_FIRE ) ) {
        return true;
    }
    if( !find_firestarter_with_charges( quantity ).is_null() ) {
        return true;
    }
    if( is_npc() ) {
        // HACK: A hack to make NPCs use their Molotovs
        return true;
    }

    return false;
}

void Character::mod_painkiller( int npkill )
{
    set_painkiller( pkill + npkill );
}

void Character::set_painkiller( int npkill )
{
    npkill = std::max( npkill, 0 );
    if( pkill != npkill ) {
        const int prev_pain = get_perceived_pain();
        pkill = npkill;
        on_stat_change( "pkill", pkill );
        const int cur_pain = get_perceived_pain();

        if( cur_pain != prev_pain ) {
            react_to_felt_pain( cur_pain - prev_pain );
            on_stat_change( "perceived_pain", cur_pain );
        }
    }
}

int Character::get_painkiller() const
{
    return pkill;
}

void Character::clear_moncams()
{
    moncams.clear();
}

void Character::remove_moncam( const mtype_id &moncam_id )
{
    moncams.erase( moncam_id );
}

void Character::add_moncam( const std::pair<mtype_id, int> &moncam )
{
    moncams.insert( moncam );
}

void Character::set_moncams( std::map<mtype_id, int> nmoncams )
{
    moncams = std::move( nmoncams );
}

std::map<mtype_id, int> const &Character::get_moncams() const
{
    return moncams;
}

Character::moncam_cache_t Character::get_active_moncams() const
{
    moncam_cache_t ret;
    for( monster const &mon : g->all_monsters() ) {
        for( const std::pair<const mtype_id, int> &moncam : get_moncams() ) {
            if( mon.type->id == moncam.first && mon.friendly != 0 &&
                rl_dist( get_avatar().pos_abs(), mon.pos_abs() ) < moncam.second ) {
                ret.insert( { &mon, mon.pos_abs() } );
            }
        }
    }
    return ret;
}

void Character::use_fire( const int quantity )
{
    //Okay, so checks for nearby fires first,
    //then held lit torch or candle, bionic tool/lighter/laser
    //tries to use 1 charge of lighters, matches, flame throwers
    //If there is enough power, will use power of one activation of the bio_lighter, bio_tools and bio_laser
    // (home made, military), hotplate, welder in that order.
    // bio_lighter, bio_laser, bio_tools, has_active_bionic("bio_tools"

    if( get_map().has_nearby_fire( pos_bub() ) ) {
        return;
    }
    if( cache_has_item_with( flag_FIRE ) ) {
        return;
    }

    const item firestarter = find_firestarter_with_charges( quantity );
    if( firestarter.is_null() ) {
        return;
    }
    if( firestarter.type->has_flag( flag_USES_BIONIC_POWER ) ) {
        mod_power_level( -quantity * 5_kJ );
        return;
    }
    if( firestarter.ammo_sufficient( this, quantity ) ) {
        use_charges( firestarter.typeId(), quantity );
        return;
    }
}

int Character::heartrate_bpm() const
{
    //Dead have no heartbeat usually and no heartbeat in omnicell
    if( is_dead_state() || has_trait( trait_SLIMESPAWNER ) ) {
        return 0;
    }

    int average_heartbeat = avg_nat_bpm * get_heartrate_index();

    // minor moment-to-moment variation
    average_heartbeat += rng( -5, 5 );

    //Chemical imbalance makes this less predictable. It's possible this range needs tweaking
    if( has_trait( trait_CHEMIMBALANCE ) ) {
        average_heartbeat += rng( -15, 15 );
    }

    return average_heartbeat;
}

void Character::on_worn_item_washed( const item &it )
{
    if( is_worn( it ) ) {
        morale->on_worn_item_washed( it );
    }
}

void Character::on_item_wear( const item &it )
{
    invalidate_inventory_validity_cache();
    invalidate_leak_level_cache();
    morale->on_item_wear( it );
}

void Character::on_item_takeoff( const item &it )
{
    invalidate_inventory_validity_cache();
    invalidate_weight_carried_cache();
    invalidate_leak_level_cache();
    morale->on_item_takeoff( it );
}

void Character::on_item_acquire( const item &it )
{
    bool check_for_zoom = is_avatar();
    bool update_overmap_seen = false;

    it.visit_items( [this, &check_for_zoom, &update_overmap_seen]( item * cont_it, item * ) {
        add_to_inv_search_caches( *cont_it );
        if( check_for_zoom && !update_overmap_seen && cont_it->has_flag( flag_ZOOM ) ) {
            update_overmap_seen = true;
        }
        return VisitResponse::NEXT;
    } );

    invalidate_weight_carried_cache();

    if( update_overmap_seen ) {
        g->update_overmap_seen();
    }
}

void Character::on_effect_int_change( const efftype_id &eid, int intensity,
                                      const bodypart_id &bp )
{
    // Adrenaline can reduce perceived pain (or increase it when you enter comedown).
    // See @ref get_perceived_pain()
    if( eid == effect_adrenaline ) {
        // Note that calling this does no harm if it wasn't changed.
        on_stat_change( "perceived_pain", get_perceived_pain() );
    }

    morale->on_effect_int_change( eid, intensity, bp );
}

void Character::on_mutation_gain( const trait_id &mid )
{
    morale->on_mutation_gain( mid );
    magic->on_mutation_gain( mid, *this );
    update_type_of_scent( mid );
    effect_on_conditions::process_reactivate( *this );
    if( is_avatar() ) {
        as_avatar()->character_mood_face( true );
    }
}

void Character::on_mutation_loss( const trait_id &mid )
{
    morale->on_mutation_loss( mid );
    magic->on_mutation_loss( mid, *this );
    update_type_of_scent( mid, false );
    effect_on_conditions::process_reactivate( *this );
    if( is_avatar() ) {
        as_avatar()->character_mood_face( true );
    }
}

void Character::on_stat_change( const std::string &stat, int value )
{
    morale->on_stat_change( stat, value );
}

bool Character::has_opposite_trait( const trait_id &flag ) const
{
    return !get_opposite_traits( flag ).empty();
}

std::unordered_set<trait_id> Character::get_opposite_traits( const trait_id &flag ) const
{
    std::unordered_set<trait_id> traits;
    for( const trait_id &i : flag->cancels ) {
        if( has_trait( i ) ) {
            traits.insert( i );
        }
    }
    for( const std::pair<const trait_id, trait_data> &mut : cached_mutations ) {
        for( const trait_id &canceled_trait : mut.first->cancels ) {
            if( canceled_trait == flag ) {
                traits.insert( mut.first );
            }
        }
    }
    return traits;
}

int Character::get_effective_focus() const
{
    int effective_focus = get_focus();
    effective_focus = enchantment_cache->modify_value( enchant_vals::mod::LEARNING_FOCUS,
                      effective_focus );
    effective_focus *= 1.0 + ( 0.01f * ( get_int() -
                                         get_option<int>( "INT_BASED_LEARNING_BASE_VALUE" ) ) *
                               get_option<int>( "INT_BASED_LEARNING_FOCUS_ADJUSTMENT" ) );
    return std::max( effective_focus, 1 );
}

float Character::adjust_for_focus( float amount ) const
{
    return amount * ( get_effective_focus() / 100.0f );
}

std::function<bool( const tripoint_bub_ms & )> Character::get_path_avoid() const
{
    const map &here = get_map();

    // TODO: Add known traps in a way that doesn't destroy performance

    return [this, &here]( const tripoint_bub_ms & p ) {
        Creature *critter = get_creature_tracker().creature_at( p, true );
        return critter && critter->is_npc() && this->sees( here, *critter );
    };
}

const pathfinding_settings &Character::get_pathfinding_settings() const
{
    return *path_settings;
}

ret_val<crush_tool_type> Character::can_crush_frozen_liquid( item_location const &loc ) const
{
    crush_tool_type tool_type = CRUSH_NO_TOOL;
    bool success = false;
    if( !loc.has_parent() || !loc.parent_pocket()->get_pocket_data()->rigid ) {
        tool_type = CRUSH_HAMMER;
        success = has_quality( qual_HAMMER );
    } else {
        bool enough_quality = false;
        tool_type = CRUSH_DRILL_OR_HAMMER_AND_SCREW;
        if( has_quality( qual_DRILL ) ) {
            enough_quality = true;
        } else if( has_quality( qual_HAMMER ) && has_quality( qual_SCREW ) ) {
            int num_hammer_tools = 0;
            int num_screw_tools = 0;
            int num_both_tools = 0;
            for( item_location &tool : const_cast<Character *>( this )->all_items_loc() ) {
                bool can_hammer = false;
                bool can_screw = false;
                if( tool->has_quality_nonrecursive( qual_HAMMER ) ) {
                    can_hammer = true;
                    num_hammer_tools++;
                }
                if( tool->has_quality_nonrecursive( qual_SCREW ) ) {
                    can_screw = true;
                    num_screw_tools++;
                }
                if( can_hammer && can_screw ) {
                    num_both_tools++;
                }
            }
            if( num_hammer_tools > 0 && num_screw_tools > 0 &&
                // Only one tool that fulfils both requirements: can't hammer itself
                !( num_hammer_tools == 1 && num_screw_tools == 1 && num_both_tools == 1 ) ) {
                enough_quality = true;
            }
        }
        success = enough_quality;
    }
    if( success ) {
        return ret_val<crush_tool_type>::make_success( tool_type );
    } else {
        return ret_val<crush_tool_type>::make_failure( tool_type,
                _( "You don't have the tools to crush frozen liquids." ) );
    }
}

bool Character::crush_frozen_liquid( item_location loc )
{
    map &here = get_map();

    ret_val<crush_tool_type> can_crush = can_crush_frozen_liquid( loc );
    bool done_crush = false;
    if( can_crush.success() ) {
        done_crush = true;
        if( can_crush.value() == CRUSH_HAMMER ) {
            item &hammering_item = best_item_with_quality( qual_HAMMER );
            //~ %1$s: item to be crushed, %2$s: hammer name
            if( query_yn( _( "Do you want to crush up %1$s with your %2$s?\n"
                             "<color_red>Be wary of fragile items nearby!</color>" ),
                          loc.get_item()->display_name(), hammering_item.tname() ) ) {

                //Risk smashing tile with hammering tool, risk is lower with higher dex, damage lower with lower strength
                if( one_in( 1 + dex_cur / 4 ) ) {
                    add_msg_if_player( colorize( _( "You swing your %s wildly!" ), c_red ),
                                       hammering_item.tname() );
                    // FIXME: Hardcoded damage type
                    const int smashskill = get_arm_str() + hammering_item.damage_melee( damage_bash );
                    here.bash( loc.pos_bub( here ), smashskill );
                }
            } else {
                done_crush = false;
            }
        }
    } else {
        std::string need_str = _( "somethings" );

        switch( can_crush.value() ) {
            case CRUSH_HAMMER:
                need_str = _( "a hammering tool" );
                break;
            case CRUSH_DRILL_OR_HAMMER_AND_SCREW:
                need_str = _( "a drill or both hammering tool and screwdriver" );
                break;
            default:
                break;
        }
        add_msg_if_player( string_format( _( "You need %s to crush up frozen liquids in rigid container!" ),
                                          need_str ) );
    }
    if( done_crush ) {
        add_msg_if_player( _( "You crush up %s." ), loc.get_item()->display_name() );
    }
    return done_crush;
}

float Character::power_rating() const
{
    int dmg = 0;
    for( const damage_type &dt : damage_type::get_all() ) {
        if( dt.melee_only ) {
            int tdmg = weapon.damage_melee( dt.id );
            dmg = dmg < tdmg ? tdmg : dmg;
        }
    }

    int ret = 2;
    // Small guns can be easily hidden from view
    if( weapon.volume() <= 250_ml ) {
        ret = 2;
    } else if( weapon.is_gun() ) {
        ret = 4;
    } else if( dmg > 12 ) {
        ret = 3; // Melee weapon or weapon-y tool
    }
    if( get_size() == creature_size::huge ) {
        ret += 1;
    }
    if( is_wearing_power_armor( nullptr ) ) {
        ret = 5; // No mercy!
    }
    return ret;
}

float Character::speed_rating() const
{
    float ret = get_speed() / 100.0f;
    ret *= 100.0f / run_cost( 100, false );
    // Adjustment for player being able to run, but not doing so at the moment
    if( !is_running() ) {
        ret *= 1.0f + ( static_cast<float>( get_stamina() ) / static_cast<float>( get_stamina_max() ) );
    }
    return ret;
}

item &Character::best_item_with_quality( const quality_id &qid )
{
    int max_lvl_found = INT_MIN;
    std::vector<item *> items = items_with( [qid, &max_lvl_found]( const item & it ) {
        int qlvl = it.get_quality_nonrecursive( qid );
        if( qlvl > max_lvl_found ) {
            max_lvl_found = qlvl;
            return true;
        }
        return false;
    } );
    if( max_lvl_found > INT_MIN ) {
        return *items.back();
    }
    return null_item_reference();
}

const item &Character::best_item_with_quality( const quality_id &qid ) const
{
    int max_lvl_found = INT_MIN;
    std::vector<const item *> items = items_with( [qid, &max_lvl_found]( const item & it ) {
        int qlvl = it.get_quality_nonrecursive( qid );
        if( qlvl > max_lvl_found ) {
            max_lvl_found = qlvl;
            return true;
        }
        return false;
    } );
    if( max_lvl_found > INT_MIN ) {
        return *items.back();
    }
    return null_item_reference();
}

int Character::run_cost( int base_cost, bool diag ) const
{
    float movecost = static_cast<float>( base_cost );
    if( diag ) {
        movecost /= M_SQRT2; // because effect logic assumes 100 base cost
    }
    run_cost_effects( movecost );
    if( diag ) {
        movecost *= M_SQRT2;
    }
    return static_cast<int>( movecost );
}

std::vector<run_cost_effect> Character::run_cost_effects( float &movecost ) const
{
    std::vector<run_cost_effect> effects;

    auto run_cost_effect_add = [&movecost, &effects]( float mod, const std::string & desc ) {
        if( mod != 0 ) {
            run_cost_effect effect { desc, 1.0, mod };
            effects.push_back( effect );
            movecost += mod;
        }
    };

    auto run_cost_effect_mul = [&movecost, &effects]( float mod, const std::string & desc ) {
        if( mod != 1.0 ) {
            run_cost_effect effect { desc, mod, 0.0 };
            effects.push_back( effect );
            movecost *= mod;
        }
    };

    const bool flatground = movecost < 105;
    map &here = get_map();
    // The "FLAT" tag includes soft surfaces, so not a good fit.
    const bool on_road = flatground && here.has_flag( ter_furn_flag::TFLAG_ROAD, pos_bub() );
    const bool on_fungus = here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FUNGUS, pos_bub() );
    const bool water_walking = here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_SWIMMABLE, pos_bub() ) &&
                               has_flag( json_flag_WATERWALKING );

    if( is_mounted() ) {
        return effects;
    }

    if( water_walking ) {
        movecost = 100;
    }

    if( movecost > 105 ) {
        float obstacle_mult = enchantment_cache->modify_value( enchant_vals::mod::MOVECOST_OBSTACLE_MOD,
                              1 );
        run_cost_effect_mul( obstacle_mult, _( "Obstacle Muts." ) );

        if( has_proficiency( proficiency_prof_parkour ) && !is_prone() ) {
            run_cost_effect_mul( 0.5, _( "Parkour" ) );
        }


        if( movecost < 100 ) {
            run_cost_effect effect { _( "Bonuses Capped" ) };
            effects.push_back( effect );
            movecost = 100;
        }
    }
    if( has_trait( trait_M_IMMUNE ) && on_fungus ) {
        if( movecost > 75 ) {
            // Mycal characters are faster on their home territory, even through things like shrubs
            run_cost_effect_add( 75 - movecost, _( "Mycus on Fungus" ) );
        }
    }

    if( is_prone() ) {
        run_cost_effect_mul( get_modifier( character_modifier_crawl_speed_movecost_mod ),
                             _( "Crawling" ) );
    } else {
        run_cost_effect_mul( get_modifier( character_modifier_limb_run_cost_mod ),
                             _( "Encum./Wounds" ) );
    }

    if( flatground ) {
        float flatground_mult = enchantment_cache->modify_value( enchant_vals::mod::MOVECOST_FLATGROUND_MOD,
                                1 );
        run_cost_effect_mul( flatground_mult, _( "Flat Ground Mut." ) );
    }

    if( worn_with_flag( flag_FIN ) ) {
        run_cost_effect_mul( 1.5f, _( "Swim Fins" ) );
    }

    if( worn_with_flag( flag_ROLLER_INLINE ) ) {
        if( on_road ) {
            if( is_running() ) {
                run_cost_effect_mul( 0.5f, _( "Inline Skates" ) );
            } else if( is_walking() ) {
                run_cost_effect_mul( 0.85f, _( "Inline Skates" ) );
            }
        } else {
            run_cost_effect_mul( 1.5f, _( "Inline Skates" ) );
        }
    }

    // Quad skates might be more stable than inlines,
    // but that also translates into a slower speed when on good surfaces.
    if( worn_with_flag( flag_ROLLER_QUAD ) ) {
        if( on_road ) {
            if( is_running() ) {
                run_cost_effect_mul( 0.7f, _( "Roller Skates" ) );
            } else if( is_walking() ) {
                run_cost_effect_mul( 0.85f, _( "Roller Skates" ) );
            }
        } else {
            run_cost_effect_mul( 1.3f, _( "Roller Skates" ) );
        }
    }

    // Skates with only one wheel (roller shoes) are fairly less stable
    // and fairly slower as well
    if( worn_with_flag( flag_ROLLER_ONE ) ) {
        if( on_road ) {
            if( is_running() ) {
                run_cost_effect_mul( 0.85f, _( "Heelys" ) );
            } else if( is_walking() ) {
                run_cost_effect_mul( 0.9f, _( "Heelys" ) );
            }
        } else {
            run_cost_effect_mul( 1.1f, _( "Heelys" ) );
        }
    }

    // Additional move cost for moving barefoot only if we're not swimming
    if( !here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, pos_bub() ) ) {
        // ROOTS3 does slow you down as your roots are probing around for nutrients,
        // whether you want them to or not.  ROOTS1 is just too squiggly without shoes
        // to give you some stability.  Plants are a bit of a slow-mover.  Deal.
        const bool mutfeet = has_flag( json_flag_TOUGH_FEET ) || worn_with_flag( flag_TOUGH_FEET );
        bool no_left_shoe = false;
        bool no_right_shoe = false;
        if( !is_wearing_shoes( side::LEFT ) && !mutfeet ) {
            no_left_shoe = true;
        }
        if( !is_wearing_shoes( side::RIGHT ) && !mutfeet ) {
            no_right_shoe = true;
        }
        if( no_left_shoe && no_right_shoe ) {
            run_cost_effect_add( 16, _( "No Shoes" ) );
        } else if( no_left_shoe ) {
            run_cost_effect_add( 8, _( "No Left Shoe" ) );
        } else if( no_right_shoe ) {
            run_cost_effect_add( 8, _( "No Right Shoe" ) );
        }
    }

    if( ( has_trait( trait_ROOTS3 ) || has_trait( trait_CHLOROMORPH ) ) &&
        here.has_flag( ter_furn_flag::TFLAG_DIGGABLE, pos_bub() ) && is_barefoot() ) {
        run_cost_effect_add( 10, _( "Roots" ) );
    }

    run_cost_effect_add( enchantment_cache->get_value_add( enchant_vals::mod::MOVE_COST ),
                         _( "Enchantments" ) );
    run_cost_effect_mul( std::max( 0.01,
                                   1.0 + enchantment_cache->get_value_multiply( enchant_vals::mod::MOVE_COST ) ),
                         _( "Enchantments" ) );

    run_cost_effect_mul( 1.0 / get_modifier( character_modifier_stamina_move_cost_mod ),
                         _( "Stamina" ) );

    run_cost_effect_mul( 1.0 / get_modifier( character_modifier_move_mode_move_cost_mod ),
                         //TODO get these strings from elsewhere
                         is_running() ? _( "Running" ) :
                         is_crouching() ? _( "Crouching" ) :
                         is_prone() ? _( "Prone" ) : _( "Walking" )
                       );

    if( !is_mounted() && !is_prone() && has_effect( effect_downed ) ) {
        run_cost_effect_mul( get_modifier( character_modifier_crawl_speed_movecost_mod ) * 2.5,
                             _( "Downed" ) );
    }

    // minimum possible movecost is 1, no infinispeed
    movecost = std::max( 1.0f, movecost );

    return effects;
}

void Character::place_corpse( map *here )
{
    //If the character/NPC is on a distant mission, don't drop their their gear when they die since they still have a local pos
    if( !death_drops ) {
        return;
    }
    std::vector<item *> tmp = inv_dump();
    item body = item::make_corpse( mtype_id::NULL_ID(), calendar::turn, get_name() );
    body.set_item_temperature( units::from_celsius( 37 ) );
    for( item *itm : tmp ) {
        body.force_insert_item( *itm, pocket_type::CONTAINER );
    }
    // One sample, as you would get from dissecting any other human.
    body.put_in( item( itype_human_sample ), pocket_type::CORPSE );

    for( const bionic &bio : *my_bionics ) {
        if( item::type_is_defined( bio.info().itype() ) ) {
            item cbm( bio.info().itype(), calendar::turn );
            cbm.set_flag( flag_FILTHY );
            cbm.set_flag( flag_NO_STERILE );
            cbm.set_flag( flag_NO_PACKED );
            cbm.set_fault( fault_bionic_salvaged );
            body.put_in( cbm, pocket_type::CORPSE );
        }
    }

    here->add_item_or_charges( pos_bub( *here ), body );
}

void Character::place_corpse( const tripoint_abs_omt &om_target )
{
    tinymap bay;
    bay.load( om_target, false );
    // Redundant as long as map operations aren't using get_map() in a transitive call chain. Added for future proofing.
    swap_map swap( *bay.cast_to_map() );
    point_omt_ms fin( rng( 1, SEEX * 2 - 2 ), rng( 1, SEEX * 2 - 2 ) );
    // This makes no sense at all. It may find a random tile without furniture, but
    // if the first try to find one fails, it will go through all tiles of the map
    // and essentially select the last one that has no furniture.
    // Q: Why check for furniture? (Check for passable or can-place-items seems more useful.) A: Furniture blocks revival?
    // Q: Why not grep a random point out of all the possible points (e.g. via random_entry)?
    // TODO: fix it, see above.
    if( bay.furn( fin ) != furn_str_id::NULL_ID() ) {
        for( const tripoint_omt_ms &p : bay.points_on_zlevel() ) {
            if( bay.furn( p ) == furn_str_id::NULL_ID() ) {
                fin.x() = p.x();
                fin.y() = p.y();
            }
        }
    }

    std::vector<item *> tmp = inv_dump();
    item body = item::make_corpse( mtype_id::NULL_ID(), calendar::turn, get_name() );
    for( item *itm : tmp ) {
        body.force_insert_item( *itm, pocket_type::CONTAINER );
    }
    // One sample, as you would get from dissecting any other human.
    body.put_in( item( itype_human_sample ), pocket_type::CORPSE );

    for( const bionic &bio : *my_bionics ) {
        const itype_id &bio_itype = bio.info().itype();
        if( item::type_is_defined( bio_itype ) ) {
            body.put_in( item( bio_itype, calendar::turn ), pocket_type::CORPSE );
        }
    }

    bay.add_item_or_charges( fin, body );
}

bool Character::is_visible_in_range( const Creature &critter, const int range ) const
{
    const map &here = get_map();

    return sees( here, critter ) && rl_dist( pos_abs(), critter.pos_abs() ) <= range;
}

std::vector<Creature *> Character::get_visible_creatures( const int range ) const
{
    const map &here = get_map();

    return g->get_creatures_if( [this, range, &here]( const Creature & critter ) -> bool {
        return this != &critter && pos_abs() != critter.pos_abs() &&
        rl_dist( pos_abs(), critter.pos_abs() ) <= range && sees( here, critter );
    } );
}

std::vector<vehicle *> Character::get_visible_vehicles( const int range ) const
{
    const map &here = get_map();

    return g->get_vehicles_if( [this, range, &here]( const vehicle & veh ) -> bool {
        return rl_dist( pos_abs(), veh.pos_abs() ) <= range && sees( here, veh.pos_bub( here ) );
    } );
}

std::vector<Creature *> Character::get_targetable_creatures( const int range, bool melee ) const
{
    map &here = get_map();
    return g->get_creatures_if( [this, range, melee, &here]( const Creature & critter ) -> bool {
        //the call to map.sees is to make sure that even if we can see it through walls
        //via a mutation or cbm we only attack targets with a line of sight
        bool can_see = ( ( sees( here, critter ) || sees_with_specials( critter ) ) && here.sees( pos_bub( here ), critter.pos_bub( here ), 100 ) );
        if( can_see && melee )  //handles the case where we can see something with glass in the way for melee attacks
        {
            std::vector<tripoint_bub_ms> path = here.find_clear_path( pos_bub( here ),
                    critter.pos_bub( here ) );
            for( const tripoint_bub_ms &point : path ) {
                if( here.impassable( point ) &&
                    !( weapon.has_flag( flag_SPEAR ) && // Fences etc. Spears can stab through those
                       here.has_flag( ter_furn_flag::TFLAG_THIN_OBSTACLE,
                                      point ) ) ) { //this mirrors melee.cpp function reach_attack
                    can_see = false;
                    break;
                }
            }
        }
        bool in_range = std::round( rl_dist_exact( pos_abs(), critter.pos_abs() ) ) <= range;
        bool valid_target = this != &critter && pos_abs() != critter.pos_abs() && attitude_to( critter ) != Creature::Attitude::FRIENDLY;
        return valid_target && in_range && can_see;
    } );
}

int Character::get_mutation_visibility_cap( const Character *observed ) const
{
    // as of now, visibility of mutations is between 0 and 10
    // 10 perception and 10 distance would see all mutations - cap 0
    // 10 perception and 30 distance - cap 5, some mutations visible
    // 3 perception and 3 distance would see all mutations - cap 0
    // 3 perception and 15 distance - cap 5, some mutations visible
    // 3 perception and 20 distance would be barely able to discern huge antlers on a person - cap 10
    const int dist = rl_dist( pos_bub(), observed->pos_bub() );
    int visibility_cap;
    if( per_cur <= 1 ) {
        visibility_cap = INT_MAX;
    } else {
        visibility_cap = std::round( dist * dist / 20.0 / ( per_cur - 1 ) );
    }
    return visibility_cap;
}

std::vector<Creature *> Character::get_hostile_creatures( int range ) const
{
    const map &here = get_map();

    return g->get_creatures_if( [this, range, &here]( const Creature & critter ) -> bool {
        // Fixes circular distance range for ranged attacks
        float dist_to_creature = std::round( rl_dist_exact( pos_abs(), critter.pos_abs() ) );
        return this != &critter && pos_abs() != critter.pos_abs() &&
        dist_to_creature <= range && critter.attitude_to( *this ) == Creature::Attitude::HOSTILE
        && sees( here, critter );
    } );
}

void Character::echo_pulse()
{
    map &here = get_map();
    int echo_volume = 0;
    int pulse_range = 10;
    // Sound travels farther underwater
    if( has_effect( effect_subaquatic_sonar ) && is_underwater() ) {
        pulse_range = 16;
        sounds::sound( this->pos_bub(), 5, sounds::sound_t::movement, _( "boop." ), true,
                       "none", "none" );
    } else if( !has_effect( effect_subaquatic_sonar ) && is_underwater() ) {
        add_msg_if_player( m_warning, _( "You can't echolocate underwater!" ) );
        return;
    } else {
        sounds::sound( this->pos_bub(), 5, sounds::sound_t::movement, _( "chirp." ), true,
                       "none", "none" );
    }
    for( tripoint_bub_ms origin : points_in_radius( pos_bub(), pulse_range ) ) {
        if( here.move_cost( origin ) == 0 && here.sees( pos_bub(), origin, pulse_range, false ) ) {
            sounds::sound( origin, 5, sounds::sound_t::sensory, _( "clack." ), true,
                           "none", "none" );
            // This only counts obstacles which can be moved through, so the echo is pretty quiet.
        } else if( is_obstacle( origin ) && here.sees( pos_bub(), origin, pulse_range, false ) ) {
            sounds::sound( origin, 1, sounds::sound_t::sensory, _( "click." ), true,
                           "none", "none" );
        }
        const trap &tr = here.tr_at( origin );
        if( !knows_trap( origin ) && tr.detected_by_echolocation() ) {
            const std::string direction = direction_name( direction_from( pos_bub(), origin ) );
            add_msg_if_player( m_warning, _( "You detect a %1$s to the %2$s!" ),
                               tr.name(), direction );
            add_known_trap( origin, tr );
        }
        Creature *critter = get_creature_tracker().creature_at( origin, true );
        if( critter && here.sees( pos_bub(), origin, pulse_range, false ) ) {
            switch( critter->get_size() ) {
                case creature_size::tiny:
                    echo_volume = 1;
                    break;
                case creature_size::small:
                    echo_volume = 2;
                    break;
                case creature_size::medium:
                    echo_volume = 3;
                    break;
                case creature_size::large:
                    echo_volume = 4;
                    break;
                case creature_size::huge:
                    echo_volume = 5;
                    break;
                case creature_size::num_sizes:
                    debugmsg( "ERROR: Invalid Creature size class." );
                    break;
            }
            // Some monsters are harder to get a read on
            if( critter->has_flag( mon_flag_PLASTIC ) ) {
                echo_volume -= std::max( 1, 1 );
            }
            if( critter->has_flag( mon_flag_HARDTOSHOOT ) ) {
                echo_volume -= std::max( 1, 1 );
            }
            const char *echo_string = nullptr;
            // bio_targeting has a visual HUD and automatically interprets the raw audio data,
            // but only for electronic SONAR. Its designers didn't anticipate bat mutations
            if( has_bionic( bio_targeting ) && has_effect( effect_subaquatic_sonar ) ) {
                switch( echo_volume ) {
                    case 1:
                        echo_string = _( "Target [Tiny]." );
                        break;
                    case 2:
                        echo_string = _( "Target [Small]." );
                        break;
                    case 3:
                        echo_string = _( "Target [Medium]." );
                        break;
                    case 4:
                        echo_string = _( "Warning!  Target [Large]." );
                        break;
                    case 5:
                        echo_string = _( "Warning!  Target [Huge]." );
                        break;
                    default:
                        debugmsg( "ERROR: Invalid echo string." );
                        break;
                }
            } else if( !has_bionic( bio_targeting ) && has_effect( effect_subaquatic_sonar ) ) {
                switch( echo_volume ) {
                    case 1:
                        echo_string = _( "tick." );
                        break;
                    case 2:
                        echo_string = _( "pii." );
                        break;
                    case 3:
                        echo_string = _( "ping." );
                        break;
                    case 4:
                        echo_string = _( "pong." );
                        break;
                    case 5:
                        echo_string = _( "bloop." );
                        break;
                    default:
                        debugmsg( "ERROR: Invalid echo string." );
                        break;
                }
            } else {
                switch( echo_volume ) {
                    case 1:
                        echo_string = _( "ch." );
                        break;
                    case 2:
                        echo_string = _( "chk." );
                        break;
                    case 3:
                        echo_string = _( "chhk." );
                        break;
                    case 4:
                        echo_string = _( "chkch." );
                        break;
                    case 5:
                        echo_string = _( "chkchh." );
                        break;
                    default:
                        debugmsg( "ERROR: Invalid echo string." );
                        break;
                }
            }
            // It's not moving. Must be an obstacle
            if( critter->has_flag( mon_flag_IMMOBILE ) ||
                critter->has_flag( json_flag_CANNOT_MOVE ) ) {
                echo_string = _( "click." );
            }
            sounds::sound( origin, echo_volume, sounds::sound_t::sensory, _( echo_string ), false,
                           "none", "none" );
        }
    }
}

bool Character::knows_trap( const tripoint_bub_ms &pos ) const
{
    const tripoint_abs_ms p = get_map().get_abs( pos );
    return known_traps.count( p ) > 0;
}

void Character::add_known_trap( const tripoint_bub_ms &pos, const trap &t )
{
    const tripoint_abs_ms p = get_map().get_abs( pos );
    if( t.is_null() ) {
        known_traps.erase( p );
    } else {
        // TODO: known_traps should map to a trap_str_id
        known_traps[p] = t.id.str();
    }
}

bool Character::can_hear( const tripoint_bub_ms &source, const int volume ) const
{
    if( is_deaf() ) {
        return false;
    }

    // source is in-ear and at our square, we can hear it
    if( source == pos_bub() && volume == 0 ) {
        return true;
    }
    const int dist = rl_dist( source, pos_bub() );
    const float volume_multiplier = hearing_ability();
    return ( volume - get_weather().weather_id->sound_attn ) * volume_multiplier >= dist;
}

float Character::hearing_ability() const
{
    float volume_multiplier = 1.0f;

    volume_multiplier = enchantment_cache->modify_value( enchant_vals::mod::HEARING_MULT,
                        volume_multiplier );

    if( has_effect( effect_deaf ) ) {
        // Scale linearly up to 30 minutes
        volume_multiplier *= ( 30_minutes - get_effect_dur( effect_deaf ) ) / 30_minutes;
    }

    return volume_multiplier;
}

std::vector<std::string> Character::short_description_parts() const
{
    std::vector<std::string> result;

    if( is_armed() ) {
        result.push_back( _( "Wielding: " ) + weapon.tname() );
    }
    const std::list<item_location> visible_worn_items = get_visible_worn_items();
    const std::string worn_str = enumerate_as_string( visible_worn_items.begin(),
                                 visible_worn_items.end(),
    []( const item_location & it ) {
        return it.get_item()->tname();
    } );
    if( !worn_str.empty() ) {
        result.push_back( _( "Wearing: " ) + worn_str );
    }
    const int visibility_cap = 0; // no cap
    const std::string trait_str = visible_mutations( visibility_cap );
    if( !trait_str.empty() ) {
        result.push_back( _( "Traits: " ) + trait_str );
    }
    return result;
}

std::string Character::short_description() const
{
    return string_join( short_description_parts(), ";   " );
}

void Character::process_one_effect( effect &it, bool is_new )
{
    bool reduced = resists_effect( it );
    double mod = 1;
    const bodypart_id &bp = it.get_bp();
    int val = 0;

    // Still hardcoded stuff, do this first since some modify their other traits
    int str = get_str_bonus();
    int dex = get_dex_bonus();
    int intl = get_int_bonus();
    int per = get_per_bonus();
    hardcoded_effects( it );
    str_bonus_hardcoded += get_str_bonus() - str;
    dex_bonus_hardcoded += get_dex_bonus() - dex;
    int_bonus_hardcoded += get_int_bonus() - intl;
    per_bonus_hardcoded += get_per_bonus() - per;

    const auto get_effect = [&it, is_new]( const std::string & arg, bool reduced ) {
        if( is_new ) {
            return it.get_amount( arg, reduced );
        }
        return it.get_mod( arg, reduced );
    };

    // Handle miss messages
    const std::vector<std::pair<translation, int>> &msgs = it.get_miss_msgs();
    for( const auto &i : msgs ) {
        add_miss_reason( i.first.translated(), static_cast<unsigned>( i.second ) );
    }

    // Handle vitamins
    for( const vitamin_applied_effect &vit : it.vit_effects( reduced ) ) {
        if( vit.tick && vit.rate && calendar::once_every( *vit.tick ) ) {
            const int mod = rng( vit.rate->first, vit.rate->second );
            vitamin_mod( vit.vitamin, mod );
        }
    }

    // Handle health mod
    val = get_effect( "H_MOD", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "H_MOD", val, reduced, mod ) ) {
            int bounded = bound_mod_to_vals(
                              get_daily_health(), val, it.get_max_val( "H_MOD", reduced ),
                              it.get_min_val( "H_MOD", reduced ) );
            // This already applies bounds, so we pass them through.
            mod_daily_health( bounded, get_daily_health() + bounded );
        }
    }

    // Handle health
    val = get_effect( "HEALTH", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "HEALTH", val, reduced, mod ) ) {
            mod_livestyle( bound_mod_to_vals( get_lifestyle(), val,
                                              it.get_max_val( "HEALTH", reduced ), it.get_min_val( "HEALTH", reduced ) ) );
        }
    }

    // Handle stim
    val = get_effect( "STIM", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "STIM", val, reduced, mod ) ) {
            mod_stim( bound_mod_to_vals( get_stim(), val, it.get_max_val( "STIM", reduced ),
                                         it.get_min_val( "STIM", reduced ) ) );
        }
    }

    // Handle hunger
    val = get_effect( "HUNGER", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "HUNGER", val, reduced, mod ) ) {
            mod_hunger( bound_mod_to_vals( get_hunger(), val, it.get_max_val( "HUNGER", reduced ),
                                           it.get_min_val( "HUNGER", reduced ) ) );
        }
    }

    // Handle thirst
    val = get_effect( "THIRST", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "THIRST", val, reduced, mod ) ) {
            mod_thirst( bound_mod_to_vals( get_thirst(), val, it.get_max_val( "THIRST", reduced ),
                                           it.get_min_val( "THIRST", reduced ) ) );
        }
    }

    // Handle perspiration
    val = get_effect( "PERSPIRATION", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "PERSPIRATION", val, reduced, mod ) ) {
            // multiplier to balance values around drench capacity of different body parts
            int mult = enchantment_cache->modify_value( enchant_vals::mod::SWEAT_MULTIPLIER,
                       1 ) * get_part_drench_capacity( bp ) / 100;
            mod_part_wetness( bp, bound_mod_to_vals( get_part_wetness( bp ), val * mult,
                              it.get_max_val( "PERSPIRATION", reduced ) * mult,
                              it.get_min_val( "PERSPIRATION", reduced ) * mult ) );
        }
    }

    // Handle sleepiness
    val = get_effect( "SLEEPINESS", reduced );
    // Prevent ongoing sleepiness effects while asleep.
    // These are meant to change how fast you get tired, not how long you sleep.
    if( val != 0 && !in_sleep_state() ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "SLEEPINESS", val, reduced, mod ) ) {
            mod_sleepiness( bound_mod_to_vals( get_sleepiness(), val, it.get_max_val( "SLEEPINESS", reduced ),
                                               it.get_min_val( "SLEEPINESS", reduced ) ) );
        }
    }

    // Handle Radiation
    val = get_effect( "RAD", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "RAD", val, reduced, mod ) ) {
            mod_rad( bound_mod_to_vals( get_rad(), val, it.get_max_val( "RAD", reduced ), 0 ) );
            // Radiation can't go negative
            if( get_rad() < 0 ) {
                set_rad( 0 );
            }
        }
    }

    // Handle Pain
    val = get_effect( "PAIN", reduced );
    if( val != 0 ) {
        mod = 1;
        if( it.get_sizing( "PAIN" ) ) {
            if( has_trait( trait_FAT ) ) {
                mod *= 1.5;
            }
            if( get_size() == creature_size::large ) {
                mod *= 2;
            }
            if( get_size() == creature_size::huge ) {
                mod *= 3;
            }
        }
        if( is_new || it.activated( calendar::turn, "PAIN", val, reduced, mod ) ) {
            int pain_inc = bound_mod_to_vals( get_pain(), val, it.get_max_val( "PAIN", reduced ), 0 );
            mod_pain( pain_inc );
            if( pain_inc > 0 ) {
                add_pain_msg( val, bp );
            }
        }
    }

    // Handle Damage
    val = get_effect( "HURT", reduced );
    if( val != 0 ) {
        mod = 1;
        if( it.get_sizing( "HURT" ) ) {
            if( has_trait( trait_FAT ) ) {
                mod *= 1.5;
            }
            if( get_size() == creature_size::large ) {
                mod *= 2;
            }
            if( get_size() == creature_size::huge ) {
                mod *= 3;
            }
        }
        if( is_new || it.activated( calendar::turn, "HURT", val, reduced, mod ) ) {
            if( bp == bodypart_str_id::NULL_ID() ) {
                if( val > 5 ) {
                    add_msg_if_player( m_bad, _( "Your %s HURTS!" ), body_part_name( body_part_torso ) );
                } else if( val > 0 ) {
                    add_msg_if_player( m_bad, _( "Your %s hurts!" ), body_part_name( body_part_torso ) );
                }
                apply_damage( nullptr, body_part_torso, val, true );
            } else {
                if( val > 5 ) {
                    add_msg_if_player( m_bad, _( "Your %s HURTS!" ), body_part_name( bp ) );
                } else if( val > 0 )  {
                    add_msg_if_player( m_bad, _( "Your %s hurts!" ), body_part_name( bp ) );
                }
                apply_damage( nullptr, bp, val, true );
            }
        }
    }

    // Handle Sleep
    val = get_effect( "SLEEP", reduced );
    if( val != 0 ) {
        mod = 1;
        if( ( is_new || it.activated( calendar::turn, "SLEEP", val, reduced, mod ) ) &&
            !has_effect( effect_sleep ) ) {
            add_msg_if_player( _( "You pass out!" ) );
            fall_asleep( time_duration::from_turns( val ) );
        }
    }

    // Handle painkillers
    val = get_effect( "PKILL", reduced );
    if( val != 0 ) {
        mod = it.get_addict_mod( "PKILL", addiction_level( addiction_opiate ) );
        if( is_new || it.activated( calendar::turn, "PKILL", val, reduced, mod ) ) {
            mod_painkiller( bound_mod_to_vals( get_painkiller(), val, it.get_max_val( "PKILL", reduced ), 0 ) );
        }
    }

    // Handle Blood Pressure
    val = get_effect( "BLOOD_PRESSURE", reduced );
    if( val != 0 ) {
        if( is_new || it.activated( calendar::turn, "BLOOD_PRESSURE", val, reduced, mod ) ) {
            modify_bp_effect_mod( bound_mod_to_vals( get_bp_effect_mod(), val, it.get_max_val( "BLOOD_PRESSURE",
                                  reduced ), 0 ) );
        }
    }

    // handle heart rate
    val = get_effect( "HEART_RATE", reduced );
    if( val != 0 ) {
        if( is_new || it.activated( calendar::turn, "HEART_RATE", val, reduced, mod ) ) {
            modify_heartrate_effect_mod( bound_mod_to_vals( get_heartrate_effect_mod(), val,
                                         it.get_max_val( "HEART_RATE", reduced ), 0 ) );
        }
    }
    // handle perspiration rate
    val = get_effect( "RESPIRATION_RATE", reduced );
    if( val != 0 ) {
        if( is_new || it.activated( calendar::turn, "RESPIRATION_RATE", val, reduced, mod ) ) {
            modify_respiration_effect_mod( bound_mod_to_vals( get_respiration_effect_mod(), val,
                                           it.get_max_val( "RESPIRATION_RATE", reduced ), 0 ) );
        }
    }


    // Handle coughing
    mod = 1;
    val = 0;
    if( it.activated( calendar::turn, "COUGH", val, reduced, mod ) ) {
        cough( it.get_harmful_cough() );
    }

    // Handle vomiting
    mod = vomit_mod();
    val = 0;
    if( it.activated( calendar::turn, "VOMIT", val, reduced, mod ) ) {
        vomit();
    }

    // Handle stamina
    val = get_effect( "STAMINA", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "STAMINA", val, reduced, mod ) ) {
            mod_stamina( bound_mod_to_vals( get_stamina(), val,
                                            it.get_max_val( "STAMINA", reduced ),
                                            it.get_min_val( "STAMINA", reduced ) ) );
        }
    }

    // Speed and stats are handled in recalc_speed_bonus and reset_stats respectively
}

void Character::process_effects()
{
    map &here = get_map();

    //Special Removals
    if( has_effect( effect_darkness ) && g->is_in_sunlight( pos_bub() ) ) {
        remove_effect( effect_darkness );
    }
    if( has_trait( trait_M_IMMUNE ) && has_effect( effect_fungus ) ) {
        vomit();
        remove_effect( effect_fungus );
        add_msg_if_player( m_bad, _( "We have mistakenly colonized a local guide!  Purging now." ) );
    }
    if( has_trait( trait_PARAIMMUNE ) && ( has_effect( effect_dermatik ) ||
                                           has_effect( effect_tapeworm ) ||
                                           has_effect( effect_blood_spiders ) ||
                                           has_effect( effect_bloodworms ) || has_effect( effect_brainworms ) ||
                                           has_effect( effect_paincysts ) ) ) {
        remove_effect( effect_dermatik );
        remove_effect( effect_tapeworm );
        remove_effect( effect_bloodworms );
        remove_effect( effect_blood_spiders );
        remove_effect( effect_brainworms );
        remove_effect( effect_paincysts );
        add_msg_if_player( m_good, _( "Something writhes inside of you as it dies." ) );
    }
    if( has_flag( json_flag_ACIDBLOOD ) && ( has_effect( effect_dermatik ) ||
            has_effect( effect_bloodworms ) ||
            has_effect( effect_blood_spiders ) ||
            has_effect( effect_brainworms ) ) ) {
        remove_effect( effect_dermatik );
        remove_effect( effect_bloodworms );
        remove_effect( effect_blood_spiders );
        remove_effect( effect_brainworms );
    }
    if( has_trait( trait_EATHEALTH ) && has_effect( effect_tapeworm ) ) {
        remove_effect( effect_tapeworm );
        add_msg_if_player( m_good, _( "Your bowels gurgle as something inside them dies." ) );
    }
    if( has_flag( json_flag_INFECTION_IMMUNE ) && ( has_effect( effect_bite ) ||
            has_effect( effect_infected ) ||
            has_effect( effect_recover ) ) ) {
        remove_effect( effect_bite );
        remove_effect( effect_infected );
        remove_effect( effect_recover );
    }

    //Clear hardcoded bonuses from last turn
    //Recalculated in process_one_effect
    str_bonus_hardcoded = 0;
    dex_bonus_hardcoded = 0;
    int_bonus_hardcoded = 0;
    per_bonus_hardcoded = 0;
    //Human only effects
    for( std::pair<const efftype_id, std::map<bodypart_id, effect>> &elem : *effects ) {
        for( std::pair<const bodypart_id, effect> &_effect_it : elem.second ) {
            process_one_effect( _effect_it.second, false );
        }
    }

    // Apply new effects from effect->effect chains
    while( !scheduled_effects.empty() ) {
        const scheduled_effect_t &effect = scheduled_effects.front();

        add_effect( effect_source::empty(),
                    effect.eff_id,
                    effect.dur,
                    effect.bp,
                    effect.permanent,
                    effect.intensity,
                    effect.force,
                    effect.deferred );

        scheduled_effects.pop();
    }

    // Perform immediate effect removals
    while( !terminating_effects.empty() ) {

        const terminating_effect_t &effect = terminating_effects.front();

        remove_effect( effect.eff_id, effect.bp );

        terminating_effects.pop();
    }

    // Being stuck in tight spaces sucks. TODO: could be expanded to apply to non-vehicle conditions.
    if( will_be_cramped_in_vehicle_tile( here, pos_abs() ) ) {
        if( is_npc() && !has_effect( effect_narcosis ) ) {
            npc &as_npc = dynamic_cast<npc &>( *this );
            as_npc.complain_about( "cramped_vehicle", 30_minutes, "<cramped_vehicle>", false );
        }
    } else {
        remove_effect( effect_cramped_space );
    }

    Creature::process_effects();
}

void Character::gravity_check()
{
    map &here = get_map();
    if( here.is_open_air( pos_bub() ) && !in_vehicle && !has_effect_with_flag( json_flag_GLIDING ) &&
        here.try_fall( pos_bub(), this ) ) {
        here.update_visibility_cache( pos_bub().z() );
    }
}

void Character::gravity_check( map *here )
{
    const tripoint_bub_ms pos = pos_bub( *here );
    if( here->is_open_air( pos ) && !in_vehicle && !has_effect_with_flag( json_flag_GLIDING ) &&
        here->try_fall( pos, this ) ) {
        here->update_visibility_cache( pos.z() );
    }
}

void Character::stagger()
{
    map &here = get_map();
    std::vector<tripoint_bub_ms> valid_stumbles;
    valid_stumbles.reserve( 11 );
    for( const tripoint_bub_ms &dest : here.points_in_radius( pos_bub(), 1 ) ) {
        if( dest != pos_bub() ) {
            if( here.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, dest ) ) {
                valid_stumbles.emplace_back( dest + tripoint::below );
            } else if( here.has_flag( ter_furn_flag::TFLAG_RAMP_UP, dest ) ) {
                valid_stumbles.emplace_back( dest + tripoint::above );
            } else {
                valid_stumbles.push_back( dest );
            }
        }
    }
    const tripoint_bub_ms below = pos_bub( here ) + tripoint::below;
    if( here.valid_move( pos_bub(), below, false, true ) ) {
        valid_stumbles.push_back( below );
    }
    creature_tracker &creatures = get_creature_tracker();
    while( !valid_stumbles.empty() ) {
        bool blocked = false;
        const tripoint_bub_ms dest = random_entry_removed( valid_stumbles );
        const optional_vpart_position vp_there = here.veh_at( dest );
        if( vp_there ) {
            vehicle &veh = vp_there->vehicle();
            if( veh.enclosed_at( &here,  dest ) ) {
                blocked = true;
            }
        }
        if( here.passable( dest ) && !blocked &&
            ( creatures.creature_at( dest, is_hallucination() ) == nullptr ) ) {
            avatar &player_avatar = get_avatar();
            if( is_avatar() && player_avatar.get_grab_type() != object_type::NONE ) {
                player_avatar.grab( object_type::NONE );
            }
            add_msg_player_or_npc( m_warning,
                                   _( "You stumble!" ),
                                   _( "<npcname> stumbles!" ) );
            setpos( here, dest );
            mod_moves( -100 );
            break;
        }
    }
}

double Character::vomit_mod()
{
    double mod = 1;

    mod = enchantment_cache->modify_value( enchant_vals::mod::VOMIT_MUL, mod );

    // If you're already nauseous, any food in your stomach greatly
    // increases chance of vomiting. Liquids don't provoke vomiting, though.
    if( stomach.contains() != 0_ml && has_effect( effect_nausea ) ) {
        mod *= get_effect_int( effect_nausea );
    }
    return mod;
}

bool Character::can_sleep()
{
    if( has_effect( effect_meth ) ) {
        // Sleep ain't happening until that meth wears off completely.
        return false;
    }

    // Since there's a bit of randomness to falling asleep, we want to
    // prevent exploiting this if can_sleep() gets called over and over.
    // Only actually check if we can fall asleep no more frequently than
    // every 30 minutes.  We're assuming that if we return true, we'll
    // immediately be falling asleep after that.
    //
    // Also if player debug menu'd time backwards this breaks, just do the
    // check anyway, this will reset the timer if 'dur' is negative.
    const time_point now = calendar::turn;
    const time_duration dur = now - last_sleep_check;
    if( dur >= 0_turns && dur < 30_minutes ) {
        return false;
    }
    last_sleep_check = now;

    int sleepy = get_comfort_at( pos_bub() ).comfort;
    if( has_addiction( addiction_sleeping_pill ) ) {
        sleepy -= 4;
    }
    sleepy = enchantment_cache->modify_value( enchant_vals::mod::SLEEPY, sleepy );
    if( get_sleepiness() < sleepiness_levels::TIRED + 1 ) {
        sleepy -= int( ( sleepiness_levels::TIRED + 1 - get_sleepiness() ) / 4 );
    } else {
        sleepy += int( ( get_sleepiness() - sleepiness_levels::TIRED + 1 ) / 16 );
    }
    const int current_stim = get_stim();
    if( current_stim > 0 || !has_trait( trait_INSOMNIA ) ) {
        sleepy -= 2 * current_stim;
    } else {
        // Make it harder for insomniac to get around the trait
        sleepy -= current_stim;
    }

    sleepy += rng( -8, 8 );
    bool result = sleepy > 0;

    if( has_active_bionic( bio_soporific ) ) {
        if( bio_soporific_powered_at_last_sleep_check && !has_power() ) {
            add_msg_if_player( m_bad, _( "Your soporific inducer runs out of power!" ) );
        } else if( !bio_soporific_powered_at_last_sleep_check && has_power() ) {
            add_msg_if_player( m_good, _( "Your soporific inducer starts back up." ) );
        }
        bio_soporific_powered_at_last_sleep_check = has_power();
    }

    return result;
}

const comfort_data &Character::get_comfort_data_for( const tripoint_bub_ms &p ) const
{
    const comfort_data *worst = nullptr;
    for( const trait_id trait : get_functioning_mutations() ) {
        for( const comfort_data &data : trait->comfort ) {
            if( data.are_conditions_true( *this, p ) ) {
                if( worst == nullptr || worst->base_comfort > data.base_comfort ) {
                    worst = &data;
                }
                break;
            }
        }
    }
    return worst == nullptr ? comfort_data::human() : *worst;
}

const comfort_data::response &Character::get_comfort_at( const tripoint_bub_ms &p )
{
    if( comfort_cache.last_time == calendar::turn && comfort_cache.last_position == p ) {
        return comfort_cache;
    }
    return comfort_cache = get_comfort_data_for( p ).get_comfort_at( p );
}

void Character::shift_destination( const point_rel_ms &shift )
{
    if( next_expected_position ) {
        *next_expected_position += {shift, 0};
    }

    for( tripoint_bub_ms &elem : auto_move_route ) {
        elem += {shift, 0};
    }
}

bool Character::has_weapon() const
{
    return !unarmed_attack();
}

int Character::get_lowest_hp() const
{
    // Set lowest_hp to an arbitrarily large number.
    int lowest_hp = 999;
    for( const std::pair<const bodypart_str_id, bodypart> &elem : get_body() ) {
        const int cur_hp = elem.second.get_hp_cur();
        if( cur_hp < lowest_hp ) {
            lowest_hp = cur_hp;
        }
    }
    return lowest_hp;
}

Creature::Attitude Character::attitude_to( const Creature &other ) const
{
    const monster *m = dynamic_cast<const monster *>( &other );
    if( m != nullptr ) {
        if( m->friendly != 0 ) {
            return Attitude::FRIENDLY;
        }
        switch( m->attitude( const_cast<Character *>( this ) ) ) {
            // player probably does not want to harm them, but doesn't care much at all.
            case MATT_FOLLOW:
            case MATT_FPASSIVE:
            case MATT_IGNORE:
            case MATT_FLEE:
                return Attitude::NEUTRAL;
            // player does not want to harm those.
            case MATT_FRIEND:
                return Attitude::FRIENDLY;
            case MATT_ATTACK:
                return Attitude::HOSTILE;
            case MATT_NULL:
            case NUM_MONSTER_ATTITUDES:
                break;
        }

        return Attitude::NEUTRAL;
    }

    const npc *p = dynamic_cast<const npc *>( &other );
    if( p != nullptr ) {
        if( p->is_enemy() ) {
            return Attitude::HOSTILE;
        } else if( p->is_player_ally() ) {
            return Attitude::FRIENDLY;
        } else {
            return Attitude::NEUTRAL;
        }
    } else if( &other == this ) {
        return Attitude::FRIENDLY;
    }

    return Attitude::NEUTRAL;
}

npc_attitude Character::get_attitude() const
{
    return NPCATT_NULL;
}

bool Character::sees( const map &here, const tripoint_bub_ms &t, bool, int ) const
{
    const int wanted_range = rl_dist( pos_bub( here ), t );
    bool can_see = this->is_avatar() ? here.pl_sees( t, std::min( sight_max, wanted_range ) ) :
                   Creature::sees( here, t );
    // Clairvoyance is now pretty cheap, so we can check it early
    if( wanted_range < MAX_CLAIRVOYANCE && wanted_range < clairvoyance() ) {
        return true;
    }

    return can_see;
}

bool Character::sees( const map &here, const Creature &critter ) const
{
    // This handles only the player/npc specific stuff (monsters don't have traits or bionics).
    const int dist = rl_dist( pos_abs(), critter.pos_abs() );
    if( std::abs( posz() - critter.posz() ) > fov_3d_z_range ) {
        return false;
    }
    if( dist < MAX_CLAIRVOYANCE && dist < clairvoyance() ) {
        return true;
    }
    // Only players can spot creatures through clairvoyance fields
    if( is_avatar() && field_fd_clairvoyant.is_valid() &&
        here.get_field( critter.pos_bub( here ), field_fd_clairvoyant ) ) {
        return true;
    }
    return Creature::sees( here, critter );
}

void Character::set_destination( const std::vector<tripoint_bub_ms> &route,
                                 const player_activity &new_destination_activity )
{
    auto_move_route = route;
    set_destination_activity( new_destination_activity );
    destination_point.emplace( get_map().get_abs( route.back() ) );
}

void Character::clear_destination()
{
    auto_move_route.clear();
    clear_destination_activity();
    destination_point = std::nullopt;
    next_expected_position = std::nullopt;
}

void Character::abort_automove()
{
    clear_destination();
    if( g->overmap_data.fast_traveling && is_avatar() ) {
        ui::omap::force_quit();
    }
}

bool Character::has_distant_destination() const
{
    return has_destination() && !get_destination_activity().is_null() &&
           get_destination_activity().id() == ACT_TRAVELLING && !omt_path.empty();
}

bool Character::is_auto_moving() const
{
    return destination_point.has_value();
}

bool Character::has_destination() const
{
    return !auto_move_route.empty();
}

bool Character::has_destination_activity() const
{
    return !get_destination_activity().is_null() && destination_point &&
           pos_bub() == get_map().get_bub( *destination_point );
}

void Character::start_destination_activity()
{
    if( !has_destination_activity() ) {
        debugmsg( "Tried to start invalid destination activity" );
        return;
    }

    assign_activity( get_destination_activity() );
    clear_destination();
}

std::vector<tripoint_bub_ms> &Character::get_auto_move_route()
{
    return auto_move_route;
}

action_id Character::get_next_auto_move_direction()
{
    if( !has_destination() ) {
        return ACTION_NULL;
    }

    if( next_expected_position ) {
        // Difference between where the character is and where we expect them to be after last auto-move.
        tripoint_rel_ms diff = ( pos_bub() - *next_expected_position ).abs();
        // This might differ by 1 (in z-direction), since the character might have crossed a ramp,
        // which teleported them a tile up or down. If the error is in x or y direction, we might as well
        // give them a turn to recover, as the move still might be vaild.
        // We cut off at 1 since 2 definitely results in an invalid move.
        // If the character is still stumbling or stuck,
        // they will cancel the auto-move on the next cycle (as the distance increases).
        if( std::max( { diff.x(), diff.y(), diff.z()} ) > 1 ) {
            // We're off course, possibly stumbling or stuck, cancel auto move
            return ACTION_NULL;
        }
    }

    next_expected_position.emplace( auto_move_route.front() );
    auto_move_route.erase( auto_move_route.begin() );

    tripoint_rel_ms dp = *next_expected_position - pos_bub();

    return get_movement_action_from_delta( dp, iso_rotate::yes );
}

int Character::persuade_skill() const
{
    /** @EFFECT_INT slightly increases talking skill */
    /** @EFFECT_PER slightly increases talking skill */
    /** @EFFECT_SPEECH increases talking skill */
    float ret = get_int() + get_per() + get_skill_level( skill_speech ) * 3;
    ret = enchantment_cache->modify_value( enchant_vals::mod::SOCIAL_PERSUADE, ret );
    return round( ret );
}

int Character::lie_skill() const
{
    /** @EFFECT_INT slightly increases talking skill */
    /** @EFFECT_PER slightly increases talking skill */
    /** @EFFECT_SPEECH increases talking skill */
    float ret = get_int() + get_per() + get_skill_level( skill_speech ) * 3;
    ret = enchantment_cache->modify_value( enchant_vals::mod::SOCIAL_LIE, ret );
    return round( ret );
}

int Character::intimidation() const
{
    /** @EFFECT_STR increases intimidation factor */
    int ret = get_str() * 2;
    if( weapon.is_gun() ) {
        ret += 10;
    }
    for( const damage_type &dt : damage_type::get_all() ) {
        if( dt.melee_only && weapon.damage_melee( dt.id ) >= 12 ) {
            ret += 5;
            break;
        }
    }

    if( get_stim() > 20 ) {
        ret += 2;
    }
    if( has_effect( effect_drunk ) ) {
        ret -= 4;
    }
    ret = enchantment_cache->modify_value( enchant_vals::mod::SOCIAL_INTIMIDATE, ret );
    return ret;
}

bool Character::defer_move( const tripoint_bub_ms &next )
{
    // next must be adjacent to current pos
    if( square_dist( next, pos_bub() ) != 1 ) {
        return false;
    }
    // next must be adjacent to subsequent move in any preexisting automove route
    if( has_destination() && square_dist( auto_move_route.front(), next ) != 1 ) {
        return false;
    }
    auto_move_route.insert( auto_move_route.begin(), next );
    next_expected_position = pos_bub();
    return true;
}

bool Character::add_or_drop_with_msg( item &it, const bool /*unloading*/, const item *avoid,
                                      const item *original_inventory_item )
{
    if( it.made_of( phase_id::LIQUID ) ) {
        liquid_handler::consume_liquid( it, 1, avoid );
        return it.charges <= 0;
    }
    if( !this->can_pickVolume( it, false, avoid ) ) {
        put_into_vehicle_or_drop( *this, item_drop_reason::too_large, { it } );
    } else if( !this->can_pickWeight( it, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
        put_into_vehicle_or_drop( *this, item_drop_reason::too_heavy, { it } );
    } else {
        bool wielded_has_it = false;
        // Cannot use weapon.has_item(it) because it skips any pockets that
        // are not containers such as magazines and magazine wells.
        for( const item *scan_contents : weapon.all_items_top() ) {
            if( scan_contents == &it ) {
                wielded_has_it = true;
                break;
            }
        }
        const bool allow_wield = !wielded_has_it && weapon.magazine_current() != &it;
        const int prev_charges = it.charges;
        item_location ni = i_add( it, true, avoid,
                                  original_inventory_item, /*allow_drop=*/false, /*allow_wield=*/allow_wield );
        if( ni == item_location::nowhere ) {
            // failed to add
            put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { it } );
        } else if( &*ni == &it ) {
            // merged into the original stack, restore original charges
            it.charges = prev_charges;
            put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { it } );
        } else {
            // successfully added
            add_msg( _( "You put the %s in your inventory." ), ni->tname() );
            add_msg( m_info, "%c - %s", ni->invlet == 0 ? ' ' : ni->invlet, ni->tname() );
        }
    }
    return true;
}

bool Character::unload( item_location &loc, bool bypass_activity,
                        const item_location &new_container )
{
    item &it = *loc;
    drop_locations locs;
    // Unload a container consuming moves per item successfully removed
    if( it.is_container() ) {
        if( it.empty() ) {
            add_msg( m_info, _( "The %s is already empty!" ), it.tname() );
            return false;
        }
        if( !it.can_unload() ) {
            add_msg( m_info, _( "The item can't be unloaded in its current state!" ) );
            return false;
        }

        int moves = 0;
        item *prev_contained = nullptr;

        for( pocket_type ptype : {
                 pocket_type::CONTAINER,
                 pocket_type::MAGAZINE_WELL,
                 pocket_type::MAGAZINE
             } ) {

            for( item *contained : it.all_items_top( ptype, true ) ) {
                if( new_container == item_location::nowhere ) {
                    if( prev_contained && prev_contained->stacks_with( *contained ) ) {
                        moves += std::max( this->item_handling_cost( *contained, true, 0, -1, true ), 1 );
                    } else {
                        moves += this->item_handling_cost( *contained );
                    }
                    prev_contained = contained;
                } else {
                    // redirect to insert actor
                    const int count = contained->count_by_charges() ? contained->charges : 1;
                    locs.emplace_back( item_location( loc, contained ), count );
                }
            }

        }
        if( new_container == item_location::nowhere ) {
            assign_activity( unload_activity_actor( moves, loc ) );
        } else if( !locs.empty() ) {
            assign_activity( insert_item_activity_actor( new_container, locs, true ) );
        }

        return true;
    }

    // If item can be unloaded more than once (currently only guns) prompt user to choose
    std::vector<std::string> msgs( 1, it.tname() );
    std::vector<item *> opts( 1, &it );

    for( item *e : it.gunmods() ) {
        if( ( e->is_gun() && !e->has_flag( flag_NO_UNLOAD ) &&
              ( e->magazine_current() || e->ammo_remaining( ) > 0 || e->casings_count() > 0 ) ) ||
            ( e->has_flag( flag_BRASS_CATCHER ) && !e->is_container_empty() ) ) {
            msgs.emplace_back( e->tname() );
            opts.emplace_back( e );
        }
    }

    item *target = nullptr;
    item_location targloc;
    if( opts.size() > 1 ) {
        const int ret = uilist( _( "Unload what?" ), msgs );
        if( ret >= 0 ) {
            target = opts[ret];
            targloc = item_location( loc, opts[ret] );
        }
    } else {
        target = &it;
        targloc = loc;
    }

    if( target == nullptr ) {
        return false;
    }

    // Next check for any reasons why the item cannot be unloaded
    if( !target->has_flag( flag_BRASS_CATCHER ) ) {
        if( !target->is_magazine() && !target->uses_magazine() ) {
            add_msg( m_info, _( "You can't unload a %s!" ), target->tname() );
            return false;
        }

        if( target->has_flag( flag_NO_UNLOAD ) ) {
            if( target->has_flag( flag_RECHARGE ) || target->has_flag( flag_USE_UPS ) ) {
                add_msg( m_info, _( "You can't unload a rechargeable %s!" ), target->tname() );
            } else {
                add_msg( m_info, _( "You can't unload a %s!" ), target->tname() );
            }
            return false;
        }

        if( !target->magazine_current() && target->ammo_remaining( ) <= 0 &&
            target->casings_count() <= 0 ) {
            if( target->is_tool() ) {
                add_msg( m_info, _( "Your %s isn't charged." ), target->tname() );
            } else {
                add_msg( m_info, _( "Your %s isn't loaded." ), target->tname() );
            }
            return false;
        }
    }

    target->casings_handle( [&]( item & e ) {
        return this->i_add_or_drop( e );
    } );

    invalidate_weight_carried_cache();

    if( target->is_magazine() ) {
        if( bypass_activity ) {
            unload_activity_actor::unload( *this, targloc );
        } else {
            int mv = 0;
            for( const item *content : target->all_items_top() ) {
                // We use the same cost for both reloading and unloading
                mv += this->item_reload_cost( it, *content, content->charges );
            }
            if( it.is_ammo_belt() ) {
                // Disassembling ammo belts is easier than assembling them
                mv /= 2;
            }
            assign_activity( unload_activity_actor( mv, targloc ) );
        }
        return true;

    } else if( target->magazine_current() ) {
        if( !this->add_or_drop_with_msg( *target->magazine_current(), true, nullptr,
                                         target->magazine_current() ) ) {
            return false;
        }
        // Eject magazine consuming half as much time as required to insert it
        this->mod_moves( -this->item_reload_cost( *target, *target->magazine_current(), -1 ) / 2 );

        target->remove_items_with( [&target]( const item & e ) {
            return target->magazine_current() == &e;
        } );

    } else if( target->ammo_remaining( ) ) {
        int qty = target->ammo_remaining( );

        // Construct a new ammo item and try to drop it
        item ammo( target->ammo_current(), calendar::turn, qty );
        if( target->is_filthy() ) {
            ammo.set_flag( flag_FILTHY );
        }

        if( ammo.made_of_from_type( phase_id::LIQUID ) ) {
            if( !this->add_or_drop_with_msg( ammo ) ) {
                qty -= ammo.charges; // only handled part (or none) of the liquid
            }
            if( qty <= 0 ) {
                return false; // no liquid was moved
            }

        } else if( !this->add_or_drop_with_msg( ammo, qty > 1 ) ) {
            return false;
        }

        // If successful remove appropriate qty of ammo consuming half as much time as required to load it
        this->mod_moves( -this->item_reload_cost( *target, ammo, qty ) / 2 );

        target->ammo_set( target->ammo_current(), target->ammo_remaining( ) - qty );
    } else if( target->has_flag( flag_BRASS_CATCHER ) ) {
        target->spill_contents( get_player_character() );
    }

    // Turn off any active tools
    if( target->is_tool() && target->active && target->ammo_remaining( ) == 0 ) {
        target->deactivate( this );
    }

    add_msg( _( "You unload your %s." ), target->tname() );

    if( it.has_flag( flag_MAG_DESTROY ) && it.ammo_remaining( ) == 0 ) {
        loc.remove_item();
    }

    get_player_character().recoil = MAX_RECOIL;

    return true;
}

book_mastery Character::get_book_mastery( const item &book ) const
{
    if( !book.is_book() || !has_identified( book.typeId() ) ) {
        return book_mastery::CANT_DETERMINE;
    }
    // TODO: add illiterate check?

    const cata::value_ptr<islot_book> &type = book.type->book;
    const skill_id &skill = type->skill;

    if( !skill ) {
        // book gives no skill
        return book_mastery::MASTERED;
    }

    const int skill_level = get_knowledge_level( skill );
    const int skill_requirement = type->req;
    const int max_skill_learnable = type->level;

    if( skill_requirement > skill_level ) {
        return book_mastery::CANT_UNDERSTAND;
    }
    if( skill_level >= max_skill_learnable ) {
        return book_mastery::MASTERED;
    }
    return book_mastery::LEARNING;
}

bool Character::fun_to_read( const item &book ) const
{
    return book_fun_for( book, *this ) > 0;
}

int Character::book_fun_for( const item &book, const Character &p ) const
{
    int fun_bonus = book.type->book->fun;
    if( !book.is_book() ) {
        debugmsg( "called avatar::book_fun_for with non-book" );
        return 0;
    }

    // If you don't have a problem with eating humans, To Serve Man becomes rewarding
    if( ( p.has_trait( trait_CANNIBAL ) || p.has_trait( trait_PSYCHOPATH ) ||
          p.has_trait( trait_SAPIOVORE ) ) &&
        book.typeId() == itype_cookbook_human ) {
        fun_bonus = std::abs( fun_bonus );
    } else if( p.has_trait( trait_SPIRITUAL ) && book.has_flag( flag_INSPIRATIONAL ) ) {
        fun_bonus = std::abs( fun_bonus * 3 );
    }

    if( has_trait( trait_LOVES_BOOKS ) ) {
        fun_bonus++;
    } else if( has_trait( trait_HATES_BOOKS ) ) {
        if( book.type->book->fun > 0 ) {
            fun_bonus = 0;
        } else {
            fun_bonus--;
        }
    }

    if( fun_bonus > 1 && book.get_chapters() > 0 && book.get_remaining_chapters( p ) == 0 ) {
        fun_bonus /= 2;
    }

    return fun_bonus;
}

bool Character::has_bionic_with_flag( const json_character_flag &flag ) const
{
    auto iter = bio_flag_cache.find( flag );
    if( iter != bio_flag_cache.end() ) {
        return iter->second;
    }
    for( const bionic &bio : *my_bionics ) {
        if( bio.info().has_flag( flag ) ) {
            bio_flag_cache[flag] = true;
            return true;
        }
        if( bio.info().activated ) {
            if( ( bio.info().has_active_flag( flag ) && has_active_bionic( bio.id ) ) ||
                ( bio.info().has_inactive_flag( flag ) && !has_active_bionic( bio.id ) ) ) {
                bio_flag_cache[flag] = true;
                return true;
            }
        }
    }
    bio_flag_cache[flag] = false;
    return false;
}

int Character::count_bionic_with_flag( const json_character_flag &flag ) const
{
    int ret = 0;
    for( const bionic &bio : *my_bionics ) {
        if( bio.info().has_flag( flag ) ) {
            ret++;
        }
        if( bio.info().activated ) {
            if( ( bio.info().has_active_flag( flag ) && has_active_bionic( bio.id ) ) ||
                ( bio.info().has_inactive_flag( flag ) && !has_active_bionic( bio.id ) ) ) {
                ret++;
            }
        }
    }

    return ret;
}

bool Character::has_bodypart_with_flag( const json_character_flag &flag ) const
{
    for( const std::pair<const bodypart_str_id, bodypart> &elem : get_body() ) {
        if( elem.first->has_flag( flag ) ) {
            return true;
        }
        if( elem.second.has_conditional_flag( *this, flag ) ) {
            // Checking for disabling effects is a bit convoluted
            bool disabled = false;
            if( has_effect_with_flag( flag_EFFECT_LIMB_DISABLE_CONDITIONAL_FLAGS ) ) {
                for( const effect &eff : get_effects_from_bp( elem.first ) ) {
                    if( eff.has_flag( flag_EFFECT_LIMB_DISABLE_CONDITIONAL_FLAGS ) ) {
                        disabled = true;
                        break;

                    }
                }
            }
            if( !disabled ) {
                return true;

            }
        }
    }
    return false;
}

int Character::count_bodypart_with_flag( const json_character_flag &flag ) const
{
    int ret = 0;
    for( const bodypart_id &bp : get_all_body_parts() ) {
        if( bp->has_flag( flag ) ) {
            ret++;
        }
        if( get_part( bp )->has_conditional_flag( *this, flag ) ) {
            bool disabled = false;
            if( has_effect_with_flag( flag_EFFECT_LIMB_DISABLE_CONDITIONAL_FLAGS ) ) {
                for( const effect &eff : get_effects_from_bp( bp ) ) {
                    if( eff.has_flag( flag_EFFECT_LIMB_DISABLE_CONDITIONAL_FLAGS ) ) {
                        disabled = true;
                        break;
                    }
                }
            }
            if( !disabled ) {
                ret++;
            }
        }
    }
    return ret;
}

bool Character::has_flag( const json_character_flag &flag ) const
{
    // If this is a performance problem create a map of flags stored for a character and updated on trait, mutation, bionic add/remove, activate/deactivate, effect gain/loss
    return has_trait_flag( flag ) ||
           has_bionic_with_flag( flag ) ||
           has_effect_with_flag( flag ) ||
           has_bodypart_with_flag( flag ) ||
           has_mabuff_flag( flag );
}

int Character::count_flag( const json_character_flag &flag ) const
{
    // If this is a performance problem create a map of flags stored for a character and updated on trait, mutation, bionic add/remove, activate/deactivate, effect gain/loss
    return count_trait_flag( flag ) +
           count_bionic_with_flag( flag ) +
           has_effect_with_flag( flag ) +
           count_bodypart_with_flag( flag ) +
           count_mabuff_flag( flag );
}

bool Character::empathizes_with_species( const species_id &species ) const
{
    if( has_flag( STATIC( json_character_flag( "CANNIBAL" ) ) ) || has_flag( json_flag_PSYCHOPATH ) ||
        has_flag( json_flag_SAPIOVORE ) ) {
        return false;
    }
    // Negative empathy list.  Takes precedence over positive traits or human
    for( const trait_id &mut : get_functioning_mutations() ) {
        const mutation_branch &mut_data = mut.obj();
        if( std::find( mut_data.no_empathize_with.begin(), mut_data.no_empathize_with.end(),
                       species ) != mut_data.no_empathize_with.end() ) {
            return false;
        }
    }

    // Human empathy by default.
    if( species == species_HUMAN ) {
        return true;
    }

    // positive empathy list
    for( const trait_id &mut : get_functioning_mutations() ) {
        const mutation_branch &mut_data = mut.obj();
        if( std::find( mut_data.empathize_with.begin(), mut_data.empathize_with.end(),
                       species ) != mut_data.empathize_with.end() ) {
            return true;
        }
    }

    // Don't empathize with any species other than HUMAN by default.
    return false;
}

bool Character::empathizes_with_monster( const mtype_id &monster ) const
{
    if( has_flag( STATIC( json_character_flag( "CANNIBAL" ) ) ) || has_flag( json_flag_PSYCHOPATH ) ||
        has_flag( json_flag_SAPIOVORE ) ) {
        return false;
    }
    for( species_id species : monster->species ) {
        if( empathizes_with_species( species ) ) {
            return true;
        }
    }
    // used since this method is called on corpse ids, which are null for npc corpses.
    if( monster == mtype_id::NULL_ID() && empathizes_with_species( species_HUMAN ) ) {
        return true;
    }
    return false;
}

bool Character::is_driving() const
{
    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( pos_bub() );
    return vp && vp->vehicle().is_moving() && vp->vehicle().player_in_control( here, *this );
}

time_duration Character::estimate_effect_dur( const skill_id &relevant_skill,
        const efftype_id &effect, const time_duration &error_magnitude,
        const time_duration &minimum_error, int threshold, const Creature &target ) const
{
    const time_duration zero_duration = 0_turns;

    int skill_lvl = get_skill_level( relevant_skill );

    time_duration estimate = std::max( zero_duration, target.get_effect_dur( effect ) +
                                       rng_float( -1, 1 ) * ( minimum_error + ( error_magnitude - minimum_error ) *
                                               std::max( 0.0, static_cast<double>( threshold - skill_lvl ) / threshold ) ) );
    return estimate;
}

void Character::invalidate_pseudo_items()
{
    pseudo_items_valid = false;
}

bool Character::avoid_trap( const tripoint_bub_ms &pos, const trap &tr ) const
{
    /** @EFFECT_DEX increases chance to avoid traps */

    /** @EFFECT_DODGE increases chance to avoid traps */
    int myroll = dice( 3, round( dex_cur + get_skill_level( skill_dodge ) * 1.5 ) );
    int traproll;
    if( tr.can_see( pos, *this ) ) {
        traproll = dice( 3, tr.get_avoidance() );
    } else {
        traproll = dice( 6, tr.get_avoidance() );
    }

    if( has_trait( trait_LIGHTSTEP ) ) {
        myroll += dice( 2, 6 );
    }

    if( has_trait( trait_CLUMSY ) ) {
        myroll -= dice( 2, 6 );
    }

    return myroll >= traproll;
}

bool Character::add_faction_warning( const faction_id &id ) const
{
    const auto it = warning_record.find( id );
    if( it != warning_record.end() ) {
        it->second.first += 1;
        if( it->second.second - calendar::turn > 5_minutes ) {
            it->second.first -= 1;
        }
        it->second.second = calendar::turn;
        if( it->second.first > 3 ) {
            return true;
        }
    } else {
        warning_record[id] = std::make_pair( 1, calendar::turn );
    }
    faction *fac = g->faction_manager_ptr->get( id );
    if( fac != nullptr && is_avatar() && !fac->lone_wolf_faction ) {
        fac->likes_u -= 1;
        fac->respects_u -= 1;
        fac->trusts_u -= 1;
    }
    return false;
}

int Character::current_warnings_fac( const faction_id &id )
{
    const auto it = warning_record.find( id );
    if( it != warning_record.end() ) {
        if( it->second.second - calendar::turn > 5_minutes ) {
            it->second.first = std::max( 0,
                                         it->second.first - 1 );
        }
        return it->second.first;
    }
    return 0;
}

bool Character::beyond_final_warning( const faction_id &id )
{
    const auto it = warning_record.find( id );
    if( it != warning_record.end() ) {
        if( it->second.second - calendar::turn > 5_minutes ) {
            it->second.first = std::max( 0,
                                         it->second.first - 1 );
        }
        return it->second.first > 3;
    }
    return false;
}

read_condition_result Character::check_read_condition( const item &book ) const
{
    map &here = get_map();

    read_condition_result result = read_condition_result::SUCCESS;
    if( !book.is_book() ) {
        result |= read_condition_result::NOT_BOOK;
    } else {
        const optional_vpart_position vp = here.veh_at( pos_bub() );
        if( vp && vp->vehicle().player_in_control( here, *this ) ) {
            result |= read_condition_result::DRIVING;
        }

        if( !fun_to_read( book ) && !has_morale_to_read() && has_identified( book.typeId() ) ) {
            result |= read_condition_result::MORALE_LOW;
        }

        const book_mastery mastery = get_book_mastery( book );
        if( mastery == book_mastery::CANT_UNDERSTAND ) {
            result |= read_condition_result::CANT_UNDERSTAND;
        }
        if( mastery == book_mastery::MASTERED ) {
            result |= read_condition_result::MASTERED;
        }

        const bool book_requires_intelligence = book.type->book->intel > 0;
        if( book_requires_intelligence && has_trait( trait_ILLITERATE ) ) {
            result |= read_condition_result::ILLITERATE;
        }
        if( has_flag( json_flag_HYPEROPIC ) &&
            !worn_with_flag( STATIC( flag_id( "FIX_FARSIGHT" ) ) ) &&
            !has_effect( effect_contacts ) &&
            !has_effect( effect_transition_contacts ) &&
            !has_flag( STATIC( json_character_flag( "ENHANCED_VISION" ) ) ) ) {
            result |= read_condition_result::NEED_GLASSES;
        }
        if( fine_detail_vision_mod() > 4 ) {
            result |= read_condition_result::TOO_DARK;
        }
        if( is_blind() ) {
            result |= read_condition_result::BLIND;
        }
    }
    return result;
}

const Character *Character::get_book_reader( const item &book,
        std::vector<std::string> &reasons ) const
{
    const map &here = get_map();

    const Character *reader = nullptr;

    if( !book.is_book() ) {
        reasons.push_back( is_avatar() ? string_format( _( "Your %s is not good reading material." ),
                           book.tname() ) :
                           string_format( _( "The %s is not good reading material." ), book.tname() )
                         );
        return nullptr;
    }

    const cata::value_ptr<islot_book> &type = book.type->book;
    const skill_id &book_skill = type->skill;
    const int book_skill_requirement = type->req;

    // Check for conditions that immediately disqualify the player from reading:
    read_condition_result condition = check_read_condition( book );
    if( condition & read_condition_result::DRIVING ) {
        reasons.emplace_back( _( "It's a bad idea to read while driving!" ) );
        return nullptr;
    }
    if( condition & read_condition_result::MORALE_LOW ) {
        // Low morale still permits skimming
        reasons.emplace_back( is_avatar() ?
                              _( "What's the point of studying?  (Your morale is too low!)" )  :
                              string_format( _( "What's the point of studying?  (%s)'s morale is too low!)" ), disp_name() ) );
        return nullptr;
    }
    if( condition & read_condition_result::CANT_UNDERSTAND ) {
        reasons.push_back( is_avatar() ? string_format( _( "%s %d needed to understand.  You have %d" ),
                           book_skill->name(), book_skill_requirement, get_knowledge_level( book_skill ) ) :
                           string_format( _( "%s %d needed to understand.  %s has %d" ), book_skill->name(),
                                          book_skill_requirement, disp_name(), get_knowledge_level( book_skill ) ) );
        return nullptr;
    }

    // Check for conditions that disqualify us only if no other Characters can read to us
    if( condition & read_condition_result::ILLITERATE ) {
        reasons.emplace_back( is_avatar() ? _( "You're illiterate!" ) : string_format(
                                  _( "%s is illiterate!" ), disp_name() ) );
    } else if( condition & read_condition_result::NEED_GLASSES ) {
        reasons.emplace_back( is_avatar() ? _( "Your eyes won't focus without reading glasses." ) :
                              string_format( _( "%s's eyes won't focus without reading glasses." ), disp_name() ) );
    } else if( condition & read_condition_result::TOO_DARK &&
               !has_flag( json_flag_READ_IN_DARKNESS ) ) {
        // Too dark to read only applies if the player can read to himself
        reasons.emplace_back( _( "It's too dark to read!" ) );
        return nullptr;
    } else {
        return this;
    }

    if( ! is_avatar() ) {
        // NPCs are too proud to ask for help, perhaps someday they will not be
        return nullptr;
    }

    //Check for other Characters to read for you, negates Illiterate and Far Sighted
    //The fastest-reading Character is chosen
    if( is_deaf() ) {
        reasons.emplace_back( _( "Maybe someone could read that to you, but you're deaf!" ) );
        return nullptr;
    }

    time_duration time_taken = time_duration::from_turns( INT_MAX );
    std::vector<Character *> candidates = get_crafting_helpers();

    for( const Character *elem : candidates ) {
        // Check for disqualifying factors:
        condition = elem->check_read_condition( book );
        if( condition & read_condition_result::ILLITERATE ) {
            reasons.push_back( string_format( _( "%s is illiterate!" ),
                                              elem->disp_name() ) );
        } else if( condition & read_condition_result::CANT_UNDERSTAND ) {
            reasons.push_back( string_format( _( "%s %d needed to understand.  %s has %d" ),
                                              book_skill->name(), book_skill_requirement, elem->disp_name(),
                                              elem->get_knowledge_level( book_skill ) ) );
        } else if( condition & read_condition_result::NEED_GLASSES ) {
            reasons.push_back( string_format( _( "%s needs reading glasses!" ),
                                              elem->disp_name() ) );
        } else if( condition & read_condition_result::TOO_DARK ) {
            reasons.push_back( string_format(
                                   _( "It's too dark for %s to read!" ),
                                   elem->disp_name() ) );
        } else if( !elem->sees( here, *this ) ) {
            reasons.push_back( string_format( _( "%s could read that to you, but they can't see you." ),
                                              elem->disp_name() ) );
        } else if( condition & read_condition_result::MORALE_LOW ) {
            // Low morale still permits skimming
            reasons.push_back( string_format( _( "%s morale is too low!" ), elem->disp_name( true ) ) );
        } else if( condition & read_condition_result::BLIND ) {
            reasons.push_back( string_format( _( "%s is blind." ), elem->disp_name() ) );
        } else {
            time_duration proj_time = time_to_read( book, *elem );
            if( proj_time < time_taken ) {
                reader = elem;
                time_taken = proj_time;
            }
        }
    }
    //end for all candidates
    return reader;
}

time_duration Character::time_to_read( const item &book, const Character &reader,
                                       const Character *learner ) const
{
    const auto &type = book.type->book;
    const skill_id &skill = type->skill;
    // The reader's reading speed has an effect only if they're trying to understand the book as they read it
    // Reading speed is assumed to be how well you learn from books (as opposed to hands-on experience)
    const bool try_understand = reader.fun_to_read( book ) ||
                                reader.get_knowledge_level( skill ) < type->level;
    int reading_speed = try_understand ? std::max( reader.read_speed(), read_speed() ) : read_speed();
    if( learner ) {
        reading_speed = std::max( reading_speed, learner->read_speed() );
    }

    time_duration retval = type->time * reading_speed / 100;
    retval *= std::min( fine_detail_vision_mod(), reader.fine_detail_vision_mod() );

    const int effective_int = std::min( { get_int(), reader.get_int(), learner ? learner->get_int() : INT_MAX } );
    if( type->intel > effective_int && !reader.has_trait( trait_PROF_DICEMASTER ) ) {
        retval += type->time * ( time_duration::from_seconds( type->intel - effective_int ) / 1_minutes );
    }
    if( !has_identified( book.typeId() ) ) {
        //skimming
        retval /= 10;
    }
    return retval;
}

int Character::thirst_speed_penalty( int thirst )
{
    // We die at 1200 thirst
    // Start by dropping speed really fast, but then level it off a bit
    static const std::vector<std::pair<float, float>> thirst_thresholds = {{
            std::make_pair( 40.0f, 0.0f ),
            std::make_pair( 300.0f, -25.0f ),
            std::make_pair( 600.0f, -50.0f ),
            std::make_pair( 1200.0f, -75.0f )
        }
    };
    return static_cast<int>( multi_lerp( thirst_thresholds, thirst ) );
}

std::vector<speed_bonus_effect> Character::get_speed_bonus_effects() const
{
    return speed_bonus_effects;
}

void Character::mod_speed_bonus( int nspeed, const std::string &desc )
{
    if( nspeed != 0 ) {
        speed_bonus_effect effect { desc, nspeed };
        speed_bonus_effects.push_back( effect );
        speed_bonus += nspeed;
    }
}

void Character::recalc_speed_bonus()
{
    speed_bonus_effects.clear();
    // Minus some for weight...
    int carry_penalty = 0;
    units::mass weight_cap = weight_capacity();
    if( weight_cap < 1_milligram ) {
        weight_cap = 1_milligram; //Prevent Clang warning about divide by zero
    }
    if( weight_carried() > weight_cap ) {
        carry_penalty = 25 * ( weight_carried() - weight_cap ) / weight_cap;
    }
    mod_speed_bonus( -carry_penalty, _( "Weight Carried" ) );

    mod_speed_bonus( +get_speedydex_bonus( get_dex() ), _( "Dexterity" ) );

    mod_speed_bonus( -get_pain_penalty().speed, _( "Pain" ) );

    if( get_thirst() > 40 ) {
        mod_speed_bonus( thirst_speed_penalty( get_thirst() ), _( "Thirst" ) );
    }
    // when underweight, you get slower. cumulative with hunger
    mod_speed_bonus( kcal_speed_penalty(), _( "Underweight" ) );

    for( const auto &maps : *effects ) {
        for( const auto &i : maps.second ) {
            bool reduced = resists_effect( i.second );
            //TODO try disp_short_desc() or disp_short_desc(true) if disp_name doesn't work well
            mod_speed_bonus( i.second.get_mod( "SPEED", reduced ), i.second.disp_name() );
        }
    }

    // add martial arts speed bonus
    mod_speed_bonus( mabuff_speed_bonus(), _( "Martial Art" ) );

    // Not sure why Sunlight Dependent is here, but OK
    // Ectothermic/COLDBLOOD4 is intended to buff folks in the Summer
    // Threshold-crossing has its charms ;-)
    if( g != nullptr ) {
        if( has_trait( trait_SUNLIGHT_DEPENDENT ) && !g->is_in_sunlight( pos_bub() ) ) {
            //FIXME get trait name directly
            mod_speed_bonus( -( g->light_level( posz() ) >= 12 ? 5 : 10 ), _( "Sunlight Dependent" ) );
        }
    }
    const int prev_speed_bonus = get_speed_bonus();
    const int speed_bonus_with_enchant = std::round( enchantment_cache->modify_value(
            enchant_vals::mod::SPEED,
            get_speed() ) - get_speed_base() );
    enchantment_speed_bonus = speed_bonus_with_enchant - prev_speed_bonus;
    mod_speed_bonus( enchantment_speed_bonus, _( "Bio/Mut/Etc Effects" ) );
    // Speed cannot be less than 25% of base speed, so minimal speed bonus is -75% base speed.
    const int min_speed_bonus = static_cast<int>( -0.75 * get_speed_base() );
    if( get_speed_bonus() < min_speed_bonus ) {
        mod_speed_bonus( min_speed_bonus - get_speed_bonus(), _( "Penalty Cap" ) );
    }
}

double Character::recoil_vehicle() const
{
    // TODO: vary penalty dependent upon vehicle part on which player is boarded

    if( in_vehicle ) {
        if( const optional_vpart_position vp = get_map().veh_at( pos_bub() ) ) {
            return static_cast<double>( std::abs( vp->vehicle().velocity ) ) * 3 / 100;
        }
    }
    return 0;
}

double Character::recoil_total() const
{
    return recoil + recoil_vehicle();
}

bool Character::is_hallucination() const
{
    return false;
}

bool Character::is_electrical() const
{
    // for now this is false. In the future should have rules
    return false;
}

bool Character::is_fae() const
{
    if( has_trait( trait_FAERIECREATURE ) ) {
        return true;
    }
    return false;
}

bool Character::is_nether() const
{
    // for now this is false. In the future should have rules
    return false;
}

bool Character::has_mind() const
{
    // Characters are all humans and thus have minds
    return true;
}

void Character::set_underwater( bool u )
{
    if( underwater != u ) {
        underwater = u;
        recalc_sight_limits();
    }
}

stat_mod Character::get_pain_penalty() const
{
    stat_mod ret;
    int pain = get_perceived_pain();
    // if less than 10 pain, do not apply any penalties
    if( pain <= 10 ) {
        return ret;
    }

    float penalty_str = pain * get_option<float>( "PAIN_PENALTY_MOD_STR" );
    float penalty_dex = pain * get_option<float>( "PAIN_PENALTY_MOD_DEX" );
    float penalty_int = pain * get_option<float>( "PAIN_PENALTY_MOD_INT" );
    float penalty_per = pain * get_option<float>( "PAIN_PENALTY_MOD_PER" );


    ret.strength = enchantment_cache->modify_value( enchant_vals::mod::PAIN_PENALTY_MOD_STR,
                   get_str() * penalty_str );

    ret.dexterity = enchantment_cache->modify_value( enchant_vals::mod::PAIN_PENALTY_MOD_DEX,
                    get_dex() * penalty_dex );

    ret.intelligence = enchantment_cache->modify_value( enchant_vals::mod::PAIN_PENALTY_MOD_INT,
                       get_int() * penalty_int );

    ret.perception = enchantment_cache->modify_value( enchant_vals::mod::PAIN_PENALTY_MOD_PER,
                     get_per() * penalty_per );


    // Prevent negative penalties, there is better ways to give bonuses for pain
    // Also not make character has 0 stats
    ret.strength = get_str() > 2 ? std::clamp( ret.strength, 1, get_str() - 1 ) :
                   std::max( 0, get_str() - 1 );
    ret.dexterity = get_dex() > 2 ? std::clamp( ret.dexterity, 1, get_dex() - 1 ) :
                    std::max( 0, get_dex() - 1 );
    ret.intelligence = get_int() > 2 ? std::clamp( ret.intelligence, 1, get_int() - 1 ) :
                       std::max( 0, get_int() - 1 );
    ret.perception = get_per() > 2 ? std::clamp( ret.perception, 1, get_per() - 1 ) :
                     std::max( 0, get_per() - 1 );


    int speed_penalty = std::pow( pain, 0.7f );

    ret.speed = std::max( enchantment_cache->modify_value( enchant_vals::mod::PAIN_PENALTY_MOD_SPEED,
                          speed_penalty ), 0.0 );

    ret.speed = std::min( ret.speed, 50 );
    return ret;
}

stat_mod Character::read_pain_penalty() const
{
    stat_mod ret;
    ret.strength = ppen_str;
    ret.dexterity = ppen_dex;
    ret.intelligence = ppen_int;
    ret.perception = ppen_per;
    ret.speed = ppen_spd;
    return ret;
}

int Character::get_lift_str() const
{
    int str = get_arm_str();
    if( has_trait( trait_STRONGBACK ) ) {
        str *= 1.35;
    } else if( has_trait( trait_BADBACK ) ) {
        str /= 1.35;
    }
    return str;
}

int Character::get_lift_assist() const
{
    int result = 0;
    for( const Character *guy : get_crafting_helpers() ) {
        result += guy->get_lift_str();
    }
    return result;
}

bool Character::immune_to( const bodypart_id &bp, damage_unit dam ) const
{
    if( is_immune_damage( dam.type ) ||
        has_effect( effect_incorporeal ) || has_flag( json_flag_CANNOT_TAKE_DAMAGE ) ) {
        return true;
    }

    passive_absorb_hit( bp, dam );

    worn.damage_mitigate( bp, dam );

    return dam.amount <= 0;
}

int Character::mod_pain( int npain )
{
    if( npain > 0 ) {
        double mult = enchantment_cache->get_value_multiply( enchant_vals::mod::PAIN );
        if( has_flag( json_flag_PAIN_IMMUNE ) || has_effect( effect_narcosis ) ) {
            return 0;
        }
        // if there is a positive multiplier we always want to add at least 1 pain
        if( mult > 0 ) {
            npain += std::max( 1, roll_remainder( npain * mult ) );
        }
        if( mult < 0 ) {
            npain = roll_remainder( npain * ( 1 + mult ) );
        }
        npain += enchantment_cache->get_value_add( enchant_vals::mod::PAIN );

        // no matter how powerful the enchantment if we are gaining pain we don't lose any
        npain = std::max( 0, npain );
    }
    Creature::mod_pain( npain );
    return npain;
}

void Character::set_pain( int npain )
{
    const int prev_pain = get_perceived_pain();
    Creature::set_pain( npain );
    const int cur_pain = get_perceived_pain();

    if( cur_pain != prev_pain ) {
        react_to_felt_pain( cur_pain - prev_pain );
        on_stat_change( "perceived_pain", cur_pain );
    }
}

int Character::get_perceived_pain() const
{
    if( get_effect_int( effect_adrenaline ) > 1 ) {
        return 0;
    }

    return std::max( get_pain() - get_painkiller(), 0 );
}

float Character::fall_damage_mod() const
{
    if( has_flag( json_flag_FEATHER_FALL ) ) {
        return 0.0f;
    }
    float ret = 1.0f;

    // Ability to land properly is 2x as important as dexterity itself
    /** @EFFECT_DEX decreases damage from falling */

    /** @EFFECT_DODGE decreases damage from falling */
    float dex_dodge = dex_cur / 2.0 + get_skill_level( skill_dodge );
    // Reactions, legwork and footing determine your landing
    dex_dodge *= get_modifier( character_modifier_limb_fall_mod );
    // But prevent it from increasing damage
    dex_dodge = std::max( 0.0f, dex_dodge ); // 0
    // 100% damage at 0, 75% at 10, 50% at 20 and so on
    ret *= ( 100.0f - ( dex_dodge * 4.0f ) ) / 100.0f;

    if( has_proficiency( proficiency_prof_parkour ) ) {
        ret *= 2.0f / 3.0f;
    }

    // TODO: Bonus for Judo, mutations. Penalty for heavy weight (including mutations)

    ret = enchantment_cache->modify_value( enchant_vals::mod::FALL_DAMAGE, ret );

    return std::max( 0.0f, ret );
}

// force is maximum damage to hp before scaling
int Character::impact( const int force, const tripoint_bub_ms &p )
{
    // Falls over ~30m are fatal more often than not
    // But that would be quite a lot considering 21 z-levels in game
    // so let's assume 1 z-level is comparable to 30 force

    if( force <= 0 ) {
        return force;
    }

    // Damage modifier (post armor)
    float mod = 1.0f;
    int effective_force = force;
    int cut = 0;
    // Percentage armor penetration - armor won't help much here
    // TODO: Make cushioned items like bike helmets help more
    float armor_eff = 1.0f;
    // Shock Absorber CBM heavily reduces damage
    const bool shock_absorbers = has_active_bionic( bio_shock_absorber );

    // Being slammed against things rather than landing means we can't
    // control the impact as well
    const bool slam = p != pos_bub();
    std::string target_name = "a swarm of bugs";
    Creature *critter = get_creature_tracker().creature_at( p );
    map &here = get_map();
    if( critter != this && critter != nullptr ) {
        target_name = critter->disp_name();
        // Slamming into creatures and NPCs
        // TODO: Handle spikes/horns and hard materials
        armor_eff = 0.5f; // 2x as much as with the ground
        // TODO: Modify based on something?
        mod = 1.0f;
        effective_force = force;
    } else if( const optional_vpart_position vp = here.veh_at( p ) ) {
        // Slamming into vehicles
        // TODO: Integrate it with vehicle collision function somehow
        target_name = vp->vehicle().disp_name();
        if( vp.part_with_feature( "SHARP", true ) ) {
            // Now we're actually getting impaled
            cut = force; // Lots of fun
        }

        mod = slam ? 1.0f : fall_damage_mod();
        armor_eff = 0.25f; // Not much
        if( !slam && vp->part_with_feature( "ROOF", true ) ) {
            // Roof offers better landing than frame or pavement
            // TODO: Make this not happen with heavy-duty/plated roof
            effective_force /= 2;
        }
    } else {
        // Slamming into terrain/furniture
        target_name = here.disp_name( p );
        int hard_ground = here.has_flag( ter_furn_flag::TFLAG_DIGGABLE, p ) ? 0 : 3;
        armor_eff = 0.25f; // Not much
        // Get cut by stuff
        // This isn't impalement on metal wreckage, more like flying through a closed window
        cut = here.has_flag( ter_furn_flag::TFLAG_SHARP, p ) ? 5 : 0;
        effective_force = force + hard_ground;
        mod = slam ? 1.0f : fall_damage_mod();
        if( here.has_furn( p ) ) {
            effective_force = std::max( 0, effective_force - here.furn( p )->fall_damage_reduction );
        } else if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, p ) ) {
            const float swim_skill = get_skill_level( skill_swimming );
            effective_force /= 4.0f + 0.1f * swim_skill;
            if( here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, p ) ) {
                effective_force /= 1.5f;
                mod /= 1.0f + ( 0.1f * swim_skill );
            }
        }
        //checking for items on floor
        if( !here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, p ) &&
        !here.items_with( p, [&]( item const & it ) {
        return it.affects_fall();
        } ).empty() ) {
            std::list<item_location> fall_affecting_items =
            here.items_with( p, [&]( const item & it ) {
                return it.affects_fall();
            } );

            for( const item_location &floor_item : fall_affecting_items ) {
                effective_force = std::max( 0, effective_force - floor_item.get_item()->fall_damage_reduction() );
            }

        }
    }
    //for wielded items
    if( !here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, p ) &&
        weapon.affects_fall() ) {
        effective_force = std::max( 0, effective_force - weapon.fall_damage_reduction() );

    }
    // Rescale for huge force
    // At >30 force, proper landing is impossible and armor helps way less
    if( effective_force > 30 ) {
        // Armor simply helps way less
        armor_eff *= 30.0f / effective_force;
        if( mod < 1.0f ) {
            // Everything past 30 damage gets a worse modifier
            const float scaled_mod = std::pow( mod, 30.0f / effective_force );
            const float scaled_damage = ( 30.0f * mod ) + scaled_mod * ( effective_force - 30.0f );
            mod = scaled_damage / effective_force;
        }
    }

    if( !slam && mod < 1.0f && mod * force < 5 ) {
        // Perfect landing, no damage (regardless of armor)
        add_msg_if_player( m_warning, _( "You land on %s." ), target_name );
        return 0;
    }

    // Shock absorbers kick in only when they need to, so if our other protections fail, fall back on them
    if( shock_absorbers ) {
        effective_force -= 15; // Provide a flat reduction to force
        if( mod > 0.25f ) {
            mod = 0.25f; // And provide a 75% reduction against that force if we don't have it already
        }
        if( effective_force < 0 ) {
            effective_force = 0;
        }
    }

    int total_dealt = 0;
    if( mod * effective_force >= 5 ) {
        for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
            const int bash = effective_force * rng( 60, 100 ) / 100;
            damage_instance di;
            // FIXME: Hardcoded damage types
            di.add_damage( damage_bash, bash, 0, armor_eff, mod );
            // No good way to land on sharp stuff, so here modifier == 1.0f
            di.add_damage( damage_cut, cut, 0, armor_eff, 1.0f );
            total_dealt += deal_damage( nullptr, bp, di ).total_damage();
        }
    }

    if( total_dealt > 0 && is_avatar() ) {
        // "You slam against the dirt" is fine
        add_msg( m_bad, _( "You are slammed against %1$s for %2$d damage." ),
                 target_name, total_dealt );
    } else if( is_avatar() && shock_absorbers ) {
        add_msg( m_bad, _( "You are slammed against %s!" ),
                 target_name, total_dealt );
        add_msg( m_good, _( "…but your shock absorbers negate the damage!" ) );
    } else if( slam ) {
        // Only print this line if it is a slam and not a landing
        // Non-players should only get this one: player doesn't know how much damage was dealt
        // and landing messages for each slammed creature would be too much
        add_msg_player_or_npc( m_bad,
                               _( "You are slammed against %s." ),
                               _( "<npcname> is slammed against %s." ),
                               target_name );
    } else {
        // No landing message for NPCs
        add_msg_if_player( m_warning, _( "You land on %s." ), target_name );
    }

    if( x_in_y( mod, 1.0f ) && !here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, p ) ) {
        add_effect( effect_downed, rng( 1_turns, 1_turns + mod * 3_turns ) );
    }

    return total_dealt;
}

bool Character::can_fly()
{
    if( !move_effects( false ) || has_effect( effect_stunned ) ) {
        return false;
    }
    // GLIDE is for artifacts or things like jetpacks that don't care if you're tired or hurt.
    if( has_flag( json_flag_GLIDE ) ) {
        return true;
    }
    // TODO: Remove grandfathering traits in after Limb Stuff
    if( has_flag( json_flag_WINGS_2 ) ||
        has_flag( json_flag_WING_GLIDE ) || count_flag( json_flag_WING_ARMS ) >= 2 ) {

        if( 100 * weight_carried() / weight_capacity() > 50 || !has_two_arms_lifting() ) {
            return false;
        }
        return true;
    }
    return false;
}

// FIXME: Relies on hardcoded bash damage type
void Character::knock_back_to( const tripoint_bub_ms &to )
{
    map &here = get_map();
    if( to == pos_bub() ) {
        return;
    }

    creature_tracker &creatures = get_creature_tracker();
    // First, see if we hit a monster
    if( monster *const critter = creatures.creature_at<monster>( to ) ) {
        deal_damage( critter, bodypart_id( "torso" ), damage_instance( damage_bash,
                     static_cast<float>( critter->type->size ) ) );
        add_effect( effect_stunned, 1_turns );
        /** @EFFECT_STR_MAX allows knocked back player to knock back, damage, stun some monsters */
        if( ( str_max - 6 ) / 4 > critter->type->size ) {
            critter->knock_back_from( pos_bub() ); // Chain reaction!
            critter->apply_damage( this, bodypart_id( "torso" ), ( str_max - 6 ) / 4 );
            critter->add_effect( effect_stunned, 1_turns );
        } else if( ( str_max - 6 ) / 4 == critter->type->size ) {
            critter->apply_damage( this, bodypart_id( "torso" ), ( str_max - 6 ) / 4 );
            critter->add_effect( effect_stunned, 1_turns );
        }
        critter->check_dead_state( &here );

        add_msg_player_or_npc( _( "You bounce off a %s!" ), _( "<npcname> bounces off a %s!" ),
                               critter->name() );
        return;
    }

    if( npc *const np = creatures.creature_at<npc>( to ) ) {
        deal_damage( np, bodypart_id( "torso" ),
                     damage_instance( damage_bash, static_cast<float>( np->get_size() ) ) );
        add_effect( effect_stunned, 1_turns );
        np->deal_damage( this, bodypart_id( "torso" ), damage_instance( damage_bash, 3 ) );
        add_msg_player_or_npc( _( "You bounce off %s!" ), _( "<npcname> bounces off %s!" ),
                               np->get_name() );
        np->check_dead_state( &here );
        return;
    }

    // If we're still in the function at this point, we're actually moving a tile!
    if( here.has_flag( ter_furn_flag::TFLAG_LIQUID, to ) &&
        here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, to ) ) {
        if( !is_npc() ) {
            avatar_action::swim( here, get_avatar(), to );
        }
        // TODO: NPCs can't swim!
    } else if( here.impassable( to ) ) { // Wait, it's a wall

        // It's some kind of wall.
        // TODO: who knocked us back? Maybe that creature should be the source of the damage?
        apply_damage( nullptr, bodypart_id( "torso" ), 3 );
        add_effect( effect_stunned, 2_turns );
        add_msg_player_or_npc( _( "You bounce off a %s!" ), _( "<npcname> bounces off a %s!" ),
                               here.obstacle_name( to ) );

    } else { // It's no wall
        setpos( here, to );
    }
}

int Character::hp_percentage() const
{
    const bodypart_id head_id = bodypart_id( "head" );
    const bodypart_id torso_id = bodypart_id( "torso" );
    int total_cur = 0;
    int total_max = 0;
    // Head and torso HP are weighted 3x and 2x, respectively
    total_cur = get_part_hp_cur( head_id ) * 3 + get_part_hp_cur( torso_id ) * 2;
    total_max = get_part_hp_max( head_id ) * 3 + get_part_hp_max( torso_id ) * 2;
    for( const std::pair< const bodypart_str_id, bodypart> &elem : get_body() ) {
        total_cur += elem.second.get_hp_cur();
        total_max += elem.second.get_hp_max();
    }

    return ( 100 * total_cur ) / total_max;
}

void Character::on_worn_item_transform( const item &old_it, const item &new_it )
{
    morale->on_worn_item_transform( old_it, new_it );
}

void Character::leak_items()
{
    map &here = get_map();

    std::vector<item_location> removed_items;
    if( weapon.is_container() ) {
        if( weapon.leak( here, this, pos_bub() ) ) {
            weapon.spill_contents( pos_bub() );
        }
    } else if( weapon.made_of( phase_id::LIQUID ) ) {
        if( weapon.leak( here, this, pos_bub() ) ) {
            here.add_item_or_charges( pos_bub(), weapon );
            removed_items.emplace_back( *this, &weapon );
            add_msg_if_player( m_warning, _( "%s spilled from your hand." ), weapon.tname() );
        }
    }

    for( item_location it : top_items_loc() ) {
        if( !it || ( !it->is_container() && !it->made_of( phase_id::LIQUID ) ) ) {
            continue;
        }
        if( it->leak( here, this, pos_bub() ) ) {
            it->spill_contents( pos_bub() );
            removed_items.push_back( it );
        }
    }
    for( item_location removed : removed_items ) {
        removed.remove_item();
    }
}

void Character::process_items( map *here )
{
    if( weapon.process( *here, this, pos_bub( *here ) ) ) {
        weapon.spill_contents( here,  pos_bub( *here ) );
        remove_weapon();
    }

    std::vector<item_location> removed_items;
    for( item_location it : top_items_loc() ) {
        if( !it ) {
            continue;
        }
        if( it->process( *here, this, pos_bub( *here ) ) ) {
            it->spill_contents( here, pos_bub( *here ) );
            removed_items.push_back( it );
        }
    }
    for( item_location removed : removed_items ) {
        removed.remove_item();
    }

    // Active item processing done, now we're recharging.
    if( worn.check_item_encumbrance_flag( get_check_encumbrance() ) ) {
        calc_encumbrance();
        set_check_encumbrance( false );
    }

    // Load all items that use the UPS and have their own battery to their minimal functional charge,
    // The tool is not really useful if its charges are below charges_to_use
    std::vector<item *> inv_use_ups = cache_get_items_with( flag_USE_UPS, [&here]( item & it ) {
        return ( it.ammo_capacity( ammo_battery ) > it.ammo_remaining_linked( *here, nullptr ) ||
                 ( it.type->battery && it.type->battery->max_capacity > it.energy_remaining( nullptr ) ) );
    } );
    if( !inv_use_ups.empty() ) {
        const units::energy available_charges = available_ups();
        units::energy ups_used = 0_kJ;
        for( item * const &it : inv_use_ups ) {
            // For powered armor, an armor-powering bionic should always be preferred over UPS usage.
            if( it->is_power_armor() && can_interface_armor() && has_power() ) {
                // Bionic power costs are handled elsewhere
                continue;
            } else if( it->active && !it->ammo_sufficient( this ) ) {
                it->deactivate();
            } else if( available_charges - ups_used >= 1_kJ &&
                       it->ammo_remaining_linked( *here, nullptr ) < it->ammo_capacity( ammo_battery ) ) {
                // Charge the battery in the UPS modded tool
                ups_used += 1_kJ;
                it->ammo_set( itype_battery, it->ammo_remaining_linked( *here, nullptr ) + 1 );
            }
        }
        if( ups_used > 0_kJ ) {
            consume_ups( ups_used );
        }
    }
}

void Character::search_surroundings()
{
    if( controlling_vehicle ) {
        return;
    }
    if( has_effect( effect_subaquatic_sonar ) && is_underwater() && calendar::once_every( 4_turns ) ) {
        echo_pulse();
    }
    map &here = get_map();
    // Search for traps in a larger area than before because this is the only
    // way we can "find" traps that aren't marked as visible.
    // Detection formula takes care of likelihood of seeing within this range.
    for( const tripoint_bub_ms &tp : here.points_in_radius( pos_bub(), 5 ) ) {
        const trap &tr = here.tr_at( tp );
        if( tr.is_null() || tp == pos_bub() ) {
            continue;
        }
        // Note that echolocation and SONAR also do this separately in echo_pulse()
        if( has_active_bionic( bio_ground_sonar ) && !knows_trap( tp ) && tr.detected_by_ground_sonar() ) {
            const std::string direction = direction_name( direction_from( pos_bub(), tp ) );
            add_msg_if_player( m_warning, _( "Your ground sonar detected a %1$s to the %2$s!" ),
                               tr.name(), direction );
            add_known_trap( tp, tr );
        }
        if( !sees( here, tp ) ) {
            continue;
        }
        if( tr.can_see( tp, *this ) ) {
            // Already seen, or can never be seen
            continue;
        }
        // Chance to detect traps we haven't yet seen.
        if( tr.detect_trap( tp, *this ) ) {
            if( !tr.is_trivial_to_spot() ) {
                // Only bug player about traps that aren't trivial to spot.
                const std::string direction = direction_name(
                                                  direction_from( pos_bub(), tp ) );
                practice_proficiency( proficiency_prof_spotting, 1_minutes );
                // Seeing a trap set properly gives you a little bonus to trapsetting profs.
                practice_proficiency( proficiency_prof_traps, 10_seconds );
                practice_proficiency( proficiency_prof_trapsetting, 10_seconds );
                add_msg_if_player( _( "You've spotted a %1$s to the %2$s!" ),
                                   tr.name(), direction );
            }
            add_known_trap( tp, tr );
        }
    }
}

bool Character::wield( item &it, std::optional<int> obtain_cost )
{
    invalidate_inventory_validity_cache();
    invalidate_leak_level_cache();

    item_location wielded = get_wielded_item();

    if( has_wield_conflicts( it ) ) {
        const bool is_unwielding = is_wielding( it );
        const auto ret = can_unwield( it );

        if( !ret.success() ) {
            add_msg_if_player( m_info, "%s", ret.c_str() );
            return false;
        }

        if( !is_unwielding && wielded->has_item( it ) ) {
            add_msg_if_player( m_info,
                               _( "You need to put the bag away before trying to wield something from it." ) );
            return false;
        }

        if( !unwield() ) {
            return false;
        }

        if( is_unwielding ) {
            if( !martial_arts_data->selected_is_none() ) {
                martial_arts_data->martialart_use_message( *this );
            }
            return true;
        }
    }

    if( !can_wield( it ).success() ) {
        return false;
    }

    bool combine_stacks = wielded && it.can_combine( *wielded );
    cached_info.erase( "weapon_value" );

    // Wielding from inventory is relatively slow and does not improve with increasing weapon skill.
    // Worn items (including guns with shoulder straps) are faster but still slower
    // than a skilled player with a holster.
    // There is an additional penalty when wielding items from the inventory whilst currently grabbed.

    bool worn = is_worn( it );

    // Ideally the cost should be calculated from wield(item_location), but as backup try it here.
    const int mv = obtain_cost.value_or( item_handling_cost( it, true,
                                         INVENTORY_HANDLING_PENALTY / ( worn ? 2 : 1 ) ) );

    if( worn ) {
        it.on_takeoff( *this );
    }

    add_msg_debug( debugmode::DF_AVATAR, "wielding took %d moves", mv );
    mod_moves( -mv );

    item to_wield;
    if( has_item( it ) ) {
        to_wield = i_rem( &it );
    } else {
        // is_null means fists
        to_wield = it.is_null() ? item() : it;
    }

    if( combine_stacks ) {
        wielded->combine( to_wield );
    } else {
        set_wielded_item( to_wield );
    }

    // set_wielded_item invalidates the weapon item_location, so get it again
    wielded = get_wielded_item();
    recoil = MAX_RECOIL;

    // if fists are wielded get_wielded_item returns item_location::nowhere, which is a nullptr
    if( wielded ) {
        last_item = wielded->typeId();
        wielded->on_wield( *this );
        inv->update_invlet( *wielded );
        inv->update_cache_with_item( *wielded );
        cata::event e = cata::event::make<event_type::character_wields_item>( getID(), last_item );
        get_event_bus().send_with_talker( this, &wielded, e );
    } else {
        last_item = to_wield.typeId();
        get_event_bus().send<event_type::character_wields_item>( getID(), item().typeId() );
    }
    return true;
}

bool Character::wield( item_location loc, bool remove_old )
{
    if( !loc ) {
        add_msg_if_player( _( "No item." ) );
        return false;
    }

    if( !wield( *loc, loc.obtain_cost( *this ) ) ) {
        return false;
    }

    if( remove_old && loc ) {
        loc.remove_item();
    }
    return true;
}

bool Character::wield_contents( item &container, item *internal_item, bool penalties,
                                int base_cost )
{
    // if index not specified and container has multiple items then ask the player to choose one
    if( internal_item == nullptr ) {
        debugmsg( "No valid target for wield contents." );
        return false;
    }

    if( !container.has_item( *internal_item ) ) {
        debugmsg( "Tried to wield non-existent item from container (Character::wield_contents)" );
        return false;
    }

    const ret_val<void> ret = can_wield( *internal_item );
    if( !ret.success() ) {
        add_msg_if_player( m_info, "%s", ret.c_str() );
        return false;
    }

    int mv = 0;

    if( has_wield_conflicts( *internal_item ) ) {
        if( !unwield() ) {
            return false;
        }
        inv->unsort();
    }

    // for holsters, we should not include the cost of wielding the holster itself
    // The cost of wielding the holster was already added earlier in avatar_action::use_item.
    // As we couldn't make sure back then what action was going to be used, we remove the cost now.
    item_location il = item_location( *this, &container );
    mv -= il.obtain_cost( *this );
    mv += item_retrieve_cost( *internal_item, container, penalties, base_cost );

    if( internal_item->stacks_with( weapon, true ) ) {
        weapon.combine( *internal_item );
    } else {
        weapon = std::move( *internal_item );
    }
    container.remove_item( *internal_item );
    container.on_contents_changed();

    inv->update_invlet( weapon );
    inv->update_cache_with_item( weapon );
    last_item = weapon.typeId();

    mod_moves( -mv );

    weapon.on_wield( *this );

    item_location loc( *this, &weapon );
    cata::event e = cata::event::make<event_type::character_wields_item>( getID(), weapon.typeId() );
    get_event_bus().send_with_talker( this, &loc, e );

    return true;
}

void Character::store( item &container, item &put, bool penalties, int base_cost,
                       pocket_type pk_type, bool check_best_pkt )
{
    mod_moves( -item_store_cost( put, container, penalties, base_cost ) );
    if( check_best_pkt && pk_type == pocket_type::CONTAINER &&
        container.get_all_contained_pockets().size() > 1 ) {
        // Bypass pocket settings (assuming the item is manually stored)
        int charges = put.count_by_charges() ? put.charges : 1;
        container.fill_with( i_rem( &put ), charges, false, false, true );
    } else {
        container.put_in( i_rem( &put ), pk_type );
    }
    calc_encumbrance();
}

void Character::store( item_pocket *pocket, item &put, bool penalties, int base_cost )
{
    if( !pocket ) {
        return;
    }

    item_location char_item( *this, &null_item_reference() );
    item_pocket *pkt_best = pocket->best_pocket_in_contents( char_item, put, nullptr, false,
                            false ).second;
    if( !!pkt_best && pocket->better_pocket( *pkt_best, put, true ) ) {
        pocket = pkt_best;
    }
    mod_moves( -std::max( item_store_cost( put, null_item_reference(), penalties, base_cost ),
                          pocket->obtain_cost( put ) ) );
    ret_val<item *> result = pocket->insert_item( i_rem( &put ) );
    result.value()->on_pickup( *this );
    calc_encumbrance();
}

void Character::use_wielded()
{
    use( -1 );
}

void Character::use( int inventory_position )
{
    item &used = i_at( inventory_position );
    item_location loc = item_location( *this, &used );

    use( loc );
}

void Character::use( item_location loc, int pre_obtain_moves, std::string const &method )
{
    map &here = get_map();

    if( has_effect( effect_incorporeal ) ) {
        add_msg_if_player( m_bad, _( "You can't use anything while incorporeal." ) );
        return;
    }

    // if -1 is passed in we don't want to change moves at all
    if( pre_obtain_moves == -1 ) {
        pre_obtain_moves = get_moves();
    }
    if( !loc ) {
        add_msg( m_info, _( "You do not have that item." ) );
        set_moves( pre_obtain_moves );
        return;
    }

    item &used = *loc;
    last_item = used.typeId();

    if( used.is_tool() ) {
        if( !used.type->has_use() ) {
            add_msg_if_player( _( "You can't do anything interesting with your %s." ), used.tname() );
            set_moves( pre_obtain_moves );
            return;
        }
        invoke_item( &used, method, loc.pos_bub( here ), pre_obtain_moves );

    } else if( used.type->can_use( "PETFOOD" ) ) { // NOLINT(bugprone-branch-clone)
        invoke_item( &used, method, loc.pos_bub( here ), pre_obtain_moves );

    } else if( !used.is_craft() && ( used.is_medication() || ( !used.type->has_use() &&
                                     used.is_food() ) ) ) {

        if( used.is_medication() && !can_use_heal_item( used ) ) {
            add_msg_if_player( m_bad, _( "Your biology is not compatible with that healing item." ) );
            set_moves( pre_obtain_moves );
            return;
        }

        if( avatar *u = as_avatar() ) {
            const ret_val<edible_rating> ret = u->will_eat( used, true );
            if( !ret.success() ) {
                set_moves( pre_obtain_moves );
                return;
            }
            u->assign_activity( consume_activity_actor( loc ) );
        } else  {
            const time_duration &consume_time = get_consume_time( used );
            mod_moves( -to_moves<int>( consume_time ) );
            consume( loc );
        }
    } else if( used.is_book() ) {
        // TODO: Handle this with dynamic dispatch.
        if( avatar *u = as_avatar() ) {
            if( loc.is_efile() ) {
                u->read( loc, loc.parent_item() );
            } else {
                u->read( loc );
            }
        }
    } else if( used.type->has_use() ) {
        invoke_item( &used, method, loc.pos_bub( here ), pre_obtain_moves );
    } else if( used.has_flag( flag_SPLINT ) ) {
        ret_val<void> need_splint = can_wear( *loc );
        if( need_splint.success() ) {
            wear_item( used );
            loc.remove_item();
        } else {
            add_msg( m_info, need_splint.str() );
        }
    } else if( used.is_relic() ) {
        invoke_item( &used, method, loc.pos_bub( here ), pre_obtain_moves );
    } else {
        if( !is_armed() ) {
            add_msg( m_info, _( "You are not wielding anything you could use." ) );
        } else {
            add_msg( m_info, _( "You can't do anything interesting with your %s." ), used.tname() );
        }
        set_moves( pre_obtain_moves );
    }
}

int Character::climbing_cost( const tripoint_bub_ms &from, const tripoint_bub_ms &to ) const
{
    map &here = get_map();
    if( !here.valid_move( from, to, false, true ) ) {
        return 0;
    }

    const int diff = here.climb_difficulty( from );

    if( diff > 5 ) {
        return 0;
    }

    return 50 + diff * 100;
    // TODO: All sorts of mutations, equipment weight etc.
}

void Character::environmental_revert_effect()
{
    addictions.clear();
    morale->clear();

    set_all_parts_hp_to_max();
    set_hunger( 0 );
    set_thirst( 0 );
    set_sleepiness( 0 );
    set_lifestyle( 0 );
    set_daily_health( 0 );
    set_stim( 0 );
    set_pain( 0 );
    set_painkiller( 0 );
    set_rad( 0 );

    recalc_sight_limits();
    calc_encumbrance();
}

bodypart_id Character::most_staunchable_bp()
{
    int max;
    return most_staunchable_bp( max );
}

bodypart_id Character::most_staunchable_bp( int &max_staunch )
{
    // Calculate max staunchable bleed level
    // Top out at 20 intensity for base, unencumbered survivors
    max_staunch = 20;
    max_staunch *= get_modifier( character_modifier_bleed_staunch_mod );
    add_msg_debug( debugmode::DF_CHARACTER, "Staunch limit after limb score modifier %d", max_staunch );

    // +5 bonus if you know your first aid
    if( has_proficiency( proficiency_prof_wound_care ) ||
        has_proficiency( proficiency_prof_wound_care_expert ) ) {
        max_staunch += 5;
        add_msg_debug( debugmode::DF_CHARACTER, "Wound care proficiency found, new limit %d", max_staunch );
    }

    int num_broken_arms = get_num_broken_body_parts_of_type( body_part_type::type::arm );
    int num_arms = get_num_body_parts_of_type( body_part_type::type::arm );

    // Don't warn about encumbrance if your arms are broken
    if( num_broken_arms ) {
        // Handle multiple arms
        max_staunch *= ( 1.0f - num_broken_arms / static_cast<float>( num_arms ) );
        add_msg_debug( debugmode::DF_CHARACTER, "%d out of %d arms broken, staunch limit %d",
                       num_broken_arms, num_arms, max_staunch );
    }

    int most = 0;
    int intensity = 0;
    bodypart_id bp_id = bodypart_str_id::NULL_ID();
    for( const bodypart_id &bp : get_all_body_parts() ) {
        intensity = get_effect_int( effect_bleed, bp );
        // Staunching a bleeding on one of your arms is hard (handle multiple arms)
        if( bp->has_type( body_part_type::type::arm ) ) {
            intensity /= ( 1.0f - 1.0f / num_arms );
        }
        // Tourniquets make staunching easier, letting you treat arterial bleeds on your legs
        if( worn_with_flag( flag_TOURNIQUET, bp ) ) {
            intensity /= 2;
        }
        if( most < get_effect_int( effect_bleed, bp ) && intensity <= max_staunch ) {
            // Don't use intensity here, we might have increased it for the arm penalty
            most = get_effect_int( effect_bleed, bp );
            bp_id = bp;
        }
    }
    add_msg_debug( debugmode::DF_CHARACTER,
                   "Selected %s to staunch (base intensity %d, modified intensity %d)", bp_id->name, intensity, most );

    return bp_id;
}

void Character::pause()
{
    set_moves( 0 );
    recoil = MAX_RECOIL;
    map &here = get_map();

    // effects of being partially/fully underwater
    if( !in_vehicle && !here.has_flag_furn( "BRIDGE", pos_bub( ) ) ) {
        if( underwater ) {
            // TODO: gain "swimming" proficiency but not "athletics" skill
            drench( 100, get_drenching_body_parts(), false );
        } else if( here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, pos_bub() ) ) {
            // TODO: gain "swimming" proficiency but not "athletics" skill
            // Same as above, except no head/eyes/mouth
            drench( 100, get_drenching_body_parts( false ), false );
        } else if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, pos_bub() ) ) {
            drench( 80, get_drenching_body_parts( false, false ),
                    false );
        }
    }

    // Try to put out clothing/hair fire
    if( has_effect( effect_onfire ) ) {
        time_duration total_removed = 0_turns;
        time_duration total_left = 0_turns;
        bool on_ground = is_prone();
        for( const bodypart_id &bp : get_all_body_parts() ) {
            effect &eff = get_effect( effect_onfire, bp );
            if( eff.is_null() ) {
                continue;
            }

            // TODO: Tools and skills
            total_left += eff.get_duration();
            // Being on the ground will smother the fire much faster because you can roll
            const time_duration dur_removed = on_ground ? eff.get_duration() / 2 + 2_turns : 1_turns;
            eff.mod_duration( -dur_removed );
            total_removed += dur_removed;
        }

        // Don't drop on the ground when the ground is on fire
        if( total_left > 1_minutes && !is_dangerous_fields( here.field_at( pos_bub() ) ) ) {
            add_effect( effect_downed, 2_turns, false, 0, true );
            add_msg_player_or_npc( m_warning,
                                   _( "You roll on the ground, trying to smother the fire!" ),
                                   _( "<npcname> rolls on the ground!" ) );
        } else if( total_removed > 0_turns ) {
            add_msg_player_or_npc( m_warning,
                                   _( "You attempt to put out the fire on you!" ),
                                   _( "<npcname> attempts to put out the fire on them!" ) );
        }
    }

    // put pressure on bleeding wound, prioritizing most severe bleeding that you can compress
    if( !controlling_vehicle && !is_armed() && has_effect( effect_bleed ) ) {
        int max = 0;
        bodypart_id bp_id = most_staunchable_bp( max );

        // Don't warn about encumbrance if your arms are broken
        int num_broken_arms = get_num_broken_body_parts_of_type( body_part_type::type::arm );
        if( num_broken_arms ) {
            add_msg_player_or_npc( m_warning,
                                   _( "Your broken limb significantly hampers your efforts to put pressure on a bleeding wound!" ),
                                   _( "<npcname>'s broken limb significantly hampers their effort to put pressure on a bleeding wound!" ) );
        } else if( max < 10 ) {
            add_msg_player_or_npc( m_warning,
                                   _( "Your hands are too encumbered to effectively put pressure on a bleeding wound!" ),
                                   _( "<npcname>'s hands are too encumbered to effectively put pressure on a bleeding wound!" ) );
        }

        if( bp_id == bodypart_str_id::NULL_ID() ) {
            // We're bleeding, but couldn't find any bp we can staunch
            add_msg_player_or_npc( m_warning,
                                   _( "Your bleeding is beyond staunching barehanded!  A tourniquet might help." ),
                                   _( "<npcname>'s bleeding is beyond staunching barehanded!" ) );
        } else {
            // 5 - 30 sec per turn (with standard hands)
            time_duration benefit = 5_turns + 1_turns * max;
            effect &e = get_effect( effect_bleed, bp_id );
            e.mod_duration( - benefit );
            add_msg_player_or_npc( m_warning,
                                   _( "You put pressure on the bleeding wound…" ),
                                   _( "<npcname> puts pressure on the bleeding wound…" ) );
            practice_proficiency( proficiency_prof_wound_care, 1_turns );
        }
    }
    // on-pause effects for martial arts
    martial_arts_data->ma_onpause_effects( *this );

    if( is_npc() ) {
        // The stuff below doesn't apply to NPCs
        // search_surroundings should eventually do, though
        return;
    }

    if( in_vehicle && one_in( 8 ) ) {
        VehicleList vehs = here.get_vehicles();
        vehicle *veh = nullptr;
        for( wrapped_vehicle &v : vehs ) {
            veh = v.v;
            if( veh && veh->is_moving() && veh->player_in_control( here, *this ) ) {
                double exp_temp = 1 + veh->total_mass( here ) / 400.0_kilogram +
                                  std::abs( veh->velocity / 3200.0 );
                int experience = static_cast<int>( exp_temp );
                if( exp_temp - experience > 0 && x_in_y( exp_temp - experience, 1.0 ) ) {
                    experience++;
                }
                practice( skill_driving, experience );
                break;
            }
        }
    }

    search_surroundings();
    wait_effects();
}

bool Character::can_lift( item &obj ) const
{
    // avoid comparing by weight as different objects use differing scales (grams vs kilograms etc)
    int str = get_lift_str();
    if( mounted_creature ) {
        auto *const mons = mounted_creature.get();
        str = mons->mech_str_addition() == 0 ? str : mons->mech_str_addition();
    }
    const int npc_str = get_lift_assist();
    return str + npc_str >= obj.lift_strength();
}

bool Character::can_lift( vehicle &veh, map &here ) const
{
    // avoid comparing by weight as different objects use differing scales (grams vs kilograms etc)
    int str = get_lift_str();
    if( mounted_creature ) {
        auto *const mons = mounted_creature.get();
        str = mons->mech_str_addition() == 0 ? str : mons->mech_str_addition();
    }
    const int npc_str = get_lift_assist();
    return str + npc_str >= veh.lift_strength( here );
}

static std::string wrap60( const std::string &text )
{
    return string_join( foldstring( text, 60 ), "\n" );
}

bool character_martial_arts::pick_style( const Character &you ) // Style selection menu
{
    enum style_selection {
        KEEP_HANDS_FREE = 0,
        STYLE_OFFSET
    };

    // Check for martial art styles known from active bionics
    std::set<matype_id> bio_styles;
    for( const bionic &bio : *you.my_bionics ) {
        const std::vector<matype_id> &bio_ma_list = bio.id->ma_styles;
        if( !bio_ma_list.empty() && you.has_active_bionic( bio.id ) ) {
            bio_styles.insert( bio_ma_list.begin(), bio_ma_list.end() );
        }
    }
    std::vector<matype_id> selectable_styles;
    if( bio_styles.empty() ) {
        selectable_styles = ma_styles;
    } else {
        selectable_styles.insert( selectable_styles.begin(), bio_styles.begin(), bio_styles.end() );
    }

    // If there are style already, cursor starts there
    // if no selected styles, cursor starts from no-style

    // Any other keys quit the menu
    input_context ctxt( "MELEE_STYLE_PICKER", keyboard_mode::keycode );
    ctxt.register_action( "SHOW_DESCRIPTION" );

    uilist kmenu;
    kmenu.title = _( "Select a style.\n" );
    kmenu.text = string_format( _( "STR: <color_white>%d</color>, DEX: <color_white>%d</color>, "
                                   "PER: <color_white>%d</color>, INT: <color_white>%d</color>\n"
                                   "Press [<color_yellow>%s</color>] for technique details and compatible weapons.\n" ),
                                you.get_str(), you.get_dex(), you.get_per(), you.get_int(),
                                ctxt.get_desc( "SHOW_DESCRIPTION" ) );
    ma_style_callback callback( static_cast<size_t>( STYLE_OFFSET ), selectable_styles );
    kmenu.callback = &callback;
    kmenu.input_category = "MELEE_STYLE_PICKER";
    kmenu.additional_actions.emplace_back( "SHOW_DESCRIPTION", translation() );
    kmenu.desc_enabled = true;
    kmenu.addentry_desc( KEEP_HANDS_FREE, true, 'h',
                         keep_hands_free ? _( "Keep hands free (on)" ) : _( "Keep hands free (off)" ),
                         wrap60( _( "When this is enabled, player won't wield things unless explicitly told to." ) ) );

    kmenu.selected = STYLE_OFFSET;

    // +1 to keep "No Style" at top
    std::sort( selectable_styles.begin() + 1, selectable_styles.end(),
    []( const matype_id & a, const matype_id & b ) {
        return localized_compare( a->name.translated(), b->name.translated() );
    } );

    for( size_t i = 0; i < selectable_styles.size(); i++ ) {
        const martialart &style = selectable_styles[i].obj();
        //Check if this style is currently selected
        const bool selected = selectable_styles[i] == style_selected;
        std::string entry_text = style.name.translated();
        if( selected ) {
            kmenu.selected = i + STYLE_OFFSET;
            entry_text = colorize( entry_text, c_pink );
        }
        kmenu.addentry_desc( i + STYLE_OFFSET, true, -1, entry_text,
                             wrap60( style.description.translated() ) );
    }

    kmenu.query();
    int selection = kmenu.ret;

    if( selection >= STYLE_OFFSET ) {
        // If the currect style is selected, do not change styles

        Character &u = const_cast<Character &>( you );
        style_selected->remove_all_buffs( u );
        style_selected = selectable_styles[selection - STYLE_OFFSET];
        ma_static_effects( u );
        martialart_use_message( you );
    } else if( selection == KEEP_HANDS_FREE ) {
        keep_hands_free = !keep_hands_free;
    } else {
        return false;
    }

    return true;
}
