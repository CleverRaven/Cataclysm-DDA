#include "creature_tracker.h"
#include "output.h"

Creature_tracker::Creature_tracker()
{
}

Creature_tracker::~Creature_tracker()
{
    clear();
}

monster &Creature_tracker::find(int index)
{
    return *(_old_monsters_list[index]);
}

int Creature_tracker::mon_at(int x_pos, int y_pos) const
{
    return mon_at(point(x_pos, y_pos));
}

int Creature_tracker::mon_at(point coords) const
{
    std::map<point, int>::const_iterator iter = _old_monsters_by_location.find(coords);
    if (iter != _old_monsters_by_location.end()) {
        const int critter_id = iter->second;
        if (!_old_monsters_list[critter_id]->dead) {
            return critter_id;
        }
    }
    return -1;
}

int Creature_tracker::dead_mon_at(point coords) const
{
    std::map<point, int>::const_iterator iter = _old_monsters_by_location.find(coords);
    if (iter != _old_monsters_by_location.end()) {
        const int critter_id = iter->second;
        if (_old_monsters_list[critter_id]->dead) {
            return critter_id;
        }
    }
    return -1;
}

bool Creature_tracker::add(monster &critter)
{
    if (critter.type->id == "mon_null") { // Don't wanna spawn null monsters o.O
        return false;
    }
    if (-1 != mon_at(critter.pos())) {
        debugmsg("add_zombie: there's already a monster at %d,%d", critter.posx(), critter.posy());
        return false;
    }
    _old_monsters_by_location[point(critter.posx(), critter.posy())] = _old_monsters_list.size();
    _old_monsters_list.push_back(new monster(critter));
    return true;
}
size_t Creature_tracker::size() const
{
    return _old_monsters_list.size();
}

bool Creature_tracker::update_pos(Creature &critter, const int new_x_pos, const int new_y_pos)
{
    if (critter.xpos() == new_x_pos && critter.ypos() == new_y_pos) {
        return true; // success?
    }
    bool success = false;
    point current_location(critter.xpos(), critter.ypos());
    const int dead_critter_id = dead_mon_at(current_location);
    const int live_critter_id = mon_at(current_location);
    const int critter_id = critter.is_dead_state() ? dead_critter_id : live_critter_id;
    const int new_critter_id = mon_at(new_x_pos, new_y_pos);
    if (new_critter_id >= 0 && !_old_monsters_list[new_critter_id]->dead) {
        debugmsg("update_zombie_pos: new location %d,%d already has zombie %d",
                 new_x_pos, new_y_pos, new_critter_id);
    } else if (critter_id >= 0) {
        if (&critter == _old_monsters_list[critter_id]) {
            _old_monsters_by_location.erase(current_location);
            _old_monsters_by_location[point(new_x_pos, new_y_pos)] = critter_id;
            success = true;
        } else {
            debugmsg("update_zombie_pos: old location %d,%d had zombie %d instead",
                     critter.xpos(), critter.xpos(), critter_id);
        }
    } else if(_creatures_by_location.find(current_location) != _creatures_by_location.end()) {
        // There's a non-monster creature at the specified position.
        _creatures_by_location.erase(current_location);
        _creatures_by_location[point(new_x_pos, new_y_pos)] = &critter;
    } else {
        // We're changing the x/y coordinates of a zombie that hasn't been added
        // to the game yet. add_zombie() will update _old_monsters_by_location for us.
        debugmsg("update_zombie_pos: no such zombie at %d,%d (moving to %d,%d)",
                 critter.xpos(), critter.ypos(), new_x_pos, new_y_pos);
    }
    return success;
}

void Creature_tracker::remove(const int idx)
{
    monster &m = *_old_monsters_list[idx];
    const point oldloc(m.posx(), m.posy());
    const std::map<point, int>::const_iterator i = _old_monsters_by_location.find(oldloc);
    const int prev = (i == _old_monsters_by_location.end() ? -1 : i->second);

    if (prev == idx) {
        _old_monsters_by_location.erase(oldloc);
    }

    delete _old_monsters_list[idx];
    _old_monsters_list.erase(_old_monsters_list.begin() + idx);

    // Fix indices in _old_monsters_by_location for any zombies that were just moved down 1 place.
    for (std::map<point, int>::iterator iter = _old_monsters_by_location.begin(); iter != _old_monsters_by_location.end(); ++iter) {
        if (iter->second > idx) {
            --iter->second;
        }
    }
}

void Creature_tracker::clear()
{
    for (size_t i = 0; i < _old_monsters_list.size(); i++) {
        delete _old_monsters_list[i];
    }
    _old_monsters_list.clear();
    _old_monsters_by_location.clear();
}

void Creature_tracker::rebuild_cache()
{
    _old_monsters_by_location.clear();
    for (int ii = 0, max_ii = size(); ii < max_ii; ii++) {
        monster &critter = *_old_monsters_list[ii];
        _old_monsters_by_location[point(critter.xpos(), critter.ypos())] = ii;
    }

    _creatures_by_location.clear();
    std::set<Creature*>::iterator creature_it;
    for (creature_it = _creature_set.begin(); creature_it != _creature_set.end(); creature_it++) {
        Creature *critter = *creature_it;
        _creatures_by_location[point(critter->xpos(), critter->ypos())] = critter;
    }
}

const std::vector<monster>& Creature_tracker::list() const
{
    static std::vector<monster> for_now;
    for_now.clear();
    for (size_t i = 0; i < _old_monsters_list.size(); i++) {
        for_now.push_back(*_old_monsters_list[i]);
    }
    return for_now;
}

Creature *Creature_tracker::creature_at(point coords) const {
    int monster_index = mon_at(coords);
    if (monster_index != -1) {
        return _old_monsters_list[monster_index];
    }

    std::map<point, Creature*>::const_iterator it = _creatures_by_location.find(coords);
    if (it != _creatures_by_location.end()) {
        return it->second;
    }

    return NULL;
}

bool Creature_tracker::add_creature(Creature *creature) {
    point location(creature->xpos(), creature->ypos());
    if (creature_at(location)) {
        return false;
    }

    _creature_set.insert(creature);

    _creatures_by_location[location] = creature;

    return true;
}

void Creature_tracker::remove_creature(Creature *creature) {
    point location(creature->xpos(), creature->ypos());
    _creatures_by_location.erase(location);
    _creature_set.erase(creature);
}

std::set<Creature*> Creature_tracker::creature_set() const {
    std::set<Creature*> return_copy(_creature_set);
    for (size_t i = 0; i < _old_monsters_list.size(); i++) {
        return_copy.insert(_old_monsters_list[i]);
    }
    return return_copy;
}
