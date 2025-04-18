#include <algorithm>
#include <functional>
#include <list>
#include <optional>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "cata_utility.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "player_helpers.h"
#include "point.h"
#include "rng.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "visitable.h"
#include "vpart_position.h"

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_bone( "bone" );
static const itype_id itype_bottle_plastic( "bottle_plastic" );
static const itype_id itype_flask_hip( "flask_hip" );
static const itype_id itype_null( "null" );
static const itype_id itype_water( "water" );

static const vproto_id vehicle_prototype_shopping_cart( "shopping_cart" );

template <typename T>
static int count_items( const T &src, const itype_id &id )
{
    int n = 0;
    src.visit_items( [&n, &id]( const item * e, item * ) {
        n += ( e->typeId() == id );
        return VisitResponse::NEXT;
    } );
    return n;
}

// NOLINTNEXTLINE(readability-function-size)
TEST_CASE( "visitable_remove", "[visitable]" )
{
    const itype_id liquid_id = itype_water;
    const itype_id container_id = itype_bottle_plastic;
    const itype_id worn_id = itype_flask_hip;
    const int count = 5;

    REQUIRE( item( container_id ).is_container() );
    REQUIRE( item( worn_id ).is_container() );

    clear_avatar();
    Character &p = get_player_character();
    p.worn.wear_item( p, item( itype_backpack ), false, false );
    p.wear_item( item( itype_backpack ) ); // so we don't drop anything
    clear_map();
    map &here = get_map();

    // check if all tiles within radius are loaded within current submap and passable
    const auto suitable = [&here]( const tripoint_bub_ms & pos, const int radius ) {
        std::vector<tripoint_bub_ms> tiles = closest_points_first( pos, radius );
        return std::all_of( tiles.begin(), tiles.end(), [&here]( const tripoint_bub_ms & e ) {
            if( !here.inbounds( e ) ) {
                return false;
            }
            if( const optional_vpart_position vp = here.veh_at( e ) ) {
                here.destroy_vehicle( &vp->vehicle() );
            }
            here.i_clear( e );
            return here.passable( e );
        } );
    };

    // move player randomly until we find a suitable position
    constexpr int num_trials = 100;
    for( int i = 0; i < num_trials && !suitable( p.pos_bub( here ), 1 ); ++i ) {
        CHECK( !p.in_vehicle );
        p.setpos( here, random_entry( closest_points_first( p.pos_bub( here ), 1 ) ) );
    }
    REQUIRE( suitable( p.pos_bub( here ), 1 ) );

    item temp_liquid( liquid_id );
    item obj = temp_liquid.in_container( temp_liquid.type->default_container.value_or( itype_null ) );
    REQUIRE( obj.num_item_stacks() == 1 );
    const auto has_liquid_filter = [&liquid_id]( const item & it ) {
        return it.typeId() == liquid_id;
    };
    REQUIRE( obj.has_item_with( has_liquid_filter ) );

    GIVEN( "A player with several bottles of water" ) {
        for( int i = 0; i != count; ++i ) {
            p.i_add( obj );
        }
        REQUIRE( count_items( p, container_id ) == count );
        REQUIRE( count_items( p, liquid_id ) == count );

        WHEN( "all the bottles are removed" ) {
            std::list<item> del = p.remove_items_with( [&container_id]( const item & e ) {
                return e.typeId() == container_id;
            } );

            THEN( "no bottles remain in the players possession" ) {
                REQUIRE( count_items( p, container_id ) == 0 );
            }
            THEN( "no water remain in the players possession" ) {
                REQUIRE( count_items( p, liquid_id ) == 0 );
            }
            THEN( "the correct number of items were removed" ) {
                REQUIRE( del.size() == count );

                AND_THEN( "the removed items were all bottles" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item & e ) {
                        return e.typeId() == container_id;
                    } ) );
                }
                AND_THEN( "the removed items all contain water" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&has_liquid_filter]( const item & e ) {
                        return e.num_item_stacks() == 1 && e.has_item_with( has_liquid_filter );
                    } ) );
                }
            }
        }

        WHEN( "one of the bottles is removed" ) {
            std::list<item> del = p.remove_items_with( [&container_id]( const item & e ) {
                return e.typeId() == container_id;
            }, 1 );

            THEN( "there is one less bottle in the players possession" ) {
                REQUIRE( count_items( p, container_id ) == count - 1 );
            }
            THEN( "there is one less water in the players possession" ) {
                REQUIRE( count_items( p, liquid_id ) == count - 1 );
            }
            THEN( "the correct number of items were removed" ) {
                REQUIRE( del.size() == 1 );

                AND_THEN( "the removed items were all bottles" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item & e ) {
                        return e.typeId() == container_id;
                    } ) );
                }
                AND_THEN( "the removed items all contained water" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&has_liquid_filter]( const item & e ) {
                        return e.num_item_stacks() == 1 && e.has_item_with( has_liquid_filter );
                    } ) );
                }
            }
        }

        WHEN( "one of the bottles is wielded" ) {
            p.wield( p.worn.front().legacy_front() );
            REQUIRE( p.get_wielded_item()->typeId() == container_id );
            REQUIRE( count_items( p, container_id ) == count );
            REQUIRE( count_items( p, liquid_id ) == count );

            AND_WHEN( "all the bottles are removed" ) {
                std::list<item> del = p.remove_items_with( [&container_id]( const item & e ) {
                    return e.typeId() == container_id;
                } );

                THEN( "no bottles remain in the players possession" ) {
                    REQUIRE( count_items( p, container_id ) == 0 );
                }
                THEN( "no water remain in the players possession" ) {
                    REQUIRE( count_items( p, liquid_id ) == 0 );
                }
                THEN( "there is no currently wielded item" ) {
                    REQUIRE( !p.get_wielded_item() );
                }
                THEN( "the correct number of items were removed" ) {
                    REQUIRE( del.size() == count );

                    AND_THEN( "the removed items were all bottles" ) {
                        CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item & e ) {
                            return e.typeId() == container_id;
                        } ) );
                    }
                    AND_THEN( "the removed items all contain water" ) {
                        CHECK( std::all_of( del.begin(), del.end(), [&has_liquid_filter]( const item & e ) {
                            return e.num_item_stacks() == 1 && e.has_item_with( has_liquid_filter );
                        } ) );
                    }
                }
            }

            AND_WHEN( "all but one of the bottles is removed" ) {
                std::list<item> del = p.remove_items_with( [&container_id]( const item & e ) {
                    return e.typeId() == container_id;
                }, count - 1 );

                THEN( "there is only one bottle remaining in the players possession" ) {
                    REQUIRE( count_items( p, container_id ) == 1 );
                    AND_THEN( "the remaining bottle is currently wielded" ) {
                        REQUIRE( p.get_wielded_item()->typeId() == container_id );

                        AND_THEN( "the remaining water is contained by the currently wielded bottle" ) {
                            REQUIRE( p.get_wielded_item()->num_item_stacks() == 1 );
                            REQUIRE( p.get_wielded_item()->has_item_with( has_liquid_filter ) );
                        }
                    }
                }
                THEN( "there is only one water remaining in the players possession" ) {
                    REQUIRE( count_items( p, liquid_id ) == 1 );
                }

                THEN( "the correct number of items were removed" ) {
                    REQUIRE( del.size() == count - 1 );

                    AND_THEN( "the removed items were all bottles" ) {
                        CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item & e ) {
                            return e.typeId() == container_id;
                        } ) );
                    }
                    AND_THEN( "the removed items all contained water" ) {
                        CHECK( std::all_of( del.begin(), del.end(), [&has_liquid_filter]( const item & e ) {
                            return e.num_item_stacks() == 1 && e.has_item_with( has_liquid_filter );
                        } ) );
                    }
                }
            }
        }

        WHEN( "a hip flask containing water is wielded" ) {
            item obj( worn_id );
            item liquid( liquid_id, calendar::turn );
            liquid.charges -= obj.fill_with( liquid, liquid.charges );
            p.wield( obj );

            REQUIRE( count_items( p, container_id ) == count );
            REQUIRE( count_items( p, liquid_id ) == count + 1 );

            AND_WHEN( "all but one of the water is removed" ) {
                std::list<item> del = p.remove_items_with( [&liquid_id]( const item & e ) {
                    return e.typeId() == liquid_id;
                }, count );

                THEN( "all of the bottles remain in the players possession" ) {
                    REQUIRE( count_items( p, container_id ) == 5 );
                    AND_THEN( "all of the bottles are now empty" ) {
                        REQUIRE( p.visit_items( [&container_id]( const item * e, item * ) {
                            return ( e->typeId() != container_id || e->empty() ) ?
                                   VisitResponse::NEXT : VisitResponse::ABORT;
                        } ) != VisitResponse::ABORT );
                    }
                }
                THEN( "the hip flask remains in the players possession" ) {
                    auto found = p.items_with( [&worn_id]( const item & e ) {
                        return e.typeId() == worn_id;
                    } );
                    REQUIRE( found.size() == 1 );
                    AND_THEN( "the hip flask is still worn" ) {
                        REQUIRE( p.is_wielding( *found[0] ) );

                        AND_THEN( "the hip flask contains water" ) {
                            REQUIRE( found[0]->num_item_stacks() == 1 );
                            REQUIRE( found[0]->has_item_with( has_liquid_filter ) );
                        }
                    }
                }
                THEN( "there is only one water remaining in the players possession" ) {
                    REQUIRE( count_items( p, liquid_id ) == 1 );
                }
                THEN( "the correct number of items were removed" ) {
                    REQUIRE( del.size() == count );

                    AND_THEN( "the removed items were all water" ) {
                        CHECK( std::all_of( del.begin(), del.end(), [&liquid_id]( const item & e ) {
                            return e.typeId() == liquid_id;
                        } ) );
                    }
                }

                AND_WHEN( "the final water is removed" ) {
                    std::list<item> del = p.remove_items_with( [&liquid_id]( const item & e ) {
                        return e.typeId() == liquid_id;
                    }, 1 );

                    THEN( "no water remain in the players possession" ) {
                        REQUIRE( count_items( p, liquid_id ) == 0 );
                    }

                    THEN( "the hip flask remains in the players possession" ) {
                        auto found = p.items_with( [&worn_id]( const item & e ) {
                            return e.typeId() == worn_id;
                        } );
                        REQUIRE( found.size() == 1 );
                        AND_THEN( "the hip flask is worn" ) {
                            REQUIRE( p.is_wielding( *found[0] ) );

                            AND_THEN( "the hip flask is empty" ) {
                                REQUIRE( found[0]->empty() );
                            }
                        }
                    }
                }
            }
        }
    }

    GIVEN( "A player surrounded by several bottles of water" ) {
        std::vector<tripoint_bub_ms> tiles = closest_points_first( p.pos_bub(), 1 );
        tiles.erase( tiles.begin() ); // player tile

        int our = 0; // bottles placed on player tile
        int adj = 0; // bottles placed on adjacent tiles

        for( int i = 0; i != count; ++i ) {
            if( i == 0 || tiles.empty() ) {
                // always place at least one bottle on player tile
                our++;
                here.add_item( p.pos_bub(), obj );
            } else {
                // randomly place bottles on adjacent tiles
                adj++;
                here.add_item( random_entry( tiles ), obj );
            }
        }
        REQUIRE( our + adj == count );

        map_selector sel( p.pos_bub(), 1 );
        map_cursor cur( p.pos_abs() );

        REQUIRE( count_items( sel, container_id ) == count );
        REQUIRE( count_items( sel, liquid_id ) == count );

        REQUIRE( count_items( cur, container_id ) == our );
        REQUIRE( count_items( cur, liquid_id ) == our );

        WHEN( "all the bottles are removed" ) {
            std::list<item> del = sel.remove_items_with( [&container_id]( const item & e ) {
                return e.typeId() == container_id;
            } );

            THEN( "no bottles remain on the map" ) {
                REQUIRE( count_items( sel, container_id ) == 0 );
            }
            THEN( "no water remains on the map" ) {
                REQUIRE( count_items( sel, liquid_id ) == 0 );
            }
            THEN( "the correct number of items were removed" ) {
                REQUIRE( del.size() == count );

                AND_THEN( "the removed items were all bottles" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item & e ) {
                        return e.typeId() == container_id;
                    } ) );
                }
                AND_THEN( "the removed items all contain water" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&has_liquid_filter]( const item & e ) {
                        return e.num_item_stacks() == 1 && e.has_item_with( has_liquid_filter );
                    } ) );
                }
            }
        }

        WHEN( "one of the bottles is removed" ) {
            std::list<item> del = sel.remove_items_with( [&container_id]( const item & e ) {
                return e.typeId() == container_id;
            }, 1 );

            THEN( "there is one less bottle on the map" ) {
                REQUIRE( count_items( sel, container_id ) == count - 1 );
            }
            THEN( "there is one less water on the map" ) {
                REQUIRE( count_items( sel, liquid_id ) == count - 1 );
            }
            THEN( "the correct number of items were removed" ) {
                REQUIRE( del.size() == 1 );

                AND_THEN( "the removed items were all bottles" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item & e ) {
                        return e.typeId() == container_id;
                    } ) );
                }
                AND_THEN( "the removed items all contained water" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&has_liquid_filter]( const item & e ) {
                        return e.num_item_stacks() == 1 && e.has_item_with( has_liquid_filter );
                    } ) );
                }
            }
        }

        WHEN( "all of the bottles on the player tile are removed" ) {
            std::list<item> del = cur.remove_items_with( [&container_id]( const item & e ) {
                return e.typeId() == container_id;
            }, our );

            THEN( "no bottles remain on the player tile" ) {
                REQUIRE( count_items( cur, container_id ) == 0 );
            }
            THEN( "no water remains on the player tile" ) {
                REQUIRE( count_items( cur, liquid_id ) == 0 );
            }
            THEN( "the correct amount of bottles remains on the map" ) {
                REQUIRE( count_items( sel, container_id ) == count - our );
            }
            THEN( "there correct amount of water remains on the map" ) {
                REQUIRE( count_items( sel, liquid_id ) == count - our );
            }
            THEN( "the correct number of items were removed" ) {
                REQUIRE( static_cast<int>( del.size() ) == our );

                AND_THEN( "the removed items were all bottles" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item & e ) {
                        return e.typeId() == container_id;
                    } ) );
                }
                AND_THEN( "the removed items all contained water" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [has_liquid_filter]( const item & e ) {
                        return e.num_item_stacks() == 1 && e.has_item_with( has_liquid_filter );
                    } ) );
                }
            }
        }
    }

    GIVEN( "An adjacent vehicle contains several bottles of water" ) {
        std::vector<tripoint_bub_ms> tiles = closest_points_first( p.pos_bub(), 1 );
        tiles.erase( tiles.begin() ); // player tile
        tripoint_bub_ms veh = tripoint_bub_ms( random_entry( tiles ) );
        REQUIRE( here.add_vehicle( vehicle_prototype_shopping_cart, veh, 0_degrees, 0, 0 ) );

        REQUIRE( std::count_if( tiles.begin(), tiles.end(), [&here]( const tripoint_bub_ms & e ) {
            return static_cast<bool>( here.veh_at( e ) );
        } ) == 1 );

        const std::optional<vpart_reference> vp = here.veh_at( veh ).cargo();
        REQUIRE( vp );
        vehicle *const v = &vp->vehicle();
        const int part = vp->part_index();
        REQUIRE( part >= 0 );
        // Empty the vehicle of any cargo.
        v->get_items( vp->part() ).clear();
        for( int i = 0; i != count; ++i ) {
            v->add_item( here,  vp->part(), obj );
        }

        vehicle_selector sel( here,  p.pos_bub( here ), 1 );

        REQUIRE( count_items( sel, container_id ) == count );
        REQUIRE( count_items( sel, liquid_id ) == count );

        WHEN( "all the bottles are removed" ) {
            std::list<item> del = sel.remove_items_with( [&container_id]( const item & e ) {
                return e.typeId() == container_id;
            } );

            THEN( "no bottles remain within the vehicle" ) {
                REQUIRE( count_items( sel, container_id ) == 0 );
            }
            THEN( "no water remains within the vehicle" ) {
                REQUIRE( count_items( sel, liquid_id ) == 0 );
            }
            THEN( "the correct number of items were removed" ) {
                REQUIRE( del.size() == count );

                AND_THEN( "the removed items were all bottles" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item & e ) {
                        return e.typeId() == container_id;
                    } ) );
                }
                AND_THEN( "the removed items all contain water" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&has_liquid_filter]( const item & e ) {
                        return e.num_item_stacks() == 1 && e.has_item_with( has_liquid_filter );
                    } ) );
                }
            }
        }

        WHEN( "one of the bottles is removed" ) {
            std::list<item> del = sel.remove_items_with( [&container_id]( const item & e ) {
                return e.typeId() == container_id;
            }, 1 );

            THEN( "there is one less bottle within the vehicle" ) {
                REQUIRE( count_items( sel, container_id ) == count - 1 );
            }
            THEN( "there is one less water within the vehicle" ) {
                REQUIRE( count_items( sel, liquid_id ) == count - 1 );
            }
            THEN( "the correct number of items were removed" ) {
                REQUIRE( del.size() == 1 );

                AND_THEN( "the removed items were all bottles" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item & e ) {
                        return e.typeId() == container_id;
                    } ) );
                }
                AND_THEN( "the removed items all contained water" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&has_liquid_filter]( const item & e ) {
                        return e.num_item_stacks() == 1 && e.has_item_with( has_liquid_filter );
                    } ) );
                }
            }
        }
    }
}

TEST_CASE( "inventory_remove_invalidates_binning_cache", "[visitable][inventory]" )
{
    inventory inv;
    std::list<item> items = { item( itype_bone ) };
    inv += items;
    CHECK( inv.amount_of( itype_bone ) == 1 );
    inv.remove_items_with( return_true<item> );
    CHECK( inv.size() == 0 );
    // The following used to be a heap use-after-free due to a caching bug.
    // Now should be safe.
    CHECK( inv.amount_of( itype_bone ) == 0 );
}
