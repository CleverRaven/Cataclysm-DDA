#include <cstdlib>
#include <functional>
#include <list>
#include <memory>
#include <string>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_attire.h"
#include "coordinates.h"
#include "flag.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"
#include "value_ptr.h"

static const efftype_id effect_antifungal( "antifungal" );
static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_bile_irritant( "bile_irritant" );
static const efftype_id effect_bloodworms( "bloodworms" );
static const efftype_id effect_boomered( "boomered" );
static const efftype_id effect_brainworms( "brainworms" );
static const efftype_id effect_conjunctivitis( "conjunctivitis" );
static const efftype_id effect_cureall( "cureall" );
static const efftype_id effect_dermatik( "dermatik" );
static const efftype_id effect_foodpoison( "foodpoison" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_glowing( "glowing" );
static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_high( "high" );
static const efftype_id effect_paincysts( "paincysts" );
static const efftype_id effect_shakes( "shakes" );
static const efftype_id effect_slimed( "slimed" );
static const efftype_id effect_smoke_lungs( "smoke_lungs" );
static const efftype_id effect_spores( "spores" );
static const efftype_id effect_tapeworm( "tapeworm" );
static const efftype_id effect_teargas( "teargas" );
static const efftype_id effect_took_anticonvulsant_visible( "took_anticonvulsant_visible" );
static const efftype_id effect_took_prozac( "took_prozac" );
static const efftype_id effect_took_prozac_visible( "took_prozac_visible" );
static const efftype_id effect_took_thorazine( "took_thorazine" );
static const efftype_id effect_took_xanax( "took_xanax" );
static const efftype_id effect_took_xanax_visible( "took_xanax_visible" );
static const efftype_id effect_valium( "valium" );
static const efftype_id effect_visuals( "visuals" );

static const furn_str_id furn_f_toilet( "f_toilet" );

static const itype_id itype_55gal_drum( "55gal_drum" );
static const itype_id itype_albuterol( "albuterol" );
static const itype_id itype_antifungal( "antifungal" );
static const itype_id itype_antiparasitic( "antiparasitic" );
static const itype_id itype_atomic_coffee( "atomic_coffee" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_coffee( "coffee" );
static const itype_id itype_diazepam( "diazepam" );
static const itype_id itype_inhaler( "inhaler" );
static const itype_id itype_oxygen( "oxygen" );
static const itype_id itype_oxygen_tank( "oxygen_tank" );
static const itype_id itype_panacea( "panacea" );
static const itype_id itype_prozac( "prozac" );
static const itype_id itype_pur_tablets( "pur_tablets" );
static const itype_id itype_saline( "saline" );
static const itype_id itype_thorazine( "thorazine" );
static const itype_id itype_towel( "towel" );
static const itype_id itype_towel_wet( "towel_wet" );
static const itype_id itype_water( "water" );
static const itype_id itype_water_clean( "water_clean" );
static const itype_id itype_water_purifying( "water_purifying" );
static const itype_id itype_xanax( "xanax" );

static const morale_type morale_wet( "morale_wet" );

static const ter_str_id ter_t_grass( "t_grass" );

TEST_CASE( "eyedrops", "[iuse][eyedrops]" )
{
    avatar dummy;
    //Give eyes to our dummy
    dummy.set_body();
    REQUIRE( dummy.has_part( bodypart_id( "eyes" ) ) );
    dummy.normalize();

    item eyedrops( itype_saline, calendar::turn_zero, item::default_charges_tag{} );

    int charges_before = eyedrops.charges;
    REQUIRE( charges_before > 0 );

    GIVEN( "avatar is boomered" ) {
        dummy.add_effect( effect_boomered, 1_hours );
        REQUIRE( dummy.has_effect( effect_boomered ) );

        WHEN( "they use eye drops" ) {
            dummy.consume( eyedrops );

            THEN( "one dose is depleted" ) {
                CHECK( eyedrops.charges == charges_before - 1 );

                AND_THEN( "it removes the boomered effect" ) {
                    CHECK_FALSE( dummy.has_effect( effect_boomered ) );
                }
            }
        }
    }

    charges_before = eyedrops.charges;
    REQUIRE( charges_before > 0 );

    GIVEN( "avatar gets conjunctivitis" ) {
        dummy.add_effect( effect_conjunctivitis, 72_hours, bodypart_id( "eyes" ) );
        REQUIRE( dummy.has_effect( effect_conjunctivitis, bodypart_id( "eyes" ) ) );
        REQUIRE( dummy.get_effect_dur( effect_conjunctivitis, bodypart_id( "eyes" ) ) > 24_hours );

        WHEN( "they use eye drops" ) {
            dummy.consume( eyedrops );

            THEN( "one dose is depleted" ) {
                CHECK( eyedrops.charges == charges_before - 1 );

                AND_THEN( "it shortens the duration of conjunctivitis" ) {
                    CHECK( dummy.get_effect_dur( effect_conjunctivitis, bodypart_id( "eyes" ) ) <= 24_hours );
                }
            }
        }
    }

    charges_before = eyedrops.charges;
    REQUIRE( charges_before > 0 );

    GIVEN( "avatar is underwater" ) {
        dummy.set_underwater( true );

        WHEN( "they try to use eye drops" ) {
            dummy.consume( eyedrops );

            THEN( "None were used" ) {
                CHECK( eyedrops.charges == charges_before );
            }
        }
    }
}

TEST_CASE( "antifungal", "[iuse][antifungal]" )
{
    avatar dummy;
    dummy.normalize();

    item_location antifungal = dummy.i_add( item( itype_antifungal, calendar::turn ) );

    REQUIRE( dummy.has_item_with( []( const item & it ) {
        return it.typeId() == itype_antifungal;
    } ) );

    GIVEN( "avatar has a fungal infection" ) {
        dummy.add_effect( effect_fungus, 1_hours );
        REQUIRE( dummy.has_effect( effect_fungus ) );

        WHEN( "they take an antifungal drug" ) {
            dummy.consume( antifungal );

            THEN( "antifungal is used up" ) {
                CHECK_FALSE( dummy.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_antifungal;
                } ) );

                AND_THEN( "it applies the antifungal effect" ) {
                    CHECK( dummy.has_effect( effect_antifungal ) );
                }
                AND_WHEN( "time passes" ) {
                    const time_duration fungal_clock = dummy.get_effect_dur( effect_fungus );
                    REQUIRE( fungal_clock == 1_hours );
                    dummy.process_effects();
                    THEN( "duration of fungal infection shortens" ) {
                        CHECK( fungal_clock > dummy.get_effect_dur( effect_fungus ) );
                    }
                }
            }
        }
    }

    GIVEN( "avatar has fungal spores" ) {
        dummy.add_effect( effect_spores, 1_hours );
        REQUIRE( dummy.has_effect( effect_spores ) );

        WHEN( "they take an antifungal drug" ) {
            dummy.consume( antifungal );

            THEN( "antifungal is used up" ) {
                CHECK_FALSE( dummy.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_antifungal;
                } ) );

                AND_THEN( "it has no effect on the spores" ) {
                    CHECK( dummy.has_effect( effect_spores ) );
                }
            }
        }
    }
}

TEST_CASE( "antiparasitic", "[iuse][antiparasitic]" )
{
    avatar dummy;
    dummy.normalize();

    item_location antiparasitic = dummy.i_add( item( itype_antiparasitic, calendar::turn ) );

    REQUIRE( dummy.has_item_with( []( const item & it ) {
        return it.typeId() == itype_antiparasitic;
    } ) );

    GIVEN( "avatar has parasite infections" ) {
        dummy.add_effect( effect_dermatik, 1_hours );
        dummy.add_effect( effect_tapeworm, 1_hours );
        dummy.add_effect( effect_bloodworms, 1_hours );
        dummy.add_effect( effect_brainworms, 1_hours );
        dummy.add_effect( effect_paincysts, 1_hours );

        REQUIRE( dummy.has_effect( effect_dermatik ) );
        REQUIRE( dummy.has_effect( effect_tapeworm ) );
        REQUIRE( dummy.has_effect( effect_bloodworms ) );
        REQUIRE( dummy.has_effect( effect_brainworms ) );
        REQUIRE( dummy.has_effect( effect_paincysts ) );

        WHEN( "they use an antiparasitic drug" ) {
            dummy.consume( antiparasitic );

            THEN( "antiparasitic was used up" ) {
                CHECK_FALSE( dummy.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_antiparasitic;
                } ) );

                AND_THEN( "it cures all parasite infections" ) {
                    CHECK_FALSE( dummy.has_effect( effect_dermatik ) );
                    CHECK_FALSE( dummy.has_effect( effect_tapeworm ) );
                    CHECK_FALSE( dummy.has_effect( effect_bloodworms ) );
                    CHECK_FALSE( dummy.has_effect( effect_brainworms ) );
                    CHECK_FALSE( dummy.has_effect( effect_paincysts ) );
                }
            }
        }
    }

    GIVEN( "avatar has a fungal infection" ) {
        REQUIRE( dummy.has_item_with( []( const item & it ) {
            return it.typeId() == itype_antiparasitic;
        } ) );

        dummy.add_effect( effect_fungus, 1_hours );
        REQUIRE( dummy.has_effect( effect_fungus ) );

        WHEN( "they use an antiparasitic drug" ) {
            dummy.consume( antiparasitic );

            THEN( "antiparasitic was used up" ) {
                CHECK_FALSE( dummy.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_antiparasitic;
                } ) );

                AND_THEN( "it has no effect on the fungal infection" ) {
                    CHECK( dummy.has_effect( effect_fungus ) );
                }
            }
        }
    }
}

TEST_CASE( "anticonvulsant", "[iuse][anticonvulsant]" )
{
    avatar dummy;
    dummy.normalize();

    item_location anticonvulsant = dummy.i_add( item( itype_diazepam, calendar::turn ) );

    REQUIRE( dummy.has_item_with( []( const item & it ) {
        return it.typeId() == itype_diazepam;
    } ) );

    GIVEN( "avatar has the shakes" ) {
        dummy.add_effect( effect_shakes, 1_hours );
        REQUIRE( dummy.has_effect( effect_shakes ) );

        WHEN( "they use an anticonvulsant drug" ) {
            dummy.consume( anticonvulsant );

            THEN( "anticonvulsant was used up" ) {
                CHECK_FALSE( dummy.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_diazepam;
                } ) );

                AND_THEN( "it cures the shakes" ) {
                    CHECK_FALSE( dummy.has_effect( effect_shakes ) );

                    AND_THEN( "it has side-effects" ) {
                        CHECK( dummy.has_effect( effect_valium ) );
                        CHECK( dummy.has_effect( effect_high ) );
                        CHECK( dummy.has_effect( effect_took_anticonvulsant_visible ) );
                    }
                }
            }
        }
    }
}

// test the `iuse::oxygen_bottle` function
TEST_CASE( "oxygen_tank", "[iuse][oxygen_bottle]" )
{
    avatar dummy;
    dummy.normalize();

    item oxygen( itype_oxygen_tank );
    oxygen.ammo_set( itype_oxygen );

    int charges_before = oxygen.ammo_remaining( );
    REQUIRE( charges_before > 0 );

    // Ensure baseline painkiller value to measure painkiller effects
    dummy.set_painkiller( 0 );
    REQUIRE( dummy.get_painkiller() == 0 );

    GIVEN( "avatar is suffering from smoke inhalation" ) {
        dummy.add_effect( effect_smoke_lungs, 1_hours );
        REQUIRE( dummy.has_effect( effect_smoke_lungs ) );

        THEN( "a dose of oxygen relieves the smoke inhalation" ) {
            dummy.invoke_item( &oxygen );
            CHECK( oxygen.ammo_remaining( ) == charges_before - 1 );
            CHECK_FALSE( dummy.has_effect( effect_smoke_lungs ) );

            AND_THEN( "it acts as a mild painkiller" ) {
                CHECK( dummy.get_painkiller() == 2 );
            }
        }
    }

    GIVEN( "avatar is suffering from tear gas" ) {
        dummy.add_effect( effect_teargas, 1_hours );
        REQUIRE( dummy.has_effect( effect_teargas ) );

        THEN( "a dose of oxygen relieves the effects of tear gas" ) {
            dummy.invoke_item( &oxygen );
            CHECK( oxygen.ammo_remaining( ) == charges_before - 1 );
            CHECK_FALSE( dummy.has_effect( effect_teargas ) );

            AND_THEN( "it acts as a mild painkiller" ) {
                CHECK( dummy.get_painkiller() == 2 );
            }
        }
    }

    GIVEN( "avatar is suffering from asthma" ) {
        dummy.add_effect( effect_asthma, 1_hours );
        REQUIRE( dummy.has_effect( effect_asthma ) );

        THEN( "a dose of oxygen relieves the effects of asthma" ) {
            dummy.invoke_item( &oxygen );
            CHECK( oxygen.ammo_remaining( ) == charges_before - 1 );
            CHECK_FALSE( dummy.has_effect( effect_asthma ) );

            AND_THEN( "it acts as a mild painkiller" ) {
                CHECK( dummy.get_painkiller() == 2 );
            }
        }
    }

    GIVEN( "avatar has no ill effects for the oxygen to treat" ) {
        REQUIRE_FALSE( dummy.has_effect( effect_smoke_lungs ) );
        REQUIRE_FALSE( dummy.has_effect( effect_teargas ) );
        REQUIRE_FALSE( dummy.has_effect( effect_asthma ) );

        WHEN( "they are not already stimulated" ) {
            dummy.set_stim( 0 );
            REQUIRE( dummy.get_stim() == 0 );

            THEN( "a dose of oxygen is stimulating" ) {
                dummy.invoke_item( &oxygen );
                CHECK( oxygen.ammo_remaining( ) == charges_before - 1 );
                // values should match iuse function `oxygen_bottle`
                CHECK( dummy.get_stim() == 8 );

                AND_THEN( "it acts as a stronger painkiller" ) {
                    CHECK( dummy.get_painkiller() == 4 );
                }
            }
        }

        WHEN( "they are already quite stimulated" ) {
            // "quite stimulated" meaning the max-stimulation cutoff defined in
            // iuse function `oxygen_bottle`, which should match `max_stim` here:
            int max_stim = 16;

            dummy.set_stim( max_stim );
            REQUIRE( dummy.get_stim() == max_stim );

            THEN( "a dose of oxygen has no additional stimulation effects" ) {
                dummy.invoke_item( &oxygen );
                CHECK( oxygen.ammo_remaining( ) == charges_before - 1 );
                CHECK( dummy.get_stim() == max_stim );

                AND_THEN( "it acts as a mild painkiller" ) {
                    CHECK( dummy.get_painkiller() == 2 );
                }
            }
        }
    }
}

// test the `iuse::caff` and `iuse::atomic_caff` functions
TEST_CASE( "caffeine_and_atomic_caffeine", "[iuse][caff][atomic_caff]" )
{
    avatar dummy;
    dummy.normalize();

    // Baseline sleepiness level before caffeinating
    int sleepiness_before = 200;
    dummy.set_sleepiness( sleepiness_before );

    // No stimulants or radiation
    dummy.set_stim( 0 );
    dummy.set_rad( 0 );
    REQUIRE( dummy.get_stim() == 0 );
    REQUIRE( dummy.get_rad() == 0 );

    SECTION( "coffee reduces sleepiness, but does not give stimulant effect" ) {
        item coffee( itype_coffee, calendar::turn_zero, item::default_charges_tag{} );
        dummy.consume( coffee );
        CHECK( dummy.get_sleepiness() == sleepiness_before - coffee.get_comestible()->sleepiness_mod );
        CHECK( dummy.get_stim() == coffee.get_comestible()->stim );
    }

    SECTION( "atomic caffeine greatly reduces sleepiness, and increases stimulant effect" ) {
        item atomic_coffee( itype_atomic_coffee, calendar::turn_zero, item::default_charges_tag{} );
        dummy.consume( atomic_coffee );
        CHECK( dummy.get_sleepiness() == sleepiness_before -
               atomic_coffee.get_comestible()->sleepiness_mod );
        CHECK( dummy.get_stim() == atomic_coffee.get_comestible()->stim );
    }
}

TEST_CASE( "towel", "[iuse][towel]" )
{
    avatar dummy;
    dummy.normalize();

    item towel( itype_towel, calendar::turn_zero, item::default_charges_tag{} );

    GIVEN( "avatar is wet" ) {
        // Saturate torso, head, and both arms
        dummy.drench( 100, { body_part_torso, body_part_head, body_part_arm_l, body_part_arm_r },
                      false );
        REQUIRE( dummy.get_part_wetness( bodypart_id( "torso" ) ) > 0 );
        REQUIRE( dummy.get_part_wetness( bodypart_id( "head" ) ) > 0 );
        REQUIRE( dummy.get_part_wetness( bodypart_id( "arm_l" ) ) > 0 );
        REQUIRE( dummy.get_part_wetness( bodypart_id( "arm_r" ) ) > 0 );

        WHEN( "they use a dry towel" ) {
            REQUIRE_FALSE( towel.has_flag( flag_WET ) );
            dummy.invoke_item( &towel );

            THEN( "it dries them off" ) {
                CHECK( dummy.get_part_wetness( bodypart_id( "torso" ) ) == 0 );
                CHECK( dummy.get_part_wetness( bodypart_id( "head" ) ) == 0 );
                CHECK( dummy.get_part_wetness( bodypart_id( "arm_l" ) ) == 0 );
                CHECK( dummy.get_part_wetness( bodypart_id( "arm_r" ) ) == 0 );

                AND_THEN( "the towel becomes wet" ) {
                    CHECK( towel.typeId().str() == "towel_wet" );
                }
            }
        }

        WHEN( "they use a wet towel" ) {
            towel.convert( itype_towel_wet );
            REQUIRE( towel.has_flag( flag_WET ) );
            dummy.invoke_item( &towel );

            THEN( "it does not dry them off" ) {
                CHECK( dummy.get_part_wetness( bodypart_id( "torso" ) ) > 0 );
                CHECK( dummy.get_part_wetness( bodypart_id( "head" ) ) > 0 );
                CHECK( dummy.get_part_wetness( bodypart_id( "arm_l" ) ) > 0 );
                CHECK( dummy.get_part_wetness( bodypart_id( "arm_r" ) ) > 0 );
            }
        }
    }

    GIVEN( "avatar has poor morale due to being wet" ) {
        dummy.drench( 100, { body_part_torso, body_part_head, body_part_arm_l, body_part_arm_r },
                      false );
        dummy.add_morale( morale_wet, -10, -10, 1_hours, 1_hours );
        REQUIRE( dummy.has_morale( morale_wet ) == -10 );

        WHEN( "they use a wet towel" ) {
            towel.convert( itype_towel_wet );
            REQUIRE( towel.has_flag( flag_WET ) );
            dummy.invoke_item( &towel );

            THEN( "it does not improve their morale" ) {
                CHECK( dummy.has_morale( morale_wet ) == -10 );
            }
        }

        WHEN( "they use a dry towel" ) {
            REQUIRE_FALSE( towel.has_flag( flag_WET ) );
            dummy.invoke_item( &towel );

            THEN( "it improves their morale" ) {
                CHECK( dummy.has_morale( morale_wet ) == 0 );

                AND_THEN( "the towel becomes wet" ) {
                    CHECK( towel.typeId() == itype_towel_wet );
                }
            }
        }
    }

    GIVEN( "avatar is slimed, boomered, glowing, and bile irritated." ) {
        dummy.add_effect( effect_slimed, 1_hours );
        dummy.add_effect( effect_boomered, 1_hours );
        dummy.add_effect( effect_glowing, 1_hours );
        dummy.add_effect( effect_bile_irritant, 1_hours );
        REQUIRE( dummy.has_effect( effect_slimed ) );
        REQUIRE( dummy.has_effect( effect_boomered ) );
        REQUIRE( dummy.has_effect( effect_glowing ) );
        REQUIRE( dummy.has_effect( effect_bile_irritant ) );

        WHEN( "they use a dry towel" ) {
            REQUIRE_FALSE( towel.has_flag( flag_WET ) );
            dummy.invoke_item( &towel );

            THEN( "it removes all those effects at once" ) {
                CHECK_FALSE( dummy.has_effect( effect_slimed ) );
                CHECK_FALSE( dummy.has_effect( effect_boomered ) );
                CHECK_FALSE( dummy.has_effect( effect_glowing ) );
                CHECK_FALSE( dummy.has_effect( effect_bile_irritant ) );

                AND_THEN( "the towel becomes filthy" ) {
                    CHECK( towel.is_filthy() );
                }
            }
        }
    }

    GIVEN( "avatar is boomered and wet" ) {
        dummy.add_effect( effect_boomered, 1_hours );
        dummy.add_morale( morale_wet, -10, -10, 1_hours, 1_hours );
        REQUIRE( std::abs( dummy.has_morale( morale_wet ) ) );

        WHEN( "they use a dry towel" ) {
            REQUIRE_FALSE( towel.has_flag( flag_WET ) );
            dummy.invoke_item( &towel );

            THEN( "it removes the boomered effect, but not the wetness" ) {
                CHECK_FALSE( dummy.has_effect( effect_boomered ) );
                CHECK( std::abs( dummy.has_morale( morale_wet ) ) );

                AND_THEN( "the towel becomes filthy" ) {
                    CHECK( towel.is_filthy() );
                }
            }
        }
    }
}

TEST_CASE( "thorazine", "[iuse][thorazine]" )
{
    avatar dummy;
    dummy.normalize();
    item_location thorazine = dummy.i_add( item( itype_thorazine, calendar::turn ) );

    REQUIRE( dummy.has_item_with( []( const item & it ) {
        return it.typeId() == itype_thorazine;
    } ) );
    dummy.set_sleepiness( 0 );

    GIVEN( "avatar has hallucination, visuals, and high effects" ) {
        dummy.add_effect( effect_hallu, 1_hours );
        dummy.add_effect( effect_visuals, 1_hours );
        dummy.add_effect( effect_high, 1_hours );
        REQUIRE( dummy.has_effect( effect_hallu ) );
        REQUIRE( dummy.has_effect( effect_visuals ) );
        REQUIRE( dummy.has_effect( effect_high ) );

        WHEN( "they take some thorazine" ) {
            dummy.consume( thorazine );

            THEN( "it relieves all those effects with a single dose" ) {
                CHECK_FALSE( dummy.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_thorazine;
                } ) );
                REQUIRE_FALSE( dummy.has_effect( effect_hallu ) );
                REQUIRE_FALSE( dummy.has_effect( effect_visuals ) );
                REQUIRE_FALSE( dummy.has_effect( effect_high ) );

                AND_THEN( "it causes some sleepiness" ) {
                    CHECK( dummy.get_sleepiness() >= 5 );
                }
            }
        }
    }

    GIVEN( "avatar has already taken some thorazine" ) {
        item thora( itype_thorazine, calendar::turn );
        dummy.consume( thora );

        REQUIRE( dummy.has_item_with( []( const item & it ) {
            return it.typeId() == itype_thorazine;
        } ) );

        REQUIRE( dummy.has_effect( effect_took_thorazine ) );

        WHEN( "they take more thorazine" ) {
            dummy.consume( thorazine );

            THEN( "it only causes more sleepiness" ) {
                CHECK_FALSE( dummy.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_thorazine;
                } ) );
                CHECK( dummy.get_sleepiness() >= 20 );
            }
        }
    }
}

TEST_CASE( "prozac", "[iuse][prozac]" )
{
    avatar dummy;
    dummy.normalize();

    item prozac( itype_prozac, calendar::turn_zero, item::default_charges_tag{} );

    SECTION( "prozac gives prozac and visible prozac effect" ) {
        REQUIRE_FALSE( dummy.has_effect( effect_took_prozac ) );
        REQUIRE_FALSE( dummy.has_effect( effect_took_prozac_visible ) );

        dummy.consume( prozac );
        CHECK( dummy.has_effect( effect_took_prozac ) );
        CHECK( dummy.has_effect( effect_took_prozac_visible ) );
    }

    SECTION( "taking prozac twice gives a stimulant effect" ) {
        dummy.set_stim( 0 );

        dummy.consume( prozac );
        CHECK( dummy.get_stim() == -5 ); // The iuse action gives +0 and item itself -5
        dummy.consume( prozac );
        CHECK( dummy.get_stim() == -7 ); // Second iuse gives +3
    }
}

TEST_CASE( "inhaler", "[iuse][inhaler]" )
{
    clear_avatar();
    avatar &dummy = get_avatar();
    item inhaler( itype_inhaler );
    inhaler.ammo_set( itype_albuterol );
    REQUIRE( inhaler.ammo_remaining( ) > 0 );

    item_location inhaler_loc = dummy.i_add( inhaler );
    REQUIRE( dummy.has_item( *inhaler_loc ) );

    GIVEN( "avatar is suffering from smoke inhalation" ) {
        dummy.add_effect( effect_smoke_lungs, 1_hours );
        REQUIRE( dummy.has_effect( effect_smoke_lungs ) );

        THEN( "inhaler relieves it" ) {
            dummy.use( inhaler_loc );
            CHECK_FALSE( dummy.has_effect( effect_smoke_lungs ) );
        }
    }

    GIVEN( "avatar is suffering from asthma" ) {
        dummy.add_effect( effect_asthma, 1_hours );
        REQUIRE( dummy.has_effect( effect_asthma ) );

        THEN( "inhaler relieves the effects of asthma" ) {
            dummy.use( inhaler_loc );
            CHECK_FALSE( dummy.has_effect( effect_asthma ) );
        }
    }

    GIVEN( "avatar is not suffering from asthma" ) {
        REQUIRE_FALSE( dummy.has_effect( effect_asthma ) );

        THEN( "inhaler reduces sleepiness" ) {
            dummy.set_sleepiness( 10 );
            dummy.use( inhaler_loc );
            CHECK( dummy.get_sleepiness() < 10 );
        }
    }
}

TEST_CASE( "panacea", "[iuse][panacea]" )
{
    avatar dummy;
    dummy.normalize();

    item panacea( itype_panacea, calendar::turn_zero, item::default_charges_tag{} );

    SECTION( "panacea gives cure-all effect" ) {
        REQUIRE_FALSE( dummy.has_effect( effect_cureall ) );

        dummy.consume( panacea );
        CHECK( dummy.has_effect( effect_cureall ) );
    }
}

TEST_CASE( "xanax", "[iuse][xanax]" )
{
    avatar dummy;
    dummy.normalize();

    item xanax( itype_xanax, calendar::turn_zero, item::default_charges_tag{} );

    SECTION( "xanax gives xanax and visible xanax effects" ) {
        REQUIRE_FALSE( dummy.has_effect( effect_took_xanax ) );
        REQUIRE_FALSE( dummy.has_effect( effect_took_xanax_visible ) );

        dummy.consume( xanax );
        CHECK( dummy.has_effect( effect_took_xanax ) );
        CHECK( dummy.has_effect( effect_took_xanax_visible ) );
    }
}

static item_location give_tablets( avatar &dummy, int count, bool in_inventory )
{
    const int existing_tablets = dummy.crafting_inventory().count_item( itype_pur_tablets );
    item_location tablets;
    if( in_inventory ) {
        for( int i = 0; i < count; ++i ) {
            tablets = dummy.i_add( item( itype_pur_tablets, calendar::turn ) );
        }
    } else {
        // Tablets fail to spawn directly on the ground, so put them in a backpack, then spawn that
        item container( itype_backpack );
        for( int i = 0; i < count; ++i ) {
            container.put_in( item( itype_pur_tablets, calendar::turn ), pocket_type::CONTAINER );
        }
        item_location container_loc = get_map().add_item_or_charges_ret_loc( dummy.pos_bub(), container,
                                      true );
        REQUIRE( container_loc );
        std::list<item *> all_tablets = container_loc->all_items_top();
        REQUIRE( !all_tablets.empty() );
        tablets = item_location( container_loc, all_tablets.front() );
    }
    REQUIRE( tablets );
    // Since we checked it earlier, the crafting inventory needs to be invalidated to force an update
    dummy.invalidate_crafting_inventory();
    REQUIRE( dummy.crafting_inventory().count_item( itype_pur_tablets ) == count + existing_tablets );
    return tablets;
}

static item_location give_water( avatar &dummy, int count, bool in_inventory )
{
    item container( itype_55gal_drum );
    item_location container_loc;
    item water( itype_water );
    water.charges = count;
    container.put_in( water, pocket_type::CONTAINER );
    if( in_inventory ) {
        container_loc = dummy.i_add( container );
    } else {
        container_loc = get_map().add_item_or_charges_ret_loc( dummy.pos_bub(), container, true );
    }
    REQUIRE( container_loc );
    // Spawning a container of water next to the player does not update the crafting inventory, so force an update
    dummy.invalidate_crafting_inventory();
    REQUIRE( dummy.crafting_inventory().charges_of( itype_water ) == count );
    return item_location( dummy, &container_loc->only_item() );
}

TEST_CASE( "water_purification_tablet_activation", "[iuse][pur_tablets]" )
{
    map &here = get_map();
    avatar dummy;
    dummy.normalize();
    build_test_map( ter_t_grass );
    const tripoint_bub_ms test_origin( 20, 20, 0 );
    dummy.setpos( here, test_origin );
    // Give the player a backpack to hold the tablets
    dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );

    SECTION( "1 tablet in inventory will purify 4 water in inventory" ) {
        item_location tablet = give_tablets( dummy, 1, true );
        item_location water_location = give_water( dummy, 4, true );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 0 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 4 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 0 );
    }

    SECTION( "1 tablet in inventory will purify 1 water in inventory" ) {
        item_location tablet = give_tablets( dummy, 1, true );
        item_location water_location = give_water( dummy, 1, true );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 0 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 1 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 0 );
    }

    SECTION( "1 tablet in inventory will not purify 5 water in inventory" ) {
        item_location tablet = give_tablets( dummy, 1, true );
        item_location water_location = give_water( dummy, 5, true );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 5 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 0 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 1 );
    }

    SECTION( "3 tablets in inventory will purify 12 water in inventory" ) {
        item_location tablet = give_tablets( dummy, 3, true );
        item_location water_location = give_water( dummy, 12, true );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 0 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 12 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 0 );
    }

    SECTION( "2 tablets in inventory will purify 4 water in inventory" ) {
        item_location tablet = give_tablets( dummy, 2, true );
        item_location water_location = give_water( dummy, 4, true );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 0 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 4 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 1 );
    }

    SECTION( "1 tablet nearby will purify 4 water nearby" ) {
        item_location tablet = give_tablets( dummy, 1, false );
        item_location water_location = give_water( dummy, 4, false );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 0 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 4 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 0 );
    }

    SECTION( "2 tablets nearby will purify 5 water nearby" ) {
        item_location tablet = give_tablets( dummy, 2, false );
        item_location water_location = give_water( dummy, 5, false );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 0 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 5 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 0 );
    }

    SECTION( "3 tablets nearby will not purify 13 water nearby" ) {
        item_location tablet = give_tablets( dummy, 3, false );
        item_location water_location = give_water( dummy, 13, false );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 13 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 0 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 3 );
    }

    SECTION( "1 tablet in inventory will purify 2 water nearby" ) {
        item_location tablet = give_tablets( dummy, 1, true );
        item_location water_location = give_water( dummy, 2, false );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 0 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 2 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 0 );
    }

    SECTION( "1 tablet nearby will purify 1 water in inventory" ) {
        item_location tablet = give_tablets( dummy, 1, false );
        item_location water_location = give_water( dummy, 1, true );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 0 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 1 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 0 );
    }

    SECTION( "1 tablet nearby and 1 tablet in inventory will purify 3 water in inventory" ) {
        item_location tablet = give_tablets( dummy, 1, false );
        tablet = give_tablets( dummy, 1, true );
        item_location water_location = give_water( dummy, 3, true );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 0 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 3 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 1 );
    }

    SECTION( "1 tablet nearby and 1 tablet in inventory will purify 8 water in inventory" ) {
        item_location tablet = give_tablets( dummy, 1, false );
        tablet = give_tablets( dummy, 1, true );
        item_location water_location = give_water( dummy, 8, true );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 0 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 8 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 0 );
    }

    SECTION( "1 tablet nearby and 1 tablet in inventory will purify 2 water nearby" ) {
        item_location tablet = give_tablets( dummy, 1, false );
        tablet = give_tablets( dummy, 1, true );
        item_location water_location = give_water( dummy, 8, false );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 0 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 8 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 0 );
    }

    SECTION( "1 tablet nearby and 1 tablet in inventory will not purify 9 water nearby" ) {
        item_location tablet = give_tablets( dummy, 1, false );
        tablet = give_tablets( dummy, 1, true );
        item_location water_location = give_water( dummy, 9, false );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 9 );
        CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 0 );
        CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 2 );
    }

    SECTION( "6 tablets will purify a toilet tank" ) {
        item_location tablet = give_tablets( dummy, 6, true );
        here.furn_set( dummy.pos_bub( here ) + tripoint::north, furn_f_toilet );
        item water( itype_water );
        water.charges = 24;
        here.add_item( dummy.pos_bub( here ) + tripoint::north, water );
        item_location water_location( map_cursor( dummy.pos_bub() + tripoint::north ), &water );

        REQUIRE( water_location );
        REQUIRE( water_location.get_item()->typeId() == itype_water );
        REQUIRE( water_location.get_item()->charges == 24 );
        REQUIRE( water_location.where_recursive() == item_location::type::map );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        // TODO: Figure out why the crafting inventory doesn't update to show water_purifying
        /*dummy.invalidate_crafting_inventory();
            CHECK( dummy.crafting_inventory().charges_of( itype_water ) == 0 );
            CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 24 );
            CHECK( dummy.crafting_inventory().count_item( itype_pur_tablets ) == 0 );*/
        // Until then, show that it works by checking the item directly:
        CHECK( water_location.get_item()->typeId() == itype_water_purifying );
        CHECK( water_location.get_item()->charges == 24 );
    }
}

TEST_CASE( "water_tablet_purification_test", "[iuse][pur_tablets]" )
{
    map &here = get_map();
    avatar dummy;
    dummy.normalize();
    build_test_map( ter_t_grass );
    const tripoint_bub_ms test_origin( 20, 20, 0 );
    dummy.setpos( here, test_origin );

    SECTION( "Test purifying time" ) {
        item_location tablet = give_tablets( dummy, 1, true );
        item_location water_location = give_water( dummy, 4, false );

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        REQUIRE( dummy.crafting_inventory().charges_of( itype_water_clean ) == 0 );
        REQUIRE( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 4 );

        SECTION( "Still purifying at ten minutes" ) {
            calendar::turn += 10_minutes;
            dummy.invoke_item( water_location.get_item() );

            CHECK( dummy.crafting_inventory().charges_of( itype_water_clean ) == 0 );
            CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 4 );
        }

        SECTION( "Still purifying at thirty minutes" ) {
            calendar::turn += 30_minutes;
            dummy.invoke_item( water_location.get_item() );

            CHECK( dummy.crafting_inventory().charges_of( itype_water_clean ) == 0 );
            CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 4 );
        }

        SECTION( "Clean water at forty minutes" ) {
            calendar::turn += 40_minutes;
            dummy.invoke_item( water_location.get_item() );

            CHECK( dummy.crafting_inventory().charges_of( itype_water_clean ) == 4 );
            CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 0 );
        }
    }

    SECTION( "Test purifying time with old water" ) {
        item_location tablet = give_tablets( dummy, 1, true );
        item_location water_location = give_water( dummy, 4, false );

        calendar::turn += 24_hours;

        iuse::purify_water( &dummy, tablet.get_item(), water_location );

        SECTION( "Still purifying ten minutes later" ) {
            calendar::turn += 10_minutes;
            dummy.invoke_item( water_location.get_item() );

            CHECK( dummy.crafting_inventory().charges_of( itype_water_clean ) == 0 );
            CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 4 );
        }

        SECTION( "Still purifying thirty minutes later" ) {
            calendar::turn += 30_minutes;
            dummy.invoke_item( water_location.get_item() );

            CHECK( dummy.crafting_inventory().charges_of( itype_water_clean ) == 0 );
            CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 4 );
        }

        SECTION( "Clean water forty minutes later" ) {
            calendar::turn += 40_minutes;
            dummy.invoke_item( water_location.get_item() );

            CHECK( dummy.crafting_inventory().charges_of( itype_water_clean ) == 4 );
            CHECK( dummy.crafting_inventory().charges_of( itype_water_purifying ) == 0 );
        }
    }

    SECTION( "Test purifying known bad water" ) {
        item_location water_location = give_water( dummy, 4, false );
        water_location.get_item()->poison = 100;

        SECTION( "The water will poison us before purification" ) {
            dummy.consume( water_location, true );
            REQUIRE( water_location.get_item()->charges == 3 );
            REQUIRE( water_location.get_item()->poison > 0 );

            CHECK( dummy.has_effect( effect_foodpoison ) );
        }

        SECTION( "The water is safe after purification" ) {
            item_location tablet = give_tablets( dummy, 1, true );
            iuse::purify_water( &dummy, tablet.get_item(), water_location );
            calendar::turn += 40_minutes;
            dummy.invoke_item( water_location.get_item() );
            REQUIRE( dummy.crafting_inventory().charges_of( itype_water_clean ) == 4 );
            dummy.consume( water_location, true );
            REQUIRE( water_location.get_item()->charges == 3 );

            CHECK_FALSE( dummy.has_effect( effect_foodpoison ) );
        }

    }
}
