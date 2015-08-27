#include "mission.h"
#include "game.h"
#include "debug.h"
#include "overmapbuffer.h"
#include "translations.h"
#include "requirements.h"
#include "overmap.h"
#include "line.h"

#include <fstream>
#include <sstream>

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

mission mission_type::create( const int npc_id ) const
{
    mission ret;
    ret.uid = g->assign_mission_id();
    ret.type = this;
    ret.npc_id = npc_id;
    ret.item_id = item_id;
    ret.item_count = item_count;
    ret.value = value;
    ret.follow_up = follow_up;

    if (deadline_low != 0 || deadline_high != 0) {
        ret.deadline = int(calendar::turn) + rng(deadline_low, deadline_high);
    } else {
        ret.deadline = 0;
    }

    return ret;
}

std::unordered_map<int, mission> mission::active_missions;

mission* mission::reserve_new( const mission_type_id type, const int npc_id )
{
    const mission tmp = mission_type::get( type )->create( npc_id );
    auto &result = active_missions[tmp.uid];
    result = tmp;
    return &result;
}

mission *mission::find( int id )
{
    const auto iter = active_missions.find( id );
    if( iter != active_missions.end() ) {
        return &iter->second;
    }
    dbg( D_ERROR ) << "requested mission with uid " << id << " does not exist";
    debugmsg( "requested mission with uid %d does not exist", id );
    return nullptr;
}

void mission::process_all()
{
    for( auto & e : active_missions ) {
        auto &m = e.second;
        if( m.deadline > 0 && !m.failed && int( calendar::turn ) > m.deadline ) {
            m.fail();
        }
    }
}

std::vector<mission*> mission::to_ptr_vector( const std::vector<int> &vec )
{
    std::vector<mission*> result;
    for( auto &id : vec ) {
        const auto miss = find( id );
        if( miss != nullptr ) {
            result.push_back( miss );
        }
    }
    return result;
}

std::vector<int> mission::to_uid_vector( const std::vector<mission*> &vec )
{
    std::vector<int> result;
    for( auto &miss : vec ) {
        result.push_back( miss->uid );
    }
    return result;
}

void mission::clear_all()
{
    active_missions.clear();
}

void mission::on_creature_death( Creature &poor_dead_dude )
{
    if( poor_dead_dude.is_hallucination() ) {
        return;
    }
    monster *mon = dynamic_cast<monster *>( &poor_dead_dude );
    if( mon != nullptr ) {
        if( mon->mission_id == -1 ) {
            return;
        }
        const auto mission = mission::find( mon->mission_id );
        const auto type = mission->type;
        if( type->goal == MGOAL_FIND_MONSTER ) {
            mission->fail();
        }
        if( type->goal == MGOAL_KILL_MONSTER ) {
            mission->step_complete( 1 );
        }
        return;
    }
    npc *p = dynamic_cast<npc *>( &poor_dead_dude );
    if( p == nullptr ) {
        // Must be the player
        for( auto &miss : g->u.get_active_missions() ) {
            // mission is free and can be reused
            miss->player_id = -1;
        }
        // The missions remains assigned to the (dead) character. This should not cause any problems
        // as the character is dismissed anyway.
        // Technically, the active missions could be moved to the failed mission section.
        return;
    }
    const auto dead_guys_id = p->getID();
    for( auto & e : active_missions ) {
        auto &i = e.second;
        //complete the mission if you needed killing
        if( i.type->goal == MGOAL_ASSASSINATE && i.target_npc_id == dead_guys_id ) {
            i.step_complete( 1 );
        }
        //fail the mission if the mission giver dies
        if( i.npc_id == dead_guys_id ) {
            i.fail();
        }
        //fail the mission if recruit target dies
        if( i.type->goal == MGOAL_RECRUIT_NPC && i.target_npc_id == dead_guys_id ) {
            i.fail();
        }
    }
}

mission* mission::reserve_random( const mission_origin origin, const tripoint &p, const int npc_id )
{
    const auto type = mission_type::get_random_id( origin, p );
    if( type == MISSION_NULL ) {
        return nullptr;
    }
    return mission::reserve_new( type, npc_id );
}

void mission::assign( player &u )
{
    if( player_id == u.getID() ) {
        debugmsg( "strange: player is already assigned to mission %d", uid );
        return;
    }
    if( player_id != -1 ) {
        debugmsg( "tried to assign mission %d to player, but mission is already assigned to %d", uid, player_id );
        return;
    }
    player_id = u.getID();
    u.on_mission_assignment( *this );
    if( !was_started ) {
        mission_start m_s;
        (m_s.*type->start)(this);
        was_started = true;
    }
}

void mission::fail()
{
    failed = true;
    if( g->u.getID() == player_id ) {
        g->u.on_mission_finished( *this );
    }
    mission_fail failfunc;
    (failfunc.*type->fail)( this );
}

void mission::set_target_to_mission_giver()
{
    const auto giver = g->find_npc( npc_id );
    if( giver != nullptr ) {
        target = giver->global_omt_location();
    } else {
        target = overmap::invalid_tripoint;
    }
}

void mission::step_complete( const int _step )
{
    step = _step;
    switch( type->goal ) {
        case MGOAL_FIND_ITEM:
        case MGOAL_FIND_MONSTER:
        case MGOAL_ASSASSINATE:
        case MGOAL_KILL_MONSTER:
            // Go back and report.
            set_target_to_mission_giver();
            break;
        default:
            //Suppress warnings
            break;
    }
}

void mission::wrap_up()
{
    auto &u = g->u;
    if( u.getID() != player_id ) {
        // This is called from npctalk.cpp, the npc should only offer the option to wrap up mission
        // that have been assigned to the current player.
        debugmsg( "mission::wrap_up called, player %d was assigned, but current player is %d", player_id, u.getID() );
    }
    u.on_mission_finished( *this );
    std::vector<item_comp> comps;
    switch( type->goal ) {
        case MGOAL_FIND_ITEM:
            comps.push_back(item_comp(type->item_id, item_count));
            u.consume_items(comps);
            break;
        case MGOAL_FIND_ANY_ITEM:
            u.remove_mission_items( uid );
            break;
        default:
            //Suppress warnings
            break;
    }
    mission_end endfunc;
    ( endfunc.*type->end )( this );
}

bool mission::is_complete( const int _npc_id ) const
{
    // TODO: maybe not g->u, but more generalized?
    auto &u = g->u;
    inventory tmp_inv = u.crafting_inventory();
    switch( type->goal ) {
        case MGOAL_GO_TO:
            {
                const tripoint cur_pos = g->u.global_omt_location();
                return ( rl_dist( cur_pos, target ) <= 1 );
            }
            break;

        case MGOAL_GO_TO_TYPE:
            {
                const auto cur_ter = overmap_buffer.ter( g->u.global_omt_location() );
                return cur_ter == type->target_id;
            }
            break;

        case MGOAL_FIND_ITEM:
            // TODO: check for count_by_charges and use appropriate player::has_* function
            if (!tmp_inv.has_amount(type->item_id, item_count)) {
                return tmp_inv.has_amount( type->item_id, 1 ) && tmp_inv.has_charges( type->item_id, item_count );
            }
            if( npc_id != -1 && npc_id != _npc_id ) {
                return false;
            }
            return true;

        case MGOAL_FIND_ANY_ITEM:
            return u.has_mission_item( uid ) && ( npc_id == -1 || npc_id == _npc_id );

        case MGOAL_FIND_MONSTER:
            if( npc_id != -1 && npc_id != _npc_id ) {
                return false;
            }
            for( size_t i = 0; i < g->num_zombies(); i++ ) {
                if( g->zombie( i ).mission_id == uid ) {
                    return true;
                }
            }
            return false;

        case MGOAL_RECRUIT_NPC:
            {
                npc *p = g->find_npc( target_npc_id );
                return p != nullptr && p->attitude == NPCATT_FOLLOW;
            }

        case MGOAL_RECRUIT_NPC_CLASS:
            {
                const auto npcs = overmap_buffer.get_npcs_near_player( 100 );
                for( auto & npc : npcs ) {
                    if( npc->myclass == recruit_class && npc->attitude == NPCATT_FOLLOW ) {
                        return true;
                    }
                }
                return false;
            }

        case MGOAL_FIND_NPC:
            return npc_id == _npc_id;

        case MGOAL_ASSASSINATE:
            return step >= 1;

        case MGOAL_KILL_MONSTER:
            return step >= 1;

        case MGOAL_KILL_MONSTER_TYPE:
            debugmsg( "%d kill count", g->kill_count( mtype_id( monster_type ) ) );
            debugmsg( "%d goal", monster_kill_goal );
            return g->kill_count( mtype_id( monster_type ) ) >= monster_kill_goal;

        case MGOAL_COMPUTER_TOGGLE:
            return step >= 1;

        default:
            return false;
    }
    return false;
}

bool mission::has_deadline() const
{
    return deadline != 0;
}

calendar mission::get_deadline() const
{
    return deadline;
}

std::string mission::get_description() const
{
    return description;
}

bool mission::has_target() const
{
    return target != overmap::invalid_tripoint;
}

const tripoint &mission::get_target() const
{
    return target;
}

const mission_type &mission::get_type() const
{
    return *type;
}

bool mission::has_follow_up() const
{
    return follow_up != MISSION_NULL;
}

mission_type_id mission::get_follow_up() const
{
    return follow_up;
}

long mission::get_value() const
{
    return value;
}

int mission::get_id() const
{
    return uid;
}

const std::string &mission::get_item_id() const
{
    return item_id;
}

bool mission::has_failed() const
{
    return failed;
}

int mission::get_npc_id() const
{
    return npc_id;
}

void mission::set_target( const tripoint &new_target )
{
    target = new_target;
}

bool mission::is_assigned() const
{
    return player_id != -1;
}

int mission::get_assigned_player_id() const
{
    return player_id;
}

std::string mission::name()
{
    if (type == NULL) {
        return "NULL";
    }
    return type->name;
}

void mission::load_info(std::istream &data)
{
    int type_id, rewtype, reward_id, rew_skill, tmpfollow, item_num, target_npc_id;
    std::string rew_item, itemid;
    data >> type_id;
    type = mission_type::get( static_cast<mission_type_id>( type_id ) );
    std::string tmpdesc;
    do {
        data >> tmpdesc;
        if (tmpdesc != "<>") {
            description += tmpdesc + " ";
        }
    } while (tmpdesc != "<>");
    description = description.substr( 0, description.size() - 1 ); // Ending ' '
    data >> failed >> value >> rewtype >> reward_id >> rew_item >> rew_skill >>
         uid >> target.x >> target.y >> itemid >> item_num >> deadline >> npc_id >>
         good_fac_id >> bad_fac_id >> step >> tmpfollow >> target_npc_id;
    target.z = 0;
    follow_up = mission_type_id(tmpfollow);
    reward.type = npc_favor_type(reward_id);
    reward.item_id = itype_id( rew_item );
    reward.skill = Skill::from_legacy_int( rew_skill );
    item_id = itype_id(itemid);
    item_count = int(item_num);
}

std::string mission_dialogue (mission_type_id id, const std::string &state)
{
    // Each mission must provide texts for the 9 mission topics.
    // Use one of the existing missions as template what topics are needed.
    // Return the text from within the mission case and leave the function there.
    // Reaching the end of the function is considered a bug and an appropriate message
    // is returned, so avoid it.
    switch (id) {
    case MISSION_GET_ANTIBIOTICS:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            switch (rng(1, 4)) {
            case 1:
                return _("Hey <name_g>... I really need your help...");
            case 2:
                return _("<swear!><punc> I'm hurting...");
            case 3:
                return _("This infection is bad, <very> bad...");
            case 4:
                return _("Oh god, it <swear> hurts...");
            }
            break;
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("\
I'm infected.  Badly.  I need you to get some antibiotics for me...");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Oh, thank god, thank you so much!  I won't last more than a couple of days, \
so hurry...");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("\
What?!  Please, <ill_die> without your help!");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("\
There's a town nearby.  Check pharmacies; it'll be behind the counter.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Find any antibiotics yet?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Oh thank god!  I'll be right as rain in no time.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What?!  You're lying, I can tell!  Ugh, forget it!");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("How am I not dead already?!");
        }
        break;

    case MISSION_GET_SOFTWARE:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("Oh man, I can't believe I forgot to download it...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("There's some important software on my computer that I need on USB.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Thanks!  Just pull the data onto this USB drive and bring it to me.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Seriously?  It's an easy job...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Take this USB drive.  Use the console, and download the software.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("So, do you have my software yet?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Excellent, thank you!");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What?!  You liar!");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("Wow, you failed?  All that work, down the drain...");
        }
        break;

    case MISSION_GET_ZOMBIE_BLOOD_ANAL:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("\
It could be very informative to perform an analysis of zombie blood...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("\
I need someone to get a sample of zombie blood, take it to a hospital, and \
perform a centrifuge analysis of it.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Excellent.  Take this vacutainer; once you've produced a zombie corpse, use it \
to extract blood from the body, then take it to a hospital for analysis.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("\
Are you sure?  The scientific value of that blood data could be priceless...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("\
The centrifuge is a bit technical; you might want to study up on the usage of \
computers before completing that part.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Well, do you have the data yet?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Excellent!  This may be the key to removing the infection.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("Wait, you couldn't possibly have the data!  Liar!");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("What a shame, that data could have proved invaluable...");
        }
        break;

    case MISSION_RESCUE_DOG:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("Oh, my poor puppy...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("\
I left my poor dog in a house, not far from here.  Can you retrieve it?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Thank you!  Please hurry back!");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("\
Please, think of my poor little puppy!");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("\
Take my dog whistle; if the dog starts running off, blow it and he'll return \
to your side.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("\
Have you found my dog yet?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("\
Thank you so much for finding him!");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What?!  You're lying, I can tell!  Ugh, forget it!");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("Oh no!  My poor puppy...");
        }
        break;

    case MISSION_KILL_ZOMBIE_MOM:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("Oh god, I can't believe it happened...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("\
My mom... she's... she was killed, but then she just got back up... she's one \
of those things now.  Can you put her out of her misery for me?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Thank you... she would've wanted it this way.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Please reconsider, I know she's suffering...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Find a gun if you can, make it quick...");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Well...?  Did you... finish things for my mom?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you.  I couldn't rest until I knew that was finished.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What?!  You're lying, I can tell!  Ugh, forget it!");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("Really... that's too bad.");
        }
        break;

    //patriot mission 1
    case MISSION_GET_FLAG:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("Does our flag still yet wave?");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Does our flag still yet wave? We're battered but not yet out of the\
 fight, we need the old colors!");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Hell ya!  Find me one of those big ol' American flags.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Seriously?  God damned commie...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Find a large federal building or school, they must have one.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Rescued the standard yet?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("America, fuck ya!");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What?!  You liar!");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("You give up?  This country fell apart because no one could find a\
good man to rely on... might as well give up, I guess.");
        }
        break;

    //patriot mission 2
    case MISSION_GET_BLACK_BOX:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We've got the flag, now we need to locate US forces.");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("We have the flag but now we need to locate US troops to see\
 what we can do to help.  I haven't seen any but I'm figure'n one of\
 those choppers that were fly'n round during th outbreak would have a good\
 idea.  If you can get me a black box from one of the wrecks I'll look\
 into where we might open'er at.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Fuck ya, America!");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Do you have any better ideas?");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Survivors were talking about them crashing but I don't know\
 where.  If I were a pilot I'd avoid crash landing in a city or forest\
 though.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("How 'bout that black box?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("America, fuck ya!");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What?!  I out'ta whip you're ass.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("Damn, I'll have to find'er myself.");
        }
        break;

    //patriot mission 3
    case MISSION_GET_BLACK_BOX_TRANSCRIPT:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("With the black box in hand, we need to find a lab.");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Thanks to your searching we've got the black box but now we\
 need to have a look'n-side her.  Now, most buildings don't have power\
 anymore but there are a few that might be of use.  Have you ever seen\
 one of those science labs that have popped up in the middle of nowhere?\
 Them suckers have a glowing terminal out front so I know they have power\
 somewhere inside'em.  If you can get inside and find a computer lab that\
 still works you ought to be able to find out what's in the black box.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Fuck ya, America!");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Do you have any better ideas?");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("When I was play'n with the terminal for the one I ran into it\
 kept asking for an ID card.  Finding one would be the first order of business.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("How 'bout that black box?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("America, fuck ya!  I was in the guard a few years back so I'm\
 confident I can make heads-or-tails of these transmissions.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What?!  I out'ta whip you're ass.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("Damn, I maybe we can find an egg-head to crack the terminal.");
        }
        break;

    //patriot mission 4
    case MISSION_EXPLORE_SARCOPHAGUS:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("You wouldn't believe what I found...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Holy hell, the crash you recovered the black box from wasn't as old as I thought.  \
Check this out, it was on its approach to pick up a team sent to secure and destroy something called a \
'Hazardous Waste Sarcophagus' in the middle of nowhere.  If the bird never picked up the team then we \
may still have a chance to meet up with them.  It includes an access code for the elevator and an encoded \
message for the team leader, I guess.  If we want to join up with what remains of the government then now \
may be our only chance.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Fuck ya, America!");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Are you going to forfeit your duty when the country needs you the most?");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("If there is a military team down there then we better go in prepared if we want to \
impress them.  Carry as much ammo as you can and prepare to ditch this place if they have a second \
bird coming to pick them up.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Having any trouble following the map?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("We got this shit!");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What?!  I out'ta whip your ass.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("Damn, we were so close.");
        }
        break;

    //martyr mission 1
    case MISSION_GET_RELIC:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("St. Michael the archangel defend me in battle...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("As the world seems to abandon the reality that we once knew, it \
becomes plausible that the old superstitions that were cast aside may \
have had some truth to them.  Please go and find me a religious relic...\
I doubt it will be of much use but I've got to hope in something.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
I wish you the best of luck, may whatever god you please guide your path.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Ya, I guess the stress may just be getting to me...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I suppose a large church or cathedral may have something.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Any luck? Please find me a small relic. Any relic will do.");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you, I need some time alone now...");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    //martyr mission 2
    case MISSION_RECOVER_PRIEST_DIARY:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("St. Michael the archangel defend me in battle...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("From what I understand, the creatures you encountered \
surrounding this relic were unlike anything I've heard of.  It is \
laughable that I now consider the living dead to be part of our \
ordinary reality.  Never-the-less, the church must have some explanation \
for these events.  I have located the residence of a local clergy man, \
could you go to this address and recover any items that may reveal what \
the church's stance is on these events?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
I wish you the best of luck, may whatever god you please guide your path.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Ya, I guess the stress may just be getting to me...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("If the information is confidential the priest must have it hidden \
within his own home.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Any luck?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you, a diary is exactly what I was looking for.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    //martyr mission 3
    case MISSION_INVESTIGATE_CULT:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("St. Michael the archangel defend me in battle...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("You have no idea how interesting this diary is.  I have two \
very promising leads...  First things first, the Catholic Church has been \
performing its own investigations into global cult phenomenon and it appears \
to have become very interested in a local cult as of recently.  Could you investigate \
a location for me?  I'm not sure what was going on here but the priest seemed \
fairly worried about it.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
I wish you the best of luck, may whatever god you please guide your path...  You may need it \
this time more than the past excursions you have gone on.  There is a note about potential human \
sacrifice in the days immediately before and after the outbreak.  The name of the cult is believed \
to be the Church of Starry Wisdom but it is noted that accounts differ.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Ya, I guess the stress may just be getting to me...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I doubt the site is still occupied but I'd carry a firearm at least... I'm not sure what \
you might be looking for but I'm positive you'll find something out of the ordinary if you look long \
enough.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("I'm positive there is something there... there has to be, any luck?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you, your account of these... demonic creations proves the fears the churches \
had were well founded.  Our priority should be routing out any survivors of this cult... I don't \
known if they are responsible for the outbreak but they certainly know more about it than I do.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    //martyr mission 4
    case MISSION_INVESTIGATE_PRISON_VISIONARY:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("St. Michael the archangel defend me in battle...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I have another task if you are feeling up to it.  There is a prisoner that \
the priest made special mention of.  I was wondering if you could see \
what may have happened to him or if he left anything in his cell.  The priest admits the individual is rather \
unstable, to put it lightly, but the priest personally believed the man was some kind of repentant visionary.  I'm \
not in a position to cast out the account just yet... it seems the man has prophesied  events accurately before \
concerning the Church of Starry Wisdom.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
I wish you the best of luck, may whatever god you please guide your path...  I can only imagine that the prison \
will be a little slice of hell.  I'm not sure what they would have decided to do with the inmates when they knew \
death was almost certain.  ");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Ya, I guess the stress may just be getting to me...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("The worst case scenario will probably be that the prisoners have escaped their cells and turned the \
building into their own little fortress.  Best case, the building went into lock-down and secured the prisoners in \
their cells.  Either way, navigating the building will pose its own difficulties.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Any luck?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you, I'm not sure what to make of this but I'll ponder your account.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_GET_RECORD_WEATHER:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("I wonder if a retreat might exist...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Everyone who dies gets back up, right?  Which means that whatever \
is causing this it must be airborne to have infected everyone.  I believe that \
if that is the case then there should be regions that were not downwind from \
where-ever the disease was released.  We need to find a record of all the \
weather patterns leading up to the outbreak.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Thanks so much, you may save both of us yet.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Ya, it was a long shot I admit.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I'm not sure, maybe a news station would have what we are looking?");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Any luck?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("These look more complicated than I thought, just give me some time.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("This isn't what we need.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("If only we could find a great valley or something.");
        }
        break;

    //humanitarian mission 1
    case MISSION_GET_RECORD_PATIENT:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("I hope I don't see many names I know...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I've lost so many friends... please find me a patient list from the \
regional hospital or doctor's office.  I just want to know who might still be out there.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Thank you, I suppose it won't change what has already happened but it will bring me closure.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Please, I just want to know what happened to everyone.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I bet you'll run into a lot of those things in the hospital, please be careful.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Any luck?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Oh dear, I thought Timmy would have made it...");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("Thanks for trying... I guess.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("I bet some of them are still out there...");
        }
        break;

    //humanitarian mission 2
    case MISSION_REACH_FEMA_CAMP:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("Maybe they escaped to one of the camps...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I can't thank you enough for bringing me the patient records but I do have \
another request.  You seem to know your way around... could you take me to one of the \
FEMA camps?  I know some were overrun but I don't want to believe all of them could have \
fallen.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Thank you, just bring me to the camp... I just want to see.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Please, I don't know what else to do.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("We should go at night, if it is overrun then we can quickly make our escape.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Any leads on where a camp might be?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("I guess this wasn't as bright an idea as I thought.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("Thanks for trying... I guess.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("I bet some of them are still out there...");
        }
        break;

    //humanitarian mission 3
    case MISSION_REACH_FARM_HOUSE:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("I just need a place to start over...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I've accepted that everyone I used to know is dead... one way or another.  I \
really wish I could have done something to save my brother but he was one of the first to go. \
I'd like to start over, just rebuild at one of the farms in the countryside.  Can you help me \
secure one?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Thank you, let's find a remote one so we don't have to worry about many zombies.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Please, I just don't know what to do otherwise.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Traveling the backroads would be a good way to search for one.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Shall we keep looking for a farm house?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Well, my adventuring days are over.  I can't thank you enough.  Trying to make \
this place self sustaining will take some work but the future is looking brighter.  At least it \
ought to be safe for now.  You'll always be welcome here.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("Thanks for trying... I guess.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("I guess it was just a pipe dream.");
        }
        break;

    //vigilante mission 1
    case MISSION_GET_RECORD_ACCOUNTING:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("Those twisted snakes...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Our world fell apart because our leaders were as crooked as the con-men that \
paid for their elections.  Just find me one of those corporate accounting books and I'll \
show you and the rest of the world just who is at fault.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
You'll see, I know I'm right.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("I know it isn't pressing but the big corporations didn't get a chance to destroy \
the evidence yet.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Try a big corporate building of some sort, they're bound to have an accounting department.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Any luck?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Great, let's see... uh... hmmm...  Fine, I didn't even do my own taxes but I'm sure this \
will prove their guilt if we get an expert to examine it.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("Thanks for trying... I guess.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("The day of reckoning will come for the corporations if it hasn't already.");
        }
        break;

    //vigilante mission 2
    case MISSION_GET_SAFE_BOX:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("Those twisted snakes...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Now I don't mean to upset you but I'm not sure what I can do with the accounting ledger at the \
moment.  We do have a new lead though, the ledger has a safe deposit box under the regional manager's name.  \
Guess what, dumb sucker wrote down his combination.  Come with me to retrieve the box and you can keep any \
of the goodies in it that I can't use to press charges against these bastards.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
You may make a tidy profit from this.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("I know it isn't pressing but the world is going to be just as corrupt when we start rebuilding unless \
we take measure to stop those who seek to rule over us.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("This shouldn't be hard unless we run into a horde.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Any luck?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Great, anything I can't use to prosecute the bastards is yours, as promised.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("Thanks for trying... I guess.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("The day of reckoning will come for the corporations if it hasn't already.");
        }
        break;

    //vigilante mission 3
    case MISSION_GET_DEPUTY_BADGE:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("Those twisted snakes...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I hope you will find use from what you got out of the deposit box but I also have another job for you \
that might lead you to an opportunity to deal out justice for those who cannot.  First things first, we can't just \
look like ruffians.  Find us a deputy badge, easy enough?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
I'd check the police station.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("We're also official... just hang in there and I'll show you what we can really do.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("They shouldn't be that hard to find... should they?");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Any luck?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Great work, Deputy.  We're in business.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("Thanks for trying... I guess.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("The day of reckoning will come for the criminals if it hasn't already.");
        }
        break;

    //demon slayer mission 1
    case MISSION_KILL_JABBERWOCK:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("The eater of the dead... something was ripping zombies to shreds and only leaving a few scattered limbs...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("A few days ago another survivor and I were trying to avoid the cities by staying in the woods \
during the day and foraging for gear at night. It worked well against the normal zed's but one night something \
caught onto our trail and chased us for ten minutes or so until we decided to split up and meet-up back here.  My \
buddy never showed up and I don't have the means to kill whatever it was.  Can you lend a hand?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Thanks, make sure you're ready for whatever the beast is.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Hey, I know I wouldn't volunteer for it either.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I'd carry a shotgun at least, it sounded pretty big.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Any luck?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("You look a little shaken up, I can't tell you how glad I am that you killed it though.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("Something in the shadows still seems to stare at me when I look at the woods.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("I'm glad you came back alive... I wasn't sure if I had sent you to your death.");
        }
        break;

    //demon slayer mission 2
    case MISSION_KILL_100_Z:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("You seem to know this new world better than most...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("You're kitted out better than most... would you be interested in making this world a \
little better for the rest of us?  The towns have enough supplies for us survivors to \
start securing a foothold but we don't have anyone with the skills and equipment to thin the masses \
of undead.  I'll lend you a hand to the best of my ability but you really \
showed promise taking out that other beast.  You, I, and a 100 regular zombies laid to rest, what do you say?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Hell ya, we may get ourselves killed but we'll be among the first legends of the apocalypse.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Hey, I know I wouldn't volunteer for it either... but then I remember that most of us survivors \
won't make it unless someone decides to take the initiative.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I'd secure an ammo cache and try to sweep a town in multiple passes.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Got this knocked out?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Man... you're a goddamn machine.  It was a pleasure working with you.  You know, you may just change \
our little neck of the world if you keep this up.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("I don't think that was quite a hundred dead zeds.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("Quitting already?");
        }
        break;

    //demon slayer mission 3
    case MISSION_KILL_HORDE_MASTER:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("I've heard some bad rumors so I hope you are up for another challenge...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Apparently one of the other survivors picked up on an unusually dense horde of undead \
moving into the area.  At the center of this throng there was a 'leader' of some sort.  The short of it is, \
kill the son of a bitch.  We don't know what it is capable of or why it is surrounded by other zombies but \
this thing reeks of trouble.  Do whatever it takes but we can't risk it getting away.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
I'll lend you a hand but I'd try and recruit another gunslinger if you can.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("What's the use of walking away, they'll track you down eventually.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Don't risk torching the building it may be hiding in if it has a basement.  The sucker may still be \
alive under the rubble and ash.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Got this knocked out?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("May that bastard never get up again.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("I don't think we got it yet.");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("Quitting already?");
        }
        break;

    //demon slayer mission 4
    case MISSION_RECRUIT_TRACKER:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("You seem to know this new world better than most...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("We've got another problem to deal with but I don't think we can handle \
it on our own.  So, I sent word out and found us a volunteer... of sorts.  He's vain \
as hell but has a little skill with firearms.  He was supposed to collect whatever he \
had of value and is going to meet us at a cabin in the woods.  Wasn't sure how long \
we were going to be so I told him to just camp there until we picked him up.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Rodger, if he's a no-show then any other gunslinger will do... but I doubt he'll quit \
before we even begin.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Hey, I know I wouldn't volunteer for it either... but then I remember that most of us survivors \
won't make it unless someone decides to take the initiative.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I hope the bastard is packing heat... else we'll need to grab him a gun before we hit our next \
target.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Found a gunslinger?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Great, just let me know when you are ready to wade knee-deep in an ocean of blood.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("I don't think so...");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("Quitting already?");
        }
        break;

    //demon slayer mission 4b
    case MISSION_JOIN_TRACKER:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("Well damn, you must be the guys here to pick me up...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I got the brief so I know what I'm getting into.  Let me be upfront, \
treat me like shit and I'm going to cover my own hide.  Without a strong band a man doesn't \
stand a chance in this world.  You ready to take charge boss?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("\
Before we get into a major fight just make sure we have the gear we need, boss.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("I don't think you're going to find many other survivors who haven't taken up a faction yet.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I'm a pretty good shot with a rifle or pistol.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Any problems boss?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Wait... are you really making me a deputy?");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("I don't think so...");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("Quitting already?");
        }
        break;

    //Free Merchants
    case MISSION_FREE_MERCHANTS_EVAC_1:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("If you really want to lend a hand we could use your help clearing out the dead "
                     "in the back bay.  Fearful of going outside during the first days of the cataclysm "
                     "we ended up throwing our dead and the zombies we managed to kill in the sealed "
                     "back bay.  Our promising leader at the time even fell... he turned into something "
                     "different.  Kill all of them and make sure they won't bother us again.  We can't "
                     "pay much but it would help us to reclaim the bay.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Please be careful, we don't need any more deaths.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we really need to start reclaiming the region.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("If you can, get a friend or two to help you.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Will they be bothering us any longer?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you, having that big of a threat close to home was nerve wrecking.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_FREE_MERCHANTS_EVAC_2:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("This is a bit more involved than the last request, we recently lost a "
                     "scavenger party coming to trade with us and would like you to "
                     "investigate.  We strongly suspect a raider band or horde caught them "
                     "off-guard. I can give you the coordinates of their last radio message "
                     "but little else.  In either case, deal with the threat so that the "
                     "scavengers can continue to pass through in relative safety.  The best "
                     "reward I can offer is a claim to the supplies they were carrying.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Our community survives on trade, we appreciate it.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we really need to start reclaiming the region.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("If you can, get a friend or two to help you.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Have you dealt with them?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you, the world is a better place without them.  Two of our more skilled "
                     "individuals accepted the assistance of the Old Guard in setting up an outpost "
                     "while you were out.  We didn't have many other options, refugees are still "
                     "streaming in and we don't know what else to do with our limited food supply.  "
                     "If you get a chance, you should see if they need your assistance in the future...");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_FREE_MERCHANTS_EVAC_3:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("The new outpost that we've started could use your assistance.  I need you to get the "
                     "foreman's prospectus before we begin to send additional refugees to the farming commune.  "
                     "Consult with the foreman to get more detailed tasks.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm sure the outpost will expand quickly with your assistance.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we really need to start reclaiming the region.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Just follow your map.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the prospectus?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("With this we'll be able to convince others to invest in the commune.  Thank you.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_FREE_MERCHANTS_EVAC_4:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("We are starting to build new infrastructure here and would like to get a few "
                     "new electrical systems online... unfortunately our existing system relies on "
                     "an array of something called RTGs.  From what I understand they work like "
                     "giant batteries of sorts.  We can expand our power system but to do so we "
                     "would need enough plutonium.  With 25 plutonium cells we would be "
                     "able to get an electrical expansion working for a year or two.  I know they "
                     "are rare but running generators isn't a viable option in the basement.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("If you can do this for us our survival options would vastly increase.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we really need to start reclaiming the region.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Can't help you much, I've never even seen a plutonium battery.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("How is the search going?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Great, I know it isn't much but we hope to continue to expand thanks to your help.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    //Old Guard
    case MISSION_OLD_GUARD_REP_1:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I don't like sending untested men into the field but if you have stayed alive so far "
                     "you might have some skills.  There are at least a pair of bandits squatting in a local cabin, "
                     "anyone who preys upon civilians meets a quick end... execute both of them for their "
                     "crimes.  Complete this and the Old Guard will consider you an asset in the region.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Contractor, I welcome you aboard.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("The States will remain a wasteland unless good men choose to save it.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("They might suspect you are coming, keep an eye out for traps.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Have you completed your mission?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("The Old Guard thanks you for eliminating the criminals.  You won't be forgotten.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_OLD_GUARD_REP_2:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("This task is going to require a little more persuasive skill.  I believe the "
                     "Hell's Raiders have an informant here to monitor who comes and goes.  I need "
                     "you to find out who it is and deal with them without letting anyone else know "
                     "of my suspicions.  We normally allow the Free Merchants to govern themselves "
                     "so I would hate to offend them.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Thank you, please keep this discreet.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we could use a few good men.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("If they draw first blood their friends are less likely to blame you...");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("You deal with the rat?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you, I'll do the explaining if anyone else asks about it.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_OLD_GUARD_REP_3:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("There is another monster troubling the merchants but this time it isn't "
                     "human... at least I don't think.  Guy just disappeared while walking "
                     "behind a packed caravan.  They didn't hear any shots but I suppose some "
                     "raider may have been real sneaky.  Check out the area and report anything "
                     "you find.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Thanks, keeping the people safe is what we try and do.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we really need to start reclaiming the region.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Search the bushes for any trace?  I'm not an expert tracker but you should "
                     "be able to find something.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("How is the search going?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Great work, wasn't sure what I was sending you after.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_OLD_GUARD_REP_4:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I've located a Hell's Raiders encampment in the region that appears to be "
                     "coordinating operations against the Free Merchants.  We know almost nothing "
                     "about the command structure in the 'gang' so I need to send someone in to "
                     "decapitate the leadership.  The raid will be held under orders of the U.S. "
                     "Marshals Service and by agreeing to the mission you will become a marshal, "
                     "swearing to assist the federal government in regaining order.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            popup("Now repeat after me...");
            popup("I, %s, do solemnly swear that I will support and defend the Constitution of the "
                  "United States against all enemies, foreign and domestic...", g->u.name.c_str());
            popup("...that I will bear true faith and allegiance to the same...");
            popup("...that I take this obligation freely, without any mental reservation or purpose of evasion...");
            popup("...and that I will well and faithfully discharge the duties of the office on which I am about to enter.");
            popup("To establish justice, insure domestic tranquility, provide for the common defense, promote the "
                  "general welfare and secure the blessings of liberty.");
            popup("So help me God.");
            return _("Congratulations Marshal, don't forget your badge and gun.  As a marshal all men or women assisting you are "
                     "considered deputy marshals so keep them in line.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we could use a few good men.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I'd recommend having two deputies... it would be a death trap if a "
                     "single man got surrounded.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Has the leadership been dealt with?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            popup("Marshal, you continue to impress us.");
            popup("If you are interested, I recently received a message that a unit was deploying into our AO.");
            popup("I don't have the exact coordinates but they said they were securing an underground facility and may require assistance.");
            popup("The bird dropped them off next to a pump station.  Can't tell you much more.");
            popup("If you cound locate the captain in charge, I'm sure he could use your skills. ");
            popup("Don't forget to wear your badge when meeting with them.");
            return _("Thank you once again marshal.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_OLD_GUARD_NEC_1:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("My communications team went to secure the radio control room after we "
                     "breached the facility.  I haven't heard from them since, I need you to "
                     "locate them.  Their first objective was to record all active channels that "
                     "were transmitting information on other survivors or facilities.  Find them "
                     "and return the frequency list to me.  I'm sure they could probably use your "
                     "help also.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Good luck, the communications room shouldn't be far from here.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("I don't know why you would bother wasting your time down here if you can't "
                     "handle a few small tasks...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("We were briefed that the communications array was on this level.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("How is the search going?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thanks, let me know when you need another tasking.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_OLD_GUARD_NEC_2:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Your assistance is greatly appreciated, we need to clear out the more "
                     "ruthless monsters that are wandering up from the lower levels.  If you "
                     "could cull twenty or so of what we refer to as 'nightmares' my men "
                     "would be much safer.  If you've cleared out most of this floor then the "
                     "lower levels should be your next target. ");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Good luck, finding a clear passage to the second level may be tricky.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("I don't know why you would bother wasting your time down here if you can't "
                     "handle a few small tasks...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("These creatures can swing their appendages surprisingly far.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("How is the hunt going?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thanks, let me know when you need another tasking.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_OLD_GUARD_NEC_COMMO_1:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("My chief responsibility is to monitor radio traffic and locate potential targets to "
                     "secure or rescue.  The majority of radio repeaters are down and those that are working "
                     "have only emergency power.  If you have a basic understanding of electronics you should "
                     "be able to fabricate  the 'radio repeater mod' found in these plans.  When this mod is "
                     "attached to a radio station's main terminal, all short range radio traffic on emergency "
                     "channels is boosted so we can pick it up at much longer ranges.  I really need you make "
                     "me one.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Thanks, I know the labs on the other side of the complex have electronic parts sitting around.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("I don't know why you would bother wasting your time down here if you can't "
                     "handle a few small tasks...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I'm sure the motorpool has a truck battery you could salvage.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Have you had any luck fabricating it?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thanks, I'll see to installing this one.  It will be some time but I could use someone to position "
                     "these around the region.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_OLD_GUARD_NEC_COMMO_2:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I guess I could use your skills once again.  There are small transmitters located in the nearby evacuation "
                    "shelters; if we don't separate them from the power grid their power systems will rapidly deteriorate "
                    "over the next few weeks.  The task is rather simple but the shelters offer us a place to redirect refugees "
                    "until this vault can be secured. ");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Thanks, I should be ready for you to install the radio repeater mods by the time you get back.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("I don't know why you would bother wasting your time down here if you can't "
                     "handle a few small tasks...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Try searching on the outskirts of towns.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Have you had any luck severing the connection?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("We are good to go!  The last of the gear is powering up now.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_OLD_GUARD_NEC_COMMO_3:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Most of my essential gear has been brought back online so it is time for you to install your "
                     "first radio repeater mod.  Head topside and locate the nearest radio station.  Install the "
                     "mod on the backup terminal and return to me so that I can verify that everything was successful.  "
                     "Radio towers must unfortunatly be ignored for now, without a dedicated emergency power system "
                     "they won't be useful for some time.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'll be standing by down here once you are done.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("I don't know why you would bother wasting your time down here if you can't "
                     "handle a few small tasks...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("If you could make some sort of directional antenna, it might help locating the radio stations.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Have you had any luck finding a radio tower?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("That's one down.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_OLD_GUARD_NEC_COMMO_4:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I could always use you to put another repeater mod up.  I don't have to remind you but every "
                     "one that goes up extends our response area just a little bit more.  With enough of them we'll "
                     "be able to maintain communication with anyone in the region.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'll be standing by.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("I don't know why you would bother wasting your time down here if you can't "
                     "handle a few small tasks...");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Getting a working vehicle is going to become important as the distance you have to travel increases.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Have you had any luck finding a radio tower?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("I'll try and update the captain with any signals that I need investigated.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_1:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Our first project is to enclose the barn so that new refugees have a safe place "
                     "to sleep.  Before we can begin serious improvements we need lumber to work with.  "
                     "Enclosing the four barn entrances will take nearly 400 2x4's... if you could "
                     "provide half of that we should be able to take care of the other half and "
                     "construction.  If you don't have a truck, I'd make finding one your first priority.  ");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("The more men we can bring to the camp the quicker improvements can be made.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Avoid destroying the furniture, we'll be needing it.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have them?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Great, I'll send word that we could use another set of hands or two to help out here.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_2:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("With the barn entrances enclosed the refugees moving into there should feel safer but the "
                     "weather can be bitter here.  We're going to need to build makeshift beds in the stalls to "
                     "ward off disease and frostbite.  We can take care of the frames but will need your help "
                     "looting blankets.  We'd need 25 blankets to provide the most basic sleeping accommodations.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'd start with looting hotels or you could sew them together... but that would be a last resort.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Avoid destroying the furniture here, we'll be needing it.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have them?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Your assistance is always appreciated.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_3:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Our supply of nails has been depleted.  I'm going to have to ask you to get us a few more "
                     "boxes.  Hardware stores should have a few if they haven't been looted.  I suppose you may "
                     "be able to find handfuls of them in building rubble.  We'd need 2500 to have enough that we "
                     "won't be sending you back out too soon.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Deconstructing furniture isn't going to be efficient, try looking for boxes or grabbing any "
                     "sitting on top of rubble piles.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Avoid destroying the furniture here, we'll be needing it.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have them?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Your assistance is always appreciated.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_4:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I'm sure you've noticed the new workers that have started trickling in.  The Free Merchant "
                     "counsel is demanding that we immediately begin projects to become self-sufficient due to "
                     "limited supplies.  We are going to need to rapidly setup an agricultural industry before "
                     "winter and starvation catches us unprepared and unsupported.  In order to get a half dozen "
                     "shovels and a couple of bags of seeds, we are going to have to trade for it.  I've already "
                     "got the deal lined up but the only thing they are willing to trade it for is salt.  I "
                     "negotiated them down from 500 units to 300... we were hoping you might have access to a source.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Salt is key to preserving meat and other perishables... without any excess food it wouldn't "
                     "do us much good now but I imagine we'll need to send you out to get more in the future.  ");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("If you can find a source of salt water you should be able to boil it down.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the salt?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("We are going to begin preparing our fields immediately.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_5:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Well, our first crop will be planted shortly but I'm starting to suspect that our profit "
                     "margin is going to be much smaller than we expect.  With limited seed for our original crop "
                     "our next course of action is to increase the soil's fertility.  Is there any way you could "
                     "find or produce a basic liquid fertilizer for us?  We'd need at least 30 units to make a "
                     "significant improvement in our output. ");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I don't know the exact recipie but I'm sure you could make it from a comercial fertilizer or "
                     "produce it from bonemeal.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I'd look through a few basic chemistry books to find a simple recipe.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the liquid fertilizer?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("This really should make the first winter easier to survive.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_6:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Our current assessment for survivability has increased significantly thanks to your efforts.  "
                     "The next priority issue is securing a cleaner water source.  Drinking from the pond on the "
                     "back end of the ranch has led to an outbreak of dysentery.  As quickly as possible we need to "
                     "improve the sanitary conditions in the camp.  To do so the first step is to dig a well and "
                     "construct stone fireplaces in the barn for the migrants to boil water.  We are going to need "
                     "at least 75 large rocks from you if we hope to accomplish the task before we all get sick.  "
                     "After we have them installed you will have free access to them, guaranteed. ");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("If you take a shovel to a pile of rubble you should be able to pull out structural grade stone.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Mining would always be an option if you had the resources.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the stone?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("I appreciate the work you do.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_7:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("To fabricate the well's hand pump and construct components for several other projects we are going to need to find or "
                     "fabricate a number of steel pipes.  Plumbing is a distant possibility for now but some form of irrigation will eventually "
                     "become desirable.  You could assist us with procuring 50 steel pipes, if you so have the chance.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Hopefully we will be able to assign new migrants to help fulfill our needs in the future.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Pipes are used in numerous metal constructions.  Smashing abandoned furniture may provide the material we need.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the pipes?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("I appreciate the work you do.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_8:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("With the well's completion we are rapidly closing the sustainability gap.  In order to expand "
                     "from here we will need massive quantities of lumber to construct fortifications and new buildings.  "
                     "We have already begun work on a manually operated sawmill but we will need two motors to control "
                     "the actual blades.  We were hoping you might be able to secure the motors for us.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Power for the motors will be provided by a salvaged truck battery, you need not bring additional mechanical components.  ");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Mining would always be an option if you had the resources.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the motors?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("I appreciate the work you do.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_9:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Disease and infection remains a persistent problem among the refugees.  Without dedicated medical "
                     "personnel and facilities I doubt everyone will be willing to stick around when the next outbreak "
                     "happens.  Until we can get a former medic or nurse I'm just going to have to improvise.  "
                     "Sterilization would be the first step I imagine.  Bring me five gallons of bleach so we can get "
                     "started.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm sure you can find bleach in most homes...");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("If you can't find a large supply I'd recommend checking hospitals or research labs.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the bleach?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("I appreciate it.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_10:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("We've started planning a medical clinic but we are going to need a lot more supplies if we hope to "
                     "warrant sending one of the few people with medical experience from the refugee center to our "
                     "outpost.  I know first aid kits are rare but they have all the basic supplies that I'm uncertain "
                     "of.  If you could bring in 3 full kits I'm sure we could supplement them to make them last a bit "
                     "longer.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("We'll do our best to make them last...");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Homes, hospitals, labs, and doctor offices would be where I'd check.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the first aid kits?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("I appreciate it.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_11:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("The work you have done has swayed the leaders at the refugee center to send someone out to assist "
                     "with our medical needs.  The next step is for us to get a sort of chop-shop setup so that we can "
                     "begin using scrap metal to build fortifications and equipment.  We are going to need a pair of "
                     "electric welders for our mechanics to use.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("With two, we should be able to repair one if it breaks.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("The only place I've seen them is garages but I imagine you could find them in some stores.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the welders?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("I appreciate it.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        default: // It's a bug.
            return "";
        }
        break;

    case MISSION_RANCH_FOREMAN_12:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("We have the basic equipment that we need but without a functioning power grid we are forced to rely "
                     "on the readily available vehicle batteries.  This is going to be a chore but I'll need twelve car "
                     "batteries to swap between charging and powering our equipment.  The good news is that they don't need "
                     "to be charged, we can take care of that.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Cars can be found in traffic jams along roads or in parking lots... I'm sure you can find a few.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the car batteries?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("I'm impressed with your abilities.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_13:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Check with the nurse when you get a chance, I know she will need help setting up our clinic.  On "
                     "the construction front, we are going to need to get dedicated scavenger teams setup to bring in "
                     "the miscellaneous supplies that we use in small quantities.  We are going to start setting up a "
                     "junk shop for them in the North end of the outpost so we could use your help with that.  "
                     "Communication is the biggest obstacle for the teams... could you bring in a pair two two-way radios?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Organizing the scavenging missions is difficult enough without communication.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("You should be able to find them in electronics stores or at a police station.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the two-way radios?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("I'm sure the scavengers will find these useful.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_14:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("The volunteers for the scavenging teams will need to be issued some basic equipment.  When the boss "
                     "sets up shop in the junk shed you should ask him what else he needs for his teams.  In the meantime "
                     "we can provide a few backpacks to get them started.  Could you find five backpacks to give to the "
                     "initial team?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("It may be easier to make them if you can't find a school or something.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the backpacks?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Having at least basic equipment greatly increases survivability.  Thanks.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_15:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Growth has lately begun to stall, we have individuals passing through the outpost but the prospect "
                     "of hard labor just to eek out an existence is driving them away.  We've asked around and decided "
                     "that, despite our pressing needs, building a bar ought to draw some of the less committed "
                     "individuals to the outpost.  Unlike other settlements, the more hands we have to more food we can "
                     "produce... at least in the long term.  Unfortunately, no one here has brewed alcoholic drinks "
                     "before so we are going to need you to find a book called the 'Homebrewer's Bible' or something "
                     "along those lines.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I guess you should search homes or libraries?");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the book?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("I guarantee we will toast to your name when the bar is finished.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_16:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Just flipping through the book I can tell that one ingredient in most of the alcoholic drinks "
                     "that we don't have a large supply of is sugar.  What alcohol we have been able to loot isn't "
                     "going to last us long so starting our first large batch is a priority.  Could you bring in 80 "
                     "units of sugar?  That should last us until we can start producing our own supply.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("You might get lucky finding it but you could always boil it out of fruit if you are familiar "
                     "with the process.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the sugar?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("There is a large group of thirsty individuals in our outpost that are truly thankful for your work.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "ALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_FOREMAN_17:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Although we are an agricultural outpost, we are restricted to growing plants that are compatible "
                     "with the New England climate during the warmer months.  The easiest way to work around that is to "
                     "build green houses to supplement our external fields.  There isn't going to be an easy way to "
                     "build these, we are going to need a massive number of glass sheets to enclose the frames.  The "
                     "first house will need 20 sheets of glass if you are still interested.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Deconstructing existing windows is the only way I know of to keep the sheets intact.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the glass sheets?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("We'll begin planting our first seeds as soon as we can get these installed.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_NURSE_1:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I've got a handful of bandages and a few first aid kits to work with at the moment... in other "
                     "words I'm completely unable to treat most serious medical emergencies.  I'm supposed to have "
                     "priority on any medical supplies that the scavengers bring in but I imagine the black market "
                     "for the stuff will prevent me from ever seeing it.  I could use your help getting a few "
                     "bottles of aspirin to start with.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Aspirin is pretty common in homes and convenience stores.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the aspirin?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("We'll go through this pretty quickly but it does help.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_NURSE_2:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I was given a few bottles of bleach when I arrived but I need a number of hotplates to actually "
                     "sterilize equipment.  I'm sure you can find them in any old house or appliance store.  Three "
                     "should be enough for now.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("It should be a fairly common item, don't know what else to say.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the hotplates?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_NURSE_3:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Disease is spreading rapidly due to poor nutrition and there is little that I can do about "
                     "it.  With food being scarce, people are willing to survive on whatever they can.  I need "
                     "to start supplementing the outpost's diet with vitamins to prevent potential deaths "
                     "indirectly attributable to nutrition.  I know it is a lot but if you could bring me 200 "
                     "vitamins I'd be able to treat the most vulnerable before they spread anything to the rest of us.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("It should be a fairly common item, don't know what else to say.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the vitamins?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_NURSE_4:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Despite my recommendations, we continue to have travelers come in with illnesses I've been "
                     "able to trace back to contaminated drinking water.  When boiling water isn't an option they "
                     "need some form of charcoal water filter that they can use.  If you could build me four "
                     "charcoal water filters I'll distribute them to groups as they pass through.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("With some basic survival and crafting skills you should be able to make them with little effort.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the charcoal water filters?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_NURSE_5:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I've been working on producing some of our own medical supplies but I'm going to need a chemistry "
                     "set to get everything that I need in order.  Is there any way you could go through one of the "
                     "school chemistry labs and steal me a chemistry set?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("You might be able to find one in a pharmacy if you can't find a school.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the chemistry set?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_NURSE_6:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("The number of airway infections that I've encountered in the past week has depleted my supply of "
                     "masks.  Could you find me 10 filter masks?  I tend to only distribute them in severe cases so "
                     "I'll be sure to make them last.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("You may be able to make one if you had the right guide.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the filter masks?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_NURSE_7:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("The bodily fluids that I often have to clean up and the caustic chemicals I deal with have done "
                     "a number on my current pair of gloves.  Could you find me four pairs of heavy rubber gloves?  "
                     "That should be enough that I can task a few laborers with cleanup details in the event of an "
                     "outbreak.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("You should be able to find them in cleaning closets.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the rubber gloves?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_NURSE_8:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I heard that we may have a physician on his way here.  The workers have already begun expanding "
                     "the clinic but I need you to locate a pair of scalpels to use in surgery when he arrives.  "
                     "I'm sure you should be able to find them in a hospital or craft shop.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Hospitals and crafting stores should have a few.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the scalpels?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_NURSE_9:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Have you heard of a book called the 'Guide to Advanced Emergency Care?'  I really need a copy.  "
                     "The doctor is requesting a lot of supplies that I'm not familiar with but I believe I could "
                     "make if I could get a copy of the book.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Libraries are the only place I'd think to look.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the Guide to Advanced Emergency Care?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_NURSE_10:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("The doctor has some ideas on how to recreate a vaccination program for the common flu.  If "
                     "you come across an unused flu shot please bring it to me.  We haven't had any fatalities "
                     "from the flu yet but it is only a matter of time until one of the older or younger members "
                     "of the outpost gets a bad case.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Hospitals or clinics might have a few that haven't been used.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the flu shot?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_NURSE_11:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("We are starting to isolate a few natural antibiotic serums but we don't have the supplies to "
                     "administer the cocktails.  I need you to bring me 10 empty syringes to use.  I'll take care of "
                     "cleaning them to prevent transferring infections.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Hospitals or clinics might have a few sitting around.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the empty syringes?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_SCAVENGER_1:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("I can usually use additional survival gear to kit-out the new recruits.  The most basic weapon "
                     "that everyone gets is the knife spear... it provides a good mix of range, power, and ease of "
                     "use when engaging the more common monsters.  Could you make me a dozen of them?  I may need "
                     "more later but that should help the first few teams.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("You should be able to make them with the most basic of skills.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the knife spears?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for the delivery.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_SCAVENGER_2:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Night is the ideal time for my team to go on raids but they are going to need better light "
                     "sources to speed up their missions.  Could you craft a set of five wearable flashlights for "
                     "them?  Keeping both hands free greatly helps during combat.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("You should be able to make them with the most basic of skills.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the wearable flashlights?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for the delivery.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_SCAVENGER_3:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Clothing that can withstand the wear and tear of climbing through windows and fighting back "
                     "wild animals is in high demand.  The best that we have been able to field is leather body "
                     "armor but it is difficult to make with our limited resources.  Could you craft us three "
                     "pairs of leather body armor?  The life-expectancy of my scavengers would drastically increase "
                     "if you did.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Finding the leather might be easier in town rather than making it yourself.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the leather armor?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for the delivery.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_SCAVENGER_4:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("When outnumbered or forced to pull back my scavengers have been taught to throw Molotov cocktails "
                     "to keep monsters from chasing them.  We go through one or two on every mission so we can always "
                     "use a few more.  Would you be willing to make us a dozen?  I'm willing to pay you what I can for "
                     "your assistance. ");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("I'm counting on you.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Alcohol or lamp oil is probably easier to refine rather than find in large quantities.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the Molotov cocktails?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for the delivery.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_BARTENDER_1:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("The wine and beer we can brew are sufficient to attract most patrons but we need something "
                     "a bit stronger to get them to forget their misery.  Could you build me a pair of stills?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("This should let us start producing whiskey, rum, and brandy when we get access to the ingredients.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("I'm sure you could find the design for the stills in home brewing books.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the stills?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for the delivery.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_BARTENDER_2:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("The last batch that I brewed was terrible, I'm fairly sure something contaminated the yeast "
                     "we have been using.  Could you locate a source of fresh yeast for us to use?  I'd need about 20 "
                     "teaspoons of dry yeast to get started.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Yeast should be common in homes or bakeries.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the yeast?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for the delivery.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_BARTENDER_3:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("Sugar and molasses remains in short supply.  I've put in a request for more but it is unlikely "
                     "that I'm going to see a reliable source for some time.  If you are interested, we are going to "
                     "need sugar beet seeds to meet future demand regardless of what the scavengers can find.  Could "
                     "you bring me at least enough seeds to plant a small 10 meter long patch?");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Farms or supply stories might have a few seeds...");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the sugar beet seeds?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for the delivery.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_BARTENDER_4:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("The drinks we brew here have become a major draw for visitors and the occasional trader.  Our "
                     "batches have become larger but we are still running out of storage space between trading runs.  "
                     "I've been able to get a few volunteers to help me build a set of standing tanks but I still need "
                     "12 metal tanks to build them.  I've talked with the scrappers but we are a low priority at the "
                     "moment.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Scrapping vehicles might be your best bet.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the metal tanks?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for the delivery.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_RANCH_BARTENDER_5:
        if( state == "TALK_MISSION_DESCRIBE" ) {
            return _("We need help...");
        } else if( state == "TALK_MISSION_OFFER" ) {
            return _("The other survivor outposts that we have contact with have been more than eager to trade for "
                     "beer or other drinks.  Transporting the drinks to them has presented a number of challenges "
                     "though.  If you could locate a pair of 55-gallon drums we should be able to keep them "
                     "supplied while turning a significant profit.");
        } else if( state == "TALK_MISSION_ACCEPTED" ) {
            return _("Thank you for your assistance.");
        } else if( state == "TALK_MISSION_REJECTED" ) {
            return _("Come back when you get a chance, we need skilled survivors.");
        } else if( state == "TALK_MISSION_ADVICE" ) {
            return _("Factories or junk yards are the only places I know where to look.");
        } else if( state == "TALK_MISSION_INQUIRE" ) {
            return _("Do you have the 55-gallon drums?");
        } else if( state == "TALK_MISSION_SUCCESS" ) {
            return _("Thank you for the delivery.");
        } else if( state == "TALK_MISSION_SUCCESS_LIE" ) {
            return _("What good does this do us?");
        } else if( state == "TALK_MISSION_FAILURE" ) {
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_REACH_SAFETY:
        // TODO: SOMEONE FILL THIS OUT!!!
        break;
    case MISSION_NULL:
    case NUM_MISSION_IDS:
        break; // fall though to error handling
    }
    return string_format( "Someone forgot to code this message id is %d, topic is %s!", static_cast<int>( id ), state.c_str() );
}

