#include "vehicle.h"
#include "map.h"
#include "output.h"
#include "game.h"
#include "item.h"
#include <sstream>
#include <stdlib.h>
#include "cursesdef.h"
#include "catacharset.h"

const ammotype fuel_types[num_fuel_types] = { "gasoline", "battery", "plutonium", "plasma", "water" };

enum vehicle_controls {
 toggle_cruise_control,
 toggle_lights,
 toggle_turrets,
 release_control,
 control_cancel,
 convert_vehicle
};

vehicle::vehicle(game *ag, vhtype_id type_id, int init_veh_fuel, int init_veh_status): g(ag), type(type_id)
{
    posx = 0;
    posy = 0;
    velocity = 0;
    turn_dir = 0;
    last_turn = 0;
    of_turn_carry = 0;
    turret_mode = 0;
    cruise_velocity = 0;
    skidding = false;
    cruise_on = true;
    lights_on = false;
    insides_dirty = true;

    if (type >= num_vehicles)
        type = 0;
    if (type > veh_custom)
    {   // get a copy of sample vehicle of this type
        if (type < g->vtypes.size())
        {
            *this = *(g->vtypes[type]);
            init_state(ag,init_veh_fuel,init_veh_status);
        }
    }
    precalc_mounts(0, face.dir());
}

vehicle::~vehicle()
{
}

bool vehicle::player_in_control (player *p)
{
    if (type == veh_null)
        return false;
    int veh_part;
    vehicle *veh = g->m.veh_at (p->posx, p->posy, veh_part);
    if (veh && veh != this)
        return false;
    return part_with_feature(veh_part, vpf_controls, false) >= 0 && p->controlling_vehicle;
}

void vehicle::load (std::ifstream &stin)
{
    int t;
    int fdir, mdir, skd, prts, cr_on, li_on, tag_count;
    std::string vehicle_tag;
    stin >>
        t >>
        posx >>
        posy >>
        fdir >>
        mdir >>
        turn_dir >>
        velocity >>
        cruise_velocity >>
        cr_on >>
        li_on >>
        turret_mode >>
        skd >>
        of_turn_carry >>
        prts;
    type = (vhtype_id) t;
    face.init (fdir);
    move.init (mdir);
    skidding = skd != 0;
    cruise_on = cr_on != 0;
    lights_on = li_on != 0;
    std::string databuff;
    getline(stin, databuff); // Clear EoL
    getline(stin, name); // read name
    int itms = 0;
    for (int p = 0; p < prts; p++)
    {
        int pid, pdx, pdy, php, pam, pbld, pbig, pflag, pass, pnit;
        stin >> pid >> pdx >> pdy >> php >> pam >> pbld >> pbig >> pflag >> pass >> pnit;
        getline(stin, databuff); // Clear EoL
        vehicle_part new_part;
        new_part.id = (vpart_id) pid;
        new_part.mount_dx = pdx;
        new_part.mount_dy = pdy;
        new_part.hp = php;
        new_part.blood = pbld;
        new_part.bigness = pbig;
        new_part.flags = pflag;
        new_part.passenger_id = pass;
        new_part.amount = pam;
        for (int j = 0; j < pnit; j++)
        {
            itms++;
            getline(stin, databuff);
            item itm;
            itm.load_info (databuff, g);
            new_part.items.push_back (itm);
            int ncont;
            stin >> ncont; // how many items inside container
            getline(stin, databuff); // Clear EoL
            for (int k = 0; k < ncont; k++)
            {
                getline(stin, databuff);
                item citm;
                citm.load_info (databuff, g);
                new_part.items[new_part.items.size()-1].put_in (citm);
            }
        }
        parts.push_back (new_part);
    }
    find_external_parts ();
    find_exhaust ();
    insides_dirty = true;
    precalc_mounts (0, face.dir());

    stin >> tag_count;
    for( int i = 0; i < tag_count; ++i )
    {
        stin >> vehicle_tag;
        tags.insert( vehicle_tag );
    }
    getline(stin, databuff); // Clear EoL
}

void vehicle::save (std::ofstream &stout)
{
    stout <<
        int(type) << " " <<
        posx << " " <<
        posy << " " <<
        face.dir() << " " <<
        move.dir() << " " <<
        turn_dir << " " <<
        velocity << " " <<
        cruise_velocity << " " <<
        (cruise_on? 1 : 0) << " " <<
        (lights_on? 1 : 0) << " " <<
        turret_mode << " " <<
        (skidding? 1 : 0) << " " <<
        of_turn_carry << " " <<
        parts.size() << std::endl;
    stout << name << std::endl;

    for (int p = 0; p < parts.size(); p++)
    {
        stout <<
            parts[p].id << " " <<
            parts[p].mount_dx << " " <<
            parts[p].mount_dy << " " <<
            parts[p].hp << " " <<
            parts[p].amount << " " <<
            parts[p].blood << " " <<
            parts[p].bigness << " " <<
            parts[p].flags << " " <<
            parts[p].passenger_id << " " <<
            parts[p].items.size() << std::endl;
            for (int i = 0; i < parts[p].items.size(); i++)
            {
                stout << parts[p].items[i].save_info() << std::endl;     // item info
                stout << parts[p].items[i].contents.size() << std::endl; // how many items inside this item
                for (int l = 0; l < parts[p].items[i].contents.size(); l++)
                    stout << parts[p].items[i].contents[l].save_info() << std::endl; // contents info
            }
    }

    stout << tags.size() << ' ';
    for( std::set<std::string>::const_iterator it = tags.begin(); it != tags.end(); ++it )
    {
        stout << *it << " ";
    }
    stout << std::endl;
}

void vehicle::init_state(game* g, int init_veh_fuel, int init_veh_status)
{
    bool destroyEngine = false;
    bool destroyTires = false;

    int consistent_bignesses[num_vparts];
    memset (consistent_bignesses, 0, sizeof(consistent_bignesses));

    // veh_fuel_multiplier is percentage of fuel
    // 0 is empty, 100 is full tank, -1 is random 1% to 7%
    int veh_fuel_mult = init_veh_fuel;
    if (init_veh_fuel == - 1)
     veh_fuel_mult = rng (1,7);
    if (init_veh_fuel > 100)
     veh_fuel_mult = 100;

    // veh_status is initial vehicle damage
    // -1 = light damage (DEFAULT)
    //  0 = undamgaed
    //  1 = disabled, destroyed tires OR engine
    int veh_status = -1;
    if (init_veh_status == 0)
     veh_status = 0;
    if (init_veh_status == 1) {
     veh_status = 1;
     if (one_in(2)) {  // either engine or tires are destroyed
      destroyEngine = true;
     } else {
      destroyTires = true;
     }
    }

    for (int p = 0; p < parts.size(); p++)
    {
        if (part_flag(p, vpf_variable_size)){ // generate its bigness attribute.?
            if(!consistent_bignesses[parts[p].id]){
                //generate an item for this type, & cache its bigness
                item tmp (g->itypes[part_info(p).item], 0);
                consistent_bignesses[parts[p].id] = tmp.bigness;
            }
            parts[p].bigness = consistent_bignesses[parts[p].id];
        }
        if (part_flag(p, vpf_fuel_tank))   // set fuel status
          parts[p].amount = part_info(p).size * veh_fuel_mult / 100;

        if (part_flag(p, vpf_openable))    // doors are closed
            parts[p].open = 0;
        if (part_flag(p, vpf_boardable))        // no passengers
            parts[p].remove_flag(vehicle_part::passenger_flag);

        // initial vehicle damage
        if (veh_status == 0) {
            // Completely mint condition vehicle
            parts[p].hp= part_info(p).durability;
        } else {
         //a bit of initial damage :)
         //clamp 4d8 to the range of [8,20]. 8=broken, 20=undamaged.
         int broken = 8, unhurt = 20;
         int roll = dice(4,8);
         if(roll < unhurt){
            if (roll <= broken)
               parts[p].hp= 0;
            else
               parts[p].hp= ((float)(roll-broken) / (unhurt-broken)) * part_info(p).durability;
         }
         else // new.
            parts[p].hp= part_info(p).durability;

         if (destroyEngine) { // vehicle is disabled because engine is dead
          if (part_flag(p, vpf_engine))
           parts[p].hp= 0;
         }
         if (destroyTires) { // vehicle is disabled because flat tires
          if (part_flag(p, vpf_wheel))
             parts[p].hp= 0;
         }
        }
    }
}

std::string vehicle::use_controls()
{
 std::vector<vehicle_controls> options_choice;
 std::vector<uimenu_entry> options_message;
 // Alway have this option
 int curent=0;
 int letgoent=0;
 options_choice.push_back(toggle_cruise_control);
 options_message.push_back(uimenu_entry((cruise_on) ? _("Disable cruise control") : _("Enable cruise control"), 'c'));
 curent++;

 bool has_lights = false;
 bool has_turrets = false;
 for (int p = 0; p < parts.size(); p++) {
  if (part_flag(p, vpf_light))
   has_lights = true;
  else if (part_flag(p, vpf_turret))
   has_turrets = true;
 }

 // Lights if they are there - Note you can turn them on even when damaged, they just don't work
 if (has_lights) {
  options_choice.push_back(toggle_lights);
  options_message.push_back(uimenu_entry((lights_on) ? _("Turn off headlights") : _("Turn on headlights"), 'h'));
  curent++;
 }

 // Turrents: off or burst mode
 if (has_turrets) {
  options_choice.push_back(toggle_turrets);
  options_message.push_back(uimenu_entry((0 == turret_mode) ? _("Switch turrets to burst mode") : _("Disable turrets"), 't'));
  curent++;
 }

 if( !g->u.controlling_vehicle && tags.count("convertible") ) {
  options_choice.push_back(convert_vehicle);
  options_message.push_back(uimenu_entry(_("Fold bicycle"), 'f'));
  curent++;
 }

 // Exit vehicle, if we are in it.
 int vpart;
 if (g->u.controlling_vehicle &&
     g->m.veh_at(g->u.posx, g->u.posy, vpart) == this) {
  options_choice.push_back(release_control);
  options_message.push_back(uimenu_entry(_("Let go of controls"), 'l'));
  letgoent=curent;
 }

 options_choice.push_back(control_cancel);
 options_message.push_back(uimenu_entry(_("Do nothing"), ' '));

 uimenu selectmenu;
 selectmenu.text=_("Vehicle controls");
 selectmenu.entries=options_message;
 selectmenu.selected=letgoent;
 selectmenu.query();
 int select=selectmenu.ret;
// int select = menu_vec(true, "Vehicle controls", options_message);

 std::string message;
 switch(options_choice[select]) {
  case toggle_cruise_control:
   cruise_on = !cruise_on;
   message = (cruise_on) ? _("Cruise control turned on") : _("Cruise control turned off");
   break;
  case toggle_lights:
   lights_on = !lights_on;
   message = (lights_on) ? _("Headlights turned on") : _("Headlights turned off");
   break;
  case toggle_turrets:
   if (++turret_mode > 1)
    turret_mode = 0;
   message = (0 == turret_mode) ? _("Turrets: Disabled") : _("Turrets: Burst mode");
   break;
  case release_control:
   g->u.controlling_vehicle = false;
   g->add_msg(_("You let go of the controls."));
   break;
  case convert_vehicle:
  {
   g->add_msg(_("You painstakingly pack the bicycle into a portable configuration."));
   // create a folding bicycle item
   item bicycle;
   bicycle.make(g->itypes["folding_bicycle"]);

   std::ostringstream part_hps;
   // Stash part HP in item
   for (int p = 0; p < parts.size(); p++)
   {
       part_hps << parts[p].hp << " ";
       if( part_flag( p, vpf_cargo ) ) {
           for( std::vector<item>::iterator it = parts[p].items.begin();
                it != parts[p].items.end(); ++it) {
               g->m.add_item_or_charges( g->u.posx, g->u.posy, *it );
           }
       }
   }
   bicycle.item_vars["folding_bicycle_parts"] = part_hps.str();

   g->m.add_item_or_charges(g->u.posx, g->u.posy, bicycle);
   // Remove vehicle
   unboard_all();
   g->m.destroy_vehicle(this);

   g->u.moves -= 500;
   break;
  }
  case control_cancel:
   break;
 }
 return message;
}

const vpart_info& vehicle::part_info (int index)
{
    vpart_id id = vp_null;
    if (index < 0 || index >= parts.size())
        id = vp_null;
    else
        id = parts[index].id;
    if (id < vp_null || id >= num_vparts)
        id = vp_null;
    return vpart_list[id];
}

// engines & solar panels all have power.
// engines consume, whilst panels provide.
int vehicle::part_power (int index){
   if (!part_flag(index, vpf_engine) &&
       !part_flag(index, vpf_solar_panel))
      return 0; //not an engine.
   if(parts[index].hp <= 0)
      return 0; //broken.
   if(part_flag (index, vpf_variable_size)){ // example: 2.42-L V-twin engine
      return parts[index].bigness;
   }
   else // example: foot crank
      return part_info(index).power;
}

bool vehicle::can_mount (int dx, int dy, vpart_id id)
{
    if (id <= 0 || id >= num_vparts)
        return false;
    bool n3ar = parts.size() < 1 || (vpart_list[id].flags & mfb(vpf_internal))
                                 || (vpart_list[id].flags & mfb(vpf_over)); // first and internal parts needs no mount point
    if (!n3ar)
        for (int i = 0; i < 4; i++)
        {
            int ndx = i < 2? (i == 0? -1 : 1) : 0;
            int ndy = i < 2? 0 : (i == 2? - 1: 1);
            std::vector<int> parts_n3ar = parts_at_relative (dx + ndx, dy + ndy);
            if (parts_n3ar.size() < 1)
                continue;
            if (part_flag(parts_n3ar[0], vpf_mount_point))
            {
                n3ar = true;
                break;
            }
        }
    if (!n3ar)
    {
        return false; // no point to mount
    }

    std::vector<int> parts_here = parts_at_relative (dx, dy);
    if (parts_here.size() < 1)
    {
        int res = vpart_list[id].flags & mfb(vpf_external);
        return res; // can be mounted if first and external
    }

    // Override for replacing a tire.
    if( vpart_list[id].flags & mfb(vpf_wheel) &&
        -1 != part_with_feature(parts_here[0], vpf_wheel, false) )
    {
        return true;
    }

    int flags1 = part_info(parts_here[0]).flags;
    if ((vpart_list[id].flags & mfb(vpf_armor)) && flags1 & mfb(vpf_no_reinforce))
    {
        return false;   // trying to put armor plates on non-reinforcable part
    }
    // Seatbelts require an anchor point
    if( vpart_list[id].flags & mfb(vpf_seatbelt) )
    {
        bool anchor_found = false;
        for( std::vector<int>::iterator it = parts_here.begin();
             it != parts_here.end(); ++it )
        {
            if( part_info(*it).flags & mfb(vpf_anchor_point) )
            {
                anchor_found = true;
            }
        }
        if( anchor_found == false)
        {
            return false;
        }
    }

    for (int vf = vpf_func_begin; vf <= vpf_func_end; vf++)
        if ((vpart_list[id].flags & mfb(vf)) && part_with_feature(parts_here[0], vf, false) >= 0)
        {
            return false;   // this part already has inner part with same unique feature
        }

    bool allow_inner = flags1 & mfb(vpf_mount_inner);
    bool allow_over  = flags1 & mfb(vpf_mount_over);
    bool this_inner  = vpart_list[id].flags & mfb(vpf_internal);
    bool this_over   = (vpart_list[id].flags & mfb(vpf_over)) || (vpart_list[id].flags & mfb(vpf_armor));
    if (allow_inner && (this_inner || this_over))
        return true; // can mount as internal part or over it
    if (allow_over && this_over)
        return true; // can mount as part over
    return false;
}

bool vehicle::can_unmount (int p)
{
    int dx = parts[p].mount_dx;
    int dy = parts[p].mount_dy;
    if (!dx && !dy)
    { // central point
        bool is_ext = false;
        for (int ep = 0; ep < external_parts.size(); ep++)
            if (external_parts[ep] == p)
            {
                is_ext = true;
                break;
            }
        if (external_parts.size() > 1 && is_ext)
            return false; // unmounting 0, 0 part anly allowed as last part
    }

    if (!part_flag (p, vpf_mount_point))
        return true;
    for (int i = 0; i < 4; i++)
    {
        int ndx = i < 2? (i == 0? -1 : 1) : 0;
        int ndy = i < 2? 0 : (i == 2? - 1: 1);
        if (!(dx + ndx) && !(dy + ndy))
            continue; // 0, 0 point is main mount
        if (parts_at_relative (dx + ndx, dy + ndy).size() > 0)
        {
            int cnt = 0;
            for (int j = 0; j < 4; j++)
            {
                int jdx = j < 2? (j == 0? -1 : 1) : 0;
                int jdy = j < 2? 0 : (j == 2? - 1: 1);
                std::vector<int> pc = parts_at_relative (dx + ndx + jdx, dy + ndy + jdy);
                if (pc.size() > 0 && part_with_feature (pc[0], vpf_mount_point) >= 0)
                    cnt++;
            }
            if (cnt < 2)
                return false;
        }
    }
    return true;
}

int vehicle::install_part (int dx, int dy, vpart_id id, int hp, bool force)
{
    if (!force && !can_mount (dx, dy, id))
        return -1;  // no money -- no ski!
    // if this is first part, add this part to list of external parts
    if (parts_at_relative (dx, dy).size () < 1)
        external_parts.push_back (parts.size());
    vehicle_part new_part;
    new_part.id = id;
    new_part.mount_dx = dx;
    new_part.mount_dy = dy;
    new_part.hp = hp < 0? vpart_list[id].durability : hp;
    new_part.amount = 0;
    new_part.blood = 0;
    item tmp(g->itypes[vpart_list[id].item], 0);
    new_part.bigness = tmp.bigness;
    parts.push_back (new_part);

    find_exhaust ();
    precalc_mounts (0, face.dir());
    insides_dirty = true;
    return parts.size() - 1;
}

// share damage & bigness betwixt veh_parts & items.
void vehicle::get_part_properties_from_item(game* g, int partnum, item& i){
    //transfer bigness if relevant.
    itype_id  pitmid = part_info(partnum).item;
    itype* itemtype = g->itypes[pitmid];
    if(itemtype->is_var_veh_part())
       parts[partnum].bigness = i.bigness;

    // item damage is 0,1,2,3, or 4. part hp is 1..durability.
    // assuming it rusts. other item materials disentigrate at different rates...
    int health = 5 - i.damage;
    health *= part_info(partnum).durability; //[0,dur]
    health /= 5;
    parts[partnum].hp = health;
}
void vehicle::give_part_properties_to_item(game* g, int partnum, item& i){
    //transfer bigness if relevant.
    itype_id  pitmid = part_info(partnum).item;
    itype* itemtype = g->itypes[pitmid];
    if(itemtype->is_var_veh_part())
       i.bigness = parts[partnum].bigness;
       
    // remove charges if part is made of a tool
    if(itemtype->is_tool())
        i.charges = 0;       

    // translate part damage to item damage.
    // max damage is 4, min damage 0.
    // this is very lossy.
    int dam;
    float hpofdur = (float)parts[partnum].hp / part_info(partnum).durability;
    dam = (1 - hpofdur) * 5;
    if (dam > 4) dam = 4;
    if (dam < 0) dam = 0;
    i.damage = dam;
}

void vehicle::remove_part (int p)
{
    parts.erase(parts.begin() + p);
    find_external_parts ();
    find_exhaust ();
    precalc_mounts (0, face.dir());
    insides_dirty = true;
}

item vehicle::item_from_part( int part )
{
    itype_id itm = part_info(part).item;
    int bigness = parts[part].bigness;
    itype* parttype = g->itypes[itm];
    item tmp(parttype, g->turn);

    //transfer damage, etc.
    give_part_properties_to_item(g, part, tmp);
    if( parttype->is_var_veh_part() ) {
        tmp.bigness = bigness;
    }
    return tmp;
}

std::vector<int> vehicle::parts_at_relative (int dx, int dy)
{
    std::vector<int> res;
    for (int i = 0; i < parts.size(); i++)
        if (parts[i].mount_dx == dx && parts[i].mount_dy == dy)
            res.push_back (i);
    return res;
}

std::vector<int> vehicle::internal_parts (int p)
{
    std::vector<int> res;
    for (int i = p + 1; i < parts.size(); i++)
        if (parts[i].mount_dx == parts[p].mount_dx && parts[i].mount_dy == parts[p].mount_dy)
            res.push_back (i);
    return res;
}

int vehicle::part_with_feature (int p, unsigned int f, bool unbroken)
{
    if (part_flag(p, f))
        return p;
    std::vector<int> parts_here = internal_parts (p);
    for (int i = 0; i < parts_here.size(); i++)
        if (part_flag(parts_here[i], f) && (!unbroken || parts[parts_here[i]].hp > 0))
            return parts_here[i];
    return -1;
}

bool vehicle::part_flag (int p, unsigned int f)
{
    if (p < 0 || p >= parts.size())
        return false;
    return (bool) (part_info(p).flags & mfb (f));
}

int vehicle::part_at(int dx, int dy)
{
    for (int i = 0; i < external_parts.size(); i++)
    {
        int p = external_parts[i];
        if (parts[p].precalc_dx[0] == dx &&
            parts[p].precalc_dy[0] == dy)
            return p;
    }
    return -1;
}

int vehicle::global_part_at(int x, int y)
{
 int dx = x - global_x();
 int dy = y - global_y();
 return part_at(dx,dy);
}

char vehicle::part_sym (int p)
{
    if (p < 0 || p >= parts.size())
        return 0;
    std::vector<int> ph = internal_parts (p);
    int po = part_with_feature(p, vpf_over, false);
    int pd = po < 0? p : po;
    if (part_flag (pd, vpf_openable) && parts[pd].open)
        return '\''; // open door
    return parts[pd].hp <= 0? part_info(pd).sym_broken : part_info(pd).sym;
}

nc_color vehicle::part_color (int p)
{
    if (p < 0 || p >= parts.size())
        return c_black;
    int parm = part_with_feature(p, vpf_armor, false);
    int po = part_with_feature(p, vpf_over, false);
    int pd = po < 0? p : po;
    if (parts[p].blood > 200)
        return c_red;
    else
    if (parts[p].blood > 0)
        return c_ltred;

    if (parts[pd].hp <= 0)
        return part_info(pd).color_broken;

    // first, check if there's a part over. then, if armor here (projects its color on part)
    if (po >= 0)
        return part_info(po).color;
    else
    if (parm >= 0)
        return part_info(parm).color;

    return part_info(pd).color;
}

void vehicle::print_part_desc (void *w, int y1, int width, int p, int hl)
{
    WINDOW *win = (WINDOW *) w;
    if (p < 0 || p >= parts.size())
        return;
    std::vector<int> pl = internal_parts (p);
    pl.insert (pl.begin(), p);
    int y = y1;
    for (int i = 0; i < pl.size(); i++)
    {
        int dur = part_info (pl[i]).durability;
        int per_cond = parts[pl[i]].hp * 100 / (dur < 1? 1 : dur);
        nc_color col_cond = c_dkgray;
        if (parts[pl[i]].hp >= dur)
            col_cond = c_green;
        else
        if (per_cond >= 80)
            col_cond = c_ltgreen;
        else
        if (per_cond >= 50)
            col_cond = c_yellow;
        else
        if (per_cond >= 20)
            col_cond = c_ltred;
        else
        if (parts[pl[i]].hp > 0)
            col_cond = c_red;

        std::stringstream nom; //part name
        // part bigness, if that's relevant.
        if (part_flag(pl[i], vpf_variable_size)){
           if (part_flag(pl[i], vpf_engine)){ //bigness == liters
              nom << rmp_format(_("<veh_adj>%4.2f-Liter "), (float)(parts[pl[i]].bigness) / 100);
           }
           else if (part_flag(pl[i], vpf_wheel)){ //bigness == inches
              nom << rmp_format(_("<veh_adj>%d\" "), parts[pl[i]].bigness);
           }
        }
        nom << part_info(pl[i]).name;
        std::string partname = nom.str();

        bool armor = part_flag(pl[i], vpf_armor);
        mvwprintz(win, y, 2, i == hl? hilite(col_cond) : col_cond, partname.c_str());
        mvwprintz(win, y, 1, i == hl? hilite(c_ltgray) : c_ltgray, armor? "(" : (i? "-" : "["));
        mvwprintz(win, y, 2 + utf8_width(partname.c_str()), i == hl? hilite(c_ltgray) : c_ltgray, armor? ")" : (i? "-" : "]"));
//         mvwprintz(win, y, 3 + strlen(part_info(pl[i]).name), c_ltred, "%d", parts[pl[i]].blood);

        if (i == 0)
            mvwprintz(win, y, width-5, c_ltgray, is_inside(pl[i])? " In " : "Out ");
        y++;
    }
}

void vehicle::print_fuel_indicator (void *w, int y, int x, bool fullsize, bool verbose)
{
    WINDOW *win = (WINDOW *) w;
    const nc_color fcs[num_fuel_types] = { c_ltred, c_yellow, c_ltgreen, c_ltblue, c_ltcyan };
    const char fsyms[5] = { 'E', '\\', '|', '/', 'F' };
    nc_color col_indf1 = c_ltgray;
    int yofs=0;
    for (int i = 0; i < num_fuel_types; i++)
    {
        int cap = fuel_capacity(fuel_types[i]);
        if (cap > 0 && ( basic_consumption(fuel_types[i]) > 0 || fullsize ) )
        {
            mvwprintz(win, y+yofs, x, col_indf1, "E...F");
            int amnt = cap > 0? fuel_left(fuel_types[i]) * 99 / cap : 0;
            int indf = (amnt / 20) % 5;
            mvwprintz(win, y+yofs, x + indf, fcs[i], "%c", fsyms[indf]);
            if(verbose) {
              if(g->debugmon) {
                mvwprintz(win, y+yofs, x+6, fcs[i], "%d/%d",fuel_left(fuel_types[i]),cap);
              } else {
                mvwprintz(win, y+yofs, x+6, fcs[i], "%d",(fuel_left(fuel_types[i])*100)/cap);
                wprintz(win, c_ltgray, "%c",045);
              }
            }
            if(fullsize) yofs++;
        }
    }
}

void vehicle::coord_translate (int reldx, int reldy, int &dx, int &dy)
{
    tileray tdir (face.dir());
    tdir.advance (reldx);
    dx = tdir.dx() + tdir.ortho_dx(reldy);
    dy = tdir.dy() + tdir.ortho_dy(reldy);
}

void vehicle::coord_translate (int dir, int reldx, int reldy, int &dx, int &dy)
{
    tileray tdir (dir);
    tdir.advance (reldx);
    dx = tdir.dx() + tdir.ortho_dx(reldy);
    dy = tdir.dy() + tdir.ortho_dy(reldy);
}

void vehicle::precalc_mounts (int idir, int dir)
{
    if (idir < 0 || idir > 1)
        idir = 0;
    for (int p = 0; p < parts.size(); p++)
    {
        int dx, dy;
        coord_translate (dir, parts[p].mount_dx, parts[p].mount_dy, dx, dy);
        parts[p].precalc_dx[idir] = dx;
        parts[p].precalc_dy[idir] = dy;
    }
}

std::vector<int> vehicle::boarded_parts()
{
    std::vector<int> res;
    for (int p = 0; p < parts.size(); p++)
        if (part_flag (p, vpf_boardable) &&
                parts[p].has_flag(vehicle_part::passenger_flag))
            res.push_back (p);
    return res;
}

int vehicle::free_seat()
{
 for (int p = 0; p < parts.size(); p++)
  if (part_flag (p, vpf_boardable) && !parts[p].has_flag(vehicle_part::passenger_flag))
   return p;

 return -1;
}

player *vehicle::get_passenger (int p)
{
    p = part_with_feature (p, vpf_boardable, false);
    if (p >= 0 && parts[p].has_flag(vehicle_part::passenger_flag))
    {
     const int player_id = parts[p].passenger_id;
     if( player_id == g->u.getID())
      return &g->u;
     int npcdex = g->npc_by_id (player_id);
     if (npcdex >= 0)
      return g->active_npc[npcdex];
    }
    return 0;
}

int vehicle::global_x ()
{
    return smx * SEEX + posx;
}

int vehicle::global_y ()
{
    return smy * SEEY + posy;
}

int vehicle::total_mass ()
{
    int m = 0;
    for (int i = 0; i < parts.size(); i++)
    {
        m += g->itypes[part_info(i).item]->weight;
        for (int j = 0; j < parts[i].items.size(); j++)
            m += parts[i].items[j].type->weight;
        if (part_flag(i,vpf_boardable) && parts[i].has_flag(vehicle_part::passenger_flag))
            m += 81500; // TODO: get real weight
    }
    return m/1000;
}

int vehicle::fuel_left (ammotype ftype, bool for_engine)
{
    int fl = 0;
    for (int p = 0; p < parts.size(); p++)
        if (part_flag(p, vpf_fuel_tank) &&
            (ftype == part_info(p).fuel_type ||
            (for_engine && ftype == "battery" && part_info(p).fuel_type == "plutonium")))
            fl += parts[p].amount;
    return fl;
}

int vehicle::fuel_capacity (ammotype ftype)
{
    int cap = 0;
    for (int p = 0; p < parts.size(); p++)
        if (part_flag(p, vpf_fuel_tank) && ftype == part_info(p).fuel_type)
            cap += part_info(p).size;
    return cap;
}

int vehicle::refill (ammotype ftype, int amount)
{
    for (int p = 0; p < parts.size(); p++)
    {
        if (part_flag(p, vpf_fuel_tank) &&
            part_info(p).fuel_type == ftype &&
            parts[p].amount < part_info(p).size)
        {
            int need = part_info(p).size - parts[p].amount;
            if (amount < need)
            {
                parts[p].amount += amount;
                return 0;
            }
            else
            {
                parts[p].amount += need;
                amount -= need;
            }
        }
    }
    return amount;
}

int vehicle::drain (ammotype ftype, int amount) {
  int drained = 0;

  for (int p = 0; p < parts.size(); p++) {
    if (part_flag(p, vpf_fuel_tank) && part_info(p).fuel_type == ftype && parts[p].amount > 0) {
      if (parts[p].amount > (amount - drained)) {
        parts[p].amount -= (amount - drained);
        drained = amount;
        break;
      } else {
        drained += parts[p].amount;
        parts[p].amount = 0;
      }
    }
  }

  return drained;
}

int vehicle::basic_consumption (ammotype ftype)
{
    if (ftype == "plutonium")
      ftype = "battery";
    int cnt = 0;
    int fcon = 0;
    for (int p = 0; p < parts.size(); p++)
        if (part_flag(p, vpf_engine) &&
            ftype == part_info(p).fuel_type &&
            parts[p].hp > 0)
        {
            fcon += part_power(p);
            cnt++;
        }
    if (fcon < 100 && cnt > 0)
        fcon = 100;
    return fcon;
}

int vehicle::total_power (bool fueled)
{
    int pwr = 0;
    int cnt = 0;
    for (int p = 0; p < parts.size(); p++)
        if (part_flag(p, vpf_engine) &&
            (fuel_left (part_info(p).fuel_type, true) || !fueled ||
             part_info(p).fuel_type == "muscle") &&
            parts[p].hp > 0)
        {
            pwr += part_power(p);
            cnt++;
        }
    if (cnt > 1)
        pwr = pwr * 4 / (4 + cnt -1);
    return pwr;
}

int vehicle::solar_power ()
{
    int pwr = 0;
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag(p, vpf_solar_panel) && parts[p].hp > 0) {
            int part_x = global_x() + parts[p].precalc_dx[0];
            int part_y = global_y() + parts[p].precalc_dy[0];
            // Can't use g->in_sunlight() because it factors in vehicle roofs.
            if( !g->m.has_flag_ter_or_furn( indoors, part_x, part_y ) ) {
                pwr += (part_power(p) * g->natural_light_level()) / DAYLIGHT_LEVEL;
            }
        }
    }
    return pwr;
}

int vehicle::acceleration (bool fueled)
{
    return (int) (safe_velocity (fueled) * k_mass() / (1 + strain ()) / 10);
}

int vehicle::max_velocity (bool fueled)
{
    return total_power (fueled) * 80;
}

int vehicle::safe_velocity (bool fueled)
{
    int pwrs = 0;
    int cnt = 0;
    for (int p = 0; p < parts.size(); p++)
        if (part_flag(p, vpf_engine) &&
            (fuel_left (part_info(p).fuel_type, true) || !fueled ||
             part_info(p).fuel_type == "muscle") &&
            parts[p].hp > 0)
        {
            int m2c = 100;

            if( part_info(p).fuel_type == "gasoline" )    m2c = 60;
            else if( part_info(p).fuel_type == "plasma" ) m2c = 75;
            else if( part_info(p).fuel_type == "battery" )   m2c = 90;
            else if( part_info(p).fuel_type == "muscle" ) m2c = 45;

            pwrs += part_power(p) * m2c / 100;
            cnt++;
        }
    if (cnt > 0)
        pwrs = pwrs * 4 / (4 + cnt -1);
    return (int) (pwrs * k_dynamics() * k_mass()) * 80;
}

int vehicle::noise (bool fueled, bool gas_only)
{
    int pwrs = 0;
    int cnt = 0;
    int muffle = 100;
    for (int p = 0; p < parts.size(); p++)
        if (part_flag(p, vpf_muffler) && parts[p].hp > 0 && part_info(p).bonus < muffle)
            muffle = part_info(p).bonus;

    for (int p = 0; p < parts.size(); p++)
        if (part_flag(p, vpf_engine) &&
            (fuel_left (part_info(p).fuel_type, true) || !fueled ||
             part_info(p).fuel_type == "muscle") &&
            parts[p].hp > 0)
        {
            int nc = 10;

            if( part_info(p).fuel_type == "gasoline" )    nc = 25;
            else if( part_info(p).fuel_type == "plasma" ) nc = 10;
            else if( part_info(p).fuel_type == "battery" )   nc = 3;
            else if( part_info(p).fuel_type == "muscle" ) nc = 5;

            if (!gas_only || part_info(p).fuel_type == "gasoline")
            {
                int pwr = part_power(p) * nc / 100;
                if (muffle < 100 && (part_info(p).fuel_type == "gasoline" ||
                    part_info(p).fuel_type == "plasma"))
                    pwr = pwr * muffle / 100;
                pwrs += pwr;
                cnt++;
            }
        }
    return pwrs;
}

float vehicle::wheels_area (int *cnt)
{
    int count = 0;
    int total_area = 0;
    for (int i = 0; i < external_parts.size(); i++)
    {
        int p = external_parts[i];
        if (part_flag(p, vpf_wheel) &&
            parts[p].hp > 0)
        {
            int width = part_info(p).wheel_width;
            int bigness = parts[p].bigness;
            // 9 inches, for reference, is about normal for cars.
            total_area += ((float)width/9) * bigness;
            count++;
        }
    }
    if (cnt)
        *cnt = count;
    return total_area;
}

float vehicle::k_dynamics ()
{
    const int max_obst = 13;
    int obst[max_obst];
    for (int o = 0; o < max_obst; o++)
        obst[o] = 0;
    for (int i = 0; i < external_parts.size(); i++)
    {
        int p = external_parts[i];
        int frame_size = part_flag(p, vpf_obstacle)? 30 : 10;
        int pos = parts[p].mount_dy + max_obst / 2;
        if (pos < 0)
            pos = 0;
        if (pos >= max_obst)
            pos = max_obst -1;
        if (obst[pos] < frame_size)
            obst[pos] = frame_size;
    }
    int frame_obst = 0;
    for (int o = 0; o < max_obst; o++)
        frame_obst += obst[o];
    float ae0 = 200.0;
    float fr0 = 1000.0;
    float wa = wheels_area();

    // calculate aerodynamic coefficient
    float ka = ae0 / (ae0 + frame_obst);

    // calculate safe speed reduction due to wheel friction
    float kf = fr0 / (fr0 + wa);

    return ka * kf;
}

float vehicle::k_mass ()
{
    float wa = wheels_area();
    if (wa <= 0)
       return 0;

    float ma0 = 50.0;

    // calculate safe speed reduction due to mass
    float km = ma0 / (ma0 + (total_mass()/8) / (8 * (float) wa));

    return km;
}

float vehicle::strain ()
{
    int mv = max_velocity();
    int sv = safe_velocity();
    if (mv <= sv)
        mv = sv + 1;
    if (velocity < safe_velocity())
        return 0;
    else
        return (float) (velocity - sv) / (float) (mv - sv);
}

bool vehicle::valid_wheel_config ()
{
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    int count = 0;
    for (int i = 0; i < external_parts.size(); i++)
    {
        int p = external_parts[i];
        if (!part_flag(p, vpf_wheel) ||
            parts[p].hp <= 0)
            continue;
        if (!count)
        {
            x1 = x2 = parts[p].mount_dx;
            y1 = y2 = parts[p].mount_dy;
        }
        if (parts[p].mount_dx < x1)
            x1 = parts[p].mount_dx;
        if (parts[p].mount_dx > x2)
            x2 = parts[p].mount_dx;
        if (parts[p].mount_dy < y1)
            y1 = parts[p].mount_dy;
        if (parts[p].mount_dy > y2)
            y2 = parts[p].mount_dy;
        count++;
    }
    if (count < 2)
        return false;
    float xo = 0, yo = 0;
    float wo = 0, w2;
    for (int p = 0; p < parts.size(); p++)
    { // lets find vehicle's center of masses
        w2 = g->itypes[part_info(p).item]->weight;
        if (w2 < 1)
            continue;
        xo = xo * wo / (wo + w2) + parts[p].mount_dx * w2 / (wo + w2);
        yo = yo * wo / (wo + w2) + parts[p].mount_dy * w2 / (wo + w2);
        wo += w2;
    }
//    g->add_msg("cm x=%.3f y=%.3f m=%d  x1=%d y1=%d x2=%d y2=%d", xo, yo, (int) wo, x1, y1, x2, y2);
    if ((int)xo < x1 || (int)xo > x2 || (int)yo < y1 || (int)yo > y2)
        return false; // center of masses not inside support of wheels (roughly)
    return true;
}

void vehicle::consume_fuel ()
{
    ammotype ftypes[3] = { "gasoline", "battery", "plasma" };
    for (int ft = 0; ft < 3; ft++)
    {
        float st = strain() * 10;
        int amnt = (int) (basic_consumption (ftypes[ft]) * (1.0 + st * st) /
                          (ftypes[ft] == "battery" ? 1 : 100));
        if (!amnt)
            continue; // no engines of that type
        bool elec = ftypes[ft] == "battery";
        bool found = false;
        for (int j = 0; j < (elec? 2 : 1); j++)
        {
            for (int p = 0; p < parts.size(); p++)
                // if this is a fuel tank
                // and its type is same we're looking for now
                // and for electric engines:
                //  - if j is 0, then we're looking for plutonium (it's first)
                //  - otherwise we're looking for batteries (second)
                if (part_flag(p, vpf_fuel_tank) &&
                    (part_info(p).fuel_type == (elec? (j ? "battery" : "plutonium") : ftypes[ft])) &&
                    parts[p].amount > 0)
                {
                    parts[p].amount -= amnt / ((elec && !j)?100:1);
                    if (parts[p].amount < 0)
                        parts[p].amount = 0;
                    found = true;
                    break;
                }
            if (found)
                break;
        }
    }
}

void vehicle::thrust (int thd)
{
    if (velocity == 0)
    {
        turn_dir = face.dir();
        last_turn = 0;
        move = face;
        of_turn_carry = 0;
        last_turn = 0;
        skidding = false;
    }

    if (!thd)
        return;

    bool pl_ctrl = player_in_control(&g->u);

    if (!valid_wheel_config() && velocity == 0)
    {
        if (pl_ctrl)
            g->add_msg (_("The %s doesn't have enough wheels to move!"), name.c_str());
        return;
    }

    bool thrusting = true;
    if(velocity){ //brake?
       int sgn = velocity < 0? -1 : 1;
       thrusting = sgn == thd;
    }

    if (thrusting)
    {
        if (total_power () < 1)
        {
            if (pl_ctrl)
            {
                if (total_power (false) < 1)
                    g->add_msg (_("The %s doesn't have an engine!"), name.c_str());
                else
                    g->add_msg (_("The %s's engine emits a sneezing sound."), name.c_str());
            }
            cruise_velocity = 0;
            return;
        }

        consume_fuel ();

        int strn = (int) (strain () * strain() * 100);

        for (int p = 0; p < parts.size(); p++)
            if (part_flag(p, vpf_engine) &&
                (fuel_left (part_info(p).fuel_type, true)) && parts[p].hp > 0 &&
                rng (1, 100) < strn)
            {
                int dmg = rng (strn * 2, strn * 4);
                damage_direct (p, dmg, 0);
                if(one_in(2))
                 g->add_msg(_("Your engine emits a high pitched whine."));
                else
                 g->add_msg(_("Your engine emits a loud grinding sound."));
            }
        // add sound and smoke
        int smk = noise (true, true);
        if (smk > 0)
        {
            int rdx, rdy;
            coord_translate (exhaust_dx, exhaust_dy, rdx, rdy);
            g->m.add_field(g, global_x() + rdx, global_y() + rdy, fd_smoke, (smk / 50) + 1);
        }
        g->sound(global_x(), global_y(), noise(), "");
    }

    if (skidding)
        return;

    int accel = acceleration();
    int max_vel = max_velocity();
    int brake = 30 * k_mass();
    int brk = abs(velocity) * brake / 100;
    if (brk < accel)
        brk = accel;
    if (brk < 10 * 100)
        brk = 10 * 100;
    int vel_inc = (thrusting? accel : brk) * thd;
    if(thd == -1 && thrusting) // reverse accel.
       vel_inc = .6 * vel_inc;
    if ((velocity > 0 && velocity + vel_inc < 0) ||
        (velocity < 0 && velocity + vel_inc > 0))
        stop ();
    else
    {
        velocity += vel_inc;
        if (velocity > max_vel)
            velocity = max_vel;
        else
        if (velocity < -max_vel / 4)
            velocity = -max_vel / 4;
    }
}

void vehicle::cruise_thrust (int amount)
{
    if (!amount)
        return;
    int max_vel = (safe_velocity() * 11 / 10000 + 1) * 1000;
    cruise_velocity += amount;
    cruise_velocity = cruise_velocity / abs(amount) * abs(amount);
    if (cruise_velocity > max_vel)
        cruise_velocity = max_vel;
    else
    if (-cruise_velocity > max_vel / 4)
        cruise_velocity = -max_vel / 4;
}

void vehicle::turn (int deg)
{
    if (deg == 0)
        return;
    if (velocity < 0)
        deg = -deg;
    last_turn = deg;
    turn_dir += deg;
    if (turn_dir < 0)
        turn_dir += 360;
    if (turn_dir >= 360)
        turn_dir -= 360;
}

void vehicle::stop ()
{
    velocity = 0;
    skidding = false;
    move = face;
    last_turn = 0;
    of_turn_carry = 0;
}

veh_collision vehicle::part_collision (int vx, int vy, int part, int x, int y)
{
    bool pl_ctrl = player_in_control (&g->u);
    int mondex = g->mon_at(x, y);
    int npcind = g->npc_at(x, y);
    bool u_here = x == g->u.posx && y == g->u.posy && !g->u.in_vehicle;
    monster *z = mondex >= 0? &g->z[mondex] : 0;
    player *ph = (npcind >= 0? g->active_npc[npcind] : (u_here? &g->u : 0));

    if (ph && ph->in_vehicle) // if in a vehicle assume it's this one
    	ph = 0;

    int target_part = -1;
    vehicle *oveh = g->m.veh_at (x, y, target_part);
    bool is_veh_collision = oveh && (oveh->posx != posx || oveh->posy != posy);
    bool is_body_collision = ph || mondex >= 0;

    veh_coll_type collision_type = veh_coll_nothing;
    std::string obs_name = g->m.name(x, y).c_str();

    // vehicle collisions are a special case. just return the collision.
    // the map takes care of the dynamic stuff.
    if (is_veh_collision){
       veh_collision ret;
       ret.type = veh_coll_veh;
       //"imp" is too simplistic for veh-veh collisions
       ret.part = part;
       ret.target = oveh;
       ret.target_part = target_part;
       ret.target_name = oveh->name.c_str();
       return ret;
    }

    int parm = part_with_feature (part, vpf_armor);
    if (parm < 0)
        parm = part;
    int dmg_mod = part_info(parm).dmg_mod;
    // let's calculate type of collision & mass of object we hit
    int mass = total_mass();
    int mass2;

    if (is_body_collision)
    { // then, check any monster/NPC/player on the way
        collision_type = veh_coll_body; // body
        if (z)
            switch (z->type->size)
            {
            case MS_TINY:    // Rodent
                mass2 = 1;
                break;
            case MS_SMALL:   // Half human
                mass2 = 41;
                break;
            default:
            case MS_MEDIUM:  // Human
                mass2 = 82;
                break;
            case MS_LARGE:   // Cow
                mass2 = 120;
                break;
            case MS_HUGE:     // TAAAANK
                mass2 = 200;
                break;
            }
        else
            mass2 = 82;// player or NPC
    }
    else // if all above fails, go for terrain which might obstruct moving
    if (g->m.has_flag_ter_or_furn (thin_obstacle, x, y))
    {
        collision_type = veh_coll_thin_obstacle; // some fence
        mass2 = 20;
    }
    else
    if (g->m.has_flag_ter_or_furn(bashable, x, y))
    {
        collision_type = veh_coll_bashable; // (door, window)
        mass2 = 50;    // special case: instead of calculating absorb based on mass of obstacle later, we let
                       // map::bash function deside, how much absorb is
    }
    else
    if (g->m.move_cost_ter_furn(x, y) == 0 && g->m.is_destructable_ter_furn(x, y))
    {
        collision_type = veh_coll_destructable; // destructible (wall)
        mass2 = 200;
    }
    else
    if (g->m.move_cost_ter_furn(x, y) == 0 && !g->m.has_flag_ter_or_furn(swimmable, x, y))
    {
        collision_type = veh_coll_other; // not destructible
        mass2 = 1000;
    }
    if (collision_type == veh_coll_nothing){  // hit nothing
       veh_collision ret;
       ret.type = veh_coll_nothing;
       return ret;
    }

    int degree = rng (70, 100);
    int imp = abs(velocity) * mass / k_mvel / 100 ;
    int imp2 = imp * mass2 / (mass + mass2) * degree / 100;
    bool smashed = true;
    std::string snd;
    if (collision_type == veh_coll_bashable)
    { // something bashable -- use map::bash to determine outcome
        int absorb = -1;
        g->m.bash(x, y, imp * dmg_mod / 100, snd, &absorb);
        if (absorb != -1)
            imp2 = absorb;
        smashed = imp * dmg_mod / 100 > absorb;
    }
    else
    if (collision_type >= veh_coll_thin_obstacle) // some other terrain
    {
        smashed = imp * rng (80, 120) / 100 > mass2;
        if (smashed)
            switch (collision_type) // destroy obstacle
            {
            case veh_coll_thin_obstacle:
                if (g->m.has_furn(x, y))
                    g->m.furn_set(x, y, f_null);
                else
                    g->m.ter_set(x, y, t_dirt);
                break;
            case veh_coll_destructable:
                g->m.destroy(g, x, y, false);
                snd = _("crash!");
                break;
            case veh_coll_other:
                smashed = false;
                break;
            default:;
            }
        g->sound (x, y, smashed? 80 : 50, "");
    }
    if (!is_body_collision)
    {
        if (pl_ctrl)
        {
            if (snd.length() > 0)
                g->add_msg (_("Your %s's %s rams into a %s with a %s"), name.c_str(), part_info(part).name, obs_name.c_str(), snd.c_str());
            else
                g->add_msg (_("Your %s's %s rams into a %s."), name.c_str(), part_info(part).name, obs_name.c_str());
        }
        else
        if (snd.length() > 0)
            g->add_msg (_("You hear a %s"), snd.c_str());
    }
    if (part_flag(part, vpf_sharp) && smashed)
        imp2 /= 2;
    int imp1 = imp - imp2;
    int vel1 = imp1 * k_mvel * 100 / mass;
    int vel2 = imp2 * k_mvel * 100 / mass2;

    if (collision_type == veh_coll_body)
    {
        int dam = imp1 * dmg_mod / 100;
        if (z)
        {
            int z_armor = part_flag(part, vpf_sharp)? z->type->armor_cut : z->type->armor_bash;
            if (z_armor < 0)
                z_armor = 0;
            if (z)
                dam -= z_armor;
        }
        if (dam < 0)
            dam = 0;

        if (part_flag(part, vpf_sharp))
            parts[part].blood += (20 + dam) * 5;
        else
        if (dam > rng (10, 30))
            parts[part].blood += (10 + dam / 2) * 5;

        int turns_stunned = rng (0, dam) > 10? rng (1, 2) + (dam > 40? rng (1, 2) : 0) : 0;
        if (part_flag(part, vpf_sharp))
            turns_stunned = 0;
        if (turns_stunned > 6)
            turns_stunned = 6;
        if (turns_stunned > 0 && z)
            z->add_effect(ME_STUNNED, turns_stunned);

        std::string dname;
        if (z)
            dname = z->name().c_str();
        else
            dname = ph->name;
        if (pl_ctrl)
            g->add_msg (_("Your %s's %s rams into %s, inflicting %d damage%s!"),
                    name.c_str(), part_info(part).name, dname.c_str(), dam,
                    turns_stunned > 0 && z? _(" and stunning it") : "");

        int angle = (100 - degree) * 2 * (one_in(2)? 1 : -1);
        if (z)
        {
            z->hurt(dam);
            if (vel2 / 100 > rng (5, 30))
                g->fling_player_or_monster (0, z, move.dir() + angle, vel2 / 100);
            if (z->hp < 1)
                g->kill_mon (mondex, pl_ctrl);
        }
        else
        {
            ph->hitall (g, dam, 40);
            if (vel2 / 100 > rng (5, 30))
                g->fling_player_or_monster (ph, 0, move.dir() + angle, vel2 / 100);
        }

        if (part_flag(part, vpf_sharp))
        {
            field &local_field = g->m.field_at(x, y);
            if (local_field.findField(fd_blood) &&
                local_field.findField(fd_blood)->getFieldDensity() < 2)
                local_field.findField(fd_blood)->setFieldDensity(local_field.findField(fd_blood)->getFieldDensity() + 1);
            else
                g->m.add_field(g, x, y, fd_blood, 1);
        }
        else
            g->sound (x, y, 20, "");
    }

    if (!smashed) // tree, wall, or bear sometimes wins
    {
        cruise_on = false;
        stop();
        imp2 = imp;
    }
    else
    {
        if (vel1 < 500)
            stop();
        else
        {
            if (velocity < 0)
                velocity = -vel1;
            else
                velocity = vel1;
        }

        int turn_roll = rng (0, 100);
        int turn_amount = rng (1, 3) * sqrt ((double)imp2);
        turn_amount /= 15;
        if (turn_amount < 1)
            turn_amount = 1;
        turn_amount *= 15;
        if (turn_amount > 120)
            turn_amount = 120;
        bool turn_veh = turn_roll < (abs(velocity) - vel1) / 100;
        if (turn_veh)
        {
            skidding = true;
            turn (one_in (2)? turn_amount : -turn_amount);
        }

    }
    damage (parm, imp2, 1);

    veh_collision ret;
    ret.type = collision_type;
    ret.imp = imp2;
    return ret;
}

void vehicle::handle_trap (int x, int y, int part)
{
    int pwh = part_with_feature (part, vpf_wheel);
    if (pwh < 0)
        return;
    trap_id t = g->m.tr_at(x, y);
    if (t == tr_null)
        return;
    int noise = 0;
    int chance = 100;
    int expl = 0;
    int shrap = 0;
    bool wreckit = false;
    std::string msg (_("The %s's %s runs over %s."));
    std::string snd;
    switch (t)
    {
        case tr_bubblewrap:
            noise = 18;
            snd = _("Pop!");
            break;
        case tr_beartrap:
        case tr_beartrap_buried:
            noise = 8;
            snd = _("SNAP!");
            wreckit = true;
            g->m.tr_at(x, y) = tr_null;
            g->m.spawn_item(x, y, "beartrap", 0);
            break;
        case tr_nailboard:
            wreckit = true;
            break;
        case tr_blade:
            noise = 1;
            snd = _("Swinnng!");
            wreckit = true;
            break;
        case tr_crossbow:
            chance = 30;
            noise = 1;
            snd = _("Clank!");
            wreckit = true;
            g->m.tr_at(x, y) = tr_null;
            g->m.spawn_item(x, y, "crossbow", 0);
            g->m.spawn_item(x, y, "string_6", 0);
            if (!one_in(10))
                g->m.spawn_item(x, y, "bolt_steel", 0);
            break;
        case tr_shotgun_2:
        case tr_shotgun_1:
            noise = 60;
            snd = _("Bang!");
            chance = 70;
            wreckit = true;
            if (t == tr_shotgun_2)
                g->m.tr_at(x, y) = tr_shotgun_1;
            else
            {
                g->m.tr_at(x, y) = tr_null;
                g->m.spawn_item(x, y, "shotgun_sawn", 0);
                g->m.spawn_item(x, y, "string_6", 0);
            }
            break;
        case tr_landmine_buried:
        case tr_landmine:
            expl = 10;
            shrap = 8;
            g->m.tr_at(x, y) = tr_null;
            break;
        case tr_boobytrap:
            expl = 18;
            shrap = 12;
            break;
        case tr_dissector:
            noise = 10;
            snd = _("BRZZZAP!");
            wreckit = true;
            break;
        case tr_sinkhole:
        case tr_pit:
        case tr_spike_pit:
        case tr_ledge:
            wreckit = true;
            break;
        case tr_goo:
        case tr_portal:
        case tr_telepad:
        case tr_temple_flood:
        case tr_temple_toggle:
            msg.clear();
        default:;
    }
    if (msg.size() > 0 && g->u_see(x, y))
        g->add_msg (msg.c_str(), name.c_str(), part_info(part).name, g->traps[t]->name.c_str());
    if (noise > 0)
        g->sound (x, y, noise, snd);
    if (wreckit && chance >= rng (1, 100))
        damage (part, 500);
    if (expl > 0)
        g->explosion(x, y, expl, shrap, false);
}

// total volume of all the things
int vehicle::stored_volume(int part) {
   if (!part_flag(part, vpf_cargo))
        return 0;
   int cur_volume = 0;
   for (int i = 0; i < parts[part].items.size(); i++) {
       cur_volume += parts[part].items[i].volume();
   }
   return cur_volume;
}
// stub, pending per vpart limits
int vehicle::max_volume(int part) {
   return MAX_VOLUME_IN_VEHICLE_STORAGE;
}

// free space
int vehicle::free_volume(int part) {
   const int maxvolume = this->max_volume(part);
   return ( maxvolume - stored_volume(part) );
}

// returns true if full, modified by arguments:
// (none):                            size >= max || volume >= max
// (addvolume >= 0):                  size+1 > max || volume + addvolume > max
// (addvolume >= 0, addnumber >= 0):  size + addnumber > max || volume + addvolume > max
bool vehicle::is_full(const int part, const int addvolume, const int addnumber) {
   const int maxitems = MAX_ITEM_IN_VEHICLE_STORAGE;
   const int maxvolume = this->max_volume(part);

   if ( addvolume == -1 ) {
       if ( parts[part].items.size() < maxitems ) return true;
       int cur_volume=stored_volume(part);
       return (cur_volume >= maxvolume ? true : false );
   } else {
       if ( parts[part].items.size() + ( addnumber == -1 ? 1 : addnumber ) > maxitems ) return true;
       int cur_volume=stored_volume(part);
       return ( cur_volume + addvolume > maxvolume ? true : false );
   }

}

bool vehicle::add_item (int part, item itm)
{
    const int max_storage = MAX_ITEM_IN_VEHICLE_STORAGE; // (game.h)
    const int maxvolume = this->max_volume(part);         // (game.h => vehicle::max_volume(part) ) in theory this could differ per vpart ( seat vs trunk )

    // const int max_weight = ?! // TODO: weight limit, calc per vpart & vehicle stats, not a hard user limit.
    // add creaking sounds and damage to overloaded vpart, outright break it past a certian point, or when hitting bumps etc

    if (!part_flag(part, vpf_cargo))
        return false;
    
    if (parts[part].items.size() >= max_storage)
        return false;
    it_ammo *ammo = dynamic_cast<it_ammo*> (itm.type);
    if (part_flag(part, vpf_turret))
        if (!ammo || (ammo->type != part_info(part).fuel_type ||
                 ammo->type == "gasoline" ||
                 ammo->type == "plasma"))
            return false;
    int cur_volume = 0;
    int add_volume = itm.volume();
    bool tryaddcharges=(itm.charges  != -1 && (itm.is_food() || itm.is_ammo()));
    // iterate anyway since we need a volume total
      for (int i = 0; i < parts[part].items.size(); i++) {
        cur_volume += parts[part].items[i].volume();
        if( tryaddcharges && parts[part].items[i].type->id == itm.type->id ) {
          parts[part].items[i].charges+=itm.charges;
          return true;
        }
      }
    
    if ( cur_volume + add_volume > maxvolume ) {
      return false;
    }
    parts[part].items.push_back (itm);
    return true;
}

void vehicle::remove_item (int part, int itemdex)
{
    if (itemdex < 0 || itemdex >= parts[part].items.size())
        return;
    parts[part].items.erase (parts[part].items.begin() + itemdex);
}

void vehicle::gain_moves (int mp)
{
    if (velocity)
    {
        of_turn = 1 + of_turn_carry;
    }
    else
    {
        of_turn = 0;
    }
    of_turn_carry = 0;

    // cruise control TODO: enable for NPC?
    if (player_in_control(&g->u))
    {
        if (cruise_on)
        if (velocity - cruise_velocity >= 10 * 100 ||
            cruise_velocity - velocity >= acceleration()/3 ||
            (cruise_velocity != 0 && velocity == 0) ||
            (cruise_velocity == 0 && velocity != 0))
            thrust (cruise_velocity > velocity? 1 : -1);
    }

    refill ("battery", solar_power());

    // check for smoking parts
    for (int ep = 0; ep < external_parts.size(); ep++)
    {
        int p = external_parts[ep];
        if (parts[p].blood > 0)
            parts[p].blood--;
        int p_eng = part_with_feature (p, vpf_engine, false);
        if (p_eng < 0 || parts[p_eng].hp > 0 || parts[p_eng].amount < 1)
            continue;
        parts[p_eng].amount--;
        int x = global_x() + parts[p_eng].precalc_dx[0];
        int y = global_y() + parts[p_eng].precalc_dy[0];
        for (int ix = -1; ix <= 1; ix++)
            for (int iy = -1; iy <= 1; iy++)
                if (!rng(0, 2))
                    g->m.add_field(g, x + ix, y + iy, fd_smoke, rng(2, 4));
    }

    if (turret_mode) // handle turrets
        for (int p = 0; p < parts.size(); p++)
            fire_turret (p);
}

void vehicle::find_external_parts ()
{
    external_parts.clear();
    for (int p = 0; p < parts.size(); p++)
    {
        bool ex = false;
        for (int i = 0; i < external_parts.size(); i++)
            if (parts[external_parts[i]].mount_dx == parts[p].mount_dx &&
                parts[external_parts[i]].mount_dy == parts[p].mount_dy)
            {
                ex = true;
                break;
            }
        if (!ex)
            external_parts.push_back (p);
    }
}

void vehicle::find_exhaust ()
{
    int en = -1;
    for (int p = 0; p < parts.size(); p++)
        if (part_flag(p, vpf_engine) && part_info(p).fuel_type == "gasoline")
        {
            en = p;
            break;
        }
    if (en < 0)
    {
        exhaust_dy = 0;
        exhaust_dx = 0;
        return;
    }
    exhaust_dy = parts[en].mount_dy;
    exhaust_dx = parts[en].mount_dx;
    for (int p = 0; p < parts.size(); p++)
        if (parts[p].mount_dy == exhaust_dy &&
            parts[p].mount_dx < exhaust_dx)
            exhaust_dx = parts[p].mount_dx;
    exhaust_dx--;
}

void vehicle::refresh_insides ()
{
    insides_dirty = false;
    for (int ep = 0; ep < external_parts.size(); ep++)
    {
        int p = external_parts[ep];
        if (part_with_feature(p, vpf_roof) < 0 || parts[p].hp <= 0)
        { // if there's no roof (or it's broken) -- it's outside!
/*            debugmsg ("part%d/%d(%s)%d,%d no roof=false", p, external_parts.size(),
                      part_info(p).name, parts[p].mount_dx, parts[p].mount_dy);*/
            parts[p].inside = false;
            continue;
        }

        parts[p].inside = true; // inside if not otherwise
        for (int i = 0; i < 4; i++)
        { // let's check four neighbour parts
            int ndx = i < 2? (i == 0? -1 : 1) : 0;
            int ndy = i < 2? 0 : (i == 2? - 1: 1);
            std::vector<int> parts_n3ar = parts_at_relative (parts[p].mount_dx + ndx, parts[p].mount_dy + ndy);
            bool cover = false; // if we aren't covered from sides, the roof at p won't save us
            for (int j = 0; j < parts_n3ar.size(); j++)
            {
                int pn = parts_n3ar[j];
                if (parts[pn].hp <= 0)
                    continue;   // it's broken = can't cover
                if (part_flag(pn, vpf_roof))
                { // another roof -- cover
                    cover = true;
                    break;
                }
                else
                if (part_flag(pn, vpf_obstacle))
                { // found an obstacle, like board or windshield or door
                    if (parts[pn].inside || (part_flag(pn, vpf_openable) && parts[pn].open))
                        continue; // door and it's open -- can't cover
                    cover = true;
                    break;
                }
            }
            if (!cover)
            {
/*                debugmsg ("part%d/%d(%s)%d,%d nb#%d(%s) no cover=false", p, external_parts.size(),
                          part_info(p).name, parts[p].mount_dx, parts[p].mount_dy,
                          i, parts_n3ar.size()> 0? part_info(parts_n3ar[0]).name : "<no part>");*/
                parts[p].inside = false;
                break;
            }
        }
    }
}

bool vehicle::is_inside (int p)
{
    if (p < 0 || p >= parts.size())
        return false;
    if (insides_dirty)
        refresh_insides ();
    return parts[p].inside;
}

void vehicle::unboard_all ()
{
    std::vector<int> bp = boarded_parts ();
    for (int i = 0; i < bp.size(); i++)
        g->m.unboard_vehicle (g, global_x() + parts[bp[i]].precalc_dx[0], global_y() + parts[bp[i]].precalc_dy[0]);
}

int vehicle::damage (int p, int dmg, int type, bool aimed)
{
    if (dmg < 1)
        return dmg;

    std::vector<int> pl = internal_parts (p);
    pl.insert (pl.begin(), p);
    if (!aimed)
    {
        bool found_obs = false;
        for (int i = 0; i < pl.size(); i++)
            if (part_flag (pl[i], vpf_obstacle) &&
                (!part_flag (pl[i], vpf_openable) || !parts[pl[i]].open))
            {
                found_obs = true;
                break;
            }
        if (!found_obs) // not aimed at this tile and no obstacle here -- fly through
            return dmg;
    }
    int parm = part_with_feature (p, vpf_armor);
    int pdm = pl[rng (0, pl.size()-1)];
    int dres;
    if (parm < 0)
        // not covered by armor -- damage part
        dres = damage_direct (pdm, dmg, type);
    else
    {
        // covered by armor -- damage armor first
        dres = damage_direct (parm, dmg, type);
        // half damage for internal part(over parts not covered)
        damage_direct (pdm, part_flag(pdm, vpf_over)? dmg : dmg / 2, type);
    }
    return dres;
}

void vehicle::damage_all (int dmg1, int dmg2, int type)
{
    if (dmg2 < dmg1)
    {
        int t = dmg2;
        dmg2 = dmg1;
        dmg1 = t;
    }
    if (dmg1 < 1)
        return;
    for (int p = 0; p < parts.size(); p++)
        if (!one_in(4))
            damage_direct (p, rng (dmg1, dmg2), type);
}

int vehicle::damage_direct (int p, int dmg, int type)
{
    if (parts[p].hp <= 0)
        return dmg;
    int tsh = part_info(p).durability / 10;
    if (tsh > 20)
        tsh = 20;
    int dres = dmg;
    if (dmg >= tsh || type != 1)
    {
        dres -= parts[p].hp;
        int last_hp = parts[p].hp;
        parts[p].hp -= dmg;
        if (parts[p].hp < 0)
            parts[p].hp = 0;
        if (!parts[p].hp && last_hp > 0)
            insides_dirty = true;
        if (part_flag(p, vpf_fuel_tank))
        {
            ammotype ft = part_info(p).fuel_type;
            if (ft == "gasoline" || ft == "plasma")
            {
                int pow = parts[p].amount / 40;
    //            debugmsg ("damage check dmg=%d pow=%d", dmg, pow);
                if (parts[p].hp <= 0)
                    leak_fuel (p);
                if (type == 2 ||
                    (one_in (ft == "gasoline" ? 2 : 4) && pow > 5 && rng (75, 150) < dmg))
                {
                    g->explosion (global_x() + parts[p].precalc_dx[0], global_y() + parts[p].precalc_dy[0],
                                pow, 0, ft == "gasoline");
                    parts[p].hp = 0;
                }
            }
        }
        else
        if (parts[p].hp <= 0 && part_flag(p, vpf_unmount_on_damage))
        {
            g->m.spawn_item(global_x() + parts[p].precalc_dx[0],
                           global_y() + parts[p].precalc_dy[0],
                           part_info(p).item, g->turn);
            remove_part (p);
        }
    }
    if (dres < 0)
        dres = 0;
    return dres;
}

void vehicle::leak_fuel (int p)
{
    if (!part_flag(p, vpf_fuel_tank))
        return;
    ammotype ft = part_info(p).fuel_type;
    if (ft == "gasoline")
    {
        int x = global_x();
        int y = global_y();
        for (int i = x - 2; i <= x + 2; i++)
            for (int j = y - 2; j <= y + 2; j++)
                if (g->m.move_cost(i, j) > 0 && one_in(2))
                {
                    if (parts[p].amount < 100)
                    {
                        parts[p].amount = 0;
                        return;
                    }
                    g->m.spawn_item(i, j, "gasoline", 0);
                    g->m.spawn_item(i, j, "gasoline", 0);
                    parts[p].amount -= 100;
                }
    }
    parts[p].amount = 0;
}

void vehicle::fire_turret (int p, bool burst)
{
    if (!part_flag (p, vpf_turret))
        return;
    it_gun *gun = dynamic_cast<it_gun*> (g->itypes[part_info(p).item]);
    if (!gun)
        return;
    int charges = burst? gun->burst : 1;
    if (!charges)
        charges = 1;
    ammotype amt = part_info (p).fuel_type;
    if (amt == "gasoline" || amt == "plasma")
    {
        if (amt == "gasoline")
            charges = 20; // hacky
        int fleft = fuel_left (amt);
        if (fleft < 1)
            return;
        it_ammo *ammo = dynamic_cast<it_ammo*>(g->itypes[amt == "gasoline" ? "gasoline" : "plasma"]);
        if (!ammo)
            return;
        if (fire_turret_internal (p, *gun, *ammo, charges))
        { // consume fuel
            if (amt == "plasma")
                charges *= 10; // hacky, too
            for (int p = 0; p < parts.size(); p++)
            {
                if (part_flag(p, vpf_fuel_tank) &&
                    part_info(p).fuel_type == amt &&
                    parts[p].amount > 0)
                {
                    parts[p].amount -= charges;
                    if (parts[p].amount < 0)
                        parts[p].amount = 0;
                }
            }
        }
    }
    else
    {
        if (parts[p].items.size() > 0)
        {
            it_ammo *ammo = dynamic_cast<it_ammo*> (parts[p].items[0].type);
            if (!ammo || ammo->type != amt ||
                parts[p].items[0].charges < 1)
                return;
            if (charges > parts[p].items[0].charges)
                charges = parts[p].items[0].charges;
            if (fire_turret_internal (p, *gun, *ammo, charges))
            { // consume ammo
                if (charges >= parts[p].items[0].charges)
                    parts[p].items.erase (parts[p].items.begin());
                else
                    parts[p].items[0].charges -= charges;
            }
        }
    }
}

bool vehicle::fire_turret_internal (int p, it_gun &gun, it_ammo &ammo, int charges)
{
    int x = global_x() + parts[p].precalc_dx[0];
    int y = global_y() + parts[p].precalc_dy[0];
    // code copied form mattack::smg, mattack::flamethrower
    int t, fire_t;
    monster *target = 0;
    int range = ammo.type == "gasoline" ? 5 : 12;
    int closest = range + 1;
    for (int i = 0; i < g->z.size(); i++)
    {
        int dist = rl_dist(x, y, g->z[i].posx, g->z[i].posy);
        if (g->z[i].friendly == 0 && dist < closest &&
            g->m.sees(x, y, g->z[i].posx, g->z[i].posy, range, t))
        {
            target = &(g->z[i]);
            closest = dist;
            fire_t = t;
        }
    }
    if (!target)
        return false;

    std::vector<point> traj = line_to(x, y, target->posx, target->posy, fire_t);
    for (int i = 0; i < traj.size(); i++)
        if (traj[i].x == g->u.posx && traj[i].y == g->u.posy)
            return false; // won't shoot at player
    if (g->u_see(x, y))
        g->add_msg(_("The %s fires its %s!"), name.c_str(), part_info(p).name);
    player tmp;
    tmp.name = rmp_format(_("<veh_player>The %s"), part_info(p).name);
    tmp.skillLevel(gun.skill_used).level(1);
    tmp.skillLevel("gun").level(0);
    tmp.recoil = abs(velocity) / 100 / 4;
    tmp.posx = x;
    tmp.posy = y;
    tmp.str_cur = 16;
    tmp.dex_cur =  6;
    tmp.per_cur =  8;
    tmp.weapon = item(&gun, 0);
    it_ammo curam = ammo;
    tmp.weapon.curammo = &curam;
    tmp.weapon.charges = charges;
    g->fire(tmp, target->posx, target->posy, traj, true);
    if (ammo.type == "gasoline")
    {
        for (int i = 0; i < traj.size(); i++)
            g->m.add_field(g, traj[i].x, traj[i].y, fd_fire, 1);
    }

    return true;
}

// a chance to stop skidding if moving in roughly the faced direction
void vehicle::possibly_recover_from_skid(){
   if (last_turn > 13)
      //turning on the initial skid is delayed, so move==face, initially. This filters out that case.
      return;
   rl_vec2d mv = move_vec();
   rl_vec2d fv = face_vec();
   float dot = mv.dot_product(fv);
   //threshold of recovery is gaussianesque.

   if (fabs(dot) * 100 > dice(9,20)){
      g->add_msg(_("The %s recovers from its skid."), name.c_str());
      skidding = false; //face_vec takes over.
      velocity *= dot; //wheels absorb horizontal velocity.
      if(dot < -.8){
         //pointed backwards, velo-wise.
         velocity *= -1; //move backwards.
      }
      move = face;
   }
}

// if not skidding, move_vec == face_vec, mv <dot> fv == 1, velocity*1 is returned.
float vehicle::forward_velocity(){
   rl_vec2d mv = move_vec();
   rl_vec2d fv = face_vec();
   float dot = mv.dot_product(fv);
   return velocity * dot;
}

rl_vec2d vehicle::velo_vec(){
    rl_vec2d ret;
    if(skidding)
       ret = move_vec();
    else
       ret = face_vec();
    ret = ret.normalized();
    ret = ret * velocity;
    return ret;
}

// normalized.
rl_vec2d vehicle::move_vec(){
    float mx,my;
    mx = cos (move.dir() * M_PI/180);
    my = sin (move.dir() * M_PI/180);
    rl_vec2d ret(mx,my);
    ret = ret.normalized();
    return ret;
}

// normalized.
rl_vec2d vehicle::face_vec(){
    float fx,fy;
    fx = cos (face.dir() * M_PI/180);
    fy = sin (face.dir() * M_PI/180);
    rl_vec2d ret(fx,fy);
    ret = ret.normalized();
    return ret;
}

