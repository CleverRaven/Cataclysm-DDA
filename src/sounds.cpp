#include "sounds.h"

#include "game.h"
#include "map.h"
#include "debug.h"
#include "enums.h"
#include "overmapbuffer.h"
#include "translations.h"
#include "messages.h"
#include "monster.h"
#include "line.h"

struct sound_event {
    int volume;
    std::string description;
    bool ambient;
    bool footstep;
    std::string id;
    std::string variant;
};

struct centroid
{
    // Values have to be floats to prevent rounding errors.
    float x;
    float y;
    float z;
    float volume;
    float weight;
};

// Static globals tracking sounds events of various kinds.
// The sound events since the last monster turn.
static std::vector<std::pair<tripoint, int>> recent_sounds;
// The sound events since the last interactive player turn. (doesn't count sleep etc)
static std::vector<std::pair<tripoint, sound_event>> sounds_since_last_turn;
// The sound events currently displayed to the player.
static std::unordered_map<tripoint, sound_event> sound_markers;

void sounds::ambient_sound( const tripoint &p, int vol, std::string description )
{
    sound( p, vol, description, true );
}

void sounds::sound( const tripoint &p, int vol, std::string description, bool ambient, const std::string& id, const std::string& variant )
{
    if( vol < 0 ) {
        // Bail out if no volume.
        debugmsg( "negative sound volume %d", vol );
        return;
    }
    recent_sounds.emplace_back( std::make_pair( p, vol ) );
    sounds_since_last_turn.emplace_back(
        std::make_pair( p, sound_event{vol, description, ambient, false, id, variant} ) );
}

void sounds::add_footstep( const tripoint &p, int volume, int, monster * )
{
    sounds_since_last_turn.emplace_back(
        std::make_pair( p, sound_event{volume, "", false, true, "", ""} ) );
}

template <typename C>
static void vector_quick_remove( std::vector<C> &source, int index )
{
    if( source.size() != 1 ) {
        // Swap the target and the last element of the vector.
        // This scrambles the vector, but makes removal O(1).
        std::iter_swap( source.begin() + index, source.end() - 1 );
    }
    source.pop_back();
}

static std::vector<centroid> cluster_sounds( std::vector<std::pair<tripoint, int>> recent_sounds )
{
    // If there are too many monsters and too many noise sources (which can be monsters, go figure),
    // applying sound events to monsters can dominate processing time for the whole game,
    // so we cluster sounds and apply the centroids of the sounds to the monster AI
    // to fight the combanatorial explosion.
    std::vector<centroid> sound_clusters;
    const int num_seed_clusters = std::max( std::min(recent_sounds.size(), (size_t)10),
                                            (size_t)log(recent_sounds.size()) );
    const size_t stopping_point = recent_sounds.size() - num_seed_clusters;
    const size_t max_map_distance = rl_dist( 0, 0, MAPSIZE * SEEX, MAPSIZE * SEEY);
    // Randomly choose cluster seeds.
    for( size_t i = recent_sounds.size(); i > stopping_point; i-- ) {
        size_t index = rng(0, i - 1);
        // The volume and cluster weight are the same for the first element.
        sound_clusters.push_back(
           // Assure the compiler that these int->float conversions are safe.
            { (float)recent_sounds[index].first.x, (float)recent_sounds[index].first.y,
              (float)recent_sounds[index].first.z,
              (float)recent_sounds[index].second, (float)recent_sounds[index].second } );
        vector_quick_remove( recent_sounds, index );
    }

    for( const auto &sound_event_pair : recent_sounds ) {
        auto found_centroid = sound_clusters.begin();
        float dist_factor = max_map_distance;
        const auto cluster_end = sound_clusters.end();
        for( auto centroid_iter = sound_clusters.begin(); centroid_iter != cluster_end;
             ++centroid_iter ) {
            // Scale the distance between the two by the max possible distance.
            tripoint centroid_pos{ (int)centroid_iter->x, (int)centroid_iter->y, (int)centroid_iter->z };
            const int dist = rl_dist( sound_event_pair.first, centroid_pos );
            if( dist * dist < dist_factor ) {
                found_centroid = centroid_iter;
                dist_factor = dist * dist;
            }
        }
        const float volume_sum = (float)sound_event_pair.second + found_centroid->weight;
        // Set the centroid location to the average of the two locations, weighted by volume.
        found_centroid->x = (float)( (sound_event_pair.first.x * sound_event_pair.second) +
                                     (found_centroid->x * found_centroid->weight) ) / volume_sum;
        found_centroid->y = (float)( (sound_event_pair.first.y * sound_event_pair.second) +
                                     (found_centroid->y * found_centroid->weight) ) / volume_sum;
        found_centroid->z = (float)( (sound_event_pair.first.z * sound_event_pair.second) +
                                     (found_centroid->z * found_centroid->weight) ) / volume_sum;
        // Set the centroid volume to the larger of the volumes.
        found_centroid->volume = std::max( found_centroid->volume, (float)sound_event_pair.second );
        // Set the centroid weight to the sum of the weights.
        found_centroid->weight = volume_sum;
    }
    return sound_clusters;
}

void sounds::process_sounds()
{
    std::vector<centroid> sound_clusters = cluster_sounds( recent_sounds );

    const int weather_vol = weather_data(g->weather).sound_attn;
    for( const auto &this_centroid : sound_clusters ) {
        // Since monsters don't go deaf ATM we can just use the weather modified volume
        // If they later get physical effects from loud noises we'll have to change this
        // to use the unmodified volume for those effects.
        const int vol = this_centroid.volume - weather_vol;
        const tripoint source = tripoint( this_centroid.x, this_centroid.y, this_centroid.z );
        // --- Monster sound handling here ---
        // Alert all hordes
        if( vol > 20 && g->get_levz() == 0 ) {
            int sig_power = ((vol > 140) ? 140 : vol) - 20;
            const point abs_ms = g->m.getabs( source.x, source.y );
            const point abs_sm = overmapbuffer::ms_to_sm_copy( abs_ms );
            const tripoint target( abs_sm.x, abs_sm.y, source.z );
            overmap_buffer.signal_hordes( target, sig_power );
        }
        // Alert all monsters (that can hear) to the sound.
        for (int i = 0, numz = g->num_zombies(); i < numz; i++) {
            monster &critter = g->zombie(i);
            // rl_dist() is faster than critter.has_flag() or critter.can_hear(), so we'll check it first.
            int dist = rl_dist( source, critter.pos3() );
            int vol_goodhearing = vol * 2 - dist;
            if (vol_goodhearing > 0 && critter.can_hear()) {
                const bool goodhearing = critter.has_flag(MF_GOODHEARING);
                int volume = goodhearing ? vol_goodhearing : (vol - dist);
                // Error is based on volume, louder sound = less error
                if (volume > 0) {
                    int max_error = 0;
                    if (volume < 2) {
                        max_error = 10;
                    } else if (volume < 5) {
                        max_error = 5;
                    } else if (volume < 10) {
                        max_error = 3;
                    } else if (volume < 20) {
                        max_error = 1;
                    }

                    int target_x = source.x + rng(-max_error, max_error);
                    int target_y = source.y + rng(-max_error, max_error);
                    // target_z will require some special check due to soil muffling sounds

                    int wander_turns = volume * (goodhearing ? 6 : 1);
                    critter.wander_to( tripoint( target_x, target_y, source.z ), wander_turns);
                    critter.process_trigger(MTRIG_SOUND, volume);
                }
            }
        }
    }
    recent_sounds.clear();
}

void sounds::process_sound_markers( player *p )
{
    bool is_deaf = p->is_deaf();
    const float volume_multiplier = p->hearing_ability();
    const int weather_vol = weather_data(g->weather).sound_attn;

    for( const auto &sound_event_pair : sounds_since_last_turn ) {
        const int volume = sound_event_pair.second.volume * volume_multiplier;
        const std::string& sfx_id = sound_event_pair.second.id;
        const std::string& sfx_variant = sound_event_pair.second.variant;
        const int max_volume = std::max( volume, sound_event_pair.second.volume ); // For deafness checks
        int dist = rl_dist( p->pos3(), sound_event_pair.first );
        bool ambient = sound_event_pair.second.ambient;

        // Too far away, we didn't hear it!
        if( dist > volume ) {
            continue;
        }

        if( is_deaf ) {
            // Has to be here as well to work for stacking deafness (loud noises prolong deafness)
            if( !p->is_immune_effect( "deaf" ) && rng((max_volume - dist) / 2, (max_volume - dist)) >= 150 ) {
                // Prolong deafness, but not as much as if it was freshly applied
                int duration = std::min(40, (max_volume - dist - 130) / 8);
                p->add_effect( "deaf", duration );
                if( !p->has_trait( "DEADENED" ) ) {
                    p->add_msg_if_player( m_bad, _("Your eardrums suddenly ache!") );
                    if( p->pain < 10 ) {
                        p->mod_pain( rng( 0, 2 ) );
                    }
                }
            }
            // We're deaf, skip rest of processing.
            continue;
        }

        // Player volume meter includes all sounds from their tile and adjacent tiles
        // TODO: Add noises from vehicle player is in.
        if( dist <= 1 ) {
            p->volume = std::max( p->volume, volume );
        }

        // Check for deafness
        if( !p->is_immune_effect( "deaf" ) && rng((max_volume - dist) / 2, (max_volume - dist)) >= 150 ) {
            int duration = (max_volume - dist - 130) / 4;
            p->add_effect("deaf", duration);
            is_deaf = true;
            continue;
        }

        // At this point we are dealing with attention (as opposed to physical effects)
        // so reduce volume by the amount of ambient noise from the weather.
        const int mod_vol = (sound_event_pair.second.volume - weather_vol) * volume_multiplier;

        // The noise was drowned out by the surroundings.
        if( mod_vol - dist < 0 ) {
            continue;
        }

        // See if we need to wake someone up
        if( p->has_effect("sleep")) {
            if( (!(p->has_trait("HEAVYSLEEPER") ||
                   p->has_trait("HEAVYSLEEPER2")) && dice(2, 15) < mod_vol - dist) ||
                (p->has_trait("HEAVYSLEEPER") && dice(3, 15) < mod_vol - dist) ||
                (p->has_trait("HEAVYSLEEPER2") && dice(6, 15) < mod_vol - dist) ) {
                //Not kidding about sleep-thru-firefight
                p->wake_up();
                add_msg(m_warning, _("Something is making noise."));
            } else {
                continue;
            }
        }

        const tripoint &pos = sound_event_pair.first;
        const std::string &description = sound_event_pair.second.description;
        if( !ambient && ( pos != p->pos3() ) && !g->m.pl_sees( pos, dist ) ) {
            if( p->activity.ignore_trivial != true ) {
                std::string query;
                if( description.empty() ) {
                    query = _("Heard a noise!");
                } else {
                    query = string_format(_("Heard %s!"),
                                          sound_event_pair.second.description.c_str());
                }

                if( g->cancel_activity_or_ignore_query(query.c_str()) ) {
                    p->activity.ignore_trivial = true;
                    for( auto activity : p->backlog ) {
                        activity.ignore_trivial = true;
                    }
                }
            }
        }

        // Only print a description if it exists
        if( !description.empty() ) {
            // If it came from us, don't print a direction
            if( pos == p->pos3() ) {
                std::string uppercased = description;
                capitalize_letter(uppercased, 0);
                add_msg("%s", uppercased.c_str());
            } else {
                // Else print a direction as well
                std::string direction = direction_name( direction_from( p->pos3(), pos ) );
                add_msg(m_warning, _("From the %s you hear %s"), direction.c_str(), description.c_str());
            }
        }

        // Play the sound effect, if any.
        if(!sfx_id.empty()) {
            int heard_volume = volume - dist;
            // for our sfx API, 100 is "normal" volume, so scale accordingly
            heard_volume *= 10;
            play_sound_effect(sfx_id, sfx_variant, heard_volume);
            //add_msg("Playing sound effect %s, %s, %d", sfx_id.c_str(), sfx_variant.c_str(), heard_volume);
        }

        // If Z coord is different, draw even when you can see the source
        const bool diff_z = pos.z != p->posz();
        // Place footstep markers.
        if( pos == p->pos3() || p->sees( pos ) ) {
            // If we are or can see the source, don't draw a marker.
            continue;
        }

        int err_offset;
        if( mod_vol / dist < 2 ) {
            err_offset = 3;
        } else if( mod_vol / dist < 3 ) {
            err_offset = 2;
        } else {
            err_offset = 1;
        }

        // Enumerate the valid points the player *cannot* see.
        // Unless the source is on a different z-level, then any point is fine
        std::vector<tripoint> unseen_points;
        tripoint newp = pos;
        int &newx = newp.x;
        int &newy = newp.y;
        for( newx = pos.x - err_offset; newx <= pos.x + err_offset; newx++ ) {
            for ( newy = pos.y - err_offset; newy <= pos.y + err_offset; newy++ ) {
                if( diff_z || !p->sees( newp ) ) {
                    unseen_points.emplace_back( newp );
                }
            }
        }
        // Then place the sound marker in a random one.
        if( !unseen_points.empty() ) {
            sound_markers.emplace(
                std::make_pair( unseen_points[rng(0, unseen_points.size() - 1)],
                                sound_event_pair.second ) );
        }
    }
    sounds_since_last_turn.clear();
}

void sounds::reset_sounds()
{
    recent_sounds.clear();
    sounds_since_last_turn.clear();
    sound_markers.clear();
}

void sounds::reset_markers()
{
    sound_markers.clear();
}

weather_type previous_weather;

void sounds::do_ambient_sfx()
{
    /* Channel assignments:
    0: Daytime outdoors env
    1: Nighttime outdoors env
    2: Underground env
    3: Indoors env
    4: Indoors rain env
    5: Outdoors snow env
    6: Outdoors flurry env
    7: Outdoors thunderstorm env
    8: Outdoors rain env
    9: Outdoors drizzle env
    Group Assignments:
    1: SFX related to weather
    2: SFX related to time of day
    */
    set_group_channels(2, 9, 1);
    set_group_channels(0, 1, 2);
    // Check weather at player position
    w_point weather_at_player = g->weatherGen.get_weather( g->u.global_square_location(), calendar::turn );
    g->weather = g->weatherGen.get_weather_conditions(weather_at_player);
    // Step in at night time / we are not indoors
    if ( calendar::turn.is_night() && !g->is_sheltered(g->u.pos()) && is_channel_playing(1) != 1 ) {
        fade_audio_group(2, 1000);
        play_ambient_sound_effect("environment", "nighttime", 100, 1, 1000);
    // Step in at day time / we are not indoors
    } else if ( !calendar::turn.is_night() && is_channel_playing(0) != 1 && !g->is_sheltered(g->u.pos())) {
        fade_audio_group(2, 1000);
        play_ambient_sound_effect("environment", "daytime", 100, 0, 1000);
    }
    // We are underground
    if (( g->get_levz() <= -1 && is_channel_playing(2) != 1 ) || ( g->get_levz() <= -1
        && g->weather != previous_weather )) {
        fade_audio_group(1, 1000);
        fade_audio_group(2, 1000);
        play_ambient_sound_effect("environment", "underground", 100, 2, 1000);
    // We are indoors
    } else if (( g->is_sheltered(g->u.pos()) &&  g->get_levz() >= 0 && is_channel_playing(3) != 1 ) ||
               (g->is_sheltered(g->u.pos()) && g->get_levz() >= 0 && g->weather != previous_weather )) {
        fade_audio_group(1, 1000);
        fade_audio_group(2, 1000);
        play_ambient_sound_effect("environment", "indoors", 100, 3, 1000);
        // We are indoors and it is also raining
        if (( g->weather == WEATHER_RAINY || g->weather == WEATHER_DRIZZLE || g->weather == WEATHER_THUNDER
            || g->weather == WEATHER_LIGHTNING) && is_channel_playing(4) != 1 ) {
            play_ambient_sound_effect("environment", "indoors_rain", 100, 4, 1000);
        }
    // We are outside
    } else if (( !g->is_sheltered(g->u.pos()) && g->weather != WEATHER_CLEAR  && is_channel_playing(5) != 1  &&
               is_channel_playing(6) != 1  && is_channel_playing(7) != 1  && is_channel_playing(8) != 1  &&
               is_channel_playing(9) != 1 ) || ( !g->is_sheltered(g->u.pos()) && g->weather != previous_weather )) {
        fade_audio_group(1, 1000);
        // We are outside and there is precipitation
        switch( g->weather ) {
        case WEATHER_DRIZZLE:
            play_ambient_sound_effect("environment", "WEATHER_DRIZZLE", 100, 9, 1000);
            break;
        case WEATHER_RAINY:
            play_ambient_sound_effect("environment", "WEATHER_RAINY", 100, 8, 1000);
            break;
        case WEATHER_THUNDER:
        case WEATHER_LIGHTNING:
            play_ambient_sound_effect("environment", "WEATHER_THUNDER", 100, 7, 1000);
            break;
        case WEATHER_FLURRIES:
            play_ambient_sound_effect("environment", "WEATHER_FLURRIES", 100, 6, 1000);
            break;
        case WEATHER_SNOW:
        case WEATHER_SNOWSTORM:
            play_ambient_sound_effect("environment", "WEATHER_SNOW", 100, 5, 1000);
            break;
        }
    }
    // Keep track of weather to compare for next iteration
    previous_weather = g->weather;
}

void sounds::draw_monster_sounds( const tripoint &offset, WINDOW *window )
{
    auto sound_clusters = cluster_sounds( recent_sounds );
    // TODO: Signal sounds on different Z-levels differently (with '^' and 'v'?)
    for( const auto &sound : recent_sounds ) {
        mvwputch( window, offset.y + sound.first.y, offset.x + sound.first.x, c_yellow, '?');
    }
    for( const auto &sound : sound_clusters ) {
        mvwputch( window, offset.y + sound.y, offset.x + sound.x, c_red, '?');
    }
}

std::vector<tripoint> sounds::get_footstep_markers()
{
    // Optimization, make this static and clear it in reset_markers?
    std::vector<tripoint> footsteps;
    footsteps.reserve( sound_markers.size() );
    for( const auto &mark : sound_markers ) {
        footsteps.push_back( mark.first );
    }
    return footsteps;
}

std::pair<std::vector<tripoint>, std::vector<tripoint>> sounds::get_monster_sounds()
{
    auto sound_clusters = cluster_sounds( recent_sounds );
    std::vector<tripoint> sound_locations;
    sound_locations.reserve( recent_sounds.size() );
    for( const auto &sound : recent_sounds ) {
        sound_locations.push_back( sound.first );
    }
    std::vector<tripoint> cluster_centroids;
    cluster_centroids.reserve( sound_clusters.size() );
    for( const auto &sound : sound_clusters ) {
        cluster_centroids.emplace_back( sound.x, sound.y, sound.z );
    }
    return { sound_locations, cluster_centroids };
}

std::string sounds::sound_at( const tripoint &location )
{
    auto this_sound = sound_markers.find( location );
    if( this_sound == sound_markers.end() ) {
        return std::string();
    }

    if( !this_sound->second.description.empty() ) {
        return this_sound->second.description;
    }

    return _("a sound");
}
