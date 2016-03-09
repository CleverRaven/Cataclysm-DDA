#include "catch/catch.hpp"

#include "game.h"
#include "player.h"
#include "visitable.h"
#include "itype.h"

TEST_CASE( "visitable_remove", "[visitable]" ) {
    const std::string liquid_id = "water";
    const std::string container_id = "bottle_plastic";
    const std::string worn_id = "flask_hip";
    const int count = 5;

    REQUIRE( item( container_id ).is_container() );
    REQUIRE( item( worn_id ).is_container() );

    player &p = g->u;
    p.worn.clear();
    p.inv.clear();
    p.remove_weapon();
    p.wear_item( item( "backpack" ) ); // so we don't drop anything

    item obj = item( liquid_id ).in_its_container();
    REQUIRE( obj.contents.size() == 1 );
    REQUIRE( obj.contents[ 0 ].typeId() == liquid_id );
    for( int i = 0; i != count; ++i ) {
        p.i_add( obj );
    }

    auto count_items = [&p]( const itype_id& id ) {
        int n;
        p.visit_items( [&n,&id]( const item *e ) {
            n += ( e->typeId() == id );
            return VisitResponse::NEXT;
        } );
        return n;
    };

    GIVEN( "A player with several bottles of water" ) {
        REQUIRE( count_items( container_id ) == count );
        REQUIRE( count_items( liquid_id ) == count );

        WHEN( "all the bottles are removed" ) {
            std::list<item> del = p.remove_items_with( [&container_id]( const item& e ) {
                return e.typeId() == container_id;
            } );

            THEN( "no bottles remain in the players possession" ) {
                REQUIRE( count_items( container_id ) == 0 );
            }
            THEN( "no water remain in the players possession" ) {
                REQUIRE( count_items( liquid_id ) == 0 );
            }
            THEN( "the correct number of items were removed" ) {
                REQUIRE( del.size() == count );

                AND_THEN( "the removed items were all bottles" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item& e ) {
                        return e.typeId() == container_id;
                    } ) );
                }
                AND_THEN( "the removed items all contain water" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&liquid_id]( const item& e ) {
                        return e.contents.size() == 1 && e.contents[ 0 ].typeId() == liquid_id;
                    } ) );
                }
            }
        }

        WHEN( "one of the bottles is removed" ) {
            std::list<item> del = p.remove_items_with( [&container_id]( const item& e ) {
                return e.typeId() == container_id;
            }, 1 );

            THEN( "there is one less bottle in the players possession" ) {
                REQUIRE( count_items( container_id ) == count - 1 );
            }
            THEN( "there is one less water in the players possession" ) {
                REQUIRE( count_items( liquid_id ) == count - 1 );
            }
            THEN( "the correct number of items were removed" ) {
                REQUIRE( del.size() == 1 );

                AND_THEN( "the removed items were all bottles" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item& e ) {
                        return e.typeId() == container_id;
                    } ) );
                }
                AND_THEN( "the removed items all contained water" ) {
                    CHECK( std::all_of( del.begin(), del.end(), [&liquid_id]( const item& e ) {
                        return e.contents.size() == 1 && e.contents[ 0 ].typeId() == liquid_id;
                    } ) );
                }
            }
        }

        WHEN( "one of the bottles is wielded" ) {
            p.wield( p.i_at( 0 ) );
            REQUIRE( p.weapon.typeId() == container_id );
            REQUIRE( count_items( container_id ) == count );
            REQUIRE( count_items( liquid_id ) == count );

            AND_WHEN( "all the bottles are removed" ) {
                std::list<item> del = p.remove_items_with( [&container_id]( const item& e ) {
                    return e.typeId() == container_id;
                } );

                THEN( "no bottles remain in the players possession" ) {
                    REQUIRE( count_items( container_id ) == 0 );
                }
                THEN( "no water remain in the players possession" ) {
                    REQUIRE( count_items( liquid_id ) == 0 );
                }
                THEN( "there is no currently wielded item" ) {
                    REQUIRE( p.weapon.is_null() );
                }
                THEN( "the correct number of items were removed" ) {
                    REQUIRE( del.size() == count );

                    AND_THEN( "the removed items were all bottles" ) {
                        CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item& e ) {
                            return e.typeId() == container_id;
                        } ) );
                    }
                    AND_THEN( "the removed items all contain water" ) {
                        CHECK( std::all_of( del.begin(), del.end(), [&liquid_id]( const item& e ) {
                            return e.contents.size() == 1 && e.contents[ 0 ].typeId() == liquid_id;
                        } ) );
                    }
                }
            }

            AND_WHEN( "all but one of the bottles is removed" ) {
                std::list<item> del = p.remove_items_with( [&container_id]( const item& e ) {
                    return e.typeId() == container_id;
                }, count - 1 );

                THEN( "there is only one bottle remaining in the players possession" ) {
                    REQUIRE( count_items( container_id ) == 1 );
                    AND_THEN( "the remaining bottle is currently wielded" ) {
                        REQUIRE( p.weapon.typeId() == container_id );

                        AND_THEN( "the remaining water is contained by the currently wielded bottle" ) {
                            REQUIRE( p.weapon.contents.size() == 1 );
                            REQUIRE( p.weapon.contents[ 0 ].typeId() == liquid_id );
                        }
                    }
                }
                THEN( "there is only one water remaining in the players possession" ) {
                    REQUIRE( count_items( liquid_id ) == 1 );
                }

                THEN( "the correct number of items were removed" ) {
                    REQUIRE( del.size() == count - 1 );

                    AND_THEN( "the removed items were all bottles" ) {
                        CHECK( std::all_of( del.begin(), del.end(), [&container_id]( const item& e ) {
                            return e.typeId() == container_id;
                        } ) );
                    }
                    AND_THEN( "the removed items all contained water" ) {
                        CHECK( std::all_of( del.begin(), del.end(), [&liquid_id]( const item& e ) {
                            return e.contents.size() == 1 && e.contents[ 0 ].typeId() == liquid_id;
                        } ) );
                    }
                }
            }
        }

        WHEN( "a hip flask containing water is worn" ) {
            item obj( worn_id );
            obj.emplace_back( liquid_id, calendar::turn, obj.type->container->contains );
            p.wear_item( obj );

            REQUIRE( count_items( container_id ) == count );
            REQUIRE( count_items( liquid_id ) == count + 1 );

            AND_WHEN( "all but one of the water is removed" ) {
                std::list<item> del = p.remove_items_with( [&liquid_id]( const item& e ) {
                    return e.typeId() == liquid_id;
                }, count );

                THEN( "all of the bottles remain in the players possession" ) {
                    REQUIRE( count_items( container_id ) == 5 );
                    AND_THEN( "all of the bottles are now empty" ) {
                        REQUIRE( p.visit_items_const( [&container_id]( const item *e ) {
                            return ( e->typeId() != container_id || e->contents.empty() ) ?
                                VisitResponse::NEXT : VisitResponse::ABORT;
                        } ) != VisitResponse::ABORT );
                    }
                }
                THEN( "the hip flask remains in the players posession" ) {
                    auto found = p.items_with( [&worn_id] ( const item& e ) {
                      return e.typeId() == worn_id;
                    } );
                    REQUIRE( found.size() == 1 );
                    AND_THEN( "the hip flask is still worn" ) {
                        REQUIRE( p.is_worn( *found[0] ) );

                        AND_THEN( "the hip flask contains water" ) {
                            REQUIRE( found[0]->contents.size() == 1 );
                            REQUIRE( found[0]->contents[0].typeId() == liquid_id );
                        }
                    }
                }
                THEN( "there is only one water remaining in the players possession" ) {
                    REQUIRE( count_items( liquid_id ) == 1 );
                }
                THEN( "the correct number of items were removed" ) {
                    REQUIRE( del.size() == count );

                    AND_THEN( "the removed items were all water" ) {
                        CHECK( std::all_of( del.begin(), del.end(), [&liquid_id]( const item& e ) {
                            return e.typeId() == liquid_id;
                        } ) );
                    }
                }

                AND_WHEN( "the final water is removed" ) {
                    std::list<item> del = p.remove_items_with( [&liquid_id]( const item& e ) {
                        return e.typeId() == liquid_id;
                    }, 1 );

                    THEN( "no water remain in the players possession" ) {
                        REQUIRE( count_items( liquid_id ) == 0 );
                    }

                    THEN( "the hip flask remains in the players posession" ) {
                        auto found = p.items_with( [&worn_id] ( const item& e ) {
                          return e.typeId() == worn_id;
                        } );
                        REQUIRE( found.size() == 1 );
                        AND_THEN( "the hip flask is worn" ) {
                            REQUIRE( p.is_worn( *found[0] ) );

                            AND_THEN( "the hip flask is empty" ) {
                                REQUIRE( found[0]->contents.empty() );
                            }
                        }
                    }
                }
            }
        }
    }
}
