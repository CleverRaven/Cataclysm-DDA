#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "monstergenerator.h"
#include "messages.h"
#include <math.h>  // rounding
#include <sstream>

void mdeath::normal(monster *z)
{
    if ((g->u_see(*z)) && (!z->no_corpse_quiet)) {
        add_msg(m_good, _("The %s dies!"),
                z->name().c_str()); //Currently it is possible to get multiple messages that a monster died.
    }

    m_size monSize = (z->type->size);
    bool leaveCorpse = !((z->type->has_flag(MF_VERMIN)) || (z->no_corpse_quiet));

    // leave some blood if we have to
    if (!z->has_flag(MF_VERMIN)) {
        field_id type_blood = z->bloodType();
        if (type_blood != fd_null) {
            g->m.add_field(z->posx(), z->posy(), type_blood, 1);
        }
    }

    int maxHP = z->type->hp;
    if (!maxHP) {
        maxHP = 1;
    }

    float overflowDamage = std::max( -(z->hp), 0 );
    float corpseDamage = 5 * (overflowDamage / (maxHP * 2));

    if (leaveCorpse) {
        int gibAmount = int(floor(corpseDamage)) - 1;
        // allow one extra gib per 5 HP
        int gibLimit = 1 + (maxHP / 5.0);
        if (gibAmount > gibLimit) {
            gibAmount = gibLimit;
        }
        bool pulverized = (corpseDamage > 5 && overflowDamage > 150);
        if (!pulverized) {
            make_mon_corpse(z, int(floor(corpseDamage)));
        } else if (monSize >= MS_MEDIUM) {
            gibAmount += rng(1, 6);
        }
        // Limit chunking to flesh, veggy and insect creatures until other kinds are supported.
        bool leaveGibs = (z->made_of("flesh") || z->made_of("hflesh") || z->made_of("veggy") ||
                          z->made_of("iflesh"));
        if (leaveGibs) {
            make_gibs( z, gibAmount );
        }
    }
}

void mdeath::acid(monster *z)
{
    if (g->u_see(*z)) {
        if(z->type->dies.size() ==
           1) { //If this death function is the only function. The corpse gets dissolved.
            add_msg(m_mixed, _("The %s's body dissolves into acid."), z->name().c_str());
        } else {
            add_msg(m_warning, _("The %s's body leaks acid."), z->name().c_str());
        }
    }
    g->m.add_field(z->posx(), z->posy(), fd_acid, 3);
}

void mdeath::boomer(monster *z)
{
    std::string explode = string_format(_("a %s explode!"), z->name().c_str());
    g->sound(z->posx(), z->posy(), 24, explode);
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            g->m.bash( z->posx() + i, z->posy() + j, 10 );
            g->m.add_field(z->posx() + i, z->posy() + j, fd_bile, 1);
            int mondex = g->mon_at(z->posx() + i, z->posy() + j);
            if (mondex != -1) {
                g->zombie(mondex).stumble(false);
                g->zombie(mondex).moves -= 250;
            }
        }
    }
    if (rl_dist( z->pos(), g->u.pos() ) == 1) {
        g->u.add_env_effect("boomered", bp_eyes, 2, 24);
    }
}

void mdeath::kill_vines(monster *z)
{
    std::vector<int> vines;
    std::vector<int> hubs;
    for (size_t i = 0; i < g->num_zombies(); i++) {
        bool isHub = g->zombie(i).type->id == "mon_creeper_hub";
        if (isHub && (g->zombie(i).posx() != z->posx() || g->zombie(i).posy() != z->posy())) {
            hubs.push_back(i);
        }
        if (g->zombie(i).type->id == "mon_creeper_vine") {
            vines.push_back(i);
        }
    }

    int curX, curY;
    for (auto &i : vines) {
        monster *vine = &(g->zombie(i));
        int dist = rl_dist( vine->pos(), z->pos() );
        bool closer = false;
        for (auto &j : hubs) {
            curX = g->zombie(j).posx();
            curY = g->zombie(j).posy();
            if (rl_dist(vine->posx(), vine->posy(), curX, curY) < dist) {
                break;
            }
        }
        if (!closer) {
            vine->die(z);
        }
    }
}

void mdeath::vine_cut(monster *z)
{
    std::vector<int> vines;
    for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
        for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
            if (x == z->posx() && y == z->posy()) {
                y++; // Skip ourselves
            }
            int mondex = g->mon_at(x, y);
            if (mondex != -1 && g->zombie(mondex).type->id == "mon_creeper_vine") {
                vines.push_back(mondex);
            }
        }
    }

    for (auto &i : vines) {
        bool found_neighbor = false;
        monster *vine = &(g->zombie( i ));
        for (int x = vine->posx() - 1; x <= vine->posx() + 1 && !found_neighbor; x++) {
            for (int y = vine->posy() - 1; y <= vine->posy() + 1 && !found_neighbor; y++) {
                if (x != z->posx() || y != z->posy()) {
                    // Not the dying vine
                    int mondex = g->mon_at(x, y);
                    if (mondex != -1 && (g->zombie(mondex).type->id == "mon_creeper_hub" ||
                                         g->zombie(mondex).type->id == "mon_creeper_vine")) {
                        found_neighbor = true;
                    }
                }
            }
        }
        if (!found_neighbor) {
            vine->die(z);
        }
    }
}

void mdeath::triffid_heart(monster *z)
{
    if (g->u_see(*z)) {
        add_msg(m_warning, _("The surrounding roots begin to crack and crumble."));
    }
    g->add_event(EVENT_ROOTS_DIE, int(calendar::turn) + 100);
}

void mdeath::fungus(monster *z)
{
    monster spore(GetMType("mon_spore"));
    bool fungal = false;
    int mondex = -1;
    int sporex, sporey;
    //~ the sound of a fungus dying
    g->sound(z->posx(), z->posy(), 10, _("Pouf!"));
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            sporex = z->posx() + i;
            sporey = z->posy() + j;
            mondex = g->mon_at(sporex, sporey);
            if (g->m.move_cost(sporex, sporey) > 0) {
                if (mondex != -1) {
                    // Spores hit a monster
                    fungal = g->zombie(mondex).type->in_species("FUNGUS");
                    if (g->u_see(sporex, sporey) && !fungal) {
                        add_msg(_("The %s is covered in tiny spores!"),
                                g->zombie(mondex).name().c_str());
                    }
                    monster &critter = g->zombie( mondex );
                    if( !critter.make_fungus() ) {
                        critter.die( z ); // counts as kill by monster z
                    }
                } else if (g->u.posx == sporex && g->u.posy == sporey) {
                    // Spores hit the player
                    if (g->u.has_trait("TAIL_CATTLE") && one_in(20 - g->u.dex_cur - g->u.skillLevel("melee"))) {
                        add_msg(_("The spores land on you, but you quickly swat them off with your tail!"));
                        return;
                    }
                    bool hit = false;
                    if (one_in(4) && g->u.add_env_effect("spores", bp_head, 3, 90, bp_head)) {
                        hit = true;
                    }
                    if (one_in(2) && g->u.add_env_effect("spores", bp_torso, 3, 90, bp_torso)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.add_env_effect("spores", bp_arm_l, 3, 90, bp_arm_l)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.add_env_effect("spores", bp_arm_r, 3, 90, bp_arm_r)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.add_env_effect("spores", bp_leg_l, 3, 90, bp_leg_l)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.add_env_effect("spores", bp_leg_r, 3, 90, bp_leg_r)) {
                        hit = true;
                    }
                    if (hit && (g->u.has_trait("TAIL_CATTLE") &&
                                one_in(20 - g->u.dex_cur - g->u.skillLevel("melee")))) {
                        add_msg(_("The spores land on you, but you quickly swat them off with your tail!"));
                        hit = false;
                    }
                    if (hit) {
                        add_msg(m_warning, _("You're covered in tiny spores!"));
                    }
                } else if (one_in(2) && g->num_zombies() <= 1000) {
                    // Spawn a spore
                    spore.spawn(sporex, sporey);
                    g->add_zombie(spore);
                }
            }
        }
    }
}

void mdeath::disintegrate(monster *z)
{
    if (g->u_see(*z)) {
        add_msg(m_good, _("The %s disintegrates!"), z->name().c_str());
    }
}

void mdeath::worm(monster *z)
{
    if (g->u_see(*z)) {
        if(z->type->dies.size() == 1) {
            add_msg(m_good, _("The %s splits in two!"), z->name().c_str());
        } else {
            add_msg(m_warning, _("Two worms crawl out of the %s's corpse."), z->name().c_str());
        }
    }

    std::vector <point> wormspots;
    int wormx, wormy;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            wormx = z->posx() + i;
            wormy = z->posy() + j;
            if (g->m.has_flag("DIGGABLE", wormx, wormy) &&
                !(g->u.posx == wormx && g->u.posy == wormy)) {
                wormspots.push_back(point(wormx, wormy));
            }
        }
    }
    int worms = 0;
    while(worms < 2 && !wormspots.empty()) {
        monster worm(GetMType("mon_halfworm"));
        int rn = rng(0, wormspots.size() - 1);
        if(-1 == g->mon_at(wormspots[rn])) {
            worm.spawn(wormspots[rn].x, wormspots[rn].y);
            g->add_zombie(worm);
            worms++;
        }
        wormspots.erase(wormspots.begin() + rn);
    }
}

void mdeath::disappear(monster *z)
{
    if (g->u_see(*z)) {
        add_msg(m_good, _("The %s disappears."), z->name().c_str());
    }
}

void mdeath::guilt(monster *z)
{
    const int MAX_GUILT_DISTANCE = 5;
    int kill_count = g->kill_count(z->type->id);
    int maxKills = 100; // this is when the player stop caring altogether.

    // different message as we kill more of the same monster
    std::string msg = _("You feel guilty for killing %s."); // default guilt message
    game_message_type msgtype = m_bad; // default guilt message type
    std::map<int, std::string> guilt_tresholds;
    guilt_tresholds[75] = _("You feel ashamed for killing %s.");
    guilt_tresholds[50] = _("You regret killing %s.");
    guilt_tresholds[25] = _("You feel remorse for killing %s.");

    if (g->u.has_trait("PSYCHOPATH") || g->u.has_trait("PRED3") || g->u.has_trait("PRED4") ) {
        return;
    }
    if (rl_dist( z->pos(), g->u.pos() ) > MAX_GUILT_DISTANCE) {
        // Too far away, we can deal with it.
        return;
    }
    if (z->hp >= 0) {
        // We probably didn't kill it
        return;
    }
    if (kill_count >= maxKills) {
        // player no longer cares
        if (kill_count == maxKills) {
            //~ Message after killing a lot of monsters which would normally affect the morale negatively. %s is the monster name, it will be pluralized with a number of 100.
            add_msg(m_good, _("After killing so many bloody %s you no longer care "
                              "about their deaths anymore."), z->name(maxKills).c_str());
        }
        return;
    } else if ((g->u.has_trait("PRED1")) || (g->u.has_trait("PRED2"))) {
        msg = (_("Culling the weak is distasteful, but necessary."));
        msgtype = m_neutral;
    } else {
        msgtype = m_bad;
        for( auto &guilt_treshold : guilt_tresholds ) {
            if( kill_count >= guilt_treshold.first ) {
                msg = guilt_treshold.second;
                break;
            }
        }
    }

    add_msg(msgtype, msg.c_str(), z->name().c_str());

    int moraleMalus = -50 * (1.0 - ((float) kill_count / maxKills));
    int maxMalus = -250 * (1.0 - ((float) kill_count / maxKills));
    int duration = 300 * (1.0 - ((float) kill_count / maxKills));
    int decayDelay = 30 * (1.0 - ((float) kill_count / maxKills));
    if (z->type->in_species("ZOMBIE")) {
        moraleMalus /= 10;
        if (g->u.has_trait("PACIFIST")) {
            moraleMalus *= 5;
        } else if (g->u.has_trait("PRED1")) {
            moraleMalus /= 4;
        } else if (g->u.has_trait("PRED2")) {
            moraleMalus /= 5;
        }
    }
    g->u.add_morale(MORALE_KILLED_MONSTER, moraleMalus, maxMalus, duration, decayDelay);

}

void mdeath::blobsplit(monster *z)
{
    int speed = z->get_speed() - rng(30, 50);
    g->m.spawn_item(z->posx(), z->posy(), "slime_scrap", 1, 0, calendar::turn, rng(1, 4));
    if( z->get_speed() <= 0) {
        if (g->u_see(*z)) {
            //  TODO:  Add vermin-tagged tiny versions of the splattered blob  :)
            add_msg(m_good, _("The %s splatters apart."), z->name().c_str());
        }
        return;
    }
    monster blob(GetMType((speed < 50 ? "mon_blob_small" : "mon_blob")));
    blob.set_speed_base( speed );
    // If we're tame, our kids are too
    blob.friendly = z->friendly;
    if (g->u_see(*z)) {
        if(z->type->dies.size() == 1) {
            add_msg(m_good, _("The %s splits in two!"), z->name().c_str());
        } else {
            add_msg(m_bad, _("Two small blobs slither out of the corpse."), z->name().c_str());
        }
    }
    blob.hp = speed;
    std::vector <point> valid;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            bool moveOK = (g->m.move_cost(z->posx() + i, z->posy() + j) > 0);
            bool monOK = g->mon_at(z->posx() + i, z->posy() + j) == -1;
            bool posOK = (g->u.posx != z->posx() + i || g->u.posy != z->posy() + j);
            if (moveOK && monOK && posOK) {
                valid.push_back(point(z->posx() + i, z->posy() + j));
            }
        }
    }

    int rn;
    for (int s = 0; s < 2 && !valid.empty(); s++) {
        rn = rng(0, valid.size() - 1);
        blob.spawn(valid[rn].x, valid[rn].y);
        g->add_zombie(blob);
        valid.erase(valid.begin() + rn);
    }
}

void mdeath::brainblob(monster *z) {
    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        monster *candidate = &g->zombie( i );
        if(candidate->type->in_species("BLOB") && candidate->type->id != "mon_blob_brain" ) {
            candidate->remove_effect("controlled");
        }
    }
    blobsplit(z);
}

void mdeath::jackson(monster *z) {
    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        monster *candidate = &g->zombie( i );
        if(candidate->type->id == "mon_zombie_dancer" ) {
            candidate->poly(GetMType("mon_zombie_hulk"));
            candidate->remove_effect("controlled");
        }
        if (g->u_see(z->posx(), z->posy())) {
            add_msg(m_warning, _("The music stops!"));
        }
    }
}

void mdeath::melt(monster *z)
{
    if (g->u_see(*z)) {
        add_msg(m_good, _("The %s melts away."), z->name().c_str());
    }
}

void mdeath::amigara(monster *z)
{
    if (!g->u.has_effect("amigara")) {
        return;
    }
    int count = 0;
    for (size_t i = 0; i < g->num_zombies(); i++) {
        if (g->zombie(i).type->id == "mon_amigara_horror") {
            count++;
        }
    }
    if (count <= 1) { // We're the last!
        g->u.remove_effect("amigara");
        add_msg(_("Your obsession with the fault fades away..."));
        g->m.spawn_artifact( z->posx(), z->posy() );
    }
}

void mdeath::thing(monster *z)
{
    monster thing(GetMType("mon_thing"));
    thing.spawn(z->posx(), z->posy());
    g->add_zombie(thing);
}

void mdeath::explode(monster *z)
{
    int size = 0;
    switch (z->type->size) {
    case MS_TINY:
        size = 4;
        break;
    case MS_SMALL:
        size = 8;
        break;
    case MS_MEDIUM:
        size = 14;
        break;
    case MS_LARGE:
        size = 20;
        break;
    case MS_HUGE:
        size = 26;
        break;
    }
    g->explosion(z->posx(), z->posy(), size, 0, false);
}

void mdeath::focused_beam(monster *z)
{

    for (int k = g->m.i_at(z->posx(), z->posy()).size() - 1; k >= 0; k--) {
        if (g->m.i_at(z->posx(), z->posy())[k].type->id == "processor") {
            g->m.i_rem(z->posx(), z->posy(), k);
        }
    }

    if (z->inv.size() > 0) {

        if (g->u_see(*z)) {
            add_msg(m_warning, _("As the final light is destroyed, it erupts in a blinding flare!"));
        }

        item &settings = z->inv[0];

        int x = z->posx() + settings.get_var( "SL_SPOT_X", 0 );
        int y = z->posy() + settings.get_var( "SL_SPOT_Y", 0 );

        std::vector <point> traj = line_to(z->posx(), z->posy(), x, y, 0);
        for( auto &elem : traj ) {
            if( !g->m.trans( elem.x, elem.y ) ) {
                break;
            }
            g->m.add_field( elem.x, elem.y, fd_dazzling, 2 );
        }
    }

    z->inv.clear();

    g->explosion(z->posx(), z->posy(), 8, 0, false);
}

void mdeath::broken(monster *z) {
    // Bail out if flagged (simulates eyebot flying away)
    if (z->no_corpse_quiet) {
        return;
    }
    std::string item_id = z->type->id;
    if (item_id.compare(0, 4, "mon_") == 0) {
        item_id.erase(0, 4);
    }
    // make "broken_manhack", or "broken_eyebot", ...
    item_id.insert(0, "broken_");
    g->m.spawn_item(z->posx(), z->posy(), item_id, 1, 0, calendar::turn);
}

void mdeath::ratking(monster *z)
{
    g->u.remove_effect("rat");
    if (g->u_see(*z)) {
        add_msg(m_warning, _("Rats suddenly swarm into view."));
    }

    std::vector <point> ratspots;
    int ratx, raty;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            ratx = z->posx() + i;
            raty = z->posy() + j;
            if (g->is_empty(ratx, raty)) {
                ratspots.push_back(point(ratx, raty));
            }
        }
    }
    monster rat(GetMType("mon_sewer_rat"));
    for (int rats = 0; rats < 7 && !ratspots.empty(); rats++) {
        int rn = rng(0, ratspots.size() - 1);
        point rp = ratspots[rn];
        ratspots.erase(ratspots.begin() + rn);
        rat.spawn(rp.x, rp.y);
        g->add_zombie(rat);
    }
}

void mdeath::darkman(monster *z)
{
    g->u.remove_effect("darkness");
    if (g->u_see(*z)) {
        add_msg(m_good, _("The %s melts away."), z->name().c_str());
    }
}

void mdeath::gas(monster *z)
{
    std::string explode = string_format(_("a %s explode!"), z->name().c_str());
    g->sound(z->posx(), z->posy(), 24, explode);
    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            g->m.add_field(z->posx() + i, z->posy() + j, fd_toxic_gas, 3);
            int mondex = g->mon_at(z->posx() + i, z->posy() + j);
            if (mondex != -1) {
                g->zombie(mondex).stumble(false);
                g->zombie(mondex).moves -= 250;
            }
        }
    }
}

void mdeath::smokeburst(monster *z)
{
    std::string explode = string_format(_("a %s explode!"), z->name().c_str());
    g->sound(z->posx(), z->posy(), 24, explode);
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            g->m.add_field(z->posx() + i, z->posy() + j, fd_smoke, 3);
            int mondex = g->mon_at(z->posx() + i, z->posy() + j);
            if (mondex != -1) {
                g->zombie(mondex).stumble(false);
                g->zombie(mondex).moves -= 250;
            }
        }
    }
}

void mdeath::gameover(monster *z)
{
    add_msg(m_bad, _("The %s was destroyed!  GAME OVER!"), z->name().c_str());
    g->u.hp_cur[hp_torso] = 0;
}

void mdeath::kill_breathers(monster *z)
{
    (void)z; //unused
    for (size_t i = 0; i < g->num_zombies(); i++) {
        const std::string monID = g->zombie(i).type->id;
        if (monID == "mon_breather_hub " || monID == "mon_breather") {
            g->zombie(i).die( nullptr );
        }
    }
}

void make_gibs(monster *z, int amount)
{
    if (amount <= 0) {
        return;
    }
    const int zposx = z->posx();
    const int zposy = z->posy();
    field_id type_blood = z->bloodType();

    for (int i = 0; i < amount; i++) {
        // leave gibs, if there are any
        const int gibX = rng(zposx - 1, zposx + 1);
        const int gibY = rng(zposy - 1, zposy + 1);
        const int gibDensity = rng(1, i + 1);
        int junk;
        if( z->gibType() != fd_null ) {
            if(  g->m.clear_path( zposx, zposy, gibX, gibY, 2, 1, 100, junk ) ) {
                // Only place gib if there's a clear path for it to get there.
                g->m.add_field( gibX, gibY, z->gibType(), gibDensity );
            }
        }
        if( type_blood != fd_null ) {
            const int bloodX = rng(zposx - 1, zposx + 1);
            const int bloodY = rng(zposy - 1, zposy + 1);
            if( g->m.clear_path( zposx, zposy, bloodX, bloodY, 2, 1, 100, junk ) ) {
                // Only place blood if there's a clear path for it to get there.
                g->m.add_field(bloodX, bloodY, type_blood, 1);
            }
        }
    }
}

void make_mon_corpse(monster *z, int damageLvl)
{
    const int MAX_DAM = 4;
    item corpse;
    corpse.make_corpse("corpse", z->type, calendar::turn);
    corpse.damage = damageLvl > MAX_DAM ? MAX_DAM : damageLvl;
    if( z->has_effect("pacified") && z->type->in_species("ZOMBIE") ) {
        // Pacified corpses have a chance of becoming un-pacified when regenerating.
        corpse.set_var( "zlave", one_in(2) ? "zlave" : "mutilated" );
    }
    g->m.add_item_or_charges(z->posx(), z->posy(), corpse);
}
