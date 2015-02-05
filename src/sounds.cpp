#include "sounds.h"

#include "game.h"
#include "enums.h"

struct sound_event {
    int volume;
    std::string description;
    bool ambient;
    bool footstep;
};

struct centroid
{
    // Values have to be floats to prevent rounding errors.
    float x;
    float y;
    float volume;
    float weight;
};

// Static globals tracking sounds events of various kinds.
// The sound events since the last monster turn.
static std::vector<std::pair<point, int>> recent_sounds;
// The sound events since the last interactive player turn. (doesn't count sleep etc)
static std::unordered_map<point, sound_event> sounds_since_last_turn;
// The sound events currently displayed to the player.
static std::unordered_map<point, sound_event> sound_markers;

void sounds::ambient_sound(int x, int y, int vol, std::string description)
{
    sound( x, y, vol, description, true );
}

void sounds::sound(int x, int y, int vol, std::string description, bool ambient)
{
    if( vol == 0 ) {
        // Bail out if no volume.
        // TODO: log an error, this shouldn't happen?
        return;
    }
    recent_sounds.emplace_back( std::make_pair( point(x, y), vol ) );
    sounds_since_last_turn.emplace(
        std::make_pair( point(x, y), sound_event{vol, description, ambient, false} ) );
}

void sounds::add_footstep(int x, int y, int volume, int, monster *)
{
    sounds_since_last_turn.emplace(
        std::make_pair( point(x, y), sound_event{volume, "", false, true} ) );
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

static std::vector<centroid> cluster_sounds( std::vector<std::pair<point, int>> recent_sounds )
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
            const int dist = rl_dist( sound_event_pair.first.x, sound_event_pair.first.y,
                                      centroid_iter->x, centroid_iter->y );
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

    for( const auto &this_centroid : sound_clusters ) {
        const int vol = this_centroid.volume;
        const point source = point(this_centroid.x, this_centroid.y);
        // --- Monster sound handling here ---
        // Alert all hordes
        if( vol > 20 && g->levz == 0 ) {
            int sig_power = ((vol > 140) ? 140 : vol) - 20;
            g->cur_om->signal_hordes( g->levx + (MAPSIZE / 2),
                                      g->levy + (MAPSIZE / 2), sig_power );
        }
        // Alert all monsters (that can hear) to the sound.
        for (int i = 0, numz = g->num_zombies(); i < numz; i++) {
            monster &critter = g->zombie(i);
            // rl_dist() is faster than critter.has_flag() or critter.can_hear(), so we'll check it first.
            int dist = rl_dist( source, critter.pos() );
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

                    int wander_turns = volume * (goodhearing ? 6 : 1);
                    critter.wander_to(target_x, target_y, wander_turns);
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

    for( const auto &sound_event_pair : sounds_since_last_turn ) {
        int volume = sound_event_pair.second.volume * volume_multiplier;
        int dist = rl_dist( p->pos(), sound_event_pair.first );
        bool ambient = sound_event_pair.second.ambient;

        // Too far away, we didn't hear it!
        if( dist > volume ) {
            continue;
        }

        if( is_deaf ) {
            // Has to be here as well to work for stacking deafness (loud noises prolong deafness)
            if( !(p->has_bionic("bio_ears") || p->worn_with_flag("DEAF") ||
                  p->is_wearing("rm13_armor_on")) && rng((volume - dist) / 2, (volume - dist)) >= 150) {
                int duration = std::min(40, (volume - dist - 130) / 4);
                p->add_effect("deaf", duration);
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
        if( !p->has_bionic("bio_ears") && !p->is_wearing("rm13_armor_on") &&
            rng((volume - dist) / 2, (volume - dist)) >= 150 ) {
            int duration = (volume - dist - 130) / 4;
            p->add_effect("deaf", duration);
            is_deaf = true;
            continue;
        }

        // See if we need to wake someone up
        if( p->has_effect("sleep")) {
            if( (!(p->has_trait("HEAVYSLEEPER") ||
                   p->has_trait("HEAVYSLEEPER2")) && dice(2, 15) < volume - dist) ||
                (p->has_trait("HEAVYSLEEPER") && dice(3, 15) < volume - dist) ||
                (p->has_trait("HEAVYSLEEPER2") && dice(6, 15) < volume - dist) ) {
                //Not kidding about sleep-thru-firefight
                p->wake_up();
                add_msg(m_warning, _("Something is making noise."));
            } else {
                continue;
            }
        }

        const int x = sound_event_pair.first.x;
        const int y = sound_event_pair.first.y;
        const std::string &description = sound_event_pair.second.description;
        if( !ambient && (x != p->posx() || y != p->posy()) && !g->m.pl_sees( x, y, dist ) ) {
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
            if( x == p->posx() && y == p->posy() ) {
                std::string uppercased = description;
                capitalize_letter(uppercased, 0);
                add_msg("%s", uppercased.c_str());
            } else {
                // Else print a direction as well
                std::string direction = direction_name(direction_from(p->posx(), p->posy(), x, y));
                add_msg(m_warning, _("From the %s you hear %s"), direction.c_str(), description.c_str());
            }
        }

        // Place footstep markers.
        if( (x == p->posx() && y == p->posy()) || p->sees(x, y) ) {
            // If we are or can see the source, don't draw a marker.
            continue;
        }

        int err_offset;
        if( volume / dist < 2 ) {
            err_offset = 3;
        } else if( volume / dist < 3 ) {
            err_offset = 2;
        } else {
            err_offset = 1;
        }

        // Enumerate the valid points the player *cannot* see.
        std::vector<point> unseen_points;
        for( int newx = x - err_offset; newx <= x + err_offset; newx++) {
            for ( int newy = y - err_offset; newy <= y + err_offset; newy++) {
                if( !p->sees( newx, newy) ) {
                    unseen_points.emplace_back( newx, newy );
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

void sounds::draw_monster_sounds( const point &offset, WINDOW *window )
{
    auto sound_clusters = cluster_sounds( recent_sounds );
    for( const auto &sound : recent_sounds ) {
        mvwputch( window, offset.y + sound.first.y, offset.x + sound.first.x, c_yellow, '?');
    }
    for( const auto &sound : sound_clusters ) {
        mvwputch( window, offset.y + sound.y, offset.x + sound.x, c_red, '?');
    }
}

std::vector<point> sounds::get_footstep_markers()
{
    // Optimization, make this static and clear it in reset_markers?
    std::vector<point> footsteps;
    footsteps.reserve( sound_markers.size() );
    for( const auto &mark : sound_markers ) {
        footsteps.push_back( mark.first );
    }
    return footsteps;
}

std::pair<std::vector<point>, std::vector<point>> sounds::get_monster_sounds()
{
    auto sound_clusters = cluster_sounds( recent_sounds );
    std::vector<point> sound_locations;
    sound_locations.reserve( recent_sounds.size() );
    for( const auto &sound : recent_sounds ) {
        sound_locations.push_back( sound.first );
    }
    std::vector<point> cluster_centroids;
    cluster_centroids.reserve( sound_clusters.size() );
    for( const auto &sound : sound_clusters ) {
        cluster_centroids.emplace_back( sound.x, sound.y );
    }
    return { sound_locations, cluster_centroids };
}

std::string sounds::sound_at( const point &location )
{
    auto this_sound = sound_markers.find( location );
    if( this_sound == sound_markers.end() ) {
        return std::string();
    }
    return this_sound->second.description;
}
