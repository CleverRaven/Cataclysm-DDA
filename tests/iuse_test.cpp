#include <cstdlib>
#include <iosfwd>
#include <memory>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "flag.h"
#include "item.h"
#include "itype.h"
#include "morale_types.h"
#include "player_helpers.h"
#include "type_id.h"
#include "value_ptr.h"

static const efftype_id effect_antifungal( "antifungal" );
static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_bloodworms( "bloodworms" );
static const efftype_id effect_boomered( "boomered" );
static const efftype_id effect_brainworms( "brainworms" );
static const efftype_id effect_cureall( "cureall" );
static const efftype_id effect_dermatik( "dermatik" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_glowing( "glowing" );
static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_high( "high" );
static const efftype_id effect_paincysts( "paincysts" );
static const efftype_id effect_shakes( "shakes" );
static const efftype_id effect_slimed( "slimed" );
static const efftype_id effect_smoke( "smoke" );
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

static const itype_id itype_albuterol( "albuterol" );
static const itype_id itype_towel_wet( "towel_wet" );

TEST_CASE( "eyedrops", "[iuse][eyedrops]" )
{
    avatar dummy;
    item eyedrops( "saline", calendar::turn_zero, item::default_charges_tag{} );

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
    item antifungal( "antifungal", calendar::turn_zero, item::default_charges_tag{} );

    int charges_before = antifungal.charges;
    REQUIRE( charges_before > 0 );

    GIVEN( "avatar has a fungal infection" ) {
        dummy.add_effect( effect_fungus, 1_hours );
        REQUIRE( dummy.has_effect( effect_fungus ) );

        WHEN( "they take an antifungal drug" ) {
            dummy.consume( antifungal );

            THEN( "one dose is depleted" ) {
                CHECK( antifungal.charges == charges_before - 1 );

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

            THEN( "one dose is depleted" ) {
                CHECK( antifungal.charges == charges_before - 1 );

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
    item antiparasitic( "antiparasitic", calendar::turn_zero, item::default_charges_tag{} );

    int charges_before = antiparasitic.charges;
    REQUIRE( charges_before > 0 );

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

            THEN( "one dose is depleted" ) {
                CHECK( antiparasitic.charges == charges_before - 1 );

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
        dummy.add_effect( effect_fungus, 1_hours );
        REQUIRE( dummy.has_effect( effect_fungus ) );

        WHEN( "they use an antiparasitic drug" ) {
            dummy.consume( antiparasitic );

            THEN( "one dose is depleted" ) {
                CHECK( antiparasitic.charges == charges_before - 1 );

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
    item anticonvulsant( "diazepam", calendar::turn_zero, item::default_charges_tag{} );

    int charges_before = anticonvulsant.charges;
    REQUIRE( charges_before > 0 );

    GIVEN( "avatar has the shakes" ) {
        dummy.add_effect( effect_shakes, 1_hours );
        REQUIRE( dummy.has_effect( effect_shakes ) );

        WHEN( "they use an anticonvulsant drug" ) {
            dummy.consume( anticonvulsant );

            THEN( "one dose is depleted" ) {
                CHECK( anticonvulsant.charges == charges_before - 1 );

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
TEST_CASE( "oxygen tank", "[iuse][oxygen_bottle]" )
{
    avatar dummy;
    item oxygen( "oxygen_tank" );
    itype_id o2_ammo( "oxygen" );
    oxygen.ammo_set( o2_ammo );

    int charges_before = oxygen.ammo_remaining();
    REQUIRE( charges_before > 0 );

    // Ensure baseline painkiller value to measure painkiller effects
    dummy.set_painkiller( 0 );
    REQUIRE( dummy.get_painkiller() == 0 );

    GIVEN( "avatar is suffering from smoke inhalation" ) {
        dummy.add_effect( effect_smoke, 1_hours );
        REQUIRE( dummy.has_effect( effect_smoke ) );

        THEN( "a dose of oxygen relieves the smoke inhalation" ) {
            dummy.invoke_item( &oxygen );
            CHECK( oxygen.ammo_remaining() == charges_before - 1 );
            CHECK_FALSE( dummy.has_effect( effect_smoke ) );

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
            CHECK( oxygen.ammo_remaining() == charges_before - 1 );
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
            CHECK( oxygen.ammo_remaining() == charges_before - 1 );
            CHECK_FALSE( dummy.has_effect( effect_asthma ) );

            AND_THEN( "it acts as a mild painkiller" ) {
                CHECK( dummy.get_painkiller() == 2 );
            }
        }
    }

    GIVEN( "avatar has no ill effects for the oxygen to treat" ) {
        REQUIRE_FALSE( dummy.has_effect( effect_smoke ) );
        REQUIRE_FALSE( dummy.has_effect( effect_teargas ) );
        REQUIRE_FALSE( dummy.has_effect( effect_asthma ) );

        WHEN( "they are not already stimulated" ) {
            dummy.set_stim( 0 );
            REQUIRE( dummy.get_stim() == 0 );

            THEN( "a dose of oxygen is stimulating" ) {
                dummy.invoke_item( &oxygen );
                CHECK( oxygen.ammo_remaining() == charges_before - 1 );
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
                CHECK( oxygen.ammo_remaining() == charges_before - 1 );
                CHECK( dummy.get_stim() == max_stim );

                AND_THEN( "it acts as a mild painkiller" ) {
                    CHECK( dummy.get_painkiller() == 2 );
                }
            }
        }
    }
}

// test the `iuse::caff` and `iuse::atomic_caff` functions
TEST_CASE( "caffeine and atomic caffeine", "[iuse][caff][atomic_caff]" )
{
    avatar dummy;

    // Baseline fatigue level before caffeinating
    int fatigue_before = 200;
    dummy.set_fatigue( fatigue_before );

    // No stimulants or radiation
    dummy.set_stim( 0 );
    dummy.set_rad( 0 );
    REQUIRE( dummy.get_stim() == 0 );
    REQUIRE( dummy.get_rad() == 0 );

    SECTION( "coffee reduces fatigue, but does not give stimulant effect" ) {
        item coffee( "coffee", calendar::turn_zero, item::default_charges_tag{} );
        dummy.consume( coffee );
        CHECK( dummy.get_fatigue() == fatigue_before - coffee.get_comestible()->fatigue_mod );
        CHECK( dummy.get_stim() == coffee.get_comestible()->stim );
    }

    SECTION( "atomic caffeine greatly reduces fatigue, and increases stimulant effect" ) {
        item atomic_coffee( "atomic_coffee", calendar::turn_zero, item::default_charges_tag{} );
        dummy.consume( atomic_coffee );
        CHECK( dummy.get_fatigue() == fatigue_before - atomic_coffee.get_comestible()->fatigue_mod );
        CHECK( dummy.get_stim() == atomic_coffee.get_comestible()->stim );
    }
}

TEST_CASE( "towel", "[iuse][towel]" )
{
    avatar dummy;
    dummy.set_body();
    item towel( "towel", calendar::turn_zero, item::default_charges_tag{} );

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
        dummy.add_morale( MORALE_WET, -10, -10, 1_hours, 1_hours );
        REQUIRE( dummy.has_morale( MORALE_WET ) == -10 );

        WHEN( "they use a wet towel" ) {
            towel.convert( itype_towel_wet );
            REQUIRE( towel.has_flag( flag_WET ) );
            dummy.invoke_item( &towel );

            THEN( "it does not improve their morale" ) {
                CHECK( dummy.has_morale( MORALE_WET ) == -10 );
            }
        }

        WHEN( "they use a dry towel" ) {
            REQUIRE_FALSE( towel.has_flag( flag_WET ) );
            dummy.invoke_item( &towel );

            THEN( "it improves their morale" ) {
                CHECK( dummy.has_morale( MORALE_WET ) == 0 );

                AND_THEN( "the towel becomes wet" ) {
                    CHECK( towel.typeId() == itype_towel_wet );
                }
            }
        }
    }

    GIVEN( "avatar is slimed, boomered, and glowing" ) {
        dummy.add_effect( effect_slimed, 1_hours );
        dummy.add_effect( effect_boomered, 1_hours );
        dummy.add_effect( effect_glowing, 1_hours );
        REQUIRE( dummy.has_effect( effect_slimed ) );
        REQUIRE( dummy.has_effect( effect_boomered ) );
        REQUIRE( dummy.has_effect( effect_glowing ) );

        WHEN( "they use a dry towel" ) {
            REQUIRE_FALSE( towel.has_flag( flag_WET ) );
            dummy.invoke_item( &towel );

            THEN( "it removes all those effects at once" ) {
                CHECK_FALSE( dummy.has_effect( effect_slimed ) );
                CHECK_FALSE( dummy.has_effect( effect_boomered ) );
                CHECK_FALSE( dummy.has_effect( effect_glowing ) );

                AND_THEN( "the towel becomes filthy" ) {
                    CHECK( towel.is_filthy() );
                }
            }
        }
    }

    GIVEN( "avatar is boomered and wet" ) {
        dummy.add_effect( effect_boomered, 1_hours );
        dummy.add_morale( MORALE_WET, -10, -10, 1_hours, 1_hours );
        REQUIRE( std::abs( dummy.has_morale( MORALE_WET ) ) );

        WHEN( "they use a dry towel" ) {
            REQUIRE_FALSE( towel.has_flag( flag_WET ) );
            dummy.invoke_item( &towel );

            THEN( "it removes the boomered effect, but not the wetness" ) {
                CHECK_FALSE( dummy.has_effect( effect_boomered ) );
                CHECK( std::abs( dummy.has_morale( MORALE_WET ) ) );

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
    dummy.set_fatigue( 0 );
    item thorazine( "thorazine", calendar::turn_zero, item::default_charges_tag{} );

    int charges_before = thorazine.charges;
    REQUIRE( charges_before >= 2 );

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
                CHECK( thorazine.charges == charges_before - 1 );
                REQUIRE_FALSE( dummy.has_effect( effect_hallu ) );
                REQUIRE_FALSE( dummy.has_effect( effect_visuals ) );
                REQUIRE_FALSE( dummy.has_effect( effect_high ) );

                AND_THEN( "it causes some fatigue" ) {
                    CHECK( dummy.get_fatigue() >= 5 );
                }
            }
        }
    }

    GIVEN( "avatar has already taken some thorazine" ) {
        dummy.consume( thorazine );
        REQUIRE( thorazine.charges == charges_before - 1 );
        REQUIRE( dummy.has_effect( effect_took_thorazine ) );

        WHEN( "they take more thorazine" ) {
            dummy.consume( thorazine );

            THEN( "it only causes more fatigue" ) {
                CHECK( thorazine.charges == charges_before - 2 );
                CHECK( dummy.get_fatigue() >= 20 );
            }
        }
    }
}

TEST_CASE( "prozac", "[iuse][prozac]" )
{
    avatar dummy;
    item prozac( "prozac", calendar::turn_zero, item::default_charges_tag{} );

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
    item inhaler( "inhaler" );
    inhaler.ammo_set( itype_albuterol );
    REQUIRE( inhaler.ammo_remaining() > 0 );

    item_location inhaler_loc = dummy.i_add( inhaler );
    REQUIRE( dummy.has_item( *inhaler_loc ) );

    GIVEN( "avatar is suffering from smoke inhalation" ) {
        dummy.add_effect( effect_smoke, 1_hours );
        REQUIRE( dummy.has_effect( effect_smoke ) );

        THEN( "inhaler relieves it" ) {
            dummy.use( inhaler_loc );
            CHECK_FALSE( dummy.has_effect( effect_smoke ) );
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

        THEN( "inhaler reduces fatigue" ) {
            dummy.set_fatigue( 10 );
            dummy.use( inhaler_loc );
            CHECK( dummy.get_fatigue() < 10 );
        }
    }
}

TEST_CASE( "panacea", "[iuse][panacea]" )
{
    avatar dummy;
    item panacea( "panacea", calendar::turn_zero, item::default_charges_tag{} );

    SECTION( "panacea gives cure-all effect" ) {
        REQUIRE_FALSE( dummy.has_effect( effect_cureall ) );

        dummy.consume( panacea );
        CHECK( dummy.has_effect( effect_cureall ) );
    }
}

TEST_CASE( "xanax", "[iuse][xanax]" )
{
    avatar dummy;
    item xanax( "xanax", calendar::turn_zero, item::default_charges_tag{} );

    SECTION( "xanax gives xanax and visible xanax effects" ) {
        REQUIRE_FALSE( dummy.has_effect( effect_took_xanax ) );
        REQUIRE_FALSE( dummy.has_effect( effect_took_xanax_visible ) );

        dummy.consume( xanax );
        CHECK( dummy.has_effect( effect_took_xanax ) );
        CHECK( dummy.has_effect( effect_took_xanax_visible ) );
    }
}

