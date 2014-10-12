#include "map.h"
#include "lightmap.h"
#include "output.h"
#include "rng.h"
#include "game.h"
#include "line.h"
#include "options.h"
#include "item_factory.h"
#include "mapbuffer.h"
#include "translations.h"
#include "monstergenerator.h"
#include <cmath>
#include <stdlib.h>
#include <fstream>
#include "debug.h"
#include "messages.h"
#include "mapsharing.h"

extern bool is_valid_in_w_terrain(int,int);

#include "overmapbuffer.h"

#define SGN(a) (((a)<0) ? -1 : 1)
#define INBOUNDS(x, y) \
 (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE)
#define dbg(x) DebugLog((DebugLevel)(x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

enum astar_list {
 ASL_NONE,
 ASL_OPEN,
 ASL_CLOSED
};

map::map(int mapsize)
{
    nulter = t_null;
    my_MAPSIZE = mapsize;
    dbg(D_INFO) << "map::map(): my_MAPSIZE: " << my_MAPSIZE;
    veh_in_active_range = true;
    transparency_cache_dirty = true;
    outside_cache_dirty = true;
    memset(veh_exists_at, 0, sizeof(veh_exists_at));
    for (size_t n = 0; n < sizeof(grid) / sizeof(grid[0]); n++) {
        grid[n] = NULL;
    }
}

map::~map()
{
}

VehicleList map::get_vehicles(){
   return get_vehicles(0,0,SEEX*my_MAPSIZE, SEEY*my_MAPSIZE);
}

VehicleList map::get_vehicles(const int sx, const int sy, const int ex, const int ey)
{
 const int chunk_sx = (sx / SEEX) - 1;
 const int chunk_ex = (ex / SEEX) + 1;
 const int chunk_sy = (sy / SEEY) - 1;
 const int chunk_ey = (ey / SEEY) + 1;
 VehicleList vehs;

 for(int cx = chunk_sx; cx <= chunk_ex; ++cx) {
  for(int cy = chunk_sy; cy <= chunk_ey; ++cy) {
   submap *current_submap = get_submap_at_grid(cx, cy);
   if(current_submap == NULL) {
       continue; // out of grid
   }

   for( size_t i = 0; i < current_submap->vehicles.size(); ++i ) {
    wrapped_vehicle w;
    w.v = current_submap->vehicles[i];
    w.x = w.v->posx + cx * SEEX;
    w.y = w.v->posy + cy * SEEY;
    w.i = cx;
    w.j = cy;
    vehs.push_back(w);
   }
  }
 }

 return vehs;
}

vehicle* map::veh_at(const int x, const int y, int &part_num)
{
    // This function is called A LOT. Move as much out of here as possible.
    if (!veh_in_active_range || !inbounds(x, y)) {
        return NULL;    // Out-of-bounds - null vehicle
    }
    if(!veh_exists_at[x][y]) {
        return NULL;    // cache cache indicates no vehicle. This should optimize a great deal.
    }
    std::pair<int,int> point(x,y);
    std::map< std::pair<int,int>, std::pair<vehicle*,int> >::iterator it;
    if ((it = veh_cached_parts.find(point)) != veh_cached_parts.end()) {
        part_num = it->second.second;
        return it->second.first;
    }
    debugmsg ("vehicle part cache cache indicated vehicle not found: %d %d",x,y);
    return NULL;
}

point map::veh_part_coordinates(const int x, const int y)
{
    int part_num;
    vehicle* veh = veh_at(x, y, part_num);

    if(veh == nullptr) {
        return point(0,0);
    }

    auto part = veh->parts[part_num];
    return point(part.mount_dx, part.mount_dy);
}

vehicle* map::veh_at(const int x, const int y)
{
 int part = 0;
 vehicle *veh = veh_at(x, y, part);
 return veh;
}

void map::reset_vehicle_cache()
{
 clear_vehicle_cache();
 // Cache all vehicles
 veh_in_active_range = false;
 for( std::set<vehicle*>::iterator veh = vehicle_list.begin(),
   it_end = vehicle_list.end(); veh != it_end; ++veh ) {
  update_vehicle_cache(*veh, true);
 }
}

void map::update_vehicle_cache(vehicle * veh, const bool brand_new)
{
 veh_in_active_range = true;
 if(!brand_new){
 // Existing must be cleared
  std::map< std::pair<int,int>, std::pair<vehicle*,int> >::iterator it =
             veh_cached_parts.begin(), end = veh_cached_parts.end(), tmp;
  while( it != end ) {
   if( it->second.first == veh ) {
    int x = it->first.first;
    int y = it->first.second;
    if ((x > 0) && (y > 0) &&
         x < SEEX*MAPSIZE &&
         y < SEEY*MAPSIZE){
     veh_exists_at[x][y] = false;
    }
    tmp = it;
    ++it;
    veh_cached_parts.erase( tmp );
   }else
    ++it;
  }
 }
 // Get parts
 std::vector<vehicle_part> & parts = veh->parts;
 const int gx = veh->global_x();
 const int gy = veh->global_y();
 int partid = 0;
 for( std::vector<vehicle_part>::iterator it = parts.begin(),
   end = parts.end(); it != end; ++it, ++partid ) {
  if (it->removed) {
      continue;
  }
  const int px = gx + it->precalc_dx[0];
  const int py = gy + it->precalc_dy[0];
  veh_cached_parts.insert( std::make_pair( std::make_pair(px,py),
                                        std::make_pair(veh,partid) ));
  if ((px > 0) && (py > 0) &&
       px < SEEX*MAPSIZE &&
       py < SEEY*MAPSIZE){
   veh_exists_at[px][py] = true;
  }
 }
}

void map::clear_vehicle_cache()
{
 std::map< std::pair<int,int>, std::pair<vehicle*,int> >::iterator part;
 while( veh_cached_parts.size() ) {
  part = veh_cached_parts.begin();
  int x = part->first.first;
  int y = part->first.second;
  if ((x > 0) && (y > 0) &&
       x < SEEX*MAPSIZE &&
       y < SEEY*MAPSIZE){
   veh_exists_at[x][y] = false;
  }
  veh_cached_parts.erase(part);
 }
}

void map::update_vehicle_list(submap *const to) {
 // Update vehicle data
 for( std::vector<vehicle*>::iterator it = to->vehicles.begin(),
      end = to->vehicles.end(); it != end; ++it ) {
   vehicle_list.insert(*it);
 }
}

void map::board_vehicle(int x, int y, player *p)
{
    if (!p) {
        debugmsg ("map::board_vehicle: null player");
        return;
    }

    int part = 0;
    vehicle *veh = veh_at(x, y, part);
    if (!veh) {
        if(p->grab_point.x == 0 && p->grab_point.y == 0) {
            debugmsg ("map::board_vehicle: vehicle not found");
        }
        return;
    }

    const int seat_part = veh->part_with_feature (part, VPFLAG_BOARDABLE);
    if (seat_part < 0) {
        debugmsg ("map::board_vehicle: boarding %s (not boardable)",
                  veh->part_info(part).name.c_str());
        return;
    }
    if (veh->parts[seat_part].has_flag(vehicle_part::passenger_flag)) {
        player *psg = veh->get_passenger (seat_part);
        debugmsg ("map::board_vehicle: passenger (%s) is already there",
                  psg ? psg->name.c_str() : "<null>");
        unboard_vehicle( x, y );
    }
    veh->parts[seat_part].set_flag(vehicle_part::passenger_flag);
    veh->parts[seat_part].passenger_id = p->getID();

    p->posx = x;
    p->posy = y;
    p->in_vehicle = true;
    if (p == &g->u &&
        (x < SEEX * int(my_MAPSIZE / 2) || y < SEEY * int(my_MAPSIZE / 2) ||
         x >= SEEX * (1 + int(my_MAPSIZE / 2)) ||
         y >= SEEY * (1 + int(my_MAPSIZE / 2))   )) {
        g->update_map(x, y);
    }
}

void map::unboard_vehicle(const int x, const int y)
{
    int part = 0;
    vehicle *veh = veh_at(x, y, part);
    player *passenger = NULL;
    if (!veh) {
        debugmsg ("map::unboard_vehicle: vehicle not found");
        // Try and force unboard the player anyway.
        if( g->u.xpos() == x && g->u.ypos() == y ) {
            passenger = &(g->u);
        } else {
            int npcdex = g->npc_at( x, y );
            if( npcdex != -1 ) {
                passenger = g->active_npc[npcdex];
            }
        }
        if( passenger ) {
            passenger->in_vehicle = false;
            passenger->driving_recoil = 0;
            passenger->controlling_vehicle = false;
        }
        return;
    }
    const int seat_part = veh->part_with_feature (part, VPFLAG_BOARDABLE, false);
    if (seat_part < 0) {
        debugmsg ("map::unboard_vehicle: unboarding %s (not boardable)",
                  veh->part_info(part).name.c_str());
        return;
    }
    passenger = veh->get_passenger(seat_part);
    if (!passenger) {
        debugmsg ("map::unboard_vehicle: passenger not found");
        return;
    }
    passenger->in_vehicle = false;
    passenger->driving_recoil = 0;
    passenger->controlling_vehicle = false;
    veh->parts[seat_part].remove_flag(vehicle_part::passenger_flag);
    veh->skidding = true;
}

void map::destroy_vehicle (vehicle *veh)
{
    if (!veh) {
        debugmsg("map::destroy_vehicle was passed NULL");
        return;
    }
    submap * const current_submap = get_submap_at_grid(veh->smx, veh->smy);
    for (size_t i = 0; i < current_submap->vehicles.size(); i++) {
        if (current_submap->vehicles[i] == veh) {
            vehicle_list.erase(veh);
            reset_vehicle_cache();
            current_submap->vehicles.erase (current_submap->vehicles.begin() + i);
            delete veh;
            return;
        }
    }
    debugmsg ("destroy_vehicle can't find it! name=%s, x=%d, y=%d", veh->name.c_str(), veh->smx, veh->smy);
}

bool map::displace_vehicle (int &x, int &y, const int dx, const int dy, bool test)
{
    const int x2 = x + dx;
    const int y2 = y + dy;
    int srcx = x;
    int srcy = y;
    int dstx = x2;
    int dsty = y2;

    if (!inbounds(srcx, srcy)){
        add_msg( m_debug, "map::displace_vehicle: coords out of bounds %d,%d->%d,%d",
                        srcx, srcy, dstx, dsty);
        return false;
    }

    int src_offset_x, src_offset_y, dst_offset_x, dst_offset_y;
    submap * const src_submap = get_submap_at(srcx, srcy, src_offset_x, src_offset_y);
    submap * const dst_submap = get_submap_at(dstx, dsty, dst_offset_x, dst_offset_y);

    if (test) {
        return src_submap != dst_submap;
    }

    // first, let's find our position in current vehicles vector
    int our_i = -1;
    for (size_t i = 0; i < src_submap->vehicles.size(); i++) {
        if (src_submap->vehicles[i]->posx == src_offset_x &&
              src_submap->vehicles[i]->posy == src_offset_y) {
            our_i = i;
            break;
        }
    }
    if (our_i < 0) {
        vehicle *v = veh_at(x, y);
        for(int a = 0; a < my_MAPSIZE * my_MAPSIZE; a++) {
            for (size_t i = 0; i < grid[a]->vehicles.size(); i++) {
                if (grid[a]->vehicles[i] == v) {
                    our_i = i;
                    const_cast<submap*&>(src_submap) = grid[a];
                    break;
                }
            }
        }
    }
    if (our_i < 0) {
        add_msg( m_debug, "displace_vehicle our_i=%d", our_i );
        return false;
    }
    // move the vehicle
    vehicle *veh = src_submap->vehicles[our_i];
    // don't let it go off grid
    if (!inbounds(x2, y2)) {
        veh->stop();
        // Silent debug
        dbg(D_ERROR) << "map:displace_vehicle: Stopping vehicle, displaced dx=" << dx << ", dy=" << dy;
        return false;
    }

    // record every passenger inside
    std::vector<int> psg_parts = veh->boarded_parts();
    std::vector<player *> psgs;
    for (auto &p : psg_parts) {
        psgs.push_back(veh->get_passenger(p));
    }

    const int rec = abs(veh->velocity) / 5 / 100;

    bool need_update = false;
    int upd_x, upd_y;
    // move passengers
    for (size_t i = 0; i < psg_parts.size(); i++) {
        player *psg = psgs[i];
        const int p = psg_parts[i];
        if (!psg) {
            debugmsg ("empty passenger part %d pcoord=%d,%d u=%d,%d?", p,
                         veh->global_x() + veh->parts[p].precalc_dx[0],
                         veh->global_y() + veh->parts[p].precalc_dy[0],
                                  g->u.posx, g->u.posy);
            veh->parts[p].remove_flag(vehicle_part::passenger_flag);
            continue;
        }
        // add recoil
        psg->driving_recoil = rec;
        // displace passenger taking in account vehicle movement (dx, dy)
        // and turning: precalc_dx/dy [0] contains previous frame direction,
        // and precalc_dx/dy[1] should contain next direction
        psg->posx += dx + veh->parts[p].precalc_dx[1] - veh->parts[p].precalc_dx[0];
        psg->posy += dy + veh->parts[p].precalc_dy[1] - veh->parts[p].precalc_dy[0];
        if (psg == &g->u) { // if passenger is you, we need to update the map
            need_update = true;
            upd_x = psg->posx;
            upd_y = psg->posy;
        }
    }

    veh->shed_loose_parts();
    for (auto &p : veh->parts) {
        p.precalc_dx[0] = p.precalc_dx[1];
        p.precalc_dy[0] = p.precalc_dy[1];
    }

    veh->posx = dst_offset_x;
    veh->posy = dst_offset_y;
    if (src_submap != dst_submap) {
        veh->set_submap_moved( int( x2 / SEEX ), int( y2 / SEEY ) );
        dst_submap->vehicles.push_back( veh );
        src_submap->vehicles.erase( src_submap->vehicles.begin() + our_i );
    }

    x += dx;
    y += dy;

    update_vehicle_cache(veh);

    bool was_update = false;
    if (need_update &&
          (upd_x < SEEX * int(my_MAPSIZE / 2) || upd_y < SEEY *int(my_MAPSIZE / 2) ||
          upd_x >= SEEX * (1+int(my_MAPSIZE / 2)) ||
          upd_y >= SEEY * (1+int(my_MAPSIZE / 2)))) {
        // map will shift, so adjust vehicle coords we've been passed
        if (upd_x < SEEX * int(my_MAPSIZE / 2)) {
            x += SEEX;
        } else if (upd_x >= SEEX * (1+int(my_MAPSIZE / 2))) {
            x -= SEEX;
        }
        if (upd_y < SEEY * int(my_MAPSIZE / 2)) {
            y += SEEY;
        } else if (upd_y >= SEEY * (1+int(my_MAPSIZE / 2))) {
            y -= SEEY;
        }
        g->update_map(upd_x, upd_y);
        was_update = true;
    }

    return (src_submap != dst_submap) || was_update;
}

void map::on_vehicle_moved() {
    set_outside_cache_dirty();
    set_transparency_cache_dirty();
}

void map::vehmove()
{
    // give vehicles movement points
    {
        VehicleList vehs = get_vehicles();
        for( size_t v = 0; v < vehs.size(); ++v ) {
            vehicle* veh = vehs[v].v;
            veh->gain_moves();
            veh->slow_leak();
        }
    }

    // 15 equals 3 >50mph vehicles, or up to 15 slow (1 square move) ones
    for( int count = 0; count < 15; count++ ) {
        if( !vehproceed() ) {
            break;
        } else {
            on_vehicle_moved();
        }
    }
    // Process item removal on the vehicles that were modified this turn.
    for (std::set<vehicle*>::iterator it = dirty_vehicle_list.begin(); it != dirty_vehicle_list.end(); ++it) {
        (*it)->part_removal_cleanup();
    }
    dirty_vehicle_list.clear();
}

bool map::vehproceed()
{
    VehicleList vehs = get_vehicles();
    vehicle* veh = NULL;
    float max_of_turn = 0;
    int x; int y;
    for( size_t v = 0; v < vehs.size(); ++v ) {
        if( vehs[v].v->of_turn > max_of_turn ) {
            veh = vehs[v].v;
            x = vehs[v].x;
            y = vehs[v].y;
            max_of_turn = veh->of_turn;
        }
    }
    if(!veh) { return false; }

    if (!inbounds(x, y)) {
        dbg( D_INFO ) << "stopping out-of-map vehicle. (x,y)=(" << x << "," << y << ")";
        veh->stop();
        veh->of_turn = 0;
        return true;
    }

    bool pl_ctrl = veh->player_in_control(&g->u);

    // k slowdown first.
    int slowdown = veh->skidding? 200 : 20; // mph lost per tile when coasting
    float kslw = (0.1 + veh->k_dynamics()) / ((0.1) + veh->k_mass());
    slowdown = (int) ceil(kslw * slowdown);
    if (abs(slowdown) > abs(veh->velocity)) {
        veh->stop();
    } else if (veh->velocity < 0) {
      veh->velocity += slowdown;
    } else {
      veh->velocity -= slowdown;
    }

    //low enough for bicycles to go in reverse.
    if (veh->velocity && abs(veh->velocity) < 20) {
        veh->stop();
    }

    if(veh->velocity == 0) {
        veh->of_turn -= .321f;
        return true;
    }

    std::vector<int> float_indices = veh->all_parts_with_feature(VPFLAG_FLOATS, false);
    if (float_indices.empty()) {
        // sink in water?
        std::vector<int> wheel_indices = veh->all_parts_with_feature(VPFLAG_WHEEL, false);
        int num_wheels = wheel_indices.size(), submerged_wheels = 0;
        for (int w = 0; w < num_wheels; w++) {
            const int p = wheel_indices[w];
            const int px = x + veh->parts[p].precalc_dx[0];
            const int py = y + veh->parts[p].precalc_dy[0];
            // deep water
            if (ter_at(px, py).has_flag(TFLAG_DEEP_WATER)) {
                submerged_wheels++;
            }
        }
        // submerged wheels threshold is 2/3.
        if (num_wheels && (float)submerged_wheels / num_wheels > .666) {
            add_msg(m_bad, _("Your %s sank."), veh->name.c_str());
            if (pl_ctrl) {
                veh->unboard_all();
            }
            // destroy vehicle (sank to nowhere)
            destroy_vehicle(veh);
            return true;
        }
    } else {

        int num = float_indices.size(), moored = 0;
        for (int w = 0; w < num; w++) {
            const int p = float_indices[w];
            const int px = x + veh->parts[p].precalc_dx[0];
            const int py = y + veh->parts[p].precalc_dy[0];

            if (!has_flag("SWIMMABLE", px, py)) {
                moored++;
            }
        }

        if (moored > num - 1) {
            veh->stop();
            veh->of_turn = 0;

            add_msg(m_info, _("Your %s is beached."), veh->name.c_str());

            return true;
        }

    }
    // One-tile step take some of movement
    //  terrain cost is 1000 on roads.
    // This is stupid btw, it makes veh magically seem
    //  to accelerate when exiting rubble areas.
    float ter_turn_cost = 500.0 * move_cost_ter_furn (x,y) / abs(veh->velocity);

    //can't afford it this turn?
    if(ter_turn_cost >= veh->of_turn) {
        veh->of_turn_carry = veh->of_turn;
        veh->of_turn = 0;
        return true;
    }

    veh->of_turn -= ter_turn_cost;

    // if not enough wheels, mess up the ground a bit.
    if (!veh->valid_wheel_config()) {
        veh->velocity += veh->velocity < 0 ? 2000 : -2000;
        for (auto &p : veh->parts) {
            const int px = x + p.precalc_dx[0];
            const int py = y + p.precalc_dy[0];
            const ter_id &pter = ter(px, py);
            if (pter == t_dirt || pter == t_grass) {
                ter_set(px, py, t_dirtmound);
            }
        }
    }

    if (veh->skidding) {
        if (one_in(4)) { // might turn uncontrollably while skidding
            veh->turn (one_in(2) ? -15 : 15);
        }
    }
    else if (pl_ctrl && rng(0, 4) > g->u.skillLevel("driving") && one_in(20)) {
        add_msg(m_warning, _("You fumble with the %s's controls."), veh->name.c_str());
        veh->turn (one_in(2) ? -15 : 15);
    }
    // eventually send it skidding if no control
    if (!veh->boarded_parts().size() && one_in (10)) {
        veh->skidding = true;
    }
    tileray mdir; // the direction we're moving
    if (veh->skidding) { // if skidding, it's the move vector
        mdir = veh->move;
    } else if (veh->turn_dir != veh->face.dir()) {
        mdir.init (veh->turn_dir); // driver turned vehicle, get turn_dir
    } else {
      mdir = veh->face;          // not turning, keep face.dir
    }
    mdir.advance (veh->velocity < 0? -1 : 1);
    const int dx = mdir.dx();           // where do we go
    const int dy = mdir.dy();           // where do we go
    bool can_move = true;
    // calculate parts' mount points @ next turn (put them into precalc[1])
    veh->precalc_mounts(1, veh->skidding ? veh->turn_dir : mdir.dir());

    int dmg_1 = 0;

    std::vector<veh_collision> veh_veh_colls;
    std::vector<veh_collision> veh_misc_colls;

    if (veh->velocity == 0) { can_move = false; }
    // find collisions
    int vel1 = veh->velocity/100; //velocity of car before collision
    veh->collision( veh_veh_colls, veh_misc_colls, dx, dy, can_move, dmg_1 );

    bool veh_veh_coll_flag = false;
    // Used to calculate the epicenter of the collision.
    point epicenter1(0, 0);
    point epicenter2(0, 0);

    if(veh_veh_colls.size()) { // we have dynamic crap!
        // effects of colliding with another vehicle:
        // transfers of momentum, skidding,
        // parts are damaged/broken on both sides,
        // remaining times are normalized,
        veh_veh_coll_flag = true;
        veh_collision c = veh_veh_colls[0]; //Note: What´s with collisions with more than 2 vehicles?
        vehicle* veh2 = (vehicle*) c.target;
        add_msg(m_bad, _("The %1$s's %2$s collides with the %3$s's %4$s."),
                       veh->name.c_str(),  veh->part_info(c.part).name.c_str(),
                       veh2->name.c_str(), veh2->part_info(c.target_part).name.c_str());

        // for reference, a cargo truck weighs ~25300, a bicycle 690,
        //  and 38mph is 3800 'velocity'
        rl_vec2d velo_veh1 = veh->velo_vec();
        rl_vec2d velo_veh2 = veh2->velo_vec();
        float m1 = veh->total_mass();
        float m2 = veh2->total_mass();
        //Energy of vehicle1 annd vehicle2 before collision
        float E = 0.5 * m1 * velo_veh1.norm() * velo_veh1.norm() +
            0.5 * m2 * velo_veh2.norm() * velo_veh2.norm();

        //collision_axis
        int x_cof1 = 0, y_cof1 = 0, x_cof2 = 0, y_cof2 = 0;
        veh ->center_of_mass(x_cof1, y_cof1);
        veh2->center_of_mass(x_cof2, y_cof2);
        rl_vec2d collision_axis_y;

        collision_axis_y.x = ( veh->global_x() + x_cof1 ) -  ( veh2->global_x() + x_cof2 );
        collision_axis_y.y = ( veh->global_y() + y_cof1 ) -  ( veh2->global_y() + y_cof2 );
        collision_axis_y = collision_axis_y.normalized();
        rl_vec2d collision_axis_x = collision_axis_y.get_vertical();
        // imp? & delta? & final? reworked:
        // newvel1 =( vel1 * ( mass1 - mass2 ) + ( 2 * mass2 * vel2 ) ) / ( mass1 + mass2 )
        // as per http://en.wikipedia.org/wiki/Elastic_collision
        //velocity of veh1 before collision in the direction of collision_axis_y
        float vel1_y = collision_axis_y.dot_product(velo_veh1);
        float vel1_x = collision_axis_x.dot_product(velo_veh1);
        //velocity of veh2 before collision in the direction of collision_axis_y
        float vel2_y = collision_axis_y.dot_product(velo_veh2);
        float vel2_x = collision_axis_x.dot_product(velo_veh2);
        // e = 0 -> inelastic collision
        // e = 1 -> elastic collision
        float e = get_collision_factor(vel1_y/100 - vel2_y/100);

        //velocity after collision
        float vel1_x_a = vel1_x;
        // vel1_x_a = vel1_x, because in x-direction we have no transmission of force
        float vel2_x_a = vel2_x;
        //transmission of force only in direction of collision_axix_y
        //equation: partially elastic collision
        float vel1_y_a = ( m2 * vel2_y * ( 1 + e ) + vel1_y * ( m1 - m2 * e) ) / ( m1 + m2);
        //equation: partially elastic collision
        float vel2_y_a = ( m1 * vel1_y * ( 1 + e ) + vel2_y * ( m2 - m1 * e) ) / ( m1 + m2);
        //add both components; Note: collision_axis is normalized
        rl_vec2d final1 = collision_axis_y * vel1_y_a + collision_axis_x * vel1_x_a;
        //add both components; Note: collision_axis is normalized
        rl_vec2d final2 = collision_axis_y * vel2_y_a + collision_axis_x * vel2_x_a;

        //Energy after collision
        float E_a = 0.5 * m1 * final1.norm() * final1.norm() +
            0.5 * m2 * final2.norm() * final2.norm();
        float d_E = E - E_a;  //Lost energy at collision -> deformation energy
        float dmg = std::abs( d_E / 1000 / 2000 );  //adjust to balance damage
        float dmg_veh1 = dmg * 0.5;
        float dmg_veh2 = dmg * 0.5;

        int coll_parts_cnt = 0; //quantity of colliding parts between veh1 and veh2
        for( size_t i = 0; i < veh_veh_colls.size(); ++i ) {
            veh_collision tmp_c = veh_veh_colls[i];
            if(veh2 == (vehicle*) tmp_c.target) { coll_parts_cnt++; }
        }

        float dmg1_part = dmg_veh1 / coll_parts_cnt;
        float dmg2_part = dmg_veh2 / coll_parts_cnt;

        //damage colliding parts (only veh1 and veh2 parts)
        for( size_t i = 0; i < veh_veh_colls.size(); ++i ) {
            veh_collision tmp_c = veh_veh_colls[i];

            if(veh2 == (vehicle*) tmp_c.target) {
                int parm1 = veh->part_with_feature (tmp_c.part, VPFLAG_ARMOR);
                if (parm1 < 0) {
                    parm1 = tmp_c.part;
                }
                int parm2 = veh2->part_with_feature (tmp_c.target_part, VPFLAG_ARMOR);
                if (parm2 < 0) {
                    parm2 = tmp_c.target_part;
                }
                epicenter1.x += veh->parts[parm1].mount_dx;
                epicenter1.y += veh->parts[parm1].mount_dy;
                veh->damage(parm1, dmg1_part, 1);

                epicenter2.x += veh2->parts[parm2].mount_dx;
                epicenter2.y += veh2->parts[parm2].mount_dy;
                veh2->damage(parm2, dmg2_part, 1);
            }
        }
        epicenter1.x /= coll_parts_cnt;
        epicenter1.y /= coll_parts_cnt;
        epicenter2.x /= coll_parts_cnt;
        epicenter2.y /= coll_parts_cnt;


        if (dmg2_part > 100) {
            // shake veh because of collision
            veh2->damage_all(dmg2_part / 2, dmg2_part, 1, epicenter2);
        }

        dmg_1 += dmg1_part;

        veh->move.init (final1.x, final1.y);
        veh->velocity = final1.norm();
        // shrug it off if the change is less than 8mph.
        if(dmg_veh1 > 800) {
            veh->skidding = 1;
        }
        veh2->move.init(final2.x, final2.y);
        veh2->velocity = final2.norm();
        if(dmg_veh2 > 800) {
            veh2->skidding = 1;
        }
        //give veh2 the initiative to proceed next before veh1
        float avg_of_turn = (veh2->of_turn + veh->of_turn) / 2;
        if(avg_of_turn < .1f)
            avg_of_turn = .1f;
        veh->of_turn = avg_of_turn * .9;
        veh2->of_turn = avg_of_turn * 1.1;
    }

    for(std::vector<veh_collision>::iterator next_collision = veh_misc_colls.begin();
            next_collision != veh_misc_colls.end(); next_collision++) {

        point collision_point(veh->parts[next_collision->part].mount_dx,
                                    veh->parts[next_collision->part].mount_dy);
        int coll_dmg = next_collision->imp;
        //Shock damage
        veh->damage_all(coll_dmg / 2, coll_dmg, 1, collision_point);
    }

    int coll_turn = 0;
    if (dmg_1 > 0) {
        int vel1_a = veh->velocity / 100; //velocity of car after collision
        int d_vel = abs(vel1 - vel1_a);

        std::vector<int> ppl = veh->boarded_parts();

        for (auto &ps : ppl) {
            player *psg = veh->get_passenger (ps);
            if (!psg) {
                debugmsg ("throw passenger: empty passenger at part %d", ps);
                continue;
            }

            bool throw_from_seat = 0;
            if (veh->part_with_feature (ps, VPFLAG_SEATBELT) == -1) {
                throw_from_seat = d_vel * rng(80, 120) / 100 > (psg->str_cur * 1.5 + 5);
            }

            //damage passengers if d_vel is too high
            if(d_vel > 60* rng(50,100)/100 && !throw_from_seat) {
                int dmg = d_vel/4*rng(70,100)/100;
                psg->hurtall(dmg);
                if (psg == &g->u) {
                    add_msg(m_bad, _("You take %d damage by the power of the impact!"), dmg);
                } else if (psg->name.length()) {
                    add_msg(m_bad, _("%s takes %d damage by the power of the impact!"),
                                   psg->name.c_str(), dmg);
                }
            }

            if (throw_from_seat) {
                if (psg == &g->u) {
                    add_msg(m_bad, _("You are hurled from the %s's seat by the power of the impact!"),
                                   veh->name.c_str());
                } else if (psg->name.length()) {
                    add_msg(m_bad, _("%s is hurled from the %s's seat by the power of the impact!"),
                                   psg->name.c_str(), veh->name.c_str());
                }
                unboard_vehicle(x + veh->parts[ps].precalc_dx[0],
                                     y + veh->parts[ps].precalc_dy[0]);
                g->fling_creature(psg, mdir.dir() + rng(0, 60) - 30,
                                           (vel1 - psg->str_cur < 10 ? 10 :
                                            vel1 - psg->str_cur));
            } else if (veh->part_with_feature (ps, "CONTROLS") >= 0) {
                // FIXME: should actually check if passenger is in control,
                // not just if there are controls there.
                const int lose_ctrl_roll = rng (0, dmg_1);
                if (lose_ctrl_roll > psg->dex_cur * 2 + psg->skillLevel("driving") * 3) {
                    if (psg == &g->u) {
                        add_msg(m_warning, _("You lose control of the %s."), veh->name.c_str());
                    } else if (psg->name.length()) {
                        add_msg(m_warning, _("%s loses control of the %s."), psg->name.c_str());
                    }
                    int turn_amount = (rng (1, 3) * sqrt((double)vel1_a) / 2) / 15;
                    if (turn_amount < 1) {
                        turn_amount = 1;
                    }
                    turn_amount *= 15;
                    if (turn_amount > 120) {
                        turn_amount = 120;
                    }
                    coll_turn = one_in (2)? turn_amount : -turn_amount;
                }
            }
        }
    }
    if(veh_veh_coll_flag) return true;

    // now we're gonna handle traps we're standing on (if we're still moving).
    // this is done here before displacement because
    // after displacement veh reference would be invdalid.
    // damn references!
    if (can_move) {
        std::vector<int> wheel_indices = veh->all_parts_with_feature("WHEEL", false);
        for (auto &w : wheel_indices) {
            const int wheel_x = x + veh->parts[w].precalc_dx[0];
            const int wheel_y = y + veh->parts[w].precalc_dy[0];
            if (one_in(2)) {
                if( displace_water( wheel_x, wheel_y) && pl_ctrl ) {
                    add_msg(m_warning, _("You hear a splash!"));
                }
            }
            veh->handle_trap( wheel_x, wheel_y, w );
            auto &item_vec = g->m.i_at( wheel_x, wheel_y );
            for( auto it = item_vec.begin(); it != item_vec.end(); ) {
                it->damage += rng( 0, 3 );
                if( it->damage > 4 ) {
                    it = item_vec.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    int last_turn_dec = 1;
    if (veh->last_turn < 0) {
        veh->last_turn += last_turn_dec;
        if (veh->last_turn > -last_turn_dec) { veh->last_turn = 0; }
    } else if (veh->last_turn > 0) {
        veh->last_turn -= last_turn_dec;
        if (veh->last_turn < last_turn_dec) { veh->last_turn = 0; }
    }

    if (can_move) {
        // accept new direction
        if (veh->skidding) {
            veh->face.init (veh->turn_dir);
            if(pl_ctrl) {
                veh->possibly_recover_from_skid();
            }
        } else {
            veh->face = mdir;
        }
        veh->move = mdir;
        if (coll_turn) {
            veh->skidding = true;
            veh->turn (coll_turn);
        }
        // accept new position
        // if submap changed, we need to process grid from the beginning.
        displace_vehicle (x, y, dx, dy);
    } else { // can_move
        veh->stop();
    }
    // If the PC is in the currently moved vehicle, adjust the
    // view offset.
    if (g->u.controlling_vehicle && veh_at(g->u.posx, g->u.posy) == veh) {
        g->calc_driving_offset(veh);
    }
    // redraw scene
    g->draw();
    return true;
}

bool map::displace_water (const int x, const int y)
{
    if (has_flag("SWIMMABLE", x, y) && !has_flag(TFLAG_DEEP_WATER, x, y)) // shallow water
    { // displace it
        int dis_places = 0, sel_place = 0;
        for (int pass = 0; pass < 2; pass++)
        { // we do 2 passes.
        // first, count how many non-water places around
        // then choose one within count and fill it with water on second pass
            if (pass)
            {
                sel_place = rng (0, dis_places - 1);
                dis_places = 0;
            }
            for (int tx = -1; tx <= 1; tx++)
                for (int ty = -1; ty <= 1; ty++)
                {
                    if ((!tx && !ty)
                            || move_cost_ter_furn(x + tx, y + ty) == 0
                            || has_flag(TFLAG_DEEP_WATER, x + tx, y + ty))
                        continue;
                    ter_id ter0 = ter (x + tx, y + ty);
                    if (ter0 == t_water_sh ||
                        ter0 == t_water_dp)
                        continue;
                    if (pass && dis_places == sel_place)
                    {
                       ter_set(x + tx, y + ty, t_water_sh);
                       ter_set(x, y, t_dirt);
                        return true;
                    }
                    dis_places++;
                }
        }
    }
    return false;
}

void map::set(const int x, const int y, const ter_id new_terrain, const furn_id new_furniture)
{
    furn_set(x, y, new_furniture);
    ter_set(x, y, new_terrain);
}

void map::set(const int x, const int y, const std::string new_terrain, const std::string new_furniture) {
    furn_set(x, y, new_furniture);
    ter_set(x, y, new_terrain);
}

std::string map::name(const int x, const int y)
{
 return has_furn(x, y) ? furnlist[furn(x, y)].name : terlist[ter(x, y)].name;
}

bool map::has_furn(const int x, const int y)
{
  return furn(x, y) != f_null;
}


std::string map::get_furn(const int x, const int y) const {
    return furnlist[ furn(x,y) ].id;
}

furn_t & map::furn_at(const int x, const int y) const
{
    return furnlist[ furn(x,y) ];
}

furn_id map::furn(const int x, const int y) const
{
 if (!INBOUNDS(x, y)) {
  return f_null;
 }

 int lx, ly;
 submap * const current_submap = get_submap_at(x, y, lx, ly);

 return current_submap->get_furn(lx, ly);
}


void map::furn_set(const int x, const int y, const furn_id new_furniture)
{
 if (!INBOUNDS(x, y)) {
  return;
 }

 int lx, ly;
 submap * const current_submap = get_submap_at(x, y, lx, ly);

 // set the dirty flags
 // TODO: consider checking if the transparency value actually changes
 set_transparency_cache_dirty();
 current_submap->set_furn(lx, ly, new_furniture);
}

void map::furn_set(const int x, const int y, const std::string new_furniture) {
    if ( furnmap.find(new_furniture) == furnmap.end() ) {
        return;
    }
    furn_set(x, y, (furn_id)furnmap[ new_furniture ].loadid );
}

bool map::can_move_furniture( const int x, const int y, player * p ) {
    furn_t furniture_type = furn_at(x, y);
    int required_str = furniture_type.move_str_req;

    // Object can not be moved (or nothing there)
    if (required_str < 0) { return false; }

    if( p != NULL && p->str_cur < required_str ) { return false; }

    return true;
}

std::string map::furnname(const int x, const int y) {
 return furnlist[furn(x, y)].name;
}
/*
 * Get the terrain integer id. This is -not- a number guaranteed to remain
 * the same across revisions; it is a load order, and can change when mods
 * are loaded or removed. The old t_floor style constants will still work but
 * are -not- guaranteed; if a mod removes t_lava, t_lava will equal t_null;
 * New terrains added to the core game generally do not need this, it's
 * retained for high performance comparisons, save/load, and gradual transition
 * to string terrain.id
 */
ter_id map::ter(const int x, const int y) const {
 if (!INBOUNDS(x, y)) {
  return t_null;
 }

 int lx, ly;
 submap * const current_submap = get_submap_at(x, y, lx, ly);

 return current_submap->ter[lx][ly];
}

/*
 * Get the terrain string id. This will remain the same across revisions,
 * unless a mod eliminates or changes it. Generally this is less efficient
 * than ter_id, but only an issue if thousands of comparisons are made.
 */
std::string map::get_ter(const int x, const int y) const {
    return terlist[ ter(x,y) ].id;
}

/*
 * Get the terrain harvestable string (what will get harvested from the terrain)
 */
std::string map::get_ter_harvestable(const int x, const int y) const {
    return terlist[ ter(x,y) ].harvestable;
}

/*
 * Get the terrain transforms_into id (what will the terrain transforms into)
 */
ter_id map::get_ter_transforms_into(const int x, const int y) const {
    return (ter_id)termap[ terlist[ ter(x,y) ].transforms_into ].loadid;
}

/*
 * Get the harvest season from the terrain
 */
int map::get_ter_harvest_season(const int x, const int y) const {
    return terlist[ ter(x,y) ].harvest_season;
}

/*
 * Get a reference to the actual terrain struct.
 */
ter_t & map::ter_at(const int x, const int y) const
{
    return terlist[ ter(x,y) ];
}

/*
 * set terrain via string; this works for -any- terrain id
 */
void map::ter_set(const int x, const int y, const std::string new_terrain) {
    if ( termap.find(new_terrain) == termap.end() ) {
        return;
    }
    ter_set(x, y, (ter_id)termap[ new_terrain ].loadid );
}

/*
 * set terrain via builtin t_keyword; only if defined, and will not work
 * for mods
 */
void map::ter_set(const int x, const int y, const ter_id new_terrain) {
    if (!INBOUNDS(x, y)) {
        return;
    }

    // set the dirty flags
    // TODO: consider checking if the transparency value actually changes
    set_transparency_cache_dirty();
    set_outside_cache_dirty();

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);
    current_submap->ter[lx][ly] = new_terrain;
}

std::string map::tername(const int x, const int y) const
{
 return terlist[ter(x, y)].name;
}

std::string map::features(const int x, const int y)
{
    // This is used in an info window that is 46 characters wide, and is expected
    // to take up one line.  So, make sure it does that.
    // FIXME: can't control length of localized text.
    // Make the caller wrap properly, if it does not already.
    std::string ret;
    if (is_bashable(x, y)) {
        ret += _("Smashable. ");
    }
    if (has_flag("DIGGABLE", x, y)) {
        ret += _("Diggable. ");
    }
    if (has_flag("ROUGH", x, y)) {
        ret += _("Rough. ");
    }
    if (has_flag("UNSTABLE", x, y)) {
        ret += _("Unstable. ");
    }
    if (has_flag("SHARP", x, y)) {
        ret += _("Sharp. ");
    }
    if (has_flag("FLAT", x, y)) {
        ret += _("Flat. ");
    }
    return ret;
}

int map::move_cost(const int x, const int y, const vehicle *ignored_vehicle) const
{
    int cost = move_cost_ter_furn(x, y); // covers !inbounds check too
    if ( cost == 0 ) {
        return 0;
    }
    if (veh_in_active_range && veh_exists_at[x][y]) {
        std::map< std::pair<int, int>, std::pair<vehicle *, int> >::const_iterator it;
        if ((it = veh_cached_parts.find( std::make_pair(x, y) )) != veh_cached_parts.end()) {
            const int vpart = it->second.second;
            vehicle *veh = it->second.first;
            if (veh != ignored_vehicle) {  // moving past vehicle cost
                const int dpart = veh->part_with_feature(vpart, VPFLAG_OBSTACLE);
                if (dpart >= 0 && (!veh->part_flag(dpart, VPFLAG_OPENABLE) || !veh->parts[dpart].open)) {
                    return 0;
                } else {
                    const int ipart = veh->part_with_feature(vpart, VPFLAG_AISLE);
                    if (ipart >= 0) {
                        return 2;
                    }
                    return 8;
                }
            }
        }
    }
//    cost+= field_at(x,y).move_cost(); // <-- unimplemented in all cases
    return cost > 0 ? cost : 0;
}

int map::move_cost_ter_furn(const int x, const int y) const
{
    if (!INBOUNDS(x, y)) {
        return 0;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    const int tercost = terlist[ current_submap->ter[lx][ly] ].movecost;
    if ( tercost == 0 ) {
        return 0;
    }
    const int furncost = furnlist[ current_submap->get_furn(lx, ly) ].movecost;
    if ( furncost < 0 ) {
        return 0;
    }
    const int cost = tercost + furncost;
    return cost > 0 ? cost : 0;
}

int map::combined_movecost(const int x1, const int y1,
                           const int x2, const int y2,
                           const vehicle *ignored_vehicle, const int modifier)
{
    int cost1 = move_cost(x1, y1, ignored_vehicle);
    int cost2 = move_cost(x2, y2, ignored_vehicle);
    // 50 moves taken per move_cost (70.71.. diagonally)
    int mult = (trigdist && x1 != x2 && y1 != y2 ? 71 : 50);
    return (cost1 + cost2 + modifier) * mult / 2;
}

bool map::trans(const int x, const int y)
{
    // Control statement is a problem. Normally returning false on an out-of-bounds
    // is how we stop rays from going on forever.  Instead we'll have to include
    // this check in the ray loop.
    int vpart = -1;
    vehicle *veh = veh_at(x, y, vpart);
    bool tertr;
    if (veh) {
        tertr = veh->part_with_feature(vpart, VPFLAG_OPAQUE) < 0;
        if (!tertr) {
            const int dpart = veh->part_with_feature(vpart, VPFLAG_OPENABLE);
            if (dpart >= 0 && veh->parts[dpart].open) {
                tertr = true; // open opaque door
            }
        }
    } else {
        tertr = has_flag_ter_and_furn(TFLAG_TRANSPARENT, x, y);
    }
    if( tertr ) {
        // Fields may obscure the view, too
        field &curfield = field_at( x,y );
        if( curfield.fieldCount() > 0 ) {
            field_entry *cur = NULL;
            for( auto field_list_it = curfield.getFieldStart();
                 field_list_it != curfield.getFieldEnd(); ++field_list_it ) {
                cur = field_list_it->second;
                if( cur == NULL ) {
                    continue;
                }
                //If ANY field blocks vision, the tile does.
                if(!fieldlist[cur->getFieldType()].transparent[cur->getFieldDensity() - 1]) {
                    return false;
                }
            }
        }
        return true; //no blockers found, this is transparent
    }
    return false; //failsafe block vision
}

bool map::has_flag(const std::string &flag, const int x, const int y) const
{
    static const std::string flag_str_REDUCE_SCENT("REDUCE_SCENT"); // construct once per runtime, slash delay 90%
    if (!INBOUNDS(x, y)) {
        return false;
    }
    // veh_at const no bueno
    if (veh_in_active_range && veh_exists_at[x][y] && flag_str_REDUCE_SCENT == flag) {
        std::map< std::pair<int, int>, std::pair<vehicle *, int> >::const_iterator it;
        if ((it = veh_cached_parts.find( std::make_pair(x, y) )) != veh_cached_parts.end()) {
            const int vpart = it->second.second;
            vehicle *veh = it->second.first;
            if (veh->parts[vpart].hp > 0 && // if there's a vehicle part here...
                veh->part_with_feature (vpart, VPFLAG_OBSTACLE) >= 0) {// & it is obstacle...
                const int p = veh->part_with_feature (vpart, VPFLAG_OPENABLE);
                if (p < 0 || !veh->parts[p].open) { // and not open door
                    return true;
                }
            }
        }
    }
    return has_flag_ter_or_furn(flag, x, y);
}

bool map::can_put_items(const int x, const int y)
{
    return !has_flag("NOITEM", x, y) && !has_flag("SEALED", x, y);
}

bool map::has_flag_ter(const std::string & flag, const int x, const int y) const
{
 return terlist[ter(x, y)].has_flag(flag);
}

bool map::has_flag_furn(const std::string & flag, const int x, const int y) const
{
 return furnlist[furn(x, y)].has_flag(flag);
}

bool map::has_flag_ter_or_furn(const std::string & flag, const int x, const int y) const
{
    if (!INBOUNDS(x, y)) {
        return false;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    return ( terlist[ current_submap->ter[lx][ly] ].has_flag(flag) || furnlist[ current_submap->get_furn(lx, ly) ].has_flag(flag) );
}

bool map::has_flag_ter_and_furn(const std::string & flag, const int x, const int y) const
{
 return terlist[ter(x, y)].has_flag(flag) && furnlist[furn(x, y)].has_flag(flag);
}
/////
bool map::has_flag(const ter_bitflags flag, const int x, const int y) const
{
    if (!INBOUNDS(x, y)) {
        return false;
    }
    // veh_at const no bueno
    if (veh_in_active_range && veh_exists_at[x][y] && flag == TFLAG_REDUCE_SCENT) {
        std::map< std::pair<int, int>, std::pair<vehicle *, int> >::const_iterator it;
        if ((it = veh_cached_parts.find( std::make_pair(x, y) )) != veh_cached_parts.end()) {
            const int vpart = it->second.second;
            vehicle *veh = it->second.first;
            if (veh->parts[vpart].hp > 0 && // if there's a vehicle part here...
                veh->part_with_feature (vpart, VPFLAG_OBSTACLE) >= 0) {// & it is obstacle...
                const int p = veh->part_with_feature (vpart, VPFLAG_OPENABLE);
                if (p < 0 || !veh->parts[p].open) { // and not open door
                    return true;
                }
            }
        }
    }
    return has_flag_ter_or_furn(flag, x, y);
}

bool map::has_flag_ter(const ter_bitflags flag, const int x, const int y) const
{
 return terlist[ter(x, y)].has_flag(flag);
}

bool map::has_flag_furn(const ter_bitflags flag, const int x, const int y) const
{
 return furnlist[furn(x, y)].has_flag(flag);
}

bool map::has_flag_ter_or_furn(const ter_bitflags flag, const int x, const int y) const
{
    if (!INBOUNDS(x, y)) {
        return false;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    return ( terlist[ current_submap->ter[lx][ly] ].has_flag(flag) || furnlist[ current_submap->get_furn(lx, ly) ].has_flag(flag) );
}

bool map::has_flag_ter_and_furn(const ter_bitflags flag, const int x, const int y) const
{
    if (!INBOUNDS(x, y)) {
        return false;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    return terlist[ current_submap->ter[lx][ly] ].has_flag(flag) && furnlist[ current_submap->get_furn(lx, ly) ].has_flag(flag);
}

/////
bool map::is_bashable(const int x, const int y)
{
    if (!inbounds(x, y)) {
        DebugLog( D_WARNING, D_MAP ) << "Looking for out-of-bounds is_bashable at "
                                     << x << ", " << y;
        return false;
    }

    if (veh_in_active_range && veh_exists_at[x][y]) {
        std::map< std::pair<int, int>, std::pair<vehicle *, int> >::const_iterator it;
        if ((it = veh_cached_parts.find( std::make_pair(x, y) )) != veh_cached_parts.end()) {
            const int vpart = it->second.second;
            vehicle *veh = it->second.first;
            if (veh->parts[vpart].hp > 0 && // if there's a vehicle part here...
                veh->part_with_feature (vpart, VPFLAG_OBSTACLE) >= 0) {// & it is obstacle...
                const int p = veh->part_with_feature (vpart, VPFLAG_OPENABLE);
                if (p < 0 || !veh->parts[p].open) { // and not open door
                    return true;
                }
            }
        }
    }
    if ( has_furn(x, y) && furn_at(x, y).bash.str_max != -1 ) {
        return true;
    } else if ( ter_at(x, y).bash.str_max != -1 ) {
        return true;
    }
    return false;
}

bool map::is_bashable_ter(const int x, const int y)
{
    if ( ter_at(x, y).bash.str_max != -1 ) {
        return true;
    }
    return false;
}

bool map::is_bashable_furn(const int x, const int y)
{
    if ( has_furn(x, y) && furn_at(x, y).bash.str_max != -1 ) {
        return true;
    }
    return false;
}

bool map::is_bashable_ter_furn(const int x, const int y)
{
    return is_bashable_furn(x, y) || is_bashable_ter(x, y);
}

int map::bash_strength(const int x, const int y)
{
    if ( has_furn(x, y) && furn_at(x, y).bash.str_max != -1 ) {
        return furn_at(x, y).bash.str_max;
    } else if ( ter_at(x, y).bash.str_max != -1 ) {
        return ter_at(x, y).bash.str_max;
    }
    return -1;
}

int map::bash_resistance(const int x, const int y)
{
    if ( has_furn(x, y) && furn_at(x, y).bash.str_min != -1 ) {
        return furn_at(x, y).bash.str_min;
    } else if ( ter_at(x, y).bash.str_min != -1 ) {
        return ter_at(x, y).bash.str_min;
    }
    return -1;
}

int map::bash_rating(const int str, const int x, const int y)
{
    if (!is_bashable(x,y)) {
        return -1;
    }
    bool furn_smash = false;
    bool ter_smash = false;
    if ( has_furn(x, y) && furn_at(x, y).bash.str_max != -1 ) {
        furn_smash = true;
    } else if ( ter_at(x, y).bash.str_max != -1 ) {
        ter_smash = true;
    }

    if (!furn_smash && !ter_smash) {
    //There must be a vehicle there!
        return 10;
    }

    int bash_min = 0;
    int bash_max = 0;
    if (furn_smash) {
        bash_min = furn_at(x, y).bash.str_min;
        bash_max = furn_at(x, y).bash.str_max;
    } else {
        bash_min = ter_at(x, y).bash.str_min;
        bash_max = ter_at(x, y).bash.str_max;
    }
    if (str < bash_min) {
        return 0;
    } else if (str >= bash_max) {
        return 10;
    }

    return (10 * (str - bash_min)) / (bash_max - bash_min);
}

void map::make_rubble(const int x, const int y, furn_id rubble_type, bool items, ter_id floor_type, bool overwrite)
{
    if (overwrite) {
        ter_set(x, y, floor_type);
        furn_set(x, y, rubble_type);
    } else {
        // First see if there is existing furniture to destroy
        if (is_bashable_furn(x, y)) {
            destroy_furn(x, y, true);
        }
        // Leave the terrain alone unless it interferes with furniture placement
        if (move_cost(x, y) <= 0 && is_bashable_ter(x, y)) {
            destroy(x, y, true);
        }
        // Check again for new terrain after potential destruction
        if (move_cost(x, y) <= 0) {
            ter_set(x, y, floor_type);
        }

        furn_set(x, y, rubble_type);
    }
    if (items) {
        //Still hardcoded, but a step up from the old stuff due to being in only one place
        if (rubble_type == f_wreckage) {
            item chunk("steel_chunk", calendar::turn);
            item scrap("scrap", calendar::turn);
            item pipe("pipe", calendar::turn);
            item wire("wire", calendar::turn);
            add_item_or_charges(x, y, chunk);
            add_item_or_charges(x, y, scrap);
            if (one_in(5)) {
                add_item_or_charges(x, y, pipe);
                add_item_or_charges(x, y, wire);
            }
        } else if (rubble_type == f_rubble_rock) {
            item rock("rock", calendar::turn);
            int rock_count = rng(1, 3);
            for (int i = 0; i < rock_count; i++) {
                add_item_or_charges(x, y, rock);
            }
        } else if (rubble_type == f_rubble) {
            item splinter("splinter", calendar::turn);
            item nail("nail", calendar::turn);
            int splinter_count = rng(2, 8);
            int nail_count = rng(5, 10);
            for (int i = 0; i < splinter_count; i++) {
                add_item_or_charges(x, y, splinter);
            }
            for (int i = 0; i < nail_count; i++) {
                add_item_or_charges(x, y, nail);
            }
        }
    }
}

/**
 * Returns whether or not the terrain at the given location can be dived into
 * (by monsters that can swim or are aquatic or nonbreathing).
 * @param x The x coordinate to look at.
 * @param y The y coordinate to look at.
 * @return true if the terrain can be dived into; false if not.
 */
bool map::is_divable(const int x, const int y)
{
  return has_flag("SWIMMABLE", x, y) && has_flag(TFLAG_DEEP_WATER, x, y);
}

bool map::is_outside(const int x, const int y)
{
 if(!INBOUNDS(x, y))
  return true;

 return outside_cache[x][y];
}

// MATERIALS-TODO: Use fire resistance
bool map::flammable_items_at(const int x, const int y)
{
    for (auto &i : i_at(x, y)) {
        int vol = i.volume();
        if (i.made_of("paper") || i.made_of("powder") ||
              i.type->id == "whiskey" || i.type->id == "vodka" ||
              i.type->id == "rum" || i.type->id == "tequila") {
            return true;
        }
        if ((i.made_of("wood") || i.made_of("veggy")) && (i.burnt < 1 || vol <= 10)) {
            return true;
        }
        if (i.made_of("cotton") && (vol <= 5 || i.burnt < 1)) {
            return true;
        }
        if (i.is_ammo() && i.ammo_type() != "battery" &&
              i.ammo_type() != "nail" && i.ammo_type() != "BB" &&
              i.ammo_type() != "bolt" && i.ammo_type() != "arrow" &&
              i.ammo_type() != "pebble" && i.ammo_type() != "fishspear" &&
              i.ammo_type() != "NULL") {
            return true;
        }
    }
    return false;
}

bool map::moppable_items_at(const int x, const int y)
{
    for (auto &i : i_at(x, y)) {
        if (i.made_of(LIQUID)) {
            return true;
        }
    }
    field &fld = field_at(x, y);
    if(fld.findField(fd_blood) != 0 || fld.findField(fd_blood_veggy) != 0 ||
          fld.findField(fd_blood_insect) != 0 || fld.findField(fd_blood_invertebrate) != 0
          || fld.findField(fd_bile) != 0 || fld.findField(fd_slime) != 0 ||
          fld.findField(fd_sludge) != 0) {
        return true;
    }
    int vpart;
    vehicle *veh = veh_at(x, y, vpart);
    if(veh != 0) {
        std::vector<int> parts_here = veh->parts_at_relative(veh->parts[vpart].mount_dx, veh->parts[vpart].mount_dy);
        for(auto &i : parts_here) {
            if(veh->parts[i].blood > 0) {
                return true;
            }
        }
    }
    return false;
}

point map::random_outdoor_tile()
{
 std::vector<point> options;
 for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
   if (is_outside(x, y))
    options.push_back(point(x, y));
  }
 }
 if (options.empty()) // Nowhere is outdoors!
  return point(-1, -1);

 return options[rng(0, options.size() - 1)];
}

bool map::has_adjacent_furniture(const int x, const int y)
{
    const signed char cx[4] = { 0, -1, 0, 1};
    const signed char cy[4] = {-1,  0, 1, 0};

    for (int i = 0; i < 4; i++)
    {
        const int adj_x = x + cx[i];
        const int adj_y = y + cy[i];
        if ( has_furn(adj_x, adj_y) && furn_at(adj_x, adj_y).has_flag("BLOCKSDOOR") ) {
            return true;
        }
    }

 return false;
}

bool map::has_nearby_fire(int x, int y, int radius)
{
    for(int dx = -radius; dx <= radius; dx++) {
        for(int dy = -radius; dy <= radius; dy++) {
            const point p(x + dx, y + dy);
            if (field_at(p.x, p.y).findField(fd_fire) != 0) {
                return true;
            }
            if (ter(p.x, p.y) == t_lava) {
                return true;
            }
        }
    }
    return false;
}

void map::mop_spills(const int x, const int y) {
    for (size_t i = 0; i < i_at(x, y).size(); i++) {
        item *it = &(i_at(x, y)[i]);
        if (it->made_of(LIQUID)) {
            i_rem(x, y, i);
            i--;
        }
    }
    field &fld = field_at(x, y);
    fld.removeField(fd_blood);
    fld.removeField(fd_blood_veggy);
    fld.removeField(fd_blood_insect);
    fld.removeField(fd_blood_invertebrate);
    fld.removeField(fd_bile);
    fld.removeField(fd_slime);
    fld.removeField(fd_sludge);
    int vpart;
    vehicle *veh = veh_at(x, y, vpart);
    if(veh != 0) {
        std::vector<int> parts_here = veh->parts_at_relative(veh->parts[vpart].mount_dx, veh->parts[vpart].mount_dy);
        for(size_t i = 0; i < parts_here.size(); i++) {
            veh->parts[parts_here[i]].blood = 0;
        }
    }
}

void map::create_spores(const int x, const int y, Creature* source)
{
    // TODO: Infect NPCs?
    monster spore(GetMType("mon_spore"));
    int mondex;
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            mondex = g->mon_at(i, j);
            if (move_cost(i, j) > 0 || (i == x && j == y)) {
                if (mondex != -1) { // Spores hit a monster
                    if (g->u_see(i, j) &&
                        !g->zombie(mondex).type->in_species("FUNGUS")) {
                        add_msg(_("The %s is covered in tiny spores!"),
                                g->zombie(mondex).name().c_str());
                    }
                    monster &critter = g->zombie( mondex );
                    if( !critter.make_fungus() ) {
                        critter.die( source ); // counts as kill by player
                    }
                } else if (g->u.posx == i && g->u.posy == j) {
                    // Spores hit the player
                    bool hit = false;
                    if (one_in(4) && g->u.infect("spores", bp_head, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(2) && g->u.infect("spores", bp_torso, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_arm_l, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_arm_r, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_leg_l, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_leg_r, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (hit) {
                        add_msg(m_warning, _("You're covered in tiny spores!"));
                    }
                } else if (((i == x && j == y) || one_in(4)) &&
                           g->num_zombies() <= 1000) { // Spawn a spore
                    spore.spawn(i, j);
                    g->add_zombie(spore);
                }
            }
        }
    }
}

int map::collapse_check(const int x, const int y)
{
    int num_supports = 0;
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            if (i == x && j == y) {
                continue;
            }
            if (has_flag("COLLAPSES", x, y)) {
                if (has_flag("COLLAPSES", i, j)) {
                    num_supports++;
                } else if (has_flag("SUPPORTS_ROOF", i, j)) {
                    num_supports += 2;
                }
            } else if (has_flag("SUPPORTS_ROOF", x, y)) {
                if (has_flag("SUPPORTS_ROOF", i, j) && !has_flag("COLLAPSES", i, j)) {
                    num_supports += 3;
                }
            }
        }
    }
    return 1.7 * num_supports;
}

void map::collapse_at(const int x, const int y)
{
    destroy (x, y, false);
    crush(x, y);
    make_rubble(x, y);
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            if ((i == x && j == y)) {
                continue;
            }
            if (has_flag("COLLAPSES", i, j) && one_in(collapse_check(i, j))) {
                destroy (i, j, false);
            // We only check for rubble spread if it doesn't already collapse to prevent double crushing
            } else if (has_flag("FLAT", i, j) && one_in(8)) {
                crush(i, j);
                make_rubble(i, j);
            }
        }
    }
}

std::pair<bool, bool> map::bash(const int x, const int y, const int str,
                                bool silent, bool destroy, vehicle *bashing_vehicle )
{
    bool success = false;
    int sound_volume = 0;
    std::string sound;
    bool smashed_something = false;
    if (field_at(x, y).findField(fd_web)) {
        smashed_something = true;
        remove_field(x, y, fd_web);
    }

    // Destroy glass items, spilling their contents.
    std::vector<item> smashed_contents;
    std::vector<item> &bashed_items = i_at(x, y);
    for( auto bashed_item = bashed_items.begin(); bashed_item != bashed_items.end(); ) {
        // the check for active supresses molotovs smashing themselves with their own explosion
        if (bashed_item->made_of("glass") && !bashed_item->active && one_in(2)) {
            sound = _("glass shattering");
            sound_volume = 12;
            smashed_something = true;
            for( auto bashed_content : bashed_item->contents ) {
                smashed_contents.push_back( bashed_content );
            }
            bashed_item = bashed_items.erase( bashed_item );
        } else {
            ++bashed_item;
        }
    }
    // Now plunk in the contents of the smashed items.
    bashed_items.insert( bashed_items.end(), smashed_contents.begin(), smashed_contents.end() );

    // Smash vehicle if present
    int vpart;
    vehicle *veh = veh_at(x, y, vpart);
    if (veh && veh != bashing_vehicle) {
        veh->damage (vpart, str, 1);
        sound = _("crash!");
        sound_volume = 18;
        smashed_something = true;
        success = true;
    } else {
        // Else smash furniture or terrain
        bool smash_furn = false;
        bool smash_ter = false;
        map_bash_info *bash = NULL;

        if ( has_furn(x, y) && furn_at(x, y).bash.str_max != -1 ) {
            bash = &(furn_at(x,y).bash);
            smash_furn = true;
        } else if ( ter_at(x, y).bash.str_max != -1 ) {
            bash = &(ter_at(x,y).bash);
            smash_ter = true;
        }
        if (has_flag("ALARMED", x, y) && !g->event_queued(EVENT_WANTED)) {
            g->sound(x, y, 40, _("An alarm sounds!"));
            g->u.add_memorial_log(pgettext("memorial_male", "Set off an alarm."),
                                  pgettext("memorial_female", "Set off an alarm."));
            g->add_event(EVENT_WANTED, int(calendar::turn) + 300, 0, g->get_abs_levx(), g->get_abs_levy());
        }

        if ( bash != NULL && (!bash->destroy_only || destroy)) {
            int smin = bash->str_min;
            int smax = bash->str_max;
            if (destroy) {
                success = true;
            } else {
                if ( bash->str_min_blocked != -1 || bash->str_max_blocked != -1 ) {
                    if( has_adjacent_furniture(x, y) ) {
                        if ( bash->str_min_blocked != -1 ) {
                            smin = bash->str_min_blocked;
                        }
                        if ( bash->str_max_blocked != -1 ) {
                            smax = bash->str_max_blocked;
                        }
                    }
                }
                if ( str >= smin && str >= rng(bash->str_min_roll, bash->str_max_roll)) {
                    success = true;
                }
            }

            if (success || destroy) {
                // Clear out any partially grown seeds
                if (has_flag_ter_or_furn("PLANT", x, y)) {
                    for (size_t i = 0; i < i_at(x, y).size(); i++) {
                        i_rem(x, y, i);
                    }
                }

                if (smash_furn) {
                    if (has_flag_furn("FUNGUS", x, y)) {
                        create_spores(x, y);
                    }
                } else if (smash_ter) {
                    if (has_flag_ter("FUNGUS", x, y)) {
                        create_spores(x, y);
                    }
                }

                if (destroy) {
                    sound_volume = smax;
                } else {
                    sound_volume = std::min(int(smin * 1.5), smax);
                }
                sound = _(bash->sound.c_str());
                // Set this now in case the ter_set below changes this
                bool collapses = has_flag("COLLAPSES", x, y) && smash_ter;
                bool supports = has_flag("SUPPORTS_ROOF", x, y) && smash_ter;
                if (smash_furn == true) {
                    furn_set(x, y, bash->furn_set);
                    // Hack alert.
                    // Signs have cosmetics associated with them on the submap since
                    // furniture can't store dynamic data to disk. To prevent writing
                    // mysteriously appearing for a sign later built here, remove the
                    // writing from the submap.
                    g->m.delete_signage(x, y);
                } else if (smash_ter == true) {
                    ter_set(x, y, bash->ter_set);
                } else {
                    debugmsg( "data/json/terrain.json does not have %s.bash.ter_set set!",
                              ter_at(x,y).id.c_str() );
                }

                spawn_item_list(bash->items, x, y);
                if (bash->explosive > 0) {
                    g->explosion(x, y, bash->explosive, 0, false);
                }

                if (collapses) {
                    collapse_at(x, y);
                }
                // Check the flag again to ensure the new terrain doesn't support anything
                if (supports && !has_flag("SUPPORTS_ROOF", x, y)) {
                    for (int i = x - 1; i <= x + 1; i++) {
                        for (int j = y - 1; j <= y + 1; j++) {
                            if ((i == x && j == y) || !has_flag("COLLAPSES", i, j)) {
                                continue;
                            }
                            if (one_in(collapse_check(i, j))) {
                                collapse_at(i, j);
                            }
                        }
                    }
                }
                smashed_something = true;
            } else {
                sound_volume = 12;
                sound = _(bash->sound_fail.c_str());
                smashed_something = true;
            }
        } else {
            furn_id furnid = furn(x, y);
            if ( furnid == f_skin_wall || furnid == f_skin_door || furnid == f_skin_door_o ||
                 furnid == f_skin_groundsheet || furnid == f_canvas_wall || furnid == f_canvas_door ||
                 furnid == f_canvas_door_o || furnid == f_groundsheet || furnid == f_fema_groundsheet) {
                if (str >= rng(0, 6) || destroy) {
                    // Special code to collapse the tent if destroyed
                    int tentx = -1, tenty = -1;
                    // Find the center of the tent
                    for (int i = -1; i <= 1; i++) {
                        for (int j = -1; j <= 1; j++) {
                            if (furn(x + i, y + j) == f_groundsheet ||
                                furn(x + i, y + j) == f_fema_groundsheet ||
                                furn(x + i, y + j) == f_skin_groundsheet){
                                tentx = x + i;
                                tenty = y + j;
                                break;
                            }
                        }
                    }
                    // Never found tent center, bail out
                    if (tentx == -1 && tenty == -1) {
                        smashed_something = true;
                    }
                    // Take the tent down
                    for (int i = -1; i <= 1; i++) {
                        for (int j = -1; j <= 1; j++) {
                            if (furn(tentx + i, tenty + j) == f_groundsheet) {
                                spawn_item(tentx + i, tenty + j, "broketent");
                            }
                            if (furn(tentx + i, tenty + j) == f_skin_groundsheet) {
                                spawn_item(tentx + i, tenty + j, "damaged_shelter_kit");
                            }
                            furn_id check_furn = furn(tentx + i, tenty + j);
                            if (check_furn == f_skin_wall || check_furn == f_skin_door ||
                                  check_furn == f_skin_door_o || check_furn == f_skin_groundsheet ||
                                  check_furn == f_canvas_wall || check_furn == f_canvas_door ||
                                  check_furn == f_canvas_door_o || check_furn == f_groundsheet ||
                                  check_furn == f_fema_groundsheet) {
                                furn_set(tentx + i, tenty + j, f_null);
                            }
                        }
                    }

                    sound_volume = 8;
                    sound = _("rrrrip!");
                    smashed_something = true;
                    success = true;
                } else {
                    sound_volume = 8;
                    sound = _("slap!");
                    smashed_something = true;
                }
            // Made furniture seperate from the other tent to facilitate destruction
            } else if (furnid == f_center_groundsheet || furnid == f_large_groundsheet ||
                     furnid == f_large_canvas_door || furnid == f_large_canvas_wall ||
                     furnid == f_large_canvas_door_o) {
                if (str >= rng(0, 6) || destroy) {
                    // Special code to collapse the tent if destroyed
                    int tentx = -1, tenty = -1;
                    // Find the center of the tent
                    for (int i = -2; i <= 2; i++) {
                        for (int j = -2; j <= 2; j++) {
                            if (furn(x + i, y + j) == f_center_groundsheet){
                                tentx = x + i;
                                tenty = y + j;
                                break;
                            }
                        }
                    }
                    // Never found tent center, bail out
                    if (tentx == -1 && tenty == -1) {
                        smashed_something = true;
                    }
                    // Take the tent down
                    for (int i = -2; i <= 2; i++) {
                        for (int j = -2; j <= 2; j++) {
                             if (furn(tentx + i, tenty + j) == f_center_groundsheet) {
                             spawn_item(tentx + i, tenty + j, "largebroketent");
                            }
                            furn_set(tentx + i, tenty + j, f_null);
                        }
                    }
                    sound_volume = 8;
                    sound = _("rrrrip!");
                    smashed_something = true;
                    success = true;
                } else {
                    sound_volume = 8;
                    sound = _("slap!");
                    smashed_something = true;
                }
            }
        }
    }
    if( move_cost(x, y) <= 0  && !smashed_something ) {
        sound = _("thump!");
        sound_volume = 18;
        smashed_something = true;
    }
    if( !sound.empty() && !silent) {
        g->sound( x, y, sound_volume, sound);
    }
    return std::pair<bool, bool> (smashed_something, success);
}

void map::spawn_item_list(const std::vector<map_bash_item_drop> &items, int x, int y) {
    for (size_t i = 0; i < items.size(); i++) {
        const map_bash_item_drop &drop = items[i];
        int chance = drop.chance;
        if ( chance == -1 || rng(0, 100) >= chance ) {
            int numitems = drop.amount;

            if ( drop.minamount != -1 ) {
                numitems = rng( drop.minamount, drop.amount );
            }
            if ( numitems > 0 ) {
                // spawn_item(x,y, drop.itemtype, numitems); // doesn't abstract amount || charges
                item new_item(drop.itemtype, calendar::turn);
                if ( new_item.count_by_charges() ) {
                    new_item.charges = numitems;
                    numitems = 1;
                }
                const bool varsize = new_item.has_flag( "VARSIZE" );
                for(int a = 0; a < numitems; a++ ) {
                    if( varsize && one_in( 3 ) ) {
                        new_item.item_tags.insert( "FIT" );
                    } else if( varsize ) {
                        // might have been added previously
                        new_item.item_tags.erase( "FIT" );
                    }
                    add_item_or_charges(x, y, new_item);
                }
            }
        }
    }
}

void map::destroy(const int x, const int y, const bool silent)
{
    // Break if it takes more than 25 destructions to remove to prevent infinite loops
    // Example: A bashes to B, B bashes to A leads to A->B->A->...
    int count = 0;
    while (count <= 25 && bash(x, y, 999, silent, true).second) {
        count++;
    }
}

void map::destroy_furn(const int x, const int y, const bool silent)
{
    // Break if it takes more than 25 destructions to remove to prevent infinite loops
    // Example: A bashes to B, B bashes to A leads to A->B->A->...
    int count = 0;
    while (count <= 25 && furn(x, y) != f_null && bash(x, y, 999, silent, true).second) {
        count++;
    }
}

void map::crush(const int x, const int y)
{
    int veh_part;
    player *crushed_player = nullptr;
    //The index of the NPC at (x,y), or -1 if there isn't one
    int npc_index = g->npc_at(x, y);
    if( g->u.posx == x && g->u.posy == y ) {
        crushed_player = &(g->u);
    } else if( npc_index != -1 ) {
        crushed_player = static_cast<player *>(g->active_npc[npc_index]);
    }

    if( crushed_player != nullptr ) {
        bool player_inside = false;
        if( crushed_player->in_vehicle ) {
            vehicle *veh = veh_at(x, y, veh_part);
            player_inside = (veh && veh->is_inside(veh_part));
        }
        if (!player_inside) { //If there's a player at (x,y) and he's not in a covered vehicle...
            //This is the roof coming down on top of us, no chance to dodge
            crushed_player->add_msg_player_or_npc( m_bad, _("You are crushed by the falling debris!"),
                                                   _("<npcname> is crushed by the falling debris!") );
            int dam = rng(0, 60);
            // Torso and head take the brunt of the blow
            body_part hit = bp_head;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .25 ) );
            hit = bp_torso;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .45 ) );
            // Legs take the next most through transfered force
            hit = bp_leg_l;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .10 ) );
            hit = bp_leg_r;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .10 ) );
            // Arms take the least
            hit = bp_arm_l;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .05 ) );
            hit = bp_arm_r;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .05 ) );

            // Add crushed pinning effect once effect processing is updated
        }
    }

    //The index of the monster at (x,y), or -1 if there isn't one
    int mon = g->mon_at(x, y);
    if (mon != -1 && size_t(mon) < g->num_zombies()) {  //If there's a monster at (x,y)...
        monster* monhit = &(g->zombie(mon));
        // 25 ~= 60 * .45 (torso)
        monhit->deal_damage(nullptr, bp_torso, damage_instance(DT_BASH, rng(0,25)));

        // Add crushed pinning effect once effect processing is updated
    }

    vehicle *veh = veh_at(x, y, veh_part);
    if (veh) {
        veh->damage(veh_part, rng(0, veh->parts[veh_part].hp), 1, false);
    }
}

void map::shoot(const int x, const int y, int &dam,
                const bool hit_items, const std::set<std::string>& ammo_effects)
{
    if (dam < 0)
    {
        return;
    }

    if (has_flag("ALARMED", x, y) && !g->event_queued(EVENT_WANTED))
    {
        g->sound(x, y, 30, _("An alarm sounds!"));
        g->add_event(EVENT_WANTED, int(calendar::turn) + 300, 0, g->get_abs_levx(), g->get_abs_levy());
    }

    int vpart;
    vehicle *veh = veh_at(x, y, vpart);
    if (veh)
    {
        const bool inc = (ammo_effects.count("INCENDIARY") || ammo_effects.count("FLAME"));
        dam = veh->damage (vpart, dam, inc? 2 : 0, hit_items);
    }

    ter_t terrain = ter_at(x, y);
    if( 0 == terrain.id.compare("t_wall_wood_broken") ||
        0 == terrain.id.compare("t_wall_log_broken") ||
        0 == terrain.id.compare("t_door_b") ) {
        if (hit_items || one_in(8)) { // 1 in 8 chance of hitting the door
            dam -= rng(20, 40);
            if (dam > 0) {
                g->sound(x, y, 10, _("crash!"));
                ter_set(x, y, t_dirt);
            }
        }
        else {
            dam -= rng(0, 1);
        }
    } else if( 0 == terrain.id.compare("t_door_c") ||
               0 == terrain.id.compare("t_door_locked") ||
               0 == terrain.id.compare("t_door_locked_peep") ||
               0 == terrain.id.compare("t_door_locked_alarm") ) {
        dam -= rng(15, 30);
        if (dam > 0) {
            g->sound(x, y, 10, _("smash!"));
            ter_set(x, y, t_door_b);
        }
    } else if( 0 == terrain.id.compare("t_door_boarded") ||
               0 == terrain.id.compare("t_door_boarded_damaged") ||
               0 == terrain.id.compare("t_rdoor_boarded") ||
               0 == terrain.id.compare("t_rdoor_boarded_damaged") ) {
        dam -= rng(15, 35);
        if (dam > 0) {
            g->sound(x, y, 10, _("crash!"));
            ter_set(x, y, t_door_b);
        }
    } else if( 0 == terrain.id.compare("t_window_domestic_taped") ||
               0 == terrain.id.compare("t_curtains") ) {
        if (ammo_effects.count("LASER")) {
            dam -= rng(1, 5);
        }
        if (ammo_effects.count("LASER")) {
            dam -= rng(0, 5);
        } else {
            dam -= rng(1,3);
            if (dam > 0) {
                g->sound(x, y, 16, _("glass breaking!"));
                ter_set(x, y, t_window_frame);
                spawn_item(x, y, "sheet", 1);
                spawn_item(x, y, "stick");
                spawn_item(x, y, "string_36");
            }
        }
    } else if( 0 == terrain.id.compare("t_window_domestic") ) {
        if (ammo_effects.count("LASER")) {
            dam -= rng(0, 5);
        } else {
            dam -= rng(1,3);
            if (dam > 0) {
                g->sound(x, y, 16, _("glass breaking!"));
                ter_set(x, y, t_window_frame);
                spawn_item(x, y, "sheet", 1);
                spawn_item(x, y, "stick");
                spawn_item(x, y, "string_36");
            }
        }
    } else if( 0 == terrain.id.compare("t_window_taped") ||
               0 == terrain.id.compare("t_window_alarm_taped") ) {
        if (ammo_effects.count("LASER")) {
            dam -= rng(1, 5);
        }
        if (ammo_effects.count("LASER")) {
            dam -= rng(0, 5);
        } else {
            dam -= rng(1,3);
            if (dam > 0) {
                g->sound(x, y, 16, _("glass breaking!"));
                ter_set(x, y, t_window_frame);
            }
        }
    } else if( 0 == terrain.id.compare("t_window") ||
               0 == terrain.id.compare("t_window_alarm") ) {
        if (ammo_effects.count("LASER")) {
            dam -= rng(0, 5);
        } else {
            dam -= rng(1,3);
            if (dam > 0) {
                g->sound(x, y, 16, _("glass breaking!"));
                ter_set(x, y, t_window_frame);
            }
        }
    } else if( 0 == terrain.id.compare("t_window_boarded") ) {
        dam -= rng(10, 30);
        if (dam > 0) {
            g->sound(x, y, 16, _("glass breaking!"));
            ter_set(x, y, t_window_frame);
        }
    } else if( 0 == terrain.id.compare("t_wall_glass_h") ||
               0 == terrain.id.compare("t_wall_glass_v") ||
               0 == terrain.id.compare("t_wall_glass_h_alarm") ||
               0 == terrain.id.compare("t_wall_glass_v_alarm") ) {
        if (ammo_effects.count("LASER")) {
            dam -= rng(0,5);
        } else {
            dam -= rng(1,8);
            if (dam > 0) {
                g->sound(x, y, 20, _("glass breaking!"));
                ter_set(x, y, t_floor);
            }
        }
    } else if( 0 == terrain.id.compare("t_reinforced_glass_v") ||
               0 == terrain.id.compare("t_reinforced_glass_h") ) {
        // reinforced glass stops most bullets
        // laser beams are attenuated
        if (ammo_effects.count("LASER")) {
            dam -= rng(0, 8);
        } else {
            //Greatly weakens power of bullets
            dam -= 40;
            if (dam <= 0) {
                add_msg(_("The shot is stopped by the reinforced glass wall!"));
            } else if (dam >= 40) {
                //high powered bullets penetrate the glass, but only extremely strong
                // ones (80 before reduction) actually destroy the glass itself.
                g->sound(x, y, 20, _("glass breaking!"));
                ter_set(x, y, t_floor);
            }
        }
    } else if( 0 == terrain.id.compare("t_paper") ) {
        dam -= rng(4, 16);
        if (dam > 0) {
            g->sound(x, y, 8, _("rrrrip!"));
            ter_set(x, y, t_dirt);
        }
        if (ammo_effects.count("INCENDIARY")) {
            add_field(x, y, fd_fire, 1);
        }
    } else if( 0 == terrain.id.compare("t_gas_pump") ) {
        if (hit_items || one_in(3)) {
            if (dam > 15) {
                if (ammo_effects.count("INCENDIARY") || ammo_effects.count("FLAME")) {
                    g->explosion(x, y, 40, 0, true);
                } else {
                    for (int i = x - 2; i <= x + 2; i++) {
                        for (int j = y - 2; j <= y + 2; j++) {
                            if (move_cost(i, j) > 0 && one_in(3)) {
                                    spawn_item(i, j, "gasoline");
                            }
                        }
                    }
                    g->sound(x, y, 10, _("smash!"));
                }
                ter_set(x, y, t_gas_pump_smashed);
            }
            dam -= 60;
        }
    } else if( 0 == terrain.id.compare("t_vat") ) {
        if (dam >= 10) {
            g->sound(x, y, 20, _("ke-rash!"));
            ter_set(x, y, t_floor);
        } else {
            dam = 0;
        }
    } else {
        if (move_cost(x, y) == 0 && !trans(x, y)) {
            dam = 0; // TODO: Bullets can go through some walls?
        } else {
            dam -= (rng(0, 1) * rng(0, 1) * rng(0, 1));
        }
    }

    if (ammo_effects.count("TRAIL") && !one_in(4)) {
        add_field(x, y, fd_smoke, rng(1, 2));
    }

    if (ammo_effects.count("STREAM") && !one_in(3)) {
        add_field(x, y, fd_fire, rng(1, 2));
    }

    if (ammo_effects.count("LIGHTNING")) {
        add_field(x, y, fd_electricity, rng(2, 3));
    }

    if (ammo_effects.count("PLASMA") && one_in(2)) {
        add_field(x, y, fd_plasma, rng(1, 2));
    }

    if (ammo_effects.count("LASER")) {
        add_field(x, y, fd_laser, 2);
    }

    // Set damage to 0 if it's less
    if (dam < 0) {
        dam = 0;
    }

    // Check fields?
    field_entry *fieldhit = field_at(x, y).findField(fd_web);
    if(fieldhit){
        if (ammo_effects.count("INCENDIARY") || ammo_effects.count("FLAME")) {
            add_field(x, y, fd_fire, fieldhit->getFieldDensity() - 1);
        } else if (dam > 5 + fieldhit->getFieldDensity() * 5 &&
                   one_in(5 - fieldhit->getFieldDensity())) {
            dam -= rng(1, 2 + fieldhit->getFieldDensity() * 2);
            remove_field(x, y,fd_web);
        }
    }

    // Now, destroy items on that tile.
    if ((move_cost(x, y) == 2 && !hit_items) || !INBOUNDS(x, y)) {
        return; // Items on floor-type spaces won't be shot up.
    }

    for (size_t i = 0; i < i_at(x, y).size(); i++) {
        bool destroyed = false;
        int chance = (i_at(x, y)[i].volume() > 0 ? i_at(x, y)[i].volume() : 1);
        // volume dependent chance

        if (dam > i_at(x, y)[i].bash_resist() && one_in(chance)) {
            i_at(x, y)[i].damage++;
        }
        if (i_at(x, y)[i].damage >= 5) {
            destroyed = true;
        }

        if (destroyed) {
            for (auto &j : i_at(x, y)[i].contents) {
                i_at(x, y).push_back(j);
            }
            i_rem(x, y, i);
            i--;
        }
    }
}

bool map::hit_with_acid( const int x, const int y )
{
    if( move_cost( x, y ) != 0 ) {
        return false;    // Didn't hit the tile!
    }
    const ter_id t = ter( x, y );
    if( t == t_wall_glass_v || t == t_wall_glass_h || t == t_wall_glass_v_alarm || t == t_wall_glass_h_alarm ||
        t == t_vat ) {
        ter_set( x, y, t_floor );
    } else if( t == t_door_c || t == t_door_locked || t == t_door_locked_peep || t == t_door_locked_alarm ) {
        if( one_in( 3 ) ) {
            ter_set( x, y, t_door_b );
        }
    } else if( t == t_door_bar_c || t == t_door_bar_o || t == t_door_bar_locked || t == t_bars ) {
        ter_set( x, y, t_floor );
        add_msg( m_warning, _( "The metal bars melt!" ) );
    } else if( t == t_door_b ) {
        if( one_in( 4 ) ) {
            ter_set( x, y, t_door_frame );
        } else {
            return false;
        }
    } else if( t == t_window || t == t_window_alarm ) {
        ter_set( x, y, t_window_empty );
    } else if( t == t_wax ) {
        ter_set( x, y, t_floor_wax );
    } else if( t == t_gas_pump || t == t_gas_pump_smashed ) {
        return false;
    } else if( t == t_card_science || t == t_card_military ) {
        ter_set( x, y, t_card_reader_broken );
    }
    return true;
}

// returns true if terrain stops fire
bool map::hit_with_fire(const int x, const int y)
{
    if (move_cost(x, y) != 0)
        return false; // Didn't hit the tile!

    // non passable but flammable terrain, set it on fire
    if (has_flag("FLAMMABLE", x, y) || has_flag("FLAMMABLE_ASH", x, y))
    {
        add_field(x, y, fd_fire, 3);
    }
    return true;
}

bool map::marlossify(const int x, const int y)
{
    if (one_in(25) && (terlist[ter(x, y)].movecost != 0 && !has_furn(x, y))
            && !ter_at(x, y).has_flag(TFLAG_DEEP_WATER)) {
        ter_set(x, y, t_marloss);
        return true;
    }
    for (int i = 0; i < 25; i++) {
        if(!g->spread_fungus(x, y)) {
            return true;
        }
    }
    return false;
}

bool map::open_door(const int x, const int y, const bool inside, const bool check_only)
{
 const std::string terid = get_ter(x,y);
 const std::string furnid = furnlist[furn(x,y)].id;
 if ( !termap[terid].open.empty() && termap[ terid ].open != "t_null" ) {
     if ( termap.find( termap[ terid ].open ) == termap.end() ) {
         debugmsg("terrain %s.open == non existant terrain '%s'\n", termap[ terid ].id.c_str(), termap[ terid ].open.c_str() );
         return false;
     }
     if ( has_flag("OPENCLOSE_INSIDE", x, y) && inside == false ) {
         return false;
     }
     if(!check_only) {
         ter_set(x, y, termap[ terid ].open );
     }
     return true;
 } else if ( !furnmap[furnid].open.empty() && furnmap[ furnid ].open != "t_null" ) {
     if ( furnmap.find( furnmap[ furnid ].open ) == furnmap.end() ) {
         debugmsg("terrain %s.open == non existant furniture '%s'\n", furnmap[ furnid ].id.c_str(), furnmap[ furnid ].open.c_str() );
         return false;
     }
     if ( has_flag("OPENCLOSE_INSIDE", x, y) && inside == false ) {
         return false;
     }
     if(!check_only) {
         furn_set(x, y, furnmap[ furnid ].open );
     }
     return true;
 }
 return false;
}

void map::translate(const ter_id from, const ter_id to)
{
 if (from == to) {
  debugmsg("map::translate %s => %s", terlist[from].name.c_str(),
                                      terlist[from].name.c_str());
  return;
 }
 for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
   if (ter(x, y) == from)
    ter_set(x, y, to);
  }
 }
}

//This function performs the translate function within a given radius of the player.
void map::translate_radius(const ter_id from, const ter_id to, float radi, int uX, int uY)
{
 if (from == to) {
  debugmsg("map::translate %s => %s", terlist[from].name.c_str(),
                                      terlist[from].name.c_str());
  return;
 }
 for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
   if (ter(x, y) == from){
    //float radiX = 0.0;
    float radiX = sqrt(float((uX-x)*(uX-x) + (uY-y)*(uY-y)));
    if (radiX <= radi){
      ter_set(x, y, to);}
    }
   }
  }
 }

bool map::close_door(const int x, const int y, const bool inside, const bool check_only)
{
 const std::string terid = get_ter(x,y);
 const std::string furnid = furnlist[furn(x,y)].id;
 if ( !termap[terid].close.empty() && termap[ terid ].close != "t_null" ) {
     if ( termap.find( termap[ terid ].close ) == termap.end() ) {
         debugmsg("terrain %s.close == non existant terrain '%s'\n", termap[ terid ].id.c_str(), termap[ terid ].close.c_str() );
         return false;
     }
     if ( has_flag("OPENCLOSE_INSIDE", x, y) && inside == false ) {
         return false;
     }
     if (!check_only) {
        ter_set(x, y, termap[ terid ].close );
     }
     return true;
 } else if ( !furnmap[furnid].close.empty() && furnmap[ furnid ].close != "t_null" ) {
     if ( furnmap.find( furnmap[ furnid ].close ) == furnmap.end() ) {
         debugmsg("terrain %s.close == non existant furniture '%s'\n", furnmap[ furnid ].id.c_str(), furnmap[ furnid ].close.c_str() );
         return false;
     }
     if ( has_flag("OPENCLOSE_INSIDE", x, y) && inside == false ) {
         return false;
     }
     if (!check_only) {
         furn_set(x, y, furnmap[ furnid ].close );
     }
     return true;
 }
 return false;
}

const std::string map::get_signage(const int x, const int y) const
{
    if (!INBOUNDS(x, y)) {
        return "";
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    return current_submap->get_signage(lx, ly);
}
void map::set_signage(const int x, const int y, std::string message) const
{
    if (!INBOUNDS(x, y)) {
        return;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    current_submap->set_signage(lx, ly, message);
}
void map::delete_signage(const int x, const int y) const
{
    if (!INBOUNDS(x, y)) {
        return;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    current_submap->delete_signage(lx, ly);
}

int map::get_radiation(const int x, const int y) const
{
    if (!INBOUNDS(x, y)) {
        return 0;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    return current_submap->get_radiation(lx, ly);
}

void map::set_radiation(const int x, const int y, const int value)
{
    if (!INBOUNDS(x, y)) {
        return;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    current_submap->set_radiation(lx, ly, value);
}

void map::adjust_radiation(const int x, const int y, const int delta)
{
    if (!INBOUNDS(x, y)) {
        return;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    int current_radiation = current_submap->get_radiation(lx, ly);
    current_submap->set_radiation(lx, ly, current_radiation + delta);
}

int& map::temperature(const int x, const int y)
{
 if (!INBOUNDS(x, y)) {
  null_temperature = 0;
  return null_temperature;
 }

 return get_submap_at(x, y)->temperature;
}

void map::set_temperature(const int x, const int y, int new_temperature)
{
    temperature(x, y) = new_temperature;
    temperature(x + SEEX, y) = new_temperature;
    temperature(x, y + SEEY) = new_temperature;
    temperature(x + SEEX, y + SEEY) = new_temperature;
}

item_stack map::item_stack_at(const int x, const int y)
{
    if (!INBOUNDS(x, y)) {
        return item_stack();
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    return item_stack(this, x, y, &(current_submap->itm[lx][ly]));
}

item_stack map::item_stack_at_vehicle(const int x, const int y)
{
    if (!INBOUNDS(x, y)) {
        return item_stack();
    }

    int veh_root_part = -1;
    vehicle *veh = veh_at( x, y, veh_root_part );
    int cargo_part = -1;
    if( veh != nullptr ) {
        cargo_part = veh->part_with_feature( veh_root_part, "CARGO", false );
    }

    if (cargo_part >= 0) {
        return item_stack(veh, cargo_part, &veh->parts[cargo_part].items);
    } else {
        return item_stack_at(x, y);
    }
}

std::vector<item>& map::i_at(int x, int y) {
    if (!INBOUNDS(x, y)) {
        nulitems.clear();
        return nulitems;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    return current_submap->itm[lx][ly];
}

itemslice map::i_stacked(std::vector<item>& items)
{
    //create a new container for our stacked items
    itemslice islice;

    //iterate through all items in the vector
    for (auto it = items.begin(); it != items.end(); it++) {
        bool list_exists = false;

        //iterate through stacked item lists
        for(auto curr = islice.begin(); curr != islice.end(); curr++) {
            //check if the ID exists
            item *first_item = curr->front();
            if (first_item->type->id == it->type->id) {
                //we've found the list of items with the same type ID

                if (it->charges != -1 && (it->is_food() || it->is_ammo())) {
                    //add charges to existing food/ammo item
                    first_item->charges += it->charges;
                    list_exists = true;
                    break;
                } else if (first_item->stacks_with(*it)) {
                    if (first_item->is_food() && first_item->has_flag("HOT")) {
                        int tmpcounter = (first_item->item_counter + it->item_counter) / 2;
                        first_item->item_counter = tmpcounter;
                        it->item_counter = tmpcounter;
                    }
                    else if (first_item->is_food() && first_item->has_flag("COLD")) {
                        int tmpcounter = (first_item->item_counter + it->item_counter) / 2;
                        first_item->item_counter = tmpcounter;
                        it->item_counter = tmpcounter;
                    }

                    //add it to the existing list
                    curr->push_back(&*it);
                    list_exists = true;
                    break;
                }
            }
        }

        if(!list_exists) {
            //add the item to a new list
            std::list<item*> newList;
            newList.push_back(&*it);

            //insert the list into islice
            islice.push_back(newList);
        }

    } //end items loop

    return islice;
}


bool map::sees_some_items(int x, int y, const player &u)
{
    // can only see items if there are any items.
    return !i_at( x, y ).empty() && could_see_items( x, y, u );
}

bool map::could_see_items(int x, int y, const player &u)
{
    const bool container = has_flag_ter_or_furn("CONTAINER", x, y);
    const bool sealed = has_flag_ter_or_furn("SEALED", x, y);
    if (sealed && container) {
        // never see inside of sealed containers
        return false;
    }
    if (container) {
        // can see inside of containers if adjacent or
        // on top of the container
        return (abs(x - u.posx) <= 1 && abs(y - u.posy) <= 1);
    }
    return true;
}

item map::water_from(const int x, const int y)
{
    item ret("water", 0);
    if (ter(x, y) == t_water_sh && one_in(3))
        ret.poison = rng(1, 4);
    else if (ter(x, y) == t_water_dp && one_in(4))
        ret.poison = rng(1, 4);
    else if (ter(x, y) == t_sewage)
        ret.poison = rng(1, 7);
    return ret;
}
item map::swater_from(const int x, const int y)
{
    (void)x; (void)y;
    item ret("salt_water", 0);

    return ret;
}
item map::acid_from(const int x, const int y)
{
    (void)x; (void)y; //all acid is acid, i guess?
    item ret("water_acid", 0);
    return ret;
}

void map::i_rem(const int x, const int y, const int index)
{
    if (index > (int)i_at(x, y).size() - 1) {
        return;
    }
    i_at(x, y).erase(i_at(x, y).begin() + index);
}

void map::i_rem(const int x, const int y, item* it)
{
    std::vector<item>& map_items = i_at(x, y);

    for(auto iter = map_items.begin(); iter < map_items.end(); iter++)
    {
        //delete the item if the pointer memory addresses are the same
        if(it == &*iter) {
            map_items.erase(iter);
            break;
        }
    }
}

void map::i_clear(const int x, const int y)
{
 i_at(x, y).clear();
}

point map::find_item(const item *it)
{
    point ret;
    for (ret.x = 0; ret.x < SEEX * my_MAPSIZE; ret.x++) {
        for (ret.y = 0; ret.y < SEEY * my_MAPSIZE; ret.y++) {
            for (auto &i : i_at(ret.x, ret.y)) {
                if (it == &i) {
                    return ret;
                }
            }
        }
    }
    ret.x = -1;
    ret.y = -1;
    return ret;
}

void map::spawn_an_item(const int x, const int y, item new_item,
                        const long charges, const int damlevel)
{
    if (charges && new_item.charges > 0)
    {
        //let's fail silently if we specify charges for an item that doesn't support it
        new_item.charges = charges;
    }
    new_item = new_item.in_its_container();
    if ((new_item.made_of(LIQUID) && has_flag("SWIMMABLE", x, y)) ||
        has_flag("DESTROY_ITEM", x, y))
    {
        return;
    }
    // bounds checking for damage level
    if (damlevel < -1)
    {
        new_item.damage = -1;
    } else if (damlevel > 4)
    {
        new_item.damage = 4;
    } else
    {
        new_item.damage = damlevel;
    }
    add_item_or_charges(x, y, new_item);
}

void map::spawn_items(const int x, const int y, const std::vector<item> &new_items)
{
    if (!inbounds(x, y) || has_flag("DESTROY_ITEM", x, y)) {
        return;
    }
    const bool swimmable = has_flag("SWIMMABLE", x, y);
    for (std::vector<item>::const_iterator a = new_items.begin(); a != new_items.end(); ++a) {
        item new_item = *a;
        if (new_item.made_of(LIQUID) && swimmable) {
            continue;
        }
        if (new_item.is_armor() && new_item.has_flag("PAIRED") && x_in_y(4, 5)) {
            //Clear old side info
            it_armor* armor = dynamic_cast<it_armor*>(new_item.type);
            new_item.covers = armor->covers;
            if (new_item.has_flag("RIGHT")) {
                new_item.item_tags.erase("RIGHT");
            }
            if (new_item.has_flag("LEFT")) {
                new_item.item_tags.erase("LEFT");
            }
            //Clone unsided item
            item new_item2 = new_item;
            //Add new sides to both items
            new_item.item_tags.insert("LEFT");
            new_item2.item_tags.insert("RIGHT");
            if (new_item.type->is_sided(bp_arm_l)) {
                new_item.covers.set(bp_arm_l);
                new_item2.covers.set(bp_arm_r);
            }
            if (new_item.type->is_sided(bp_hand_l)) {
                new_item.covers.set(bp_hand_l);
                new_item2.covers.set(bp_hand_r);
            }
            if (new_item.type->is_sided(bp_leg_l)) {
                new_item.covers.set(bp_leg_l);
                new_item2.covers.set(bp_leg_r);
            }
            if (new_item.type->is_sided(bp_foot_l)) {
                new_item.covers.set(bp_foot_l);
                new_item2.covers.set(bp_foot_r);
            }
            add_item_or_charges(x, y, new_item2);
        }
        add_item_or_charges(x, y, new_item);
    }
}

void map::spawn_artifact(const int x, const int y)
{
    add_item_or_charges( x, y, item( new_artifact(), 0 ) );
}

void map::spawn_natural_artifact(const int x, const int y, artifact_natural_property prop)
{
    add_item_or_charges( x, y, item( new_natural_artifact( prop ), 0 ) );
}

//New spawn_item method, using item factory
// added argument to spawn at various damage levels
void map::spawn_item(const int x, const int y, const std::string &type_id,
                     const unsigned quantity, const long charges,
                     const unsigned birthday, const int damlevel, const bool rand)
{
    if(type_id == "null") {
        return;
    }
    if(item_is_blacklisted(type_id)) {
        return;
    }
    // recurse to spawn (quantity - 1) items
    for(unsigned i = 1; i < quantity; i++)
    {
        spawn_item(x, y, type_id, 1, charges, birthday, damlevel);
    }
    // spawn the item
    item new_item(type_id, birthday, rand);
    if( one_in( 3 ) && new_item.has_flag( "VARSIZE" ) ) {
        new_item.item_tags.insert( "FIT" );
    }
    spawn_an_item(x, y, new_item, charges, damlevel);
}

int map::max_volume(const int x, const int y)
{
    const ter_t &ter = ter_at(x, y);
    if (has_furn(x, y)) {
        return furn_at(x, y).max_volume;
    }
    return ter.max_volume;
}

// total volume of all the things
int map::stored_volume(const int x, const int y) {
    if(!INBOUNDS(x, y)) {
        return 0;
    }
    int cur_volume=0;
    for (auto &n : i_at(x, y)) {
        item* curit = &n;
        cur_volume += curit->volume();
    }
    return cur_volume;
}

// free space
int map::free_volume(const int x, const int y) {
   const int maxvolume = this->max_volume(x, y);
   if(!INBOUNDS(x, y)) return 0;
   return ( maxvolume - stored_volume(x, y) );
}

// returns true if full, modified by arguments:
// (none):                            size >= max || volume >= max
// (addvolume >= 0):                  size+1 > max || volume + addvolume > max
// (addvolume >= 0, addnumber >= 0):  size + addnumber > max || volume + addvolume > max
bool map::is_full(const int x, const int y, const int addvolume, const int addnumber ) {
   const int maxitems = MAX_ITEM_IN_SQUARE; // (game.h) 1024
   const int maxvolume = this->max_volume(x, y);

   if( ! (INBOUNDS(x, y) && move_cost(x, y) > 0 && !has_flag("NOITEM", x, y) ) ) {
       return true;
   }

   if ( addvolume == -1 ) {
       if ( i_at(x, y).size() < maxitems ) return true;
       int cur_volume=stored_volume(x, y);
       return (cur_volume >= maxvolume ? true : false );
   } else {
       if ( i_at(x, y).size() + ( addnumber == -1 ? 1 : addnumber ) > maxitems ) return true;
       int cur_volume=stored_volume(x, y);
       return ( cur_volume + addvolume > maxvolume ? true : false );
   }

}

// adds an item to map point, or stacks charges.
// returns false if item exceeds tile's weight limits or item count. This function is expensive, and meant for
// user initiated actions, not mapgen!
// overflow_radius > 0: if x,y is full, attempt to drop item up to overflow_radius squares away, if x,y is full
bool map::add_item_or_charges(const int x, const int y, item new_item, int overflow_radius) {

    if(!INBOUNDS(x,y) ) {
        // Complain about things that should never happen.
        dbg(D_INFO) << x << "," << y << ", liquid "
                    <<(new_item.made_of(LIQUID) && has_flag("SWIMMABLE", x, y)) <<
                    ", destroy_item "<<has_flag("DESTROY_ITEM", x, y);

        return false;
    }
    if( (new_item.made_of(LIQUID) && has_flag("SWIMMABLE", x, y)) ||
            has_flag("DESTROY_ITEM", x, y) || new_item.has_flag("NO_DROP") ) {
        // Silently fail on mundane things that prevent item spawn.
        return false;
    }


    bool tryaddcharges = (new_item.charges  != -1 && (new_item.is_food() || new_item.is_ammo()));
    std::vector<point> ps = closest_points_first(overflow_radius, x, y);
    for(std::vector<point>::iterator p_it = ps.begin(); p_it != ps.end(); p_it++)
    {
        itype_id add_type = new_item.type->id; // caching this here = ~25% speed increase
        if (!INBOUNDS(p_it->x, p_it->y) || new_item.volume() > this->free_volume(p_it->x, p_it->y) ||
                has_flag("DESTROY_ITEM", p_it->x, p_it->y) || has_flag("NOITEM", p_it->x, p_it->y)){
            continue;
        }

        if (tryaddcharges) {
            for (auto &i : i_at(p_it->x,p_it->y))
            {
                if(i.type->id == add_type)
                {
                    i.charges += new_item.charges;
                    return true;
                }
            }
        }
        if (i_at(p_it->x, p_it->y).size() < MAX_ITEM_IN_SQUARE)
        {
            add_item(p_it->x, p_it->y, new_item, MAX_ITEM_IN_SQUARE);
            return true;
        }
    }
    return false;
}

// Place an item on the map, despite the parameter name, this is not necessaraly a new item.
// WARNING: does -not- check volume or stack charges. player functions (drop etc) should use
// map::add_item_or_charges
void map::add_item(const int x, const int y, item new_item, const int maxitems)
{
    if (new_item.made_of(LIQUID) && has_flag("SWIMMABLE", x, y)) {
        return;
    }
    if (!INBOUNDS(x, y)) {
        return;
    }
    if (has_flag("DESTROY_ITEM", x, y) || ((int)i_at(x,y).size() >= maxitems)) {
        return;
    }
    field &fld = field_at(x, y);
    if (new_item.has_flag("ACT_IN_FIRE") && (fld.findField(fd_fire) != 0)) {
        new_item.active = true;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);
    current_submap->itm[lx][ly].push_back(new_item);
    if( new_item.needs_processing() ) {
        current_submap->active_item_count++;
    }
}

// Check if it's in a fridge and is food, set the fridge
// date to current time, and also check contents.
static void apply_in_fridge(item &it)
{
    if (it.is_food() && it.fridge == 0) {
        it.fridge = (int) calendar::turn;
        // cool down of the HOT flag, is unsigned, don't go below 1
        if ((it.has_flag("HOT")) && (it.item_counter > 10)) {
            it.item_counter -= 10;
        }
        // This sets the COLD flag, and doesn't go above 600
        if ((it.has_flag("EATEN_COLD")) && (!it.has_flag("COLD"))) {
            it.item_tags.insert("COLD");
            it.active = true;
        }
        if ((it.has_flag("COLD")) && (it.item_counter <= 590) && it.fridge > 0) {
            it.item_counter += 10;
        }
    }
    if (it.is_container()) {
        for (size_t a = 0; a < it.contents.size(); a++) {
            apply_in_fridge(it.contents[a]);
        }
    }
}

void map::process_active_items()
{
    for (int gx = 0; gx < my_MAPSIZE; gx++) {
        for (int gy = 0; gy < my_MAPSIZE; gy++) {
            submap * const current_submap = get_submap_at_grid(gx, gy);
            if (current_submap->active_item_count > 0) {
                process_active_items_in_submap(current_submap, gx, gy);
            }
            if (!current_submap->vehicles.empty()) {
                process_active_items_in_vehicles(current_submap);
            }
        }
    }
}

extern std::pair<item, point> tmp_active_item_pos;

void map::process_active_items_in_submap(submap * const current_submap, int gridx, int gridy)
{
    for (int i = 0; i < SEEX; i++) {
        for (int j = 0; j < SEEY; j++) {
            tmp_active_item_pos.second.x = gridx * SEEX + i;
            tmp_active_item_pos.second.y = gridy * SEEY + j;
            std::vector<item> &items = current_submap->itm[i][j];
            //Do a count-down loop, as some items may be removed
            for (size_t n = 0; n < items.size(); n++) {
                // The following code is expensive, don't run it for non-active
                // items, that would be useless anyway
                if( !items[n].needs_processing() ) {
                    continue;
                }
                // make a temporary copy, remove the item (in advance)
                // and use that copy to process it
                tmp_active_item_pos.first = items[n];
                items.erase(items.begin() + n);
                if( !tmp_active_item_pos.first.process( nullptr, tmp_active_item_pos.second ) ) {
                    // Not destroyed, must be inserted again, but make sure
                    // we don't insert far behind the end of the vector
                    n = std::min(items.size(), n);
                    items.insert(items.begin() + n, tmp_active_item_pos.first);
                    // Other note: the address of the items vector is
                    // not affected by any explosion, but they could reduce
                    // the amount of items in it.
                    continue;
                }
                // Item is destroyed, don't reinsert it.
                // Note: this might lead to items not being processed:
                // vector: 10 glass items, mininuke, mininuke
                // the first nuke explodes, destroys some of the glass items
                // now the index of the second nuke is not 11, but less, but
                // one can not know which it is now.
                current_submap->active_item_count--;
                n--;
            }
        }
    }
}

void map::process_active_items_in_vehicles(submap * const current_submap)
{
    std::vector<vehicle*> &veh_in_nonant = current_submap->vehicles;
    // a copy, important if the vehicle list changes because a
    // vehicle got destroyed by a bomb (an active item!), this list
    // won't change, but veh_in_nonant will change.
    std::vector<vehicle*> vehicles = veh_in_nonant;
    for (size_t v = 0; v < vehicles.size(); v++) {
        vehicle *cur_veh = vehicles[v];
        if (std::find(veh_in_nonant.begin(), veh_in_nonant.end(), cur_veh) == veh_in_nonant.end()) {
            // vehicle not in the vehicle list of the nonant, has been
            // destroyed (or moved to another nonant?)
            // Can't be sure that it still exist, so skip it
            continue;
        }
        process_active_items_in_vehicle(cur_veh, current_submap);
    }
}

void map::process_active_items_in_vehicle(vehicle *cur_veh, submap * const current_submap)
{
    std::vector<int> cargo_parts = cur_veh->all_parts_with_feature(VPFLAG_CARGO, false);
    for(size_t part_index = 0; part_index < cargo_parts.size(); part_index++) {
        const int part = cargo_parts[part_index];
        vehicle_part &vp = cur_veh->parts[part];
        // Cache the mount position and the type, to identify
        // the vehicle part in case cur_veh->parts got changed
        const point mnt(vp.precalc_dx[0], vp.precalc_dy[0]);
        const int vp_type = vp.iid;
        // This is used in game::find_item. Because otherwise the
        // temporary item would nowhere to be found.
        tmp_active_item_pos.second = point(cur_veh->global_x() + vp.precalc_dx[0], cur_veh->global_y() + vp.precalc_dy[0]);
        std::vector<item> *items_in_part = &vp.items;
        const bool fridge_here = cur_veh->fridge_on && cur_veh->part_flag(part, VPFLAG_FRIDGE);
        for(int n = items_in_part->size() - 1; n >= 0; n--) {
            item *it = &(*items_in_part)[n];
            if (fridge_here) {
                apply_in_fridge(*it);
            }
            if (it->has_flag("RECHARGE") && cur_veh->part_with_feature(part, VPFLAG_RECHARGE) >= 0 &&
                cur_veh->recharger_on) {
                int full_charge = dynamic_cast<it_tool*>(it->type)->max_charges;
                if (it->has_flag("DOUBLE_AMMO")) {
                    full_charge = full_charge * 2;
                }
                if (it->is_tool() && full_charge > it->charges ) {
                    if (one_in(10)) {
                        it->charges++;
                    }
                }
            }
            // The following code is expensive, don't run it for non-active
            // items, that would be useless anyway
            if( !it->needs_processing() ) {
                continue;
            }
            // make a temporary copy, remove the item (in advance)
            // and use that copy to process it
            tmp_active_item_pos.first = *it;
            items_in_part->erase(items_in_part->begin() + n);
            if( !tmp_active_item_pos.first.process( nullptr, tmp_active_item_pos.second ) ) {
                // item still exist, most likely it didn't just explode,
                // put it back
                items_in_part->insert(items_in_part->begin() + n, tmp_active_item_pos.first);
                continue;
            }
            n--; // to process the correct next item.
            // item does not exist anymore, might have been an exploding bomb,
            // check if the vehicle is still valid (does exist)
            std::vector<vehicle*> &veh_in_nonant = current_submap->vehicles;
            if(std::find(veh_in_nonant.begin(), veh_in_nonant.end(), cur_veh) == veh_in_nonant.end()) {
                // Nope, vehicle is not in the vehicle list of the submap,
                // it might have moved to another submap (unlikely)
                // or be destroyed, anywaay it does not need to be processed here
                return;
            }
            // Vehicle still valid, find the current vehicle part again,
            // the list of cargo parts might have changed (image a part with
            // a low index has been removed by an explosion, all the other
            // parts would move up to fill the gap).
            cargo_parts = cur_veh->all_parts_with_feature(VPFLAG_CARGO, false);
            for(part_index = 0; part_index < cargo_parts.size(); part_index++) {
                vehicle_part &vp = cur_veh->parts[cargo_parts[part_index]];
                if(mnt.x == vp.precalc_dx[0] && mnt.y == vp.precalc_dy[0] && vp_type == vp.iid) {
                    // Found it, this is the vehicle part we are currently iterating overall
                    // update the item vector (if the part index changed,
                    // the address of the item vector changed, too.
                    items_in_part = &vp.items;
                    break;
                }
            }
            if (part_index >= cargo_parts.size()) {
                // went over all parts and did not found the current cargo part
                // it is gone, now we bail out, because we can not know
                // which cargo part to do next, it could be one that has
                // already been handled.
                return;
            }
            // Now we can continue as if nothing happened, the item has
            // already been remove and it is not put back,
        }
    }
}

std::list<item> use_amount_map_or_vehicle(std::vector<item> &vec, const itype_id type, int &quantity, const bool use_container)
{
    std::list<item> ret;
    for (std::vector<item>::iterator a = vec.begin(); a != vec.end() && quantity > 0; ) {
        if (a->use_amount(type, quantity, use_container, ret)) {
            a = vec.erase(a);
        } else {
            ++a;
        }
    }
    return ret;
}

std::list<item> map::use_amount_square(const int x, const int y, const itype_id type,
                                       int &quantity, const bool use_container)
{
  std::list<item> ret;
  int vpart = -1;
  vehicle *veh = veh_at(x,y, vpart);

  if (veh) {
    const int cargo = veh->part_with_feature(vpart, "CARGO");
    if (cargo >= 0) {
      std::list<item> tmp = use_amount_map_or_vehicle(veh->parts[cargo].items, type,
                                                      quantity, use_container);
      ret.splice(ret.end(), tmp);
    }
  }
  std::list<item> tmp = use_amount_map_or_vehicle(i_at(x,y), type, quantity, use_container);
  ret.splice(ret.end(), tmp);
  return ret;
}

std::list<item> map::use_amount(const point origin, const int range, const itype_id type,
                                const int amount, const bool use_container)
{
  std::list<item> ret;
  int quantity = amount;
  for (int radius = 0; radius <= range && quantity > 0; radius++) {
    for (int x = origin.x - radius; x <= origin.x + radius; x++) {
      for (int y = origin.y - radius; y <= origin.y + radius; y++) {
        if (rl_dist(origin.x, origin.y, x, y) >= radius) {
          std::list<item> tmp;
          tmp = use_amount_square(x, y, type, quantity, use_container);
          ret.splice(ret.end(), tmp);
        }
      }
    }
  }
  return ret;
}

std::list<item> use_charges_from_map_or_vehicle(std::vector<item> &vec, const itype_id type,
                                                long &quantity)
{
    std::list<item> ret;
    for (std::vector<item>::iterator a = vec.begin(); a != vec.end() && quantity > 0; ) {
        if (a->use_charges(type, quantity, ret)) {
            a = vec.erase(a);
        } else {
            ++a;
        }
    }
    return ret;
}

extern long remove_charges_in_list(const itype *type, std::vector<item> &items, long quantity);
void use_charges_from_furn(const furn_t &f, const itype_id &type, long &quantity, std::vector<item> &items, std::list<item> &ret)
{
    itype *itt = f.crafting_pseudo_item_type();
    if (itt == NULL || itt->id != type) {
        return;
    }
    const itype *ammo = f.crafting_ammo_item_type();
    if (ammo != NULL) {
        item furn_item(itt->id, 0);
        furn_item.charges = remove_charges_in_list(ammo, items, quantity);
        if (furn_item.charges > 0) {
            ret.push_back(furn_item);
            quantity -= furn_item.charges;
        }
    }
}

std::list<item> map::use_charges(const point origin, const int range,
                                 const itype_id type, const long amount)
{
    std::list<item> ret;
    long quantity = amount;
    for (int radius = 0; radius <= range && quantity > 0; radius++) {
        for (int x = origin.x - radius; x <= origin.x + radius; x++) {
            for (int y = origin.y - radius; y <= origin.y + radius; y++) {
                if (has_furn(x, y) && accessable_furniture(origin.x, origin.y, x, y, range)) {
                    use_charges_from_furn(furn_at(x, y), type, quantity, i_at(x, y), ret);
                    if (quantity <= 0) {
                        return ret;
                    }
                }
                if(accessable_items( origin.x, origin.y, x, y, range) ) {
                    continue;
                }
                if (rl_dist(origin.x, origin.y, x, y) >= radius) {
                    int vpart = -1;
                    vehicle *veh = veh_at(x, y, vpart);

                    if (veh) { // check if a vehicle part is present to provide water/power
                        const int kpart = veh->part_with_feature(vpart, "KITCHEN");
                        const int weldpart = veh->part_with_feature(vpart, "WELDRIG");
                        const int craftpart = veh->part_with_feature(vpart, "CRAFTRIG");
                        const int forgepart = veh->part_with_feature(vpart, "FORGE");
                        const int chempart = veh->part_with_feature(vpart, "CHEMLAB");
                        const int cargo = veh->part_with_feature(vpart, "CARGO");

                        if (kpart >= 0) { // we have a kitchen, now to see what to drain
                            ammotype ftype = "NULL";

                            if (type == "water_clean") {
                                ftype = "water";
                            } else if (type == "hotplate") {
                                ftype = "battery";
                            }

                            item tmp(type, 0); //TODO add a sane birthday arg
                            tmp.charges = veh->drain(ftype, quantity);
                            quantity -= tmp.charges;
                            ret.push_back(tmp);

                            if (quantity == 0) {
                                return ret;
                            }
                        }

                        if (weldpart >= 0) { // we have a weldrig, now to see what to drain
                            ammotype ftype = "NULL";

                            if (type == "welder") {
                                ftype = "battery";
                            } else if (type == "soldering_iron") {
                                ftype = "battery";
                            }

                            item tmp(type, 0); //TODO add a sane birthday arg
                            tmp.charges = veh->drain(ftype, quantity);
                            quantity -= tmp.charges;
                            ret.push_back(tmp);

                            if (quantity == 0) {
                                return ret;
                            }
                        }

                        if (craftpart >= 0) { // we have a craftrig, now to see what to drain
                            ammotype ftype = "NULL";

                            if (type == "press") {
                                ftype = "battery";
                            } else if (type == "vac_sealer") {
                                ftype = "battery";
                            } else if (type == "dehydrator") {
                                ftype = "battery";
                            }

                            item tmp(type, 0); //TODO add a sane birthday arg
                            tmp.charges = veh->drain(ftype, quantity);
                            quantity -= tmp.charges;
                            ret.push_back(tmp);

                            if (quantity == 0) {
                                return ret;
                            }
                        }

                        if (forgepart >= 0) { // we have a veh_forge, now to see what to drain
                            ammotype ftype = "NULL";

                            if (type == "forge") {
                                ftype = "battery";
                            }

                            item tmp(type, 0); //TODO add a sane birthday arg
                            tmp.charges = veh->drain(ftype, quantity);
                            quantity -= tmp.charges;
                            ret.push_back(tmp);

                            if (quantity == 0) {
                                return ret;
                            }
                        }

                        if (chempart >= 0) { // we have a chem_lab, now to see what to drain
                            ammotype ftype = "NULL";

                            if (type == "chemistry_set") {
                                ftype = "battery";
                            } else if (type == "hotplate") {
                                ftype = "battery";
                            }

                            item tmp(type, 0); //TODO add a sane birthday arg
                            tmp.charges = veh->drain(ftype, quantity);
                            quantity -= tmp.charges;
                            ret.push_back(tmp);

                            if (quantity == 0) {
                                return ret;
                            }
                        }

                        if (cargo >= 0) {
                            std::list<item> tmp =
                                use_charges_from_map_or_vehicle(veh->parts[cargo].items, type, quantity);
                            ret.splice(ret.end(), tmp);
                            if (quantity <= 0) {
                                return ret;
                            }
                        }
                    }

                    std::list<item> tmp = use_charges_from_map_or_vehicle(i_at(x,y), type, quantity);
                    ret.splice(ret.end(), tmp);
                    if (quantity <= 0) {
                        return ret;
                    }
                }
            }
        }
    }
    return ret;
}

std::list<std::pair<tripoint, item *> > map::get_rc_items( int x, int y, int z )
{
    std::list<std::pair<tripoint, item *> > rc_pairs;
    tripoint pos;
    (void)z;
    pos.z = abs_sub.z;
    for( pos.x = 0; pos.x < SEEX * MAPSIZE; pos.x++ ) {
        if( x != -1 && x != pos.x ) {
            continue;
        }
        for( pos.y = 0; pos.y < SEEY * MAPSIZE; pos.y++ ) {
            if( y != -1 && y != pos.y ) {
                continue;
            }
            std::vector<item> &item_stack = i_at( pos.x, pos.y );
            for( auto item_ref = item_stack.begin(); item_ref != item_stack.end(); ++item_ref ) {
                if( ( item_ref->has_flag("RADIO_ACTIVATION") ||
                      item_ref->has_flag("RADIO_CONTAINER") ) &&
                    item_ref->active ) {
                    rc_pairs.push_back( std::make_pair( pos, &(*item_ref) ) );
                }
            }
        }
    }

    return rc_pairs;
}

std::string map::trap_get(const int x, const int y) const {
    return traplist[ tr_at(x, y) ]->id;
}

void map::trap_set(const int x, const int y, const std::string & sid) {
    if ( trapmap.find(sid) == trapmap.end() ) {
        return;
    }
    add_trap(x, y, (trap_id)trapmap[ sid ]);
}
void map::trap_set(const int x, const int y, const trap_id id) {
    add_trap(x, y, id);
}
// todo: to be consistent with ???_at(...) this should return ref to the actual trap object
trap_id map::tr_at(const int x, const int y) const
{
 if (!INBOUNDS(x, y)) {
  return tr_null;
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/

 int lx, ly;
 submap * const current_submap = get_submap_at(x, y, lx, ly);

 if (terlist[ current_submap->ter[lx][ly] ].trap != tr_null) {
  return terlist[ current_submap->ter[lx][ly] ].trap;
 }

 return current_submap->get_trap(lx, ly);
}

void map::add_trap(const int x, const int y, const trap_id t)
{
    if (!INBOUNDS(x, y)) { return; }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    // If there was already a trap here, remove it.
    if (current_submap->get_trap(lx, ly) != tr_null) {
        remove_trap(x, y);
    }

    current_submap->set_trap(lx, ly, t);
    if (t != tr_null) {
        traplocs[t].insert(point(x, y));
    }
}

void map::disarm_trap(const int x, const int y)
{
    int skillLevel = g->u.skillLevel("traps");

    if (tr_at(x, y) == tr_null) {
        debugmsg("Tried to disarm a trap where there was none (%d %d)", x, y);
        return;
    }

    trap* tr = traplist[tr_at(x, y)];
    const int tSkillLevel = g->u.skillLevel("traps");
    const int diff = tr->get_difficulty();
    int roll = rng(tSkillLevel, 4 * tSkillLevel);

    // Some traps are not actual traps. Skip the rolls, different message and give the option to grab it right away.
    if (tr->get_avoidance() ==  0 && tr->get_difficulty() == 0) {
        add_msg(_("You take down the %s."), tr->name.c_str());
        std::vector<itype_id> comp = tr->components;
        for (auto &i : comp) {
            if (i != "null") {
                spawn_item(x, y, i, 1, 1);
                remove_trap(x, y);
            }
        }
        return;
    }

    while ((rng(5, 20) < g->u.per_cur || rng(1, 20) < g->u.dex_cur) && roll < 50) {
        roll++;
    }
    if (roll >= diff) {
        add_msg(_("You disarm the trap!"));
        std::vector<itype_id> comp = tr->components;
        for (auto &i : comp) {
            if (i != "null") {
                spawn_item(x, y, i, 1, 1);
            }
        }
        if (tr_at(x, y) == tr_engine) {
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    if (i != 0 || j != 0) {
                        remove_trap(x + i, y + j);
                    }
                }
            }
        }
        if (tr_at(x, y) == tr_shotgun_1 || tr_at(x,y) == tr_shotgun_2) {
            spawn_item(x,y,"shot_00",1,2);
        }
        remove_trap(x, y);
        if(diff > 1.25 * skillLevel) { // failure might have set off trap
            g->u.practice( "traps", 1.5*(diff - skillLevel) );
        }
    } else if (roll >= diff * .8) {
        add_msg(_("You fail to disarm the trap."));
        if(diff > 1.25 * skillLevel) {
            g->u.practice( "traps", 1.5*(diff - skillLevel) );
        }
    } else {
        add_msg(m_bad, _("You fail to disarm the trap, and you set it off!"));
        tr->trigger(&g->u, x, y);
        if(diff - roll <= 6) {
            // Give xp for failing, but not if we failed terribly (in which
            // case the trap may not be disarmable).
            g->u.practice( "traps", 2*diff );
        }
    }
}

void map::remove_trap(const int x, const int y)
{
    if (!INBOUNDS(x, y)) { return; }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    trap_id t = current_submap->get_trap(lx, ly);
    if (t != tr_null) {
        if (g != NULL && this == &g->m) {
            g->u.add_known_trap(x, y, "tr_null");
        }
        current_submap->set_trap(lx, ly, tr_null);
        traplocs[t].erase(point(x, y));
    }
}
/*
 * Get wrapper for all fields at xy
 */
field& map::field_at(const int x, const int y)
{
 if (!INBOUNDS(x, y)) {
  nulfield = field();
  return nulfield;
 }

 int lx, ly;
 submap * const current_submap = get_submap_at(x, y, lx, ly);

 return current_submap->fld[lx][ly];
}

/*
 * Increment/decrement age of field type at point.
 * returns resulting age or -1 if not present.
 */
int map::adjust_field_age(const point p, const field_id t, const int offset) {
    return set_field_age( p, t, offset, true);
}

/*
 * Increment/decrement strength of field type at point, creating if not present, removing if strength becomes 0
 * returns resulting strength, or 0 for not present
 */
int map::adjust_field_strength(const point p, const field_id t, const int offset) {
    return set_field_strength(p, t, offset, true);
}

/*
 * Set age of field type at point, or increment/decrement if offset=true
 * returns resulting age or -1 if not present.
 */
int map::set_field_age(const point p, const field_id t, const int age, bool isoffset) {
    field_entry * field_ptr = get_field( p, t );
    if ( field_ptr != NULL ) {
        int adj = ( isoffset ? field_ptr->getFieldAge() : 0 ) + age;
        field_ptr->setFieldAge( adj );
        return adj;
    }
    return -1;
}

/*
 * set strength of field type at point, creating if not present, removing if strength is 0
 * returns resulting strength, or 0 for not present
 */
int map::set_field_strength(const point p, const field_id t, const int str, bool isoffset) {
    field_entry * field_ptr = get_field( p, t );
    if ( field_ptr != NULL ) {
        int adj = ( isoffset ? field_ptr->getFieldDensity() : 0 ) + str;
        if ( adj > 0 ) {
            field_ptr->setFieldDensity( adj );
            return adj;
        } else {
            remove_field( p.x, p.y, t );
            return 0;
        }
    } else if ( 0 + str > 0 ) {
        return ( add_field( p, t, str, 0 ) ? str : 0 );
    }
    return 0;
}

/*
 * get age of field type at point. -1 = not present
 */
int map::get_field_age( const point p, const field_id t ) {
    field_entry * field_ptr = get_field( p, t );
    return ( field_ptr == NULL ? -1 : field_ptr->getFieldAge() );
}

/*
 * get strength of field type at point. 0 = not present
 */
int map::get_field_strength( const point p, const field_id t ) {
    field_entry * field_ptr = get_field( p, t );
    return ( field_ptr == NULL ? 0 : field_ptr->getFieldDensity() );
}

/*
 * get field type at point. NULL if not present
 */
field_entry * map::get_field( const point p, const field_id t ) {
    if (!INBOUNDS(p.x, p.y))
        return NULL;
    int lx, ly;
    submap * const current_submap = get_submap_at(p.x, p.y, lx, ly);
    return current_submap->fld[lx][ly].findField(t);
}

/*
 * add field type at point, or set density if present
 */
bool map::add_field(const point p, const field_id t, int density, const int age)
{
    if (!INBOUNDS(p.x, p.y)) {
        return false;
    }

    if (density > 3) {
        density = 3;
    }
    if (density <= 0) {
        return false;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(p.x, p.y, lx, ly);

    if (!current_submap->fld[lx][ly].findField(t)) {
        // TODO: Update overall field_count appropriately.
        // This is the spirit of "fd_null" that it used to be.
        current_submap->field_count++; //Only adding it to the count if it doesn't exist.
    }
    current_submap->fld[lx][ly].addField(t, density, age); //This will insert and/or update the field.
    if(g != NULL && this == &g->m && p.x == g->u.posx && p.y == g->u.posy) {
        step_in_field(p.x, p.y); //Hit the player with the field if it spawned on top of them.
    }
    return true;
}

/*
 * add field type at xy, or set denity if present
 */
bool map::add_field(const int x, const int y, const field_id t, const int new_density)
{
    return this->add_field(point(x,y), t, new_density, 0);
}


/*
 * remove field type at xy
 */
void map::remove_field(const int x, const int y, const field_id field_to_remove)
{
 if (!INBOUNDS(x, y)) {
  return;
 }

 int lx, ly;
 submap * const current_submap = get_submap_at(x, y, lx, ly);

 if (current_submap->fld[lx][ly].findField(field_to_remove)) { //same as checking for fd_null in the old system
  current_submap->field_count--;
 }
 current_submap->fld[lx][ly].removeField(field_to_remove);
}

computer* map::computer_at(const int x, const int y)
{
 if (!INBOUNDS(x, y))
  return NULL;

 submap * const current_submap = get_submap_at(x, y);

 if (current_submap->comp.name == "") {
  return NULL;
 }
 return &(current_submap->comp);
}

bool map::allow_camp(const int x, const int y, const int radius)
{
    return camp_at(x, y, radius) == NULL;
}

// locate the nearest camp in some radius (default CAMPSIZE)
basecamp* map::camp_at(const int x, const int y, const int radius)
{
    if (!INBOUNDS(x, y)) {
        return NULL;
    }

    const int sx = std::max(0, x / SEEX - radius);
    const int sy = std::max(0, y / SEEY - radius);
    const int ex = std::min(MAPSIZE - 1, x / SEEX + radius);
    const int ey = std::min(MAPSIZE - 1, y / SEEY + radius);

    for (int ly = sy; ly < ey; ++ly) {
        for (int lx = sx; lx < ex; ++lx) {
            submap * const current_submap = get_submap_at(x, y);
            if (current_submap->camp.is_valid()) {
                // we only allow on camp per size radius, kinda
                return &(current_submap->camp);
            }
        }
    }

    return NULL;
}

void map::add_camp(const std::string& name, const int x, const int y)
{
    if (!allow_camp(x, y)) {
        dbg(D_ERROR) << "map::add_camp: Attempting to add camp when one in local area.";
        return;
    }

    get_submap_at(x, y)->camp = basecamp(name, x, y);
}

void map::debug()
{
 mvprintw(0, 0, "MAP DEBUG");
 getch();
 for (int i = 0; i <= SEEX * 2; i++) {
  for (int j = 0; j <= SEEY * 2; j++) {
   if (i_at(i, j).size() > 0) {
    mvprintw(1, 0, "%d, %d: %d items", i, j, i_at(i, j).size());
    mvprintw(2, 0, "%c, %d", i_at(i, j)[0].symbol(), i_at(i, j)[0].color());
    getch();
   }
  }
 }
 getch();
}

void map::draw(WINDOW* w, const point center)
{
 // We only need to draw anything if we're not in tiles mode.
 if(is_draw_tiles_mode()) {
     return;
 }

 g->reset_light_level();
 const int g_light_level = (int)g->light_level();
 const int natural_sight_range = g->u.sight_range(1);
 const int light_sight_range = g->u.sight_range(g_light_level);
 const int lowlight_sight_range = std::max(g_light_level / 2, natural_sight_range);
 const int max_sight_range = g->u.unimpaired_range();
 const bool u_is_boomered = g->u.has_effect("boomered");
 const int u_clairvoyance = g->u.clairvoyance();
 const bool u_sight_impaired = g->u.sight_impaired();
 const bool bio_night_active = g->u.has_active_bionic("bio_night");

 for (int i = 0; i < my_MAPSIZE * my_MAPSIZE; i++) {
  if (!grid[i])
   debugmsg("grid %d (%d, %d) is null!", i, i % my_MAPSIZE, i / my_MAPSIZE);
 }

 for  (int realx = center.x - getmaxx(w)/2; realx <= center.x + getmaxx(w)/2; realx++) {
  for (int realy = center.y - getmaxy(w)/2; realy <= center.y + getmaxy(w)/2; realy++) {
   const int dist = rl_dist(g->u.posx, g->u.posy, realx, realy);
   int sight_range = light_sight_range;
   int low_sight_range = lowlight_sight_range;
   bool bRainOutside = false;
   // While viewing indoor areas use lightmap model
   if (!is_outside(realx, realy)) {
    sight_range = natural_sight_range;
   // Don't display area as shadowy if it's outside and illuminated by natural light
   //and illuminated by source of light
   } else if (this->light_at(realx, realy) > LL_LOW || dist <= light_sight_range) {
    low_sight_range = std::max(g_light_level, natural_sight_range);
    bRainOutside = true;
   }

   // I've moved this part above loops without even thinking that
   // this must stay here...
   int real_max_sight_range = light_sight_range > max_sight_range ? light_sight_range : max_sight_range;
   int distance_to_look = DAYLIGHT_LEVEL;

   bool can_see = pl_sees(g->u.posx, g->u.posy, realx, realy, distance_to_look);
   lit_level lit = light_at(realx, realy);

   // now we're gonna adjust real_max_sight, to cover some nearby "highlights",
   // but at the same time changing light-level depending on distance,
   // to create actual "gradual" stuff
   // Also we'll try to ALWAYS show LL_BRIGHT stuff independent of where it is...
   if (lit != LL_BRIGHT) {
       if (dist > real_max_sight_range) {
           int intLit = (int)lit - (dist - real_max_sight_range)/2;
           if (intLit < 0) intLit = LL_DARK;
           lit = (lit_level)intLit;
       }
   }
   // additional case for real_max_sight_range
   // if both light_sight_range and max_sight_range were small
   // it means we really have limited visibility (e.g. inside a pit)
   // and we shouldn't touch that
   if (lit > LL_DARK && real_max_sight_range > 1) {
       real_max_sight_range = distance_to_look;
   }

   if ((bio_night_active && dist < 15 && dist > natural_sight_range) || // if bio_night active, blackout 15 tile radius around player
       dist > real_max_sight_range ||
       (dist > light_sight_range &&
         (lit == LL_DARK ||
         (u_sight_impaired && lit != LL_BRIGHT) ||
          !can_see))) {
    if (u_is_boomered)
     mvwputch(w, realy+getmaxy(w)/2 - center.y, realx+getmaxx(w)/2 - center.x, c_magenta, '#');
    else
         mvwputch(w, realy+getmaxy(w)/2 - center.y, realx+getmaxx(w)/2 - center.x, c_dkgray, '#');
   } else if (dist > light_sight_range && u_sight_impaired && lit == LL_BRIGHT) {
    if (u_is_boomered)
     mvwputch(w, realy+getmaxy(w)/2 - center.y, realx+getmaxx(w)/2 - center.x, c_pink, '#');
    else
     mvwputch(w, realy+getmaxy(w)/2 - center.y, realx+getmaxx(w)/2 - center.x, c_ltgray, '#');
   } else if (dist <= u_clairvoyance || can_see) {
    if (bRainOutside && INBOUNDS(realx, realy) && is_outside(realx, realy))
     g->mapRain[realy + getmaxy(w)/2 - center.y][realx + getmaxx(w)/2 - center.x] = true;
    drawsq(w, g->u, realx, realy, false, true, center.x, center.y,
           (dist > low_sight_range && LL_LIT > lit) ||
           (dist > sight_range && LL_LOW == lit),
           LL_BRIGHT == lit);
   } else {
    mvwputch(w, realy+getmaxy(w)/2 - center.y, realx+getmaxx(w)/2 - center.x, c_black,' ');
   }
  }
 }
    g->draw_critter( g->u, center );
}

void map::drawsq(WINDOW* w, player &u, const int x, const int y, const bool invert_arg,
                 const bool show_items_arg, const int view_center_x_arg, const int view_center_y_arg,
                 const bool low_light, const bool bright_light)
{
    // We only need to draw anything if we're not in tiles mode.
    if(is_draw_tiles_mode()) {
        return;
    }

    bool invert = invert_arg;
    bool show_items = show_items_arg;
    int cx = view_center_x_arg;
    int cy = view_center_y_arg;
    if (!INBOUNDS(x, y))
        return; // Out of bounds
    if (cx == -1)
        cx = u.posx;
    if (cy == -1)
        cy = u.posy;
    const int k = x + getmaxx(w)/2 - cx;
    const int j = y + getmaxy(w)/2 - cy;
    nc_color tercol;
    const ter_id curr_ter = ter(x,y);
    const furn_id curr_furn = furn(x,y);
    const trap_id curr_trap = tr_at(x, y);
    field &curr_field = field_at(x, y);
    const std::vector<item> &curr_items = i_at(x, y);
    long sym;
    bool hi = false;
    bool graf = false;
    bool draw_item_sym = false;


    if (has_furn(x, y)) {
        sym = furnlist[curr_furn].sym;
        tercol = furnlist[curr_furn].color;
    } else {
        sym = terlist[curr_ter].sym;
        tercol = terlist[curr_ter].color;
    }
    if (has_flag(TFLAG_SWIMMABLE, x, y) && has_flag(TFLAG_DEEP_WATER, x, y) && !u.is_underwater()) {
        show_items = false; // Can only see underwater items if WE are underwater
    }
    // If there's a trap here, and we have sufficient perception, draw that instead
    if (curr_trap != tr_null && traplist[curr_trap]->can_see(g->u, x, y)) {
        tercol = traplist[curr_trap]->color;
        if (traplist[curr_trap]->sym == '%') {
            switch(rng(1, 5)) {
            case 1: sym = '*'; break;
            case 2: sym = '0'; break;
            case 3: sym = '8'; break;
            case 4: sym = '&'; break;
            case 5: sym = '+'; break;
            }
        } else {
            sym = traplist[curr_trap]->sym;
        }
    }
    if (curr_field.fieldCount() > 0) {
        const field_id& fid = curr_field.fieldSymbol();
        const field_entry* fe = curr_field.findField(fid);
        const field_t& f = fieldlist[fid];
        if (f.sym == '&' || fe == NULL) {
            // Do nothing, a '&' indicates invisible fields.
        } else if (f.sym == '*') {
            // A random symbol.
            switch (rng(1, 5)) {
            case 1: sym = '*'; break;
            case 2: sym = '0'; break;
            case 3: sym = '8'; break;
            case 4: sym = '&'; break;
            case 5: sym = '+'; break;
            }
        } else {
            // A field symbol '%' indicates the field should not hide
            // items/terrain. When the symbol is not '%' it will
            // hide items (the color is still inverted if there are items,
            // but the tile symbol is not changed).
            // draw_item_sym indicates that the item symbol should be used
            // even if sym is not '.'.
            // As we don't know at this stage if there are any items
            // (that are visible to the player!), we always set the symbol.
            // If there are items and the field does not hide them,
            // the code handling items will override it.
            draw_item_sym = (f.sym == '%');
            // If field priority is > 1, draw the field anyway as it obscures what's under it.
            if( f.priority > 1 || (sym == '.' && f.sym != '%') ) {
                // default terrain '.' and
                // non-default field symbol -> field symbol overrides terrain
                sym = f.sym;
            }
            tercol = f.color[fe->getFieldDensity() - 1];
        }
    }

    // If there are items here, draw those instead
    if (show_items && sees_some_items(x, y, g->u)) {
        // if there's furniture/terrain/trap/fields (sym!='.')
        // and we should not override it, then only highlight the square
        if (sym != '.' && sym != '%' && !draw_item_sym) {
            hi = true;
        } else {
            // otherwise override with the symbol of the last item
            sym = curr_items[curr_items.size() - 1].symbol();
            if (!draw_item_sym) {
                tercol = curr_items[curr_items.size() - 1].color();
            }
            if (curr_items.size() > 1) {
                invert = !invert;
            }
        }
    }

    int veh_part = 0;
    vehicle *veh = veh_at(x, y, veh_part);
    if (veh) {
        sym = special_symbol (veh->face.dir_symbol(veh->part_sym(veh_part)));
        tercol = veh->part_color(veh_part);
    }
    // If there's graffiti here, change background color
    if( has_graffiti_at( x, y ) ) {
        graf = true;
    }

    //suprise, we're not done, if it's a wall adjacent to an other, put the right glyph
    if(sym == LINE_XOXO || sym == LINE_OXOX) { //vertical or horizontal
        sym = determine_wall_corner(x, y, sym);
    }

    if (u.has_effect("boomered")) {
        tercol = c_magenta;
    } else if ( u.has_nv() ) {
        tercol = (bright_light) ? c_white : c_ltgreen;
    } else if (low_light) {
        tercol = c_dkgray;
    } else if (u.has_disease("darkness")) {
        tercol = c_dkgray;
    }

    if (invert) {
        mvwputch_inv(w, j, k, tercol, sym);
    } else if (hi) {
        mvwputch_hi (w, j, k, tercol, sym);
    } else if (graf) {
        mvwputch    (w, j, k, red_background(tercol), sym);
    } else {
        mvwputch    (w, j, k, tercol, sym);
    }
}

/*
map::sees based off code by Steve Register [arns@arns.freeservers.com]
http://roguebasin.roguelikedevelopment.org/index.php?title=Simple_Line_of_Sight
*/
bool map::sees(const int Fx, const int Fy, const int Tx, const int Ty,
               const int range, int &tc)
{
 const int dx = Tx - Fx;
 const int dy = Ty - Fy;
 const int ax = abs(dx) << 1;
 const int ay = abs(dy) << 1;
 const int sx = SGN(dx);
 const int sy = SGN(dy);
 int x = Fx;
 int y = Fy;
 int t = 0;
 int st;

 if (range >= 0 && (abs(dx) > range || abs(dy) > range))
  return false; // Out of range!
 if (ax > ay) { // Mostly-horizontal line
  st = SGN(ay - (ax >> 1));
// Doing it "backwards" prioritizes straight lines before diagonal.
// This will help avoid creating a string of zombies behind you and will
// promote "mobbing" behavior (zombies surround you to beat on you)
  for (tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
   t = tc * st;
   x = Fx;
   y = Fy;
   do {
    if (t > 0) {
     y += sy;
     t -= ax;
    }
    x += sx;
    t += ay;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while ((trans(x, y)) && (INBOUNDS(x,y)));
  }
  return false;
 } else { // Same as above, for mostly-vertical lines
  st = SGN(ax - (ay >> 1));
  for (tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
  t = tc * st;
  x = Fx;
  y = Fy;
   do {
    if (t > 0) {
     x += sx;
     t -= ay;
    }
    y += sy;
    t += ax;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while ((trans(x, y)) && (INBOUNDS(x,y)));
  }
  return false;
 }
 return false; // Shouldn't ever be reached, but there it is.
}

bool map::clear_path(const int Fx, const int Fy, const int Tx, const int Ty,
                     const int range, const int cost_min, const int cost_max, int &tc) const
{
 const int dx = Tx - Fx;
 const int dy = Ty - Fy;
 const int ax = abs(dx) << 1;
 const int ay = abs(dy) << 1;
 const int sx = SGN(dx);
 const int sy = SGN(dy);
 int x = Fx;
 int y = Fy;
 int t = 0;
 int st;

 if (range >= 0 && (abs(dx) > range || abs(dy) > range))
  return false; // Out of range!
 if (ax > ay) { // Mostly-horizontal line
  st = SGN(ay - (ax >> 1));
// Doing it "backwards" prioritizes straight lines before diagonal.
// This will help avoid creating a string of zombies behind you and will
// promote "mobbing" behavior (zombies surround you to beat on you)
  for (tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
   t = tc * st;
   x = Fx;
   y = Fy;
   do {
    if (t > 0) {
     y += sy;
     t -= ax;
    }
    x += sx;
    t += ay;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while (move_cost(x, y) >= cost_min && move_cost(x, y) <= cost_max &&
            INBOUNDS(x, y));
  }
  return false;
 } else { // Same as above, for mostly-vertical lines
  st = SGN(ax - (ay >> 1));
  for (tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
  t = tc * st;
  x = Fx;
  y = Fy;
   do {
    if (t > 0) {
     x += sx;
     t -= ay;
    }
    y += sy;
    t += ax;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while (move_cost(x, y) >= cost_min && move_cost(x, y) <= cost_max &&
            INBOUNDS(x,y));
  }
  return false;
 }
 return false; // Shouldn't ever be reached, but there it is.
}

bool map::accessable_items(const int Fx, const int Fy, const int Tx, const int Ty, const int range) const
{
    int junk = 0;
    return (has_flag("SEALED", Tx, Ty) && !has_flag("LIQUIDCONT", Tx, Ty)) ||
        ((Fx != Tx || Fy != Ty) &&
         !clear_path( Fx, Fy, Tx, Ty, range, 1, 100, junk ) );
}

bool map::accessable_furniture(const int Fx, const int Fy, const int Tx, const int Ty, const int range) const
{
    int junk = 0;
    return ((Fx == Tx && Fy == Ty) ||
           clear_path( Fx, Fy, Tx, Ty, range, 1, 100, junk ) );
}

std::vector<point> map::getDirCircle(const int Fx, const int Fy, const int Tx, const int Ty)
{
    std::vector<point> vCircle;
    vCircle.resize(8);

    const std::vector<point> vLine = line_to(Fx, Fy, Tx, Ty, 0);
    const std::vector<point> vSpiral = closest_points_first(1, Fx, Fy);
    const std::vector<int> vPos {1,2,4,6,8,7,5,3};

    //  All possible constelations (closest_points_first goes clockwise)
    //  753  531  312  124  246  468  687  875
    //  8 1  7 2  5 4  3 6  1 8  2 7  4 5  6 3
    //  642  864  786  578  357  135  213  421

    int iPosOffset = 0;
    for (unsigned int i = 1; i < vSpiral.size(); i++) {
        if (vSpiral[i].x == vLine[0].x && vSpiral[i].y == vLine[0].y) {
            iPosOffset = i-1;
            break;
        }
    }

    for (unsigned int i = 1; i < vSpiral.size(); i++) {
        if (iPosOffset >= (int)vPos.size()) {
            iPosOffset = 0;
        }

        vCircle[vPos[iPosOffset++]-1] = point(vSpiral[i].x, vSpiral[i].y);
    }

    return vCircle;
}

// Bash defaults to true.
std::vector<point> map::route(const int Fx, const int Fy, const int Tx, const int Ty, const bool can_bash)
{
    /* TODO: If the origin or destination is out of bound, figure out the closest
     * in-bounds point and go to that, then to the real origin/destination.
     */

    if (!INBOUNDS(Fx, Fy) || !INBOUNDS(Tx, Ty)) {
        int linet;
        if (sees(Fx, Fy, Tx, Ty, -1, linet)) {
            return line_to(Fx, Fy, Tx, Ty, linet);
        } else {
            std::vector<point> empty;
            return empty;
        }
    }
    // First, check for a simple straight line on flat ground
    int linet = 0;
    if (clear_path(Fx, Fy, Tx, Ty, -1, 2, 2, linet)) {
        return line_to(Fx, Fy, Tx, Ty, linet);
    }
    /*
    if (move_cost(Tx, Ty) == 0) {
        debugmsg("%d:%d wanted to move to %d:%d, a %s!", Fx, Fy, Tx, Ty,
                    tername(Tx, Ty).c_str());
    }
    if (move_cost(Fx, Fy) == 0) {
        debugmsg("%d:%d, a %s, wanted to move to %d:%d!", Fx, Fy,
                    tername(Fx, Fy).c_str(), Tx, Ty);
    }
    */
    std::vector<point> open;
    astar_list list[SEEX * MAPSIZE][SEEY * MAPSIZE];
    int score[SEEX * MAPSIZE][SEEY * MAPSIZE];
    int gscore[SEEX * MAPSIZE][SEEY * MAPSIZE];
    point parent[SEEX * MAPSIZE][SEEY * MAPSIZE];
    int startx = Fx - 4, endx = Tx + 4, starty = Fy - 4, endy = Ty + 4;
    if (Tx < Fx) {
        startx = Tx - 4;
        endx = Fx + 4;
    }
    if (Ty < Fy) {
        starty = Ty - 4;
        endy = Fy + 4;
    }
    if (startx < 0) {
        startx = 0;
    }
    if (starty < 0) {
        starty = 0;
    }
    if (endx > SEEX * my_MAPSIZE - 1) {
        endx = SEEX * my_MAPSIZE - 1;
    }
    if (endy > SEEY * my_MAPSIZE - 1) {
        endy = SEEY * my_MAPSIZE - 1;
    }

    for (int x = startx; x <= endx; x++) {
        for (int y = starty; y <= endy; y++) {
            list  [x][y] = ASL_NONE; // Init to not being on any list
            score [x][y] = 0;        // No score!
            gscore[x][y] = 0;        // No score!
            parent[x][y] = point(-1, -1);
        }
    }
    list[Fx][Fy] = ASL_OPEN;
    open.push_back(point(Fx, Fy));

    bool done = false;

    do {
        //debugmsg("Open.size() = %d", open.size());
        int best = 9999;
        int index = -1;
        for (size_t i = 0; i < open.size(); i++) {
            if (i == 0 || score[open[i].x][open[i].y] < best) {
                best = score[open[i].x][open[i].y];
                index = i;
            }
        }

        std::vector<point> vDirCircle = getDirCircle(open[index].x, open[index].y, Tx, Ty);

        for (unsigned int i = 0; i < vDirCircle.size(); ++i) {
            const int x = vDirCircle[i].x;
            const int y = vDirCircle[i].y;

            if (x == Tx && y == Ty) {
                done = true;
                parent[x][y] = open[index];
            } else if (x >= startx && x <= endx && y >= starty && y <= endy &&
                        (move_cost(x, y) > 0 || (can_bash && is_bashable(x, y)))) {
                if (list[x][y] == ASL_NONE) { // Not listed, so make it open
                    list[x][y] = ASL_OPEN;
                    open.push_back(point(x, y));
                    parent[x][y] = open[index];
                    gscore[x][y] = gscore[open[index].x][open[index].y] + move_cost(x, y) + ((open[index].x - x != 0 && open[index].y - y != 0) ? 1 : 0);
                    if (ter(x, y) == t_door_c) {
                        gscore[x][y] += 4; // A turn to open it and a turn to move there
                    } else if (move_cost(x, y) == 0 && (can_bash && is_bashable(x, y))) {
                        gscore[x][y] += 18; // Worst case scenario with damage penalty
                    }
                    score[x][y] = gscore[x][y] + 2 * rl_dist(x, y, Tx, Ty);
                } else if (list[x][y] == ASL_OPEN) { // It's open, but make it our child
                    int newg = gscore[open[index].x][open[index].y] + move_cost(x, y) + ((open[index].x - x != 0 && open[index].y - y != 0) ? 1 : 0);
                    if (ter(x, y) == t_door_c) {
                        newg += 4; // A turn to open it and a turn to move there
                    } else if (move_cost(x, y) == 0 && (can_bash && is_bashable(x, y))) {
                        newg += 18; // Worst case scenario with damage penalty
                    }
                    if (newg < gscore[x][y]) {
                        gscore[x][y] = newg;
                        parent[x][y] = open[index];
                        score [x][y] = gscore[x][y] + 2 * rl_dist(x, y, Tx, Ty);
                    }
                }
            }
        }
        list[open[index].x][open[index].y] = ASL_CLOSED;
        open.erase(open.begin() + index);
    } while (!done && !open.empty());

    std::vector<point> tmp;
    std::vector<point> ret;
    if (done) {
        point cur(Tx, Ty);
        while (cur.x != Fx || cur.y != Fy) {
            //debugmsg("Retracing... (%d:%d) => [%d:%d] => (%d:%d)", Tx, Ty, cur.x, cur.y, Fx, Fy);
            tmp.push_back(cur);
            if (rl_dist(cur.x, cur.y, parent[cur.x][cur.y].x, parent[cur.x][cur.y].y)>1){
                debugmsg("Jump in our route! %d:%d->%d:%d", cur.x, cur.y,
                            parent[cur.x][cur.y].x, parent[cur.x][cur.y].y);
                return ret;
            }
            cur = parent[cur.x][cur.y];
        }
        for (int i = tmp.size() - 1; i >= 0; i--) {
            ret.push_back(tmp[i]);
        }
    }
    return ret;
}

int map::coord_to_angle ( const int x, const int y, const int tgtx, const int tgty ) {
  const double DBLRAD2DEG = 57.2957795130823f;
  //const double PI = 3.14159265358979f;
  const double DBLPI = 6.28318530717958f;
  double rad = atan2 ( static_cast<double>(tgty - y), static_cast<double>(tgtx - x) );
  if ( rad < 0 ) rad = DBLPI - (0 - rad);
  return int( rad * DBLRAD2DEG );
}

void map::save()
{
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            saven( abs_sub.x, abs_sub.y, abs_sub.z, gridx, gridy );
        }
    }
}

void map::load_abs(const int wx, const int wy, const int wz, const bool update_vehicle)
{
    traplocs.clear();
    for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
        for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
            loadn(wx, wy, wz, gridx, gridy, update_vehicle);
        }
    }
}

void map::load(const int wx, const int wy, const int wz, const bool update_vehicle, overmap *om)
{
    const int awx = om->pos().x * OMAPX * 2 + wx;
    const int awy = om->pos().y * OMAPY * 2 + wy;
    load_abs(awx, awy, wz, update_vehicle);
}

void map::forget_traps(int gridx, int gridy)
{
    const int n = gridx + gridy * my_MAPSIZE;

    for (int x = 0; x < SEEX; x++) {
        for (int y = 0; y < SEEY; y++) {
            trap_id t = grid[n]->get_trap(x, y);
            if (t != tr_null) {
                const int fx = x + gridx * SEEX;
                const int fy = y + gridy * SEEY;
                traplocs[t].erase(point(fx, fy));
            }
        }
    }
}

void map::shift(const int sx, const int sy)
{
// Special case of 0-shift; refresh the map
    if (sx == 0 && sy == 0) {
        return; // Skip this?
    }
    const int absx = get_abs_sub().x;
    const int absy = get_abs_sub().y;
    const int wz = get_abs_sub().z;
    set_abs_sub( absx + sx, absy + sy, wz );

// if player is in vehicle, (s)he must be shifted with vehicle too
    if( g->u.in_vehicle ) {
        g->u.posx -= sx * SEEX;
        g->u.posy -= sy * SEEY;
    }

    // Forget about traps in submaps that are being unloaded.
    if (sx != 0) {
        const int gridx = (sx > 0) ? (my_MAPSIZE - 1) : 0;
        for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
            forget_traps(gridx, gridy);
        }
    }
    if (sy != 0) {
        const int gridy = (sy > 0) ? (my_MAPSIZE - 1) : 0;
        for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
            forget_traps(gridx, gridy);
        }
    }

    for( vehicle *veh : vehicle_list ) {
        veh->smx += sx;
        veh->smy += sy;
    }

// Clear vehicle list and rebuild after shift
    clear_vehicle_cache();
    vehicle_list.clear();
// Shift the map sx submaps to the right and sy submaps down.
// sx and sy should never be bigger than +/-1.
// absx and absy are our position in the world, for saving/loading purposes.
    if (sx >= 0) {
        for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
            if (sy >= 0) {
                for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
                    if (gridx + sx < my_MAPSIZE && gridy + sy < my_MAPSIZE) {
                        copy_grid(gridx + gridy * my_MAPSIZE,
                                  gridx + sx + (gridy + sy) * my_MAPSIZE);
                        update_vehicle_list(get_submap_at_grid(gridx, gridy));
                    } else {
                        loadn(absx + sx, absy + sy, wz, gridx, gridy);
                    }
                }
            } else { // sy < 0; work through it backwards
                for (int gridy = my_MAPSIZE - 1; gridy >= 0; gridy--) {
                    if (gridx + sx < my_MAPSIZE && gridy + sy >= 0) {
                        copy_grid(gridx + gridy * my_MAPSIZE,
                                  gridx + sx + (gridy + sy) * my_MAPSIZE);
                        update_vehicle_list(get_submap_at_grid(gridx, gridy));
                    } else {
                        loadn(absx + sx, absy + sy, wz, gridx, gridy);
                    }
                }
            }
        }
    } else { // sx < 0; work through it backwards
        for (int gridx = my_MAPSIZE - 1; gridx >= 0; gridx--) {
            if (sy >= 0) {
                for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
                    if (gridx + sx >= 0 && gridy + sy < my_MAPSIZE) {
                        copy_grid(gridx + gridy * my_MAPSIZE,
                        gridx + sx + (gridy + sy) * my_MAPSIZE);
                        update_vehicle_list(get_submap_at_grid(gridx, gridy));
                    } else {
                        loadn(absx + sx, absy + sy, wz, gridx, gridy);
                    }
                }
            } else { // sy < 0; work through it backwards
                for (int gridy = my_MAPSIZE - 1; gridy >= 0; gridy--) {
                    if (gridx + sx >= 0 && gridy + sy >= 0) {
                        copy_grid(gridx + gridy * my_MAPSIZE,
                                  gridx + sx + (gridy + sy) * my_MAPSIZE);
                        update_vehicle_list(get_submap_at_grid(gridx, gridy));
                    } else {
                        loadn(absx + sx, absy + sy, wz, gridx, gridy);
                    }
                }
            }
        }
    }
    reset_vehicle_cache();
}

// saven saves a single nonant.  worldx and worldy are used for the file
// name and specifies where in the world this nonant is.  gridx and gridy are
// the offset from the top left nonant:
// 0,0 1,0 2,0
// 0,1 1,1 2,1
// 0,2 1,2 2,2
// (worldx,worldy,worldz) denotes the absolute coordinate of the submap
// in grid[0].
void map::saven( const int worldx, const int worldy, const int worldz,
                 const int gridx, const int gridy )
{
    dbg( D_INFO ) << "map::saven(worldx[" << worldx << "], worldy[" << worldy << "], gridx[" << gridx <<
                  "], gridy[" << gridy << "])";
    submap *submap_to_save = get_submap_at_grid( gridx, gridy );
    if( submap_to_save == NULL || submap_to_save->ter[0][0] == t_null ) {
        dbg( D_ERROR ) << "map::saven grid NULL!";
        return;
    }
    const int abs_x = worldx + gridx;
    const int abs_y = worldy + gridy;
    dbg( D_INFO ) << "map::saven abs_x: " << abs_x << "  abs_y: " << abs_y;
    submap_to_save->turn_last_touched = int(calendar::turn);
    MAPBUFFER.add_submap( abs_x, abs_y, worldz, submap_to_save );
}

// worldx & worldy specify where in the world this is;
// gridx & gridy specify which nonant:
// 0,0  1,0  2,0
// 0,1  1,1  2,1
// 0,2  1,2  2,2 etc
// (worldx,worldy,worldz) denotes the absolute coordinate of the submap
// in grid[0].
void map::loadn(const int worldx, const int worldy, const int worldz,
                const int gridx, const int gridy, const bool update_vehicles) {

 dbg(D_INFO) << "map::loadn(game[" << g << "], worldx["<<worldx<<"], worldy["<<worldy<<"], gridx["<<gridx<<"], gridy["<<gridy<<"])";

 const int absx = worldx + gridx,
           absy = worldy + gridy,
           gridn = gridx + gridy * my_MAPSIZE;

 dbg(D_INFO) << "map::loadn absx: " << absx << "  absy: " << absy
            << "  gridn: " << gridn;

 submap *tmpsub = MAPBUFFER.lookup_submap(absx, absy, worldz);

 if ( gridx == 0 && gridy == 0 ) {
     set_abs_sub(absx, absy, worldz);
 }

 if (tmpsub) {
    // New submap changes the content of the map and all caches must be recalculated
    set_transparency_cache_dirty();
    set_outside_cache_dirty();

  grid[gridn] = tmpsub;

  // Update vehicle data
  for( std::vector<vehicle*>::iterator it = tmpsub->vehicles.begin(),
        end = tmpsub->vehicles.end(); update_vehicles && it != end; ++it ) {

   // Only add if not tracking already.
   if( vehicle_list.find( *it ) == vehicle_list.end() ) {
    // gridx/y not correct. TODO: Fix
    (*it)->smx = gridx;
    (*it)->smy = gridy;
    vehicle_list.insert(*it);
    update_vehicle_cache(*it);
   }
  }

  // fixme; roll off into some function elsewhere ---v

    // check traps
    std::map<point, trap_id> rain_backlog;
    bool do_funnels = ( worldz >= 0 ); // empty if just loaded a save here
    for (int x = 0; x < SEEX; x++) {
        for (int y = 0; y < SEEY; y++) {
            const trap_id t = tmpsub->get_trap(x, y);
            if (t != tr_null) {

                const int fx = x + gridx * SEEX;
                const int fy = y + gridy * SEEY;
                traplocs[t].insert(point(fx, fy));
                if ( do_funnels &&
                     traplist[t]->funnel_radius_mm > 0 &&             // funnel
                     has_flag_ter_or_furn(TFLAG_INDOORS, fx, fy) == false // we have no outside_cache
                   ) {
                    rain_backlog[point(x, y)] = t;
                }
            }
        }
    }

  // check spoiled stuff, and fill up funnels while we're at it
  for (int x = 0; x < SEEX; x++) {
      for (int y = 0; y < SEEY; y++) {
          int biggest_container_idx = -1;
          int maxvolume = 0;
          bool do_container_check = false;

          if ( do_funnels && ! rain_backlog.empty() &&
               rain_backlog.find(point(x,y)) != rain_backlog.end() ) {
              do_container_check = true;
          }
          int intidx = 0;

          for(std::vector<item, std::allocator<item> >::iterator it = tmpsub->itm[x][y].begin();
              it != tmpsub->itm[x][y].end();) {
              if ( do_container_check == true ) { // cannot link trap to mapitems
                  if ( it->is_funnel_container(maxvolume) ) {                      // biggest
                      biggest_container_idx = intidx;             // this will survive erases below, it ptr may not
                  }
              }
              if (it->is_corpse()) {
                  it->calc_rot(point(x,y));

                  //remove corpse after 10 days (dependent on temperature)
                  if(it->get_rot() > DAYS( 10 ) && !it->can_revive() ) {
                      it = tmpsub->itm[x][y].erase(it);
                  } else { ++it; intidx++; }

                  continue;
              }
              if(it->goes_bad() && biggest_container_idx != intidx) { // you never know...
                  it->calc_rot(point(x,y));
                  if( it->has_rotten_away() ) {
                      it = tmpsub->itm[x][y].erase(it);
                  } else { ++it; intidx++; }
              } else { ++it; intidx++; }
          }

          if ( do_container_check == true && biggest_container_idx != -1 ) { // funnel: check. bucket: check
              item * it = &tmpsub->itm[x][y][biggest_container_idx];
              trap_id fun_trap_id = rain_backlog[point(x,y)];
              retroactively_fill_from_funnel( it, fun_trap_id, calendar(calendar::turn), point(x,y) ); // bucket: what inside??
          }

      }
  }

  // plantEpoch is half a season; 3 epochs pass from plant to harvest
  const int plantEpoch = 14400 * int(calendar::season_length()) / 2;

  // check plants for crops and seasonal harvesting.
  for (int x = 0; x < SEEX; x++) {
    for (int y = 0; y < SEEY; y++) {
      furn_id furn = tmpsub->get_furn(x, y);
      if (furn && furnlist[furn].has_flag("PLANT")) {
          if( tmpsub->itm[x][y].empty() ) {
              // No seed there anymore, we don't know what kind of plant it was.
              tmpsub->set_furn( x, y, f_null );
              continue;
          }
        item seed = tmpsub->itm[x][y][0];

        while (calendar::turn > seed.bday + plantEpoch && furn < f_plant_harvest) {
          furn = (furn_id((int)furn + 1));
          seed.bday += plantEpoch;

          // fixme; Lazy farmer drop rake on dirt mound. What happen rake?!
          tmpsub->itm[x][y].resize(1);
          tmpsub->itm[x][y][0].bday = seed.bday;
          tmpsub->set_furn(x, y, furn);
        }
      }
      ter_id ter = tmpsub->ter[x][y];
      //if the fruit-bearing season of the already harvested terrain has passed, make it harvestable again
      if ((ter) && (terlist[ter].has_flag(TFLAG_HARVESTED))){
        if ((terlist[ter].harvest_season != calendar::turn.get_season()) ||
        (calendar::turn - tmpsub->turn_last_touched > calendar::season_length()*14400)){
          tmpsub->set_ter(x, y, terfind(terlist[ter].transforms_into));
        }
      }
    }
  }
  // fixme; roll off into some function elsewhere ---^

  //Merchants will restock their inventories every three days
  const int merchantRestock = 14400 * 3; //14400 is the length of one day
  //Check for Merchants to restock
  if (g->active_npc.size() >= 1){
    for (auto &i : g->active_npc){
        if (i->restock != -1 && calendar::turn > (i->restock + merchantRestock)){
            i->shop_restock();
            i->restock = int(calendar::turn);
        }
    }
  }

  tmpsub->turn_last_touched = int(calendar::turn); // the last time we touched the submap, is right now.

 } else { // It doesn't exist; we must generate it!
  dbg(D_INFO|D_WARNING) << "map::loadn: Missing mapbuffer data. Regenerating.";
  tinymap tmp_map;
// overx, overy is where in the overmap we need to pull data from
// Each overmap square is two nonants; to prevent overlap, generate only at
//  squares divisible by 2.
  int newmapx = worldx + gridx - (abs(worldx + gridx) % 2);
  int newmapy = worldy + gridy - (abs(worldy + gridy) % 2);
  tmp_map.generate(newmapx, newmapy, worldz, calendar::turn);
  // This function is called again, but if mapgen failed (and did not create a submap),
  // this would lead to a infinite loop, this must be avoided.
  // This is the same call to MAPBUFFER as above!
  if( MAPBUFFER.lookup_submap( absx, absy, worldz ) == nullptr ) {
      dbg( D_ERROR ) << "failed to generate a submap at " << absx << absy << worldz;
      return;
  }
  loadn( worldx, worldy, worldz, gridx, gridy, update_vehicles );
 }
}

void map::copy_grid(const int to, const int from)
{
 grid[to] = grid[from];
 for( std::vector<vehicle*>::iterator it = grid[to]->vehicles.begin(),
       end = grid[to]->vehicles.end(); it != end; ++it ) {
  (*it)->smx = to % my_MAPSIZE;
  (*it)->smy = to / my_MAPSIZE;
 }

    const int oldx = (from % my_MAPSIZE) * SEEX;
    const int oldy = (from / my_MAPSIZE) * SEEY;
    const int newx = (to % my_MAPSIZE) * SEEX;
    const int newy = (to / my_MAPSIZE) * SEEY;
    for (int x = 0; x < SEEX; x++) {
        for (int y = 0; y < SEEY; y++) {
            trap_id t = grid[to]->get_trap(x, y);
            if (t != tr_null) {
                traplocs[t].erase(point(oldx + x, oldy + y));
                traplocs[t].insert(point(newx + x, newy + y));
            }
        }
    }
}

void map::spawn_monsters( int gx, int gy, mongroup &group, bool ignore_sight )
{
    const int s_range = std::min(SEEX * (MAPSIZE / 2), g->u.sight_range( g->light_level() ) );
    int pop = group.population;
    std::vector<point> locations;
    if( !ignore_sight ) {
        // If the submap is one of the outermost submaps, assume that monsters are
        // invisible there.
        // When the map shifts because of the player moving (called from game::plmove),
        // the player has still their *old* (not shifted) coordinates.
        // That makes the submaps that have come into view visible (if the sight range
        // is big enough).
        if( gx == 0 || gy == 0 || gx + 1 == MAPSIZE || gy + 1 == MAPSIZE ) {
            ignore_sight = true;
        }
    }
    for( int x = 0; x < SEEX; ++x ) {
        for( int y = 0; y < SEEY; ++y ) {
            int fx = x + SEEX * gx;
            int fy = y + SEEY * gy;
            if( g->critter_at( fx, fy ) != nullptr ) {
                continue; // there is already some creature
            }
            if( move_cost( fx, fy ) == 0 ) {
                continue; // solid area, impassable
            }
            int t;
            if( !ignore_sight && sees( g->u.posx, g->u.posy, fx, fy, s_range, t ) ) {
                continue; // monster must spawn outside the viewing range of the player
            }
            if( has_flag_ter_or_furn( TFLAG_INDOORS, fx, fy ) ) {
                continue; // monster must spawn outside.
            }
            locations.push_back( point( fx, fy ) );
        }
    }
    if( locations.empty() ) {
        // TODO: what now? there is now possible place to spawn monsters, most
        // likely because the player can see all the places.
        dbg( D_ERROR ) << "Empty locations for group " << group.type << " at " << gx << "," << gy;
        return;
    }
    for( int m = 0; m < pop; m++ ) {
        MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( group.type, &pop );
        if( spawn_details.name == "mon_null" ) {
            continue;
        }
        monster tmp( GetMType( spawn_details.name ) );
        for( int i = 0; i < spawn_details.pack_size; i++) {
            for( int tries = 0; tries < 10 && !locations.empty(); tries++ ) {
                const size_t index = rng( 0, locations.size() - 1 );
                const point p = locations[index];
                if( !tmp.can_move_to( p.x, p.y ) ) {
                    continue; // target can not contain the monster
                }
                tmp.spawn( p.x, p.y );
                g->add_zombie( tmp );
                locations.erase( locations.begin() + index );
                break;
            }
        }
    }
    // indicates the group is empty, and can be removed later
    group.population = 0;
}

void map::spawn_monsters(bool ignore_sight)
{
    for (int gx = 0; gx < my_MAPSIZE; gx++) {
        for (int gy = 0; gy < my_MAPSIZE; gy++) {
            auto groups = overmap_buffer.groups_at( abs_sub.x + gx, abs_sub.y + gy, abs_sub.z );
            for( auto &mgp : groups ) {
                spawn_monsters( gx, gy, *mgp, ignore_sight );
            }

            submap * const current_submap = get_submap_at_grid(gx, gy);
            for (auto &i : current_submap->spawns) {
                for (int j = 0; j < i.count; j++) {
                    int tries = 0;
                    int mx = i.posx, my = i.posy;
                    monster tmp(GetMType(i.type));
                    tmp.faction_id = i.faction_id;
                    tmp.mission_id = i.mission_id;
                    if (i.name != "NONE") {
                        tmp.unique_name = i.name;
                    }
                    if (i.friendly) {
                        tmp.friendly = -1;
                    }
                    int fx = mx + gx * SEEX, fy = my + gy * SEEY;

                    while ((!g->is_empty(fx, fy) || !tmp.can_move_to(fx, fy)) && tries < 10) {
                        mx = (i.posx + rng(-3, 3)) % SEEX;
                        my = (i.posy + rng(-3, 3)) % SEEY;
                        if (mx < 0) {
                            mx += SEEX;
                        }
                        if (my < 0) {
                            my += SEEY;
                        }
                        fx = mx + gx * SEEX;
                        fy = my + gy * SEEY;
                        tries++;
                    }
                    if (tries != 10) {
                        tmp.spawn(fx, fy);
                        g->add_zombie(tmp);
                    }
                }
            }
            current_submap->spawns.clear();
            overmap_buffer.spawn_monster( abs_sub.x + gx, abs_sub.y + gy, abs_sub.z );
        }
    }
}

void map::clear_spawns()
{
 for (int i = 0; i < my_MAPSIZE * my_MAPSIZE; i++) {
  grid[i]->spawns.clear();
 }
}

void map::clear_traps()
{
    for (int i = 0; i < my_MAPSIZE * my_MAPSIZE; i++) {
        for (int x = 0; x < SEEX; x++) {
            for (int y = 0; y < SEEY; y++) {
                grid[i]->set_trap(x, y, tr_null);
            }
        }
    }

    // Forget about all trap locations.
    std::map<trap_id, std::set<point> >::iterator i;
    for(i = traplocs.begin(); i != traplocs.end(); ++i) {
        i->second.clear();
    }
}

const std::set<point> &map::trap_locations(trap_id t) const
{
    const auto it = traplocs.find(t);
    if(it != traplocs.end()) {
        return it->second;
    }
    static std::set<point> empty_set;
    return empty_set;
}

bool map::inbounds(const int x, const int y) const
{
 return (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE);
}

void map::set_graffiti( int x, int y, const std::string &contents )
{
    if( !inbounds( x, y ) ) {
        return;
    }
    int lx, ly;
    submap *const current_submap = get_submap_at( x, y, lx, ly );
    current_submap->set_graffiti( lx, ly, contents );
}

void map::delete_graffiti( int x, int y )
{
    if( !inbounds( x, y ) ) {
        return;
    }
    int lx, ly;
    submap *const current_submap = get_submap_at( x, y, lx, ly );
    current_submap->delete_graffiti( lx, ly );
}

const std::string &map::graffiti_at( int x, int y ) const
{
    if( !inbounds( x, y ) ) {
        static const std::string empty_string;
        return empty_string;
    }
    int lx, ly;
    submap *const current_submap = get_submap_at( x, y, lx, ly );
    return current_submap->get_graffiti( lx, ly );
}

bool map::has_graffiti_at( int x, int y ) const
{
    if( !inbounds( x, y ) ) {
        return false;
    }
    int lx, ly;
    submap *const current_submap = get_submap_at( x, y, lx, ly );
    return current_submap->has_graffiti( lx, ly );
}

long map::determine_wall_corner(const int x, const int y, const long orig_sym)
{
    long sym = orig_sym;
    //LINE_NESW
    const long above = terlist[ter(x, y-1)].sym;
    const long below = terlist[ter(x, y+1)].sym;
    const long left  = terlist[ter(x-1, y)].sym;
    const long right = terlist[ter(x+1, y)].sym;

    const bool above_connects = above == sym || (above == '"' || above == '+' || above == '\'');
    const bool below_connects = below == sym || (below == '"' || below == '+' || below == '\'');
    const bool left_connects  = left  == sym || (left  == '"' || left  == '+' || left  == '\'');
    const bool right_connects = right == sym || (right == '"' || right == '+' || right == '\'');

    // -
    // |      this = - and above = | or a connectable
    if(sym == LINE_OXOX &&  (above == LINE_XOXO || above_connects))
    {
        //connects to upper
        if(left_connects)
            sym = LINE_XOOX; // ┘ left coming wall
        else if(right_connects)
            sym = LINE_XXOO;//└   right coming wall
        if(left_connects && right_connects)
            sym = LINE_XXOX; // ┴ passing by
    }

    // |
    // -      this = - and below = | or a connectable
    else if(sym == LINE_OXOX && (below == LINE_XOXO || below_connects))
    {
        //connects to lower
        if(left_connects)
            sym = LINE_OOXX; // ┐ left coming wall
        else if(right_connects)
            sym = LINE_OXXO;//┌   right coming wall
        if(left_connects && right_connects)
            sym = LINE_OXXX; // ┬ passing by
    }

    // -|       this = | and left = - or a connectable
    else if(sym == LINE_XOXO && (left == LINE_OXOX || left_connects))
    {
        //connexts to left
        if(above_connects)
            sym = LINE_XOOX; // ┘ north coming wall
        else if(below_connects )
            sym = LINE_OOXX;//┐   south coming wall
        if(above_connects && below_connects)
            sym = LINE_XOXX; // ┤ passing by
    }

    // |-       this = | and right = - or a connectable
    else if(sym == LINE_XOXO && (right == LINE_OXOX || right_connects))
    {
        //connects to right
        if(above_connects)
            sym = LINE_XXOO; // └ north coming wall
        else if(below_connects)
            sym = LINE_OXXO;// ┌   south coming wall
        if(above_connects && below_connects)
            sym = LINE_XXXO; // ├ passing by
    }

    if(above == LINE_XOXO && left == LINE_OXOX && above == below && left == right)
        sym = LINE_XXXX; // ┼ crossway

    return sym;
}

float map::light_transparency(const int x, const int y) const
{
  return transparency_cache[x][y];
}

void map::build_outside_cache()
{
    if (!outside_cache_dirty) {
        return;
    }

    if (g->levz < 0)
    {
        memset(outside_cache, false, sizeof(outside_cache));
        return;
    }
    memset(outside_cache, true, sizeof(outside_cache));

    for(int x = 0; x < SEEX * my_MAPSIZE; x++)
    {
        for(int y = 0; y < SEEY * my_MAPSIZE; y++)
        {
            if( has_flag_ter_or_furn(TFLAG_INDOORS, x, y))
            {
                for( int dx = -1; dx <= 1; dx++ )
                {
                    for( int dy = -1; dy <= 1; dy++ )
                    {
                        if(INBOUNDS(x + dx, y + dy))
                        {
                            outside_cache[x + dx][y + dy] = false;
                        }
                    }
                }
            }
        }
    }

    outside_cache_dirty = false;
}

// TODO Consider making this just clear the cache and dynamically fill it in as trans() is called
void map::build_transparency_cache()
{
    if( !transparency_cache_dirty ) {
        return;
    }
    for( int x = 0; x < my_MAPSIZE * SEEX; x++ ) {
        for( int y = 0; y < my_MAPSIZE * SEEY; y++ ) {
            // Default to fully transparent.
            transparency_cache[x][y] = LIGHT_TRANSPARENCY_CLEAR;

            if( !terlist[ter(x, y)].transparent || !furnlist[furn(x, y)].transparent ) {
                transparency_cache[x][y] = LIGHT_TRANSPARENCY_SOLID;
                continue;
            }

            field &curfield = field_at(x,y);
            if(curfield.fieldCount() > 0){
                field_entry *cur = NULL;
                for( auto field_list_it = curfield.getFieldStart();
                     field_list_it != curfield.getFieldEnd(); ++field_list_it ) {
                    cur = field_list_it->second;
                    if(cur == NULL) {
                        continue;
                    }

                    if( !fieldlist[cur->getFieldType()].transparent[cur->getFieldDensity() - 1] ) {
                        // Fields are either transparent or not, however we want some to be translucent
                        switch(cur->getFieldType()) {
                        case fd_cigsmoke:
                        case fd_weedsmoke:
                        case fd_cracksmoke:
                        case fd_methsmoke:
                        case fd_relax_gas:
                            transparency_cache[x][y] *= 0.7;
                            break;
                        case fd_smoke:
                        case fd_incendiary:
                        case fd_toxic_gas:
                        case fd_tear_gas:
                            if(cur->getFieldDensity() == 3) {
                                transparency_cache[x][y] = LIGHT_TRANSPARENCY_SOLID;
                            }
                            if(cur->getFieldDensity() == 2) {
                                transparency_cache[x][y] *= 0.5;
                            }
                            break;
                        case fd_nuke_gas:
                            transparency_cache[x][y] *= 0.5;
                            break;
                        default:
                            transparency_cache[x][y] = LIGHT_TRANSPARENCY_SOLID;
                            break;
                        }
                    }
                    // TODO: [lightmap] Have glass reduce light as well
                }
            }
        }
    }
    transparency_cache_dirty = false;
}

void map::build_map_cache()
{
    build_outside_cache();

    build_transparency_cache();

    // Cache all the vehicle stuff in one loop
    VehicleList vehs = get_vehicles();
    for(auto &v : vehs) {
        for (size_t part = 0; part < v.v->parts.size(); part++) {
            int px = v.x + v.v->parts[part].precalc_dx[0];
            int py = v.y + v.v->parts[part].precalc_dy[0];
            if(INBOUNDS(px, py)) {
                if (v.v->is_inside(part)) {
                    outside_cache[px][py] = false;
                }
                if (v.v->part_flag(part, VPFLAG_OPAQUE) && v.v->parts[part].hp > 0) {
                    int dpart = v.v->part_with_feature(part , VPFLAG_OPENABLE);
                    if (dpart < 0 || !v.v->parts[dpart].open) {
                        transparency_cache[px][py] = LIGHT_TRANSPARENCY_SOLID;
                    }
                }
            }
        }
    }

    build_seen_cache();
    generate_lightmap();
}

std::vector<point> closest_points_first(int radius, point p)
{
    return closest_points_first(radius, p.x, p.y);
}

//this returns points in a spiral pattern starting at center_x/center_y until it hits the radius. clockwise fashion
//credit to Tom J Nowell; http://stackoverflow.com/a/1555236/1269969
std::vector<point> closest_points_first(int radius, int center_x, int center_y)
{
    std::vector<point> points;
    int X,Y,x,y,dx,dy;
    X = Y = (radius * 2) + 1;
    x = y = dx = 0;
    dy = -1;
    int t = std::max(X,Y);
    int maxI = t * t;
    for(int i = 0; i < maxI; i++)
    {
        if ((-X/2 <= x) && (x <= X/2) && (-Y/2 <= y) && (y <= Y/2))
        {
            points.push_back(point(x + center_x, y + center_y));
        }
        if( (x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y)))
        {
            t = dx;
            dx = -dy;
            dy = t;
        }
        x += dx;
        y += dy;
    }
    return points;
}
//////////
///// coordinate helpers

point map::getabs(const int x, const int y) const
{
    return point( x + abs_sub.x * SEEX, y + abs_sub.y * SEEY );
}

point map::getlocal(const int x, const int y) const {
    return point( x - abs_sub.x * SEEX, y - abs_sub.y * SEEY );
}

void map::set_abs_sub(const int x, const int y, const int z)
{
    abs_sub = tripoint( x, y, z );
}

tripoint map::get_abs_sub() const
{
   return abs_sub;
}

submap * map::getsubmap( const int grididx ) {
    return grid[grididx];
}

submap *map::get_submap_at(int x, int y) const {
    // Do a bound check first.
    if(x >= SEEX * my_MAPSIZE || y >= SEEY * my_MAPSIZE || x < 0 || y < 0) {
        return NULL;
    }

    const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;
    return grid[nonant];
}

submap *map::get_submap_at(int x, int y, int& offset_x, int& offset_y) const {
    offset_x = x % SEEX;
    offset_y = y % SEEY;

    return get_submap_at(x, y);
}

submap *map::get_submap_at_grid(int gridx, int gridy) const {
    // Do a bound check first.
    if(gridx >= my_MAPSIZE || gridy >= my_MAPSIZE || gridx < 0 || gridy < 0) {
        return NULL;
    }

    const int nonant = gridx + gridy * my_MAPSIZE;
    return grid[nonant];
}

tinymap::tinymap(int mapsize)
: map(mapsize)
{
}

ter_id find_ter_id(const std::string id, bool complain=true) {
    (void)complain; //FIXME: complain unused
    if( termap.find(id) == termap.end() ) {
         debugmsg("Can't find termap[%s]",id.c_str());
         return 0;
    }
    return termap[id].loadid;
};

ter_id find_furn_id(const std::string id, bool complain=true) {
    (void)complain; //FIXME: complain unused
    if( furnmap.find(id) == furnmap.end() ) {
         debugmsg("Can't find furnmap[%s]",id.c_str());
         return 0;
    }
    return furnmap[id].loadid;
};
void map::draw_line_ter(const ter_id type, int x1, int y1, int x2, int y2)
{
    std::vector<point> line = line_to(x1, y1, x2, y2, 0);
    for (auto &i : line) {
        ter_set(i.x, i.y, type);
    }
    ter_set(x1, y1, type);
}
void map::draw_line_ter(const std::string type, int x1, int y1, int x2, int y2) {
    draw_line_ter(find_ter_id(type), x1, y1, x2, y2);
}


void map::draw_line_furn(furn_id type, int x1, int y1, int x2, int y2) {
    std::vector<point> line = line_to(x1, y1, x2, y2, 0);
    for (auto &i : line) {
        furn_set(i.x, i.y, type);
    }
    furn_set(x1, y1, type);
}
void map::draw_line_furn(const std::string type, int x1, int y1, int x2, int y2) {
    draw_line_furn(find_furn_id(type), x1, y1, x2, y2);
}

void map::draw_fill_background(ter_id type) {
    draw_square_ter(type, 0, 0, SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1);
}
void map::draw_fill_background(std::string type) {
    draw_square_ter(find_ter_id(type), 0, 0, SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1);
}
void map::draw_fill_background(ter_id (*f)()) {
    draw_square_ter(f, 0, 0, SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1);
}
void map::draw_fill_background(const id_or_id & f) {
    draw_square_ter(f, 0, 0, SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1);
}


void map::draw_square_ter(ter_id type, int x1, int y1, int x2, int y2) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            ter_set(x, y, type);
        }
    }
}
void map::draw_square_ter(std::string type, int x1, int y1, int x2, int y2) {
    draw_square_ter(find_ter_id(type), x1, y1, x2, y2);
}

void map::draw_square_furn(furn_id type, int x1, int y1, int x2, int y2) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            furn_set(x, y, type);
        }
    }
}
void map::draw_square_furn(std::string type, int x1, int y1, int x2, int y2) {
    draw_square_furn(find_furn_id(type), x1, y1, x2, y2);
}

void map::draw_square_ter(ter_id (*f)(), int x1, int y1, int x2, int y2) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            ter_set(x, y, f());
        }
    }
}

void map::draw_square_ter(const id_or_id & f, int x1, int y1, int x2, int y2) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            ter_set(x, y, f.get());
        }
    }
}

void map::draw_rough_circle(ter_id type, int x, int y, int rad) {
    for (int i = x - rad; i <= x + rad; i++) {
        for (int j = y - rad; j <= y + rad; j++) {
            if (rl_dist(x, y, i, j) + rng(0, 3) <= rad) {
                ter_set(i, j, type);
            }
        }
    }
}
void map::draw_rough_circle(std::string type, int x, int y, int rad) {
    draw_rough_circle(find_ter_id(type), x, y, rad);
}

void map::draw_rough_circle_furn(furn_id type, int x, int y, int rad) {
    for (int i = x - rad; i <= x + rad; i++) {
        for (int j = y - rad; j <= y + rad; j++) {
            if (rl_dist(x, y, i, j) + rng(0, 3) <= rad) {
                furn_set(i, j, type);
            }
        }
    }
}
void map::draw_rough_circle_furn(std::string type, int x, int y, int rad) {
    draw_rough_circle(find_furn_id(type), x, y, rad);
}

void map::add_corpse(int x, int y) {
    item body;

    const bool isReviveSpecial = one_in(10);

    if (!isReviveSpecial){
        body.make_corpse("corpse", GetMType("mon_null"), 0);
    } else {
        body.make_corpse("corpse", GetMType("mon_zombie"), 0);
        body.item_tags.insert("REVIVE_SPECIAL");
        body.active = true;
    }

    add_item_or_charges(x, y, body);
    put_items_from("shoes",  1, x, y, 0, 0, 0);
    put_items_from("pants",  1, x, y, 0, 0, 0);
    put_items_from("shirts", 1, x, y, 0, 0, 0);
    if (one_in(6)) {
        put_items_from("jackets", 1, x, y, 0, 0, 0);
    }
    if (one_in(15)) {
        put_items_from("bags", 1, x, y, 0, 0, 0);
    }
}

/**
 * Adds vehicles to the current submap, selected from a random weighted
 * distribution of possible vehicles. If the road has a pavement, then set the
 * 'city' flag to true to spawn wrecks. If it doesn't (ie, highway or country
 * road,) then set 'city' to false to spawn far fewer vehicles that are out
 * of gas instead of wrecked.
 * @param city Whether or not to spawn city wrecks.
 * @param facing The direction the spawned car should face (multiple of 90).
 */
void map::add_road_vehicles(bool city, int facing)
{
    if (city) {
        int spawn_type = rng(0, 100);
        if(spawn_type <= 33) {
            //Randomly-distributed wrecks
            int maxwrecks = rng(1, 3);
            for (int nv = 0; nv < maxwrecks; nv++) {
                int vx = rng(0, 19);
                int vy = rng(0, 19);
                int car_type = rng(1, 100);
                if (car_type <= 25) {
                    add_vehicle("car", vx, vy, facing, -1, 1);
                } else if (car_type <= 30) {
                    add_vehicle("policecar", vx, vy, facing, -1, 1);
                } else if (car_type <= 40) {
                    add_vehicle("ambulance", vx, vy, facing, -1, 1);
                } else if (car_type <= 45) {
                    add_vehicle("beetle", vx, vy, facing, -1, 1);
                } else if (car_type <= 48) {
                    add_vehicle("car_sports", vx, vy, facing, -1, 1);
                } else if (car_type <= 50) {
                    add_vehicle("scooter", vx, vy, facing, -1, 1);
                } else if (car_type <= 53) {
                    add_vehicle("scooter_electric", vx, vy, facing, -1, 1);
                } else if (car_type <= 55) {
                    add_vehicle("motorcycle", vx, vy, facing, -1, 1);
                } else if (car_type <= 65) {
                    add_vehicle("hippie_van", vx, vy, facing, -1, 1);
                } else if (car_type <= 70) {
                    add_vehicle("cube_van_cheap", vx, vy, facing, -1, 1);
                } else if (car_type <= 75) {
                    add_vehicle("cube_van", vx, vy, facing, -1, 1);
                } else if (car_type <= 80) {
                    add_vehicle("electric_car", vx, vy, facing, -1, 1);
                } else if (car_type <= 90) {
                    add_vehicle("flatbed_truck", vx, vy, facing, -1, 1);
                } else if (car_type <= 95) {
                    add_vehicle("rv", vx, vy, facing, -1, 1);
                } else if (car_type <= 96) {
                    add_vehicle("lux_rv", vx, vy, facing, -1, 1);
                } else if (car_type <= 98) {
                    add_vehicle("meth_lab", vx, vy, facing, -1, 1);
                } else if (car_type <= 99) {
                    add_vehicle("apc", vx, vy, facing, -1, 1);
                } else {
                    add_vehicle("motorcycle_sidecart", vx, vy, facing, -1, 1);
                }
            }
        } else if(spawn_type <= 66) {
            //Parked vehicles
            int veh_x = 0;
            int veh_y = 0;
            if(facing == 0) {
                veh_x = rng(4, 16);
                veh_y = 17;
            } else if(facing == 90) {
                veh_x = 6;
                veh_y = rng(4, 16);
            } else if(facing == 180) {
                veh_x = rng(4, 16);
                veh_y = 6;
            } else if(facing == 270) {
                veh_x = 17;
                veh_y = rng(4, 16);
            }
            int veh_type = rng(0, 100);
            if(veh_type <= 70) {
                add_vehicle("car", veh_x, veh_y, facing, -1, 1);
            } else if(veh_type <= 92) {
                add_vehicle("electric_car", veh_x, veh_y, facing, -1, 1);
            } else if(veh_type <= 95) {
                add_vehicle("road_roller", veh_x, veh_y, facing, -1, 1);
            } else {
                add_vehicle("policecar", veh_x, veh_y, facing, -1, 1);
            }
        } else if(spawn_type <= 99) {
            //Totally clear section of road
            return;
        } else {
            //Road-blocking obstacle of some kind.
            int block_type = rng(0, 100);
            if(block_type <= 75) {
                //Jack-knifed semi
                int semi_x = 0;
                int semi_y = 0;
                int trailer_x = 0;
                int trailer_y = 0;
                if(facing == 0) {
                    semi_x = rng(0, 16);
                    semi_y = rng(14, 16);
                    trailer_x = semi_x + 4;
                    trailer_y = semi_y - 10;
                } else if(facing == 90) {
                    semi_x = rng(0, 8);
                    semi_y = rng(4, 15);
                    trailer_x = semi_x + 12;
                    trailer_y = semi_y + 1;
                } else if(facing == 180) {
                    semi_x = rng(4, 16);
                    semi_y = rng(4, 6);
                    trailer_x = semi_x - 4;
                    trailer_y = semi_y + 10;
                } else {
                    semi_x = rng(12, 20);
                    semi_y = rng(5, 16);
                    trailer_x = semi_x - 12;
                    trailer_y = semi_y - 1;
                }
                add_vehicle("semi_truck", semi_x, semi_y, (facing + 135) % 360, -1, 1);
                add_vehicle("truck_trailer", trailer_x, trailer_y, (facing + 90) % 360, -1, 1);
            } else {
                //Huge pileup of random vehicles
                std::string next_vehicle;
                int num_cars = rng(18, 22);
                bool policecars = block_type >= 95; //Policecar pileup, Blues Brothers style
                vehicle *last_added_car = NULL;
                for(int i = 0; i < num_cars; i++) {
                    if(policecars) {
                        next_vehicle = "policecar";
                    } else {
                        //Random car
                        int car_type = rng(0, 100);
                        if(car_type <= 70) {
                            next_vehicle = "car";
                        } else if(car_type <= 90) {
                            next_vehicle = "flatbed_truck";
                        } else if(car_type <= 95) {
                            next_vehicle = "cube_van";
                        } else {
                            next_vehicle = "hippie_van";
                        }
                    }
                    last_added_car = add_vehicle(next_vehicle, rng(4, 16), rng(4, 16), rng(0, 3) * 90, -1, 1);
                }

                //Hopefully by the last one we've got a giant pileup, so name it
                if (last_added_car != NULL) {
                    if(policecars) {
                        last_added_car->name = _("policecar pile-up");
                    } else {
                        last_added_car->name = _("pile-up");
                    }
                }
            }
        }
    } else {
        // spawn regular road out of fuel vehicles
        if (one_in(40)) {
            int vx = rng(8, 16);
            int vy = rng(8, 16);
            int car_type = rng(1, 27);
            if (car_type <= 10) {
                add_vehicle("car", vx, vy, facing, 0, -1);
            } else if (car_type <= 14) {
                add_vehicle("car_sports", vx, vy, facing, 0, -1);
            } else if (car_type <= 16) {
                add_vehicle("flatbed_truck", vx, vy, facing, 0, -1);
            } else if (car_type <= 18) {
                add_vehicle("semi_truck", vx, vy, facing, 0, -1);
            } else if (car_type <= 20) {
                add_vehicle("humvee", vx, vy, facing, 0, -1);
            } else if (car_type <= 24) {
                add_vehicle("rara_x", vx, vy, facing, 0, -1);
            } else if (car_type <= 25) {
                add_vehicle("apc", vx, vy, facing, 0, -1);
            } else {
                add_vehicle("armored_car", vx, vy, facing, 0, -1);
            }
        }
    }
}

// Item stack functions.
item_stack::iterator item_stack::begin() {
    return item_stack::iterator(this, 0);
}

item_stack::iterator item_stack::end() {
    return item_stack::iterator(this, this->size());
}

item_stack::iterator item_stack::at(int index) {
    return begin() + index;
}

size_t item_stack::size() const {
    if (!items) {
        return 0;
    }
    return items->size();
}

bool item_stack::empty() const {
    if (!items) {
        return 1;
    }
    return items->empty();
}

const item &item_stack::operator[](size_t index) {
    return (*items)[index];
}

item_stack::item_modifier item_stack::iterator::modify() {
    item_stack::item_modifier rval(*this);
    rval = (*stack->items)[stack_index];
    return rval;
}

void item_stack::push_back(const item& new_item) {
    if (!is_vehicle) {
        map_container->add_item_or_charges(x, y, new_item);
    } else {
        vehicle_container->add_item(part, new_item);
    }
}

void item_stack::iterator::erase() {
    if (!stack->is_vehicle) {
        stack->map_container->i_rem(stack->x, stack->y, stack_index);
    } else {
        stack->vehicle_container->remove_item(stack->part, stack_index);
    }
    stack_index--;
}

item_stack::iterator& item_stack::iterator::operator++(int) {
    stack_index++;
    return *this;
}

item_stack::iterator& item_stack::iterator::operator++() {
    stack_index++;
    return *this;
}

item_stack::iterator item_stack::iterator::operator+(int offset) {
    iterator rval = *this;
    rval.stack_index += offset;
    return rval;
}

bool item_stack::iterator::operator!=(const item_stack::iterator& other) const {
    return (stack_index != other.stack_index);
}

const item& item_stack::iterator::operator*() const {
    return *(this->operator->());
}

const item* item_stack::iterator::operator->() const {
    return &(*stack->items)[stack_index];
}

void item_stack::iterator::commit(const item_modifier& it) {
    (*stack->items)[stack_index] = it;
}

item_stack::iterator::iterator(const item_stack *stack, int stack_index)
    : stack(stack), stack_index(stack_index) { }

void item_stack::item_modifier::operator=(const item& other) {
    item::operator=(other);
}

void item_stack::item_modifier::commit() {
    iter.commit(*this);
}
