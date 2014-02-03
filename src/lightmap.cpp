#include "mapdata.h"
#include "map.h"
#include "game.h"
#include "lightmap.h"
#include "options.h"

#define INBOUNDS(x, y) \
    (x >= 0 && x < SEEX * MAPSIZE && y >= 0 && y < SEEY * MAPSIZE)
#define LIGHTMAP_CACHE_X SEEX * MAPSIZE
#define LIGHTMAP_CACHE_Y SEEY * MAPSIZE

void map::generate_lightmap()
{
    memset(lm, 0, sizeof(lm));
    memset(sm, 0, sizeof(sm));

    /* Bulk light sources wastefully cast rays into neighbors; a burning hospital can produce
         significant slowdown, so for stuff like fire and lava:
     * Step 1: Store the position and luminance in buffer via add_light_source, for efficient
         checking of neighbors.
     * Step 2: After everything else, iterate buffer and apply_light_source only in non-redundant
         directions
     * Step 3: Profit!
     */
    memset(light_source_buffer, 0, sizeof(light_source_buffer));


    const int dir_x[] = { 1, 0 , -1,  0 };
    const int dir_y[] = { 0, 1 ,  0, -1 };
    const int dir_d[] = { 180, 270, 0, 90 };
    const float held_luminance = g->u.active_light();
    const float natural_light = g->natural_light_level();

    if (natural_light > LIGHT_SOURCE_BRIGHT) {
        // Apply sunlight, first light source so just assign
        for(int sx = DAYLIGHT_LEVEL - (natural_light / 2);
            sx < LIGHTMAP_CACHE_X - (natural_light / 2); ++sx) {
            for(int sy = DAYLIGHT_LEVEL - (natural_light / 2);
                sy < LIGHTMAP_CACHE_Y - (natural_light / 2); ++sy) {
                // In bright light indoor light exists to some degree
                if (!g->m.is_outside(sx, sy)) {
                    lm[sx][sy] = LIGHT_AMBIENT_LOW;
                } else if (g->u.posx == sx && g->u.posy == sy ) {
                    //Only apply daylight on square where player is standing to avoid flooding
                    // the lightmap  when in less than total sunlight.
                    lm[sx][sy] = natural_light;
                }
            }
        }
    }

    // Apply player light sources
    if (held_luminance > LIGHT_AMBIENT_LOW) {
        apply_light_source(g->u.posx, g->u.posy, held_luminance, trigdist);
    }
    for(int sx = 0; sx < LIGHTMAP_CACHE_X; ++sx) {
        for(int sy = 0; sy < LIGHTMAP_CACHE_Y; ++sy) {
            const ter_id terrain = g->m.ter(sx, sy);
            const std::vector<item> &items = g->m.i_at(sx, sy);
            field &current_field = g->m.field_at(sx, sy);
            // When underground natural_light is 0, if this changes we need to revisit
            // Only apply this whole thing if the player is inside,
            // buildings will be shadowed when outside looking in.
            if (natural_light > LIGHT_AMBIENT_LOW && !g->m.is_outside(g->u.posx, g->u.posy) ) {
                if (!g->m.is_outside(sx, sy)) {
                    // Apply light sources for external/internal divide
                    for(int i = 0; i < 4; ++i) {
                        if (INBOUNDS(sx + dir_x[i], sy + dir_y[i]) &&
                            g->m.is_outside(sx + dir_x[i], sy + dir_y[i])) {
                            lm[sx][sy] = natural_light;

                            if (g->m.light_transparency(sx, sy) > LIGHT_TRANSPARENCY_SOLID) {
                                apply_light_arc(sx, sy, dir_d[i], natural_light);
                            }
                        }
                    }
                }
            }
            for( std::vector<item>::const_iterator itm = items.begin(); itm != items.end(); ++itm ) {

                float ilum = 0.0; // brightness
                int iwidth = 0; // 0-360 degrees. 0 is a circular light_source
                int idir = 0;   // otherwise, it's a light_arc pointed in this direction
                if ( itm->getlight(ilum, iwidth, idir ) ) {
                    if ( iwidth > 0 ) {
                        apply_light_arc( sx, sy, idir, ilum, iwidth );
                    } else {
                        add_light_source(sx, sy, ilum);
                    }
                }
            }
            if(terrain == t_lava) {
                add_light_source(sx, sy, 50 );
            }

            if(terrain == t_console) {
                add_light_source(sx, sy, 3 );
            }

            if(terrain == t_emergency_light) {
                add_light_source(sx, sy, 3 );
            }

            field_entry *cur = NULL;
            for(std::map<field_id, field_entry *>::iterator field_list_it = current_field.getFieldStart();
                field_list_it != current_field.getFieldEnd(); ++field_list_it) {
                cur = field_list_it->second;

                if(cur == NULL) {
                    continue;
                }
                // TODO: [lightmap] Attach light brightness to fields
                switch(cur->getFieldType()) {
                case fd_fire:
                    if (3 == cur->getFieldDensity()) {
                        add_light_source(sx, sy, 160);
                    } else if (2 == cur->getFieldDensity()) {
                        add_light_source(sx, sy, 60);
                    } else {
                        add_light_source(sx, sy, 16);
                    }
                    break;
                case fd_fire_vent:
                case fd_flame_burst:
                    add_light_source(sx, sy, 8);
                    break;
                case fd_electricity:
                case fd_plasma:
                    if (3 == cur->getFieldDensity()) {
                        add_light_source(sx, sy, 8);
                    } else if (2 == cur->getFieldDensity()) {
                        add_light_source(sx, sy, 1);
                    } else {
                        apply_light_source(sx, sy, LIGHT_SOURCE_LOCAL,
                                           trigdist);    // kinda a hack as the square will still get marked
                    }
                    break;
                case fd_laser:
                    apply_light_source(sx, sy, 1, trigdist);
                    break;
                }
            }
        }
    }

    for (int i = 0; i < g->num_zombies(); ++i) {
        int mx = g->zombie(i).posx();
        int my = g->zombie(i).posy();
        if (INBOUNDS(mx, my)) {
            if (g->zombie(i).has_effect("onfire")) {
                apply_light_source(mx, my, 3, trigdist);
            }
            // TODO: [lightmap] Attach natural light brightness to creatures
            // TODO: [lightmap] Allow creatures to have light attacks (ie: eyebot)
            // TODO: [lightmap] Allow creatures to have facing and arc lights
            if (g->zombie(i).type->luminance > 0) {
                apply_light_source(mx, my, g->zombie(i).type->luminance, trigdist);
            }
        }
    }

    // Apply any vehicle light sources
    VehicleList vehs = g->m.get_vehicles();
    for(int v = 0; v < vehs.size(); ++v) {
        if(vehs[v].v->lights_on) {
            int dir = vehs[v].v->face.dir();
            float veh_luminance = 0.0;
            float iteration = 1.0;
            std::vector<int> light_indices = vehs[v].v->all_parts_with_feature(VPFLAG_CONE_LIGHT);
            for (std::vector<int>::iterator part = light_indices.begin();
                 part != light_indices.end(); ++part) {
                veh_luminance += ( vehs[v].v->part_info(*part).bonus / iteration );
                iteration = iteration * 1.1;
            }
            if (veh_luminance > LL_LIT) {
                for (std::vector<int>::iterator part = light_indices.begin();
                     part != light_indices.end(); ++part) {
                    int px = vehs[v].x + vehs[v].v->parts[*part].precalc_dx[0];
                    int py = vehs[v].y + vehs[v].v->parts[*part].precalc_dy[0];
                    if(INBOUNDS(px, py)) {
                        apply_light_arc(px, py, dir + vehs[v].v->parts[*part].direction, veh_luminance, 45);
                    }
                }
            }
        }
        if(vehs[v].v->overhead_lights_on) {
            std::vector<int> light_indices = vehs[v].v->all_parts_with_feature(VPFLAG_CIRCLE_LIGHT);
            for (std::vector<int>::iterator part = light_indices.begin();
                 part != light_indices.end(); ++part) {
                if((g->turn % 2 && vehs[v].v->part_info(*part).has_flag(VPFLAG_ODDTURN)) ||
                   (!(g->turn % 2) && vehs[v].v->part_info(*part).has_flag(VPFLAG_EVENTURN)) ||
                   (!vehs[v].v->part_info(*part).has_flag(VPFLAG_EVENTURN) &&
                    !vehs[v].v->part_info(*part).has_flag(VPFLAG_ODDTURN))) {
                    int px = vehs[v].x + vehs[v].v->parts[*part].precalc_dx[0];
                    int py = vehs[v].y + vehs[v].v->parts[*part].precalc_dy[0];
                    if(INBOUNDS(px, py)) {
                        add_light_source( px, py, vehs[v].v->part_info(*part).bonus );
                    }
                }
            }
        }
    }

    /* Now that we have position and intensity of all bulk light sources, apply_ them
      This may seem like extra work, but take a 12x12 raging inferno:
        unbuffered: (12^2)*(160*4) = apply_light_ray x 92160
        buffered:   (12*4)*(160)   = apply_light_ray x 7680
    */
    for(int sx = 0; sx < LIGHTMAP_CACHE_X; ++sx) {
        for(int sy = 0; sy < LIGHTMAP_CACHE_Y; ++sy) {
            if ( light_source_buffer[sx][sy] > 0. ) {
                apply_light_source(sx, sy, light_source_buffer[sx][sy],
                                   ( trigdist && light_source_buffer[sx][sy] > 3. ) );
            }
        }
    }


    if (g->u.has_active_bionic("bio_night") ) {
        for(int sx = 0; sx < LIGHTMAP_CACHE_X; ++sx) {
            for(int sy = 0; sy < LIGHTMAP_CACHE_Y; ++sy) {
                if (rl_dist(sx, sy, g->u.posx, g->u.posy) < 15) {
                    lm[sx][sy] = 0;
                }
            }
        }
    }
}

void map::add_light_source(int x, int y, float luminance )
{
    light_source_buffer[x][y] = luminance;
}

lit_level map::light_at(int dx, int dy)
{
    if (!INBOUNDS(dx, dy)) {
        return LL_DARK;    // Out of bounds
    }

    if (sm[dx][dy] >= LIGHT_SOURCE_BRIGHT) {
        return LL_BRIGHT;
    }

    if (lm[dx][dy] >= LIGHT_AMBIENT_LIT) {
        return LL_LIT;
    }

    if (lm[dx][dy] >= LIGHT_AMBIENT_LOW) {
        return LL_LOW;
    }

    return LL_DARK;
}

float map::ambient_light_at(int dx, int dy)
{
    if (!INBOUNDS(dx, dy)) {
        return 0.0f;
    }

    return lm[dx][dy];
}

bool map::pl_sees(int fx, int fy, int tx, int ty, int max_range)
{
    if (!INBOUNDS(tx, ty)) {
        return false;
    }

    if (max_range >= 0 && (abs(tx - fx) > max_range || abs(ty - fy) > max_range)) {
        return false;    // Out of range!
    }

    return seen_cache[tx][ty];
}



/**
 * Calculates the Field Of View for the provided map from the given x, y
 * coordinates. Returns a lightmap for a result where the values represent a
 * percentage of fully lit.
 *
 * A value equal to or below 0 means that cell is not in the
 * field of view, whereas a value equal to or above 1 means that cell is
 * in the field of view.
 *
 * @param startx the horizontal component of the starting location
 * @param starty the vertical component of the starting location
 * @param radius the maximum distance to draw the FOV
 */
void map::build_seen_cache()
{
    memset(seen_cache, false, sizeof(seen_cache));
    seen_cache[g->u.posx][g->u.posy] = true;

    castLight( 1, 1.0f, 0.0f, 0, 1, 1, 0 );
    castLight( 1, 1.0f, 0.0f, 1, 0, 0, 1 );

    castLight( 1, 1.0f, 0.0f, 0, -1, 1, 0 );
    castLight( 1, 1.0f, 0.0f, -1, 0, 0, 1 );

    castLight( 1, 1.0f, 0.0f, 0, 1, -1, 0 );
    castLight( 1, 1.0f, 0.0f, 1, 0, 0, -1 );

    castLight( 1, 1.0f, 0.0f, 0, -1, -1, 0 );
    castLight( 1, 1.0f, 0.0f, -1, 0, 0, -1 );
}

void map::castLight( int row, float start, float end, int xx, int xy, int yx, int yy )
{
    float newStart = 0.0f;
    float radius = 60.0f;
    if( start < end ) {
        return;
    }
    bool blocked = false;
    for( int distance = row; distance <= radius && !blocked; distance++ ) {
        int deltaY = -distance;
        for( int deltaX = -distance; deltaX <= 0; deltaX++ ) {
            int currentX = g->u.posx + deltaX * xx + deltaY * xy;
            int currentY = g->u.posy + deltaX * yx + deltaY * yy;
            float leftSlope = (deltaX - 0.5f) / (deltaY + 0.5f);
            float rightSlope = (deltaX + 0.5f) / (deltaY - 0.5f);

            if( !(currentX >= 0 && currentY >= 0 && currentX < SEEX * my_MAPSIZE &&
                  currentY < SEEY * my_MAPSIZE) || start < rightSlope ) {
                continue;
            } else if( end > leftSlope ) {
                break;
            }

            //check if it's within the visible area and mark visible if so
            if( rl_dist(0, 0, deltaX, deltaY) <= radius ) {
                /*
                float bright = (float) (1 - (rStrat.radius(deltaX, deltaY) / radius));
                lightMap[currentX][currentY] = bright;
                */
                seen_cache[currentX][currentY] = true;
            }

            if( blocked ) {
                //previous cell was a blocking one
                if( light_transparency(currentX, currentY) == LIGHT_TRANSPARENCY_SOLID ) {
                    //hit a wall
                    newStart = rightSlope;
                    continue;
                } else {
                    blocked = false;
                    start = newStart;
                }
            } else {
                if( light_transparency(currentX, currentY) == LIGHT_TRANSPARENCY_SOLID &&
                    distance < radius ) {
                    //hit a wall within sight line
                    blocked = true;
                    castLight(distance + 1, start, leftSlope, xx, xy, yx, yy);
                    newStart = rightSlope;
                }
            }
        }
    }
}

void map::apply_light_source(int x, int y, float luminance, bool trig_brightcalc )
{
    static bool lit[LIGHTMAP_CACHE_X][LIGHTMAP_CACHE_Y];
    memset(lit, 0, sizeof(lit));

    if (INBOUNDS(x, y)) {
        lit[x][y] = true;
        lm[x][y] += std::max(luminance, static_cast<float>(LL_LOW));
        sm[x][y] += luminance;
    }
    if ( luminance <= 1 ) {
        return;
    } else if ( luminance <= 2 ) {
        luminance = 1.49f;
    }
    /* If we're a 5 luminance fire , we skip casting rays into ey && sx if we have
         neighboring fires to the north and west that were applied via light_source_buffer
       If there's a 1 luminance candle east in buffer, we still cast rays into ex since it's smaller
       If there's a 100 luminance magnesium flare south added via apply_light_source instead od
         add_light_source, it's unbuffered so we'll still cast rays into sy.

          ey
        nnnNnnn
        w     e
        w  5 +e
     sx W 5*1+E ex
        w ++++e
        w+++++e
        sssSsss
           sy
    */
    const int peer_inbounds = LIGHTMAP_CACHE_X - 1;
    bool north = (y != 0 && light_source_buffer[x][y - 1] < luminance );
    bool south = (y != peer_inbounds && light_source_buffer[x][y + 1] < luminance );
    bool east = (x != peer_inbounds && light_source_buffer[x + 1][y] < luminance );
    bool west = (x != 0 && light_source_buffer[x - 1][y] < luminance );

    if (luminance > LIGHT_SOURCE_LOCAL) {
        int range = LIGHT_RANGE(luminance);
        int sx = x - range;
        int ex = x + range;
        int sy = y - range;
        int ey = y + range;

        for(int off = sx; off <= ex; ++off) {
            if ( south ) {
                apply_light_ray(lit, x, y, off, sy, luminance, trig_brightcalc);
            }
            if ( north ) {
                apply_light_ray(lit, x, y, off, ey, luminance, trig_brightcalc);
            }
        }

        // Skip corners with + 1 and < as they were done
        for(int off = sy + 1; off < ey; ++off) {
            if ( west ) {
                apply_light_ray(lit, x, y, sx, off, luminance, trig_brightcalc);
            }
            if ( east ) {
                apply_light_ray(lit, x, y, ex, off, luminance, trig_brightcalc);
            }
        }
    }
}


void map::apply_light_arc(int x, int y, int angle, float luminance, int wideangle )
{
    if (luminance <= LIGHT_SOURCE_LOCAL) {
        return;
    }

    static bool lit[LIGHTMAP_CACHE_X][LIGHTMAP_CACHE_Y];
    memset(lit, 0, sizeof(lit));

#define lum_mult 3.0

    luminance = luminance * lum_mult;

    int range = LIGHT_RANGE(luminance);
    apply_light_source(x, y, LIGHT_SOURCE_LOCAL, trigdist);

    // Normalise (should work with negative values too)

    const double PI = 3.14159265358979f;
    const double HALFPI = 1.570796326794895f;
    const double wangle = wideangle / 2.0;

    int nangle = angle % 360;

    int endx, endy;
    double rad = PI * (double)nangle / 180;
    calc_ray_end(nangle, range, x, y, &endx, &endy);
    apply_light_ray(lit, x, y, endx, endy , luminance, trigdist);

    int testx, testy;
    calc_ray_end(wangle + nangle, range, x, y, &testx, &testy);

    double wdist = sqrt(double((endx - testx) * (endx - testx) + (endy - testy) * (endy - testy)));
    if(wdist > 0.5) {
        double wstep = ( wangle / ( wdist *
                                    1.42 ) ); // attempt to determine beam density required to cover all squares

        for (double ao = wstep; ao <= wangle; ao += wstep) {
            if ( trigdist ) {
                double fdist = (ao * HALFPI) / wangle;
                double orad = ( PI * ao / 180.0 );
                endx = int( x + ( (double)range - fdist * 2.0) * cos(rad + orad) );
                endy = int( y + ( (double)range - fdist * 2.0) * sin(rad + orad) );
                apply_light_ray(lit, x, y, endx, endy , luminance, true);

                endx = int( x + ( (double)range - fdist * 2.0) * cos(rad - orad) );
                endy = int( y + ( (double)range - fdist * 2.0) * sin(rad - orad) );
                apply_light_ray(lit, x, y, endx, endy , luminance, true);
            } else {
                calc_ray_end(nangle + ao, range, x, y, &endx, &endy);
                apply_light_ray(lit, x, y, endx, endy , luminance, false);
                calc_ray_end(nangle - ao, range, x, y, &endx, &endy);
                apply_light_ray(lit, x, y, endx, endy , luminance, false);
            }
        }
    }
}

void map::calc_ray_end(int angle, int range, int x, int y, int *outx, int *outy)
{
    const double PI = 3.14159265358979f;
    double rad = (PI * angle) / 180;
    if (trigdist) {
        *outx = x + range * cos(rad);
        *outy = y + range * sin(rad);
    } else {
        int mult = 0;
        if (angle >= 135 && angle <= 315) {
            mult = -1;
        } else {
            mult = 1;
        }

        if (angle <= 45 || (135 <= angle && angle <= 215) || 315 < angle) {
            *outx = x + range * mult;
            *outy = y + range * tan(rad) * mult;
        } else {
            *outx = x + range * 1 / tan(rad) * mult;
            *outy = y + range * mult;
        }
    }
}

void map::apply_light_ray(bool lit[LIGHTMAP_CACHE_X][LIGHTMAP_CACHE_Y],
                          int sx, int sy, int ex, int ey, float luminance, bool trig_brightcalc)
{
    int ax = abs(ex - sx) << 1;
    int ay = abs(ey - sy) << 1;
    int dx = (sx < ex) ? 1 : -1;
    int dy = (sy < ey) ? 1 : -1;
    int x = sx;
    int y = sy;

    if( sx == ex && sy == ey ) {
        return;
    }


    float transparency = LIGHT_TRANSPARENCY_CLEAR;
    float light = 0.0;
    int td = 0;
    // TODO: [lightmap] Pull out the common code here rather than duplication
    if (ax > ay) {
        int t = ay - (ax >> 1);
        do {
            if(t >= 0) {
                y += dy;
                t -= ax;
            }

            x += dx;
            t += ay;

            if (INBOUNDS(x, y)) {
                if (!lit[x][y]) {
                    // Multiple rays will pass through the same squares so we need to record that
                    lit[x][y] = true;

                    // We know x is the longest angle here and squares can ignore the abs calculation
                    light = 0.0;
                    if ( trig_brightcalc ) {
                        td = trig_dist(sx, sy, x, y);
                        light = luminance / ( td * td );
                    } else {
                        light = luminance / ((sx - x) * (sx - x));
                    }
                    lm[x][y] += light * transparency;
                }
                transparency *= light_transparency(x, y);
            }

            if (transparency <= LIGHT_TRANSPARENCY_SOLID) {
                break;
            }

        } while(!(x == ex && y == ey));
    } else {
        int t = ax - (ay >> 1);
        do {
            if(t >= 0) {
                x += dx;
                t -= ay;
            }

            y += dy;
            t += ax;

            if (INBOUNDS(x, y)) {
                if(!lit[x][y]) {
                    // Multiple rays will pass through the same squares so we need to record that
                    lit[x][y] = true;

                    // We know y is the longest angle here and squares can ignore the abs calculation
                    light = 0.0;
                    if ( trig_brightcalc ) {
                        td = trig_dist(sx, sy, x, y);
                        light = luminance / ( td * td );
                    } else {
                        light = luminance / ((sy - y) * (sy - y));
                    }
                    lm[x][y] += light * transparency;
                }
                transparency *= light_transparency(x, y);
            }

            if (transparency <= LIGHT_TRANSPARENCY_SOLID) {
                break;
            }

        } while(!(x == ex && y == ey));
    }
}
