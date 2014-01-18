#include "game.h"
#include "keypress.h"
#include "output.h"
#include "map.h"
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>
#include <iterator>
#include "catacharset.h"
#include "translations.h"
#include "uistate.h"
#include "helper.h"
#include "item_factory.h"
#include "auto_pickup.h"
#ifdef _MSC_VER
// MSVC doesn't have c99-compatible "snprintf", so do what picojson does and use _snprintf_s instead
#define snprintf _snprintf_s
#endif

#include "advanced_inv.h"

#define ADVINVOFS 7
// abstract of selected origin which can be inventory, or  map tile / vehicle storage / aggregate

// should probably move to an adv_inv_pane class
enum advanced_inv_sortby {
    SORTBY_NONE = 1, SORTBY_NAME, SORTBY_WEIGHT, SORTBY_VOLUME, SORTBY_CHARGES, SORTBY_CATEGORY, NUM_SORTBY
};

bool advanced_inventory::isDirectionalDragged(int area1, int area2) {

    if(!(area1 == isdrag || area2 == isdrag)) { return false; }
    // one of the areas is drag square
    advanced_inv_area other = (area1 == isdrag ? squares[area2] : squares[area1]);
    if(other.offx == p->grab_point.x && other.offy == p->grab_point.y) { return true; }
    return false;
}

int getsquare(int c, int &off_x, int &off_y, std::string &areastring, advanced_inv_area *squares) {
    int ret =- 1;
    if (!( c >= 0 && c <= 11 )) return ret;
    ret=c;
    off_x = squares[ret].offx;
    off_y = squares[ret].offy;
    areastring = squares[ret].name;
    return ret;
}

int getsquare(char c , int &off_x, int &off_y, std::string &areastring, advanced_inv_area *squares)
{
    int ret =- 1;
    switch(c) {
        case '0':
        case 'I':
            ret = 0;
            break;
        case '1':
        case 'B':
            ret = 1;
            break;
        case '2':
        case 'J':
            ret = 2;
            break;
        case '3':
        case 'N':
            ret = 3;
            break;
        case '4':
        case 'H':
            ret = 4;
            break;
        case '5':
        case 'G':
            ret = 5;
            break;
        case '6':
        case 'L':
            ret = 6;
            break;
        case '7':
        case 'Y':
            ret = 7;
            break;
        case '8':
        case 'K':
            ret = 8;
            break;
        case '9':
        case 'U':
            ret = 9;
            break;
        case 'a':
            ret = 10;
            break;
        case 'D':
            ret = 11;
            break;
        default :
            return -1;
    }
    return getsquare(ret, off_x, off_y, areastring, squares);
}

void advanced_inventory::print_items(advanced_inventory_pane &pane, bool active)
{
    std::vector<advanced_inv_listitem> &items = pane.items;
    WINDOW* window = pane.window;
    int page = pane.page;
    int selected_index = pane.index;
    bool isinventory = ( pane.area == 0 );
    bool isall = ( pane.area == 10 );
    int itemsPerPage;
    itemsPerPage = getmaxy( window ) - ADVINVOFS; // fixme
    int columns = getmaxx( window );
    int rightcol = columns - 8;
    int amount_column = columns - 15;
    nc_color norm = active ? c_white : c_dkgray;
    std::string spaces(getmaxx(window) - 4, ' ');
    bool compact = (TERMX <= 100);

    if(isinventory) {
        //right align
        int hrightcol = columns -
            helper::to_string(g->u.convert_weight(g->u.weight_carried())).length() - 3 - //"xxx.y/"
            helper::to_string(g->u.convert_weight(g->u.weight_capacity())).length() - 3 - //"xxx.y_"
            helper::to_string(g->u.volume_carried()).length() - 1 - //"xxx/"
            helper::to_string(g->u.volume_capacity() - 2).length() - 1;//"xxx|"
        nc_color color = c_ltgreen;//red color if overload
        if (g->u.weight_carried() > g->u.weight_capacity()) {
            color = c_red;
        }
        mvwprintz( window, 4, hrightcol, color, "%.1f", g->u.convert_weight(g->u.weight_carried()) );
        wprintz(window, c_ltgray, "/%.1f ", g->u.convert_weight(g->u.weight_capacity()) );
        if (g->u.volume_carried() > g->u.volume_capacity() - 2) {
            color = c_red;
        } else {
            color = c_ltgreen;
        }
        wprintz(window, color, "%d", g->u.volume_carried() );
        wprintz(window, c_ltgray, "/%d ", g->u.volume_capacity() - 2 );
    } else {
        int hrightcol=rightcol; // intentionally -not- shifting rightcol since heavy items are rare, and we're stingy on screenspace
        if (g->u.convert_weight(squares[pane.area].weight) > 9.9 ) {
          hrightcol--;
          if (g->u.convert_weight(squares[pane.area].weight) > 99.9 ) { // not uncommon
            hrightcol--;
            if (g->u.convert_weight(squares[pane.area].weight) > 999.9 ) {
              hrightcol--;
              if (g->u.convert_weight(squares[pane.area].weight) > 9999.9 ) { // hohum. time to consider tile destruction and sinkholes elsewhere?
                hrightcol--;
              }
            }
          }
        }
        if ( squares[pane.area].volume > 999 ) { // pile 'o dead bears
          hrightcol--;
          if ( squares[pane.area].volume > 9999 ) { // theoretical limit; 1024*9
            hrightcol--;
          }
        }

        mvwprintz( window, 4, hrightcol, norm, "%3.1f %3d", g->u.convert_weight(squares[pane.area].weight), squares[pane.area].volume);
    }

    mvwprintz( window, 5, ( compact ? 1 : 4 ), c_ltgray, _("Name (charges)") );
    if (isinventory) {
        //~ advanced inventory; "amount", "weight", "volume"; 14 letters
        mvwprintz( window, 5, rightcol - 7, c_ltgray, _("amt weight vol") );
    } else if (isall) {
        //~ advanced inventory; "source", "weight", "volume"; 14 letters
        mvwprintz( window, 5, rightcol - 7, c_ltgray, _("src weight vol") );
    } else {
        //~ advanced inventory; "weight", "volume"; 14 letters, right-aligned
        mvwprintz( window, 5, rightcol - 7, c_ltgray, _("    weight vol") );
    }

    for(int i = page * itemsPerPage , x = 0 ; i < items.size() && x < itemsPerPage ; i++ ,x++) {
      if ( items[i].volume == -8 ) { // I'm a header!
        mvwprintz(window,6+x,( columns - items[i].name.size()-6 )/2,c_cyan, "[%s]", items[i].name.c_str() );
      } else {
        nc_color thiscolor = active ? items[i].it->color(&g->u) : norm;
        nc_color thiscolordark = c_dkgray;

        if(active && selected_index == x)
        {
            thiscolor = (inCategoryMode && panes[src].sortby == SORTBY_CATEGORY) ? c_white_red : hilite(thiscolor);
            thiscolordark = hilite(thiscolordark);
            if ( compact ) {
                mvwprintz(window,6+x,1,thiscolor, "  %s", spaces.c_str());
            } else {
                mvwprintz(window,6+x,1,thiscolor, ">>%s", spaces.c_str());
            }

        }
        mvwprintz(window, 6 + x, ( compact ? 1 : 4 ), thiscolor, "%s", items[i].it->tname().c_str() );

        if(items[i].it->charges > 0) {
            wprintz(window, thiscolor, " (%d)",items[i].it->charges);
        } else if(items[i].it->contents.size() == 1 && items[i].it->contents[0].charges > 0) {
            wprintz(window, thiscolor, " (%d)",items[i].it->contents[0].charges);
        }

        if( isinventory && items[i].stacks > 1 ) {
            mvwprintz(window, 6 + x, amount_column, thiscolor, "x %d", items[i].stacks);
        } else if ( isall ) {
            mvwprintz(window, 6 + x, amount_column, thiscolor, "%s", squares[items[i].area].shortname.c_str());
        }
        int xrightcol=rightcol;
        if (g->u.convert_weight(items[i].weight) > 9.9 ) {
          xrightcol--;
          if (g->u.convert_weight(items[i].weight) > 99.9 ) {
            xrightcol--;
            if (g->u.convert_weight(items[i].weight) > 999.9 ) { // anything beyond this is excessive. Enjoy your clear plastic bottle of neutronium
              xrightcol--;
            }
          }
        }
        if ( items[i].volume > 999 ) { // does not exist, but can fit in 1024 tile limit
          xrightcol--;
          if ( items[i].volume > 9999 ) { // oh hey what about z levels. best give up now
            xrightcol--;
          }
        }
        mvwprintz(window, 6 + x, xrightcol, (g->u.convert_weight(items[i].weight) > 0 ? thiscolor : thiscolordark),
            "%3.1f", g->u.convert_weight(items[i].weight) );

        wprintz(window, (items[i].volume > 0 ? thiscolor : thiscolordark), " %3d", items[i].volume );
        if(active && items[i].autopickup==true) {
          mvwprintz(window,6+x,1, magenta_background(items[i].it->color(&g->u)),"%s",(compact?items[i].it->tname().substr(0,1):">").c_str());
        }
      }
    }
}

struct advanced_inv_sort_case_insensitive_less : public std::binary_function< char,char,bool > {
    bool operator () (char x, char y) const {
        return toupper( static_cast< unsigned char >(x)) < toupper( static_cast< unsigned char >(y));
    }
};

struct advanced_inv_sorter {
    int sortby;
    advanced_inv_sorter(int sort) { sortby=sort; };
    bool operator()(const advanced_inv_listitem& d1, const advanced_inv_listitem& d2) {
        if ( sortby != SORTBY_NAME ) {
            switch(sortby) {
                case SORTBY_WEIGHT: {
                    if ( d1.weight != d2.weight ) return d1.weight > d2.weight;
                    break;
                }
                case SORTBY_VOLUME: {
                    if ( d1.volume != d2.volume ) return d1.volume > d2.volume;
                    break;
                }
                case SORTBY_CHARGES: {
                    if ( d1.it->charges != d2.it->charges ) return d1.it->charges > d2.it->charges;
                    break;
                }
                case SORTBY_CATEGORY: {
                    if ( d1.cat != d2.cat ) {
                      return *d1.cat < *d2.cat;
                    } else if ( d1.volume == -8 ) {
                      return true;
                    } else if ( d2.volume == -8 ) {
                      return false;
                    }
                    break;
                }
                default: return d1.idx > d2.idx; break;
            };
        }
        // secondary sort by name
        std::string n1;
        std::string n2;
        if (d1.name_without_prefix == d2.name_without_prefix){//if names without prefix equal, compare full name
            n1 = d1.name;
            n2 = d2.name;
        } else {//else compare name without prefix
            n1 = d1.name_without_prefix;
            n2 = d2.name_without_prefix;
        }
        return std::lexicographical_compare( n1.begin(), n1.end(),
                n2.begin(), n2.end(), advanced_inv_sort_case_insensitive_less() );
    };
};

void advanced_inv_menu_square(advanced_inv_area* squares, uimenu *menu )
{
    int ofs=-25-4;
    int sel=menu->selected+1;
    for ( int i=1; i < 10; i++ ) {
        char key=(char)(i+48);
        char bracket[3]="[]";
        if ( squares[i].vstor >= 0 ) strcpy(bracket,"<>");
        bool canputitems=( squares[i].canputitems && menu->entries[i-1].enabled ? true : false);
        nc_color bcolor = ( canputitems ? ( sel == i ? h_cyan : c_cyan ) : c_dkgray );
        nc_color kcolor = ( canputitems ? ( sel == i ? h_ltgreen : c_ltgreen ) : c_dkgray );
        mvwprintz(menu->window,squares[i].hscreenx+5,squares[i].hscreeny+ofs, bcolor, "%c", bracket[0]);
        wprintz(menu->window, kcolor, "%c", key);
        wprintz(menu->window, bcolor, "%c", bracket[1]);
    }
}

void advanced_inv_print_header(advanced_inv_area* squares, advanced_inventory_pane &pane, int sel=-1 )
{
    WINDOW* window=pane.window;
    int area=pane.area;
    int wwidth=getmaxx(window);
    int ofs=wwidth-25-2-14;
    for ( int i=0; i < 12; i++ ) {
        char key=( i == 0 ? 'I' : ( i == 10 ? 'A' : ( i == 11 ? 'D' : (char)(i+48) ) ) );
        char bracket[3]="[]";
        if ( squares[i].vstor >= 0 ) strcpy(bracket,"<>");
        nc_color bcolor = ( squares[i].canputitems ? ( area == i || ( area == 10 && i != 0 ) ? c_cyan : c_ltgray ) : c_red );
        nc_color kcolor = ( squares[i].canputitems ? ( area == i ? c_ltgreen : ( i == sel ? c_cyan : c_ltgray ) ) : c_red );
        mvwprintz(window,squares[i].hscreenx,squares[i].hscreeny+ofs, bcolor, "%c", bracket[0]);
        wprintz(window, kcolor, "%c", key);
        wprintz(window, bcolor, "%c", bracket[1]);
    }
}

void advanced_inv_update_area( advanced_inv_area &area )
{
    int i = area.id;
    player u = g->u;
    area.x = g->u.posx+area.offx;
    area.y = g->u.posy+area.offy;
    area.size = 0;
    area.veh = NULL;
    area.vstor = -1;
    area.desc = "";
    if( i > 0 && i < 10 ) {
        int vp = 0;
        area.veh = g->m.veh_at( u.posx+area.offx,u.posy+area.offy, vp );
        if ( area.veh ) {
            area.vstor = area.veh->part_with_feature(vp, "CARGO", false);
        }
        if ( area.vstor >= 0 ) {
            area.desc = area.veh->name;
            area.canputitems=true;
            area.size = area.veh->parts[area.vstor].items.size();
            area.max_size = MAX_ITEM_IN_VEHICLE_STORAGE;
            area.max_volume = area.veh->max_volume(area.vstor);
        } else {
            area.canputitems = g->m.can_put_items(u.posx+area.offx, u.posy+area.offy);
            area.size = g->m.i_at(u.posx+area.offx,u.posy+area.offy).size();
            area.max_size = MAX_ITEM_IN_SQUARE;
            area.max_volume = g->m.max_volume(u.posx+area.offx,u.posy+area.offy);
            if (g->m.graffiti_at(u.posx+area.offx,u.posy+area.offy).contents) {
                area.desc = g->m.graffiti_at(u.posx+area.offx,u.posy+area.offy).contents->c_str();
            }
        }
    } else if ( i == 0 ) {
        area.size=u.inv.size();
        area.canputitems=true;
    } else if (i == 11 ) {
        int vp = 0;
        area.veh = g->m.veh_at( u.posx+u.grab_point.x, u.posy+u.grab_point.y, vp);
        if( area.veh ) {
            area.vstor = area.veh->part_with_feature(vp, "CARGO", false);
        }
        if( area.vstor >= 0 ) {
            area.desc = area.veh->name;
            area.canputitems = true;
            area.size = area.veh->parts[area.vstor].items.size();
            area.max_size = MAX_ITEM_IN_VEHICLE_STORAGE;
            area.max_volume = area.veh->max_volume(area.vstor);
        } else {
            area.canputitems = false;
            area.desc = _("No dragged vehicle");
        }
    } else {
        area.desc = _("All 9 squares");
        area.canputitems=true;
    }
    area.volume=0; // must update in main function
    area.weight=0; // must update in main function
}

std::string center_text(const char *str, int width)
{
    std::string spaces;
    int numSpaces = width - strlen(str);
    for (int i = 0; i < numSpaces / 2; i++) {
        spaces += " ";
    }
    return spaces + std::string(str);
}

void advanced_inventory::init(player *pp)
{
    this->p = pp;

    advanced_inv_area initsquares[12] = {
        {0, 2, 25, 0, 0, 0, 0, _("Inventory"), "IN", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {1, 3, 30, -1, 1, 0, 0, _("South West"), "SW", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {2, 3, 33, 0, 1, 0, 0, _("South"), "S", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {3, 3, 36, 1, 1, 0, 0, _("South East"), "SE", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {4, 2, 30, -1, 0, 0, 0, _("West"), "W", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {5, 2, 33, 0, 0, 0, 0, _("Directly below you"), "DN", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {6, 2, 36, 1, 0, 0, 0, _("East"), "E", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {7, 1, 30, -1, -1, 0, 0, _("North West"), "NW", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {8, 1, 33, 0, -1, 0, 0, _("North"), "N", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {9, 1, 36, 1, -1, 0, 0, _("North East"), "NE", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {10, 3, 25, 0, 0, 0, 0, _("Surrounding area"), "AL", false, NULL, -1, 0, "", 0, 0, 0, 0 },
        {11, 1, 25, 0, 0, 0, 0, _("Grabbed Vehicle"), "GR", false, NULL, -1, 0, "", 0, 0, 0, 0 }
    };
    for ( int i = 0; i < 12; i++ ) {
        squares[i] = initsquares[i];
        advanced_inv_update_area(squares[i]);
    }

    panes[left].pos = 0;
    panes[left].area = 10;
    panes[right].pos = 1;
    panes[right].area = isinventory;

    panes[left].sortby = uistate.adv_inv_leftsort;
    panes[right].sortby = uistate.adv_inv_rightsort;
    panes[left].area = uistate.adv_inv_leftarea;
    panes[right].area = uistate.adv_inv_rightarea;
    bool moved = ( uistate.adv_inv_last_coords.x != p->posx || uistate.adv_inv_last_coords.y != p->posy );
    if ( !moved || panes[left].area == isinventory ) {
        panes[left].index = uistate.adv_inv_leftindex;
        panes[left].page = uistate.adv_inv_leftpage;
    }
    if ( !moved || panes[right].area == isinventory ) {
        panes[right].index = uistate.adv_inv_rightindex;
        panes[right].page = uistate.adv_inv_rightpage;
    }

    panes[left].filter = uistate.adv_inv_leftfilter;
    panes[right].filter = uistate.adv_inv_rightfilter;


    checkshowmsg = false;
    showmsg = false;

    itemsPerPage = 10;
    w_height = (TERMY < min_w_height + head_height) ? min_w_height : TERMY - head_height;
    w_width = (TERMX < min_w_width) ? min_w_width : (TERMX > max_w_width) ? max_w_width : (int)TERMX;

    headstart = 0; //(TERMY>w_height)?(TERMY-w_height)/2:0;
    colstart = (TERMX > w_width) ? (TERMX - w_width) / 2 : 0;

    // todo: awaiting ui::menu // last_tmpdest=-1;
    exit = false;
    redraw = true;
    recalc = true;
    lastCh = 0;

    src = left; // the active screen , 0 for left , 1 for right.
    dest = right;
    examineScroll = false;
    filter_edit = false;
}

bool cached_lcmatch(const std::string &str, const std::string &findstr, std::map<std::string, bool> & filtercache)
{
    if ( filtercache.find(str) == filtercache.end() ) {
        std::string ret = "";
        ret.reserve( str.size() );
        transform( str.begin(), str.end(), std::back_inserter(ret), tolower );
        bool ismatch = ( ret.find( findstr ) != -1 );
        filtercache[ str ] = ismatch;
        return ismatch;
    } else {
        return filtercache[ str ];
    }
}


void advanced_inventory::recalc_pane(int i)
{
    panes[i].recalc = false;
    bool filtering = ( panes[i].filter.size() > 0 );
    player &u = *p;
    map &m = g->m;
    int idest = (i == left ? right : left);
    panes[i].items.clear();
    std::set<std::string> has_category;
    panes[i].numcats = 0;
    int avolume = 0;
    int aweight = 0;

    if(panes[i].area == isinventory) {
       const invslice & stacks = u.inv.slice();
        for (unsigned x = 0; x < stacks.size(); ++x ) {
            item &item = stacks[x]->front();
            advanced_inv_listitem it;
            it.name = item.tname();
            it.name_without_prefix = item.tname( false );
            if ( filtering && ! cached_lcmatch(it.name, panes[i].filter, panes[i].filtercache ) ) {
                continue;
            }
            it.idx = x;
            int size = u.inv.const_stack(x).size();
            if ( size < 1 ) {
                size = 1;
            }
            it.autopickup = hasPickupRule(it.name);
            it.stacks = size;
            it.weight = item.weight() * size;
            it.volume = item.volume() * size;
            it.cat = &(item.get_category());
            it.it = &item;
            it.area = panes[i].area;
            if( has_category.count(it.cat->id) == 0 ) {
                has_category.insert(it.cat->id);
                panes[i].numcats++;
                if(panes[i].sortby == SORTBY_CATEGORY) {
                    advanced_inv_listitem itc;
                    itc.idx = -8;
                    itc.stacks = -8;
                    itc.weight = -8;
                    itc.volume = -8;
                    itc.cat = it.cat;
                    itc.name = it.cat->name;
                    itc.area = panes[i].area;
                    panes[i].items.push_back(itc);
                }
            }
            avolume += it.volume;
            aweight += it.weight;
            panes[i].items.push_back(it);
        }
    } else {
        int s1 = panes[i].area;
        int s2 = panes[i].area;
        if ( panes[i].area == isall ) {
            s1 = 1;
            s2 = 9;
        }
        for(int s = s1; s <= s2; s++) {
            int savolume = 0;
            int saweight = 0;
            advanced_inv_update_area(squares[s]);

            if( panes[idest].area != s && squares[s].canputitems && !isDirectionalDragged(s, panes[idest].area)) {
                std::vector<item>& items = squares[s].vstor >= 0 ?
                                           squares[s].veh->parts[squares[s].vstor].items :
                                           m.i_at(squares[s].x , squares[s].y );
                for (unsigned x = 0; x < items.size(); x++) {
                    advanced_inv_listitem it;
                    it.idx = x;
                    it.name = items[x].tname();
                    it.name_without_prefix = items[x].tname( false );
                    if ( filtering && ! cached_lcmatch(it.name, panes[i].filter, panes[i].filtercache ) ) {
                        continue;
                    }

                    it.autopickup = hasPickupRule(it.name);
                    it.stacks = 1;
                    it.weight = items[x].weight();
                    it.volume = items[x].volume();
                    it.cat = &(items[x].get_category());
                    it.it = &items[x];
                    it.area = s;
                    if( has_category.count(it.cat->id) == 0 ) {
                        has_category.insert(it.cat->id);
                        panes[i].numcats++;
                        if(panes[i].sortby == SORTBY_CATEGORY) {
                            advanced_inv_listitem itc;
                            itc.idx = -8;
                            itc.stacks = -8;
                            itc.weight = -8;
                            itc.volume = -8;
                            itc.cat = it.cat;
                            itc.name = it.cat->name;
                            itc.area = s;
                            panes[i].items.push_back(itc);
                        }
                    }

                    savolume += it.volume;
                    saweight += it.weight;
                    panes[i].items.push_back(it);

                } // for(int x = 0; x < items.size() ; x++)

            } // if( panes[idest].area != s && squares[s].canputitems )
            avolume += savolume;
            aweight += saweight;
        } // for(int s = s1; s <= s2; s++)

    } // if(panes[i].area ?? isinventory)

    advanced_inv_update_area(squares[panes[i].area]);

    squares[panes[i].area].volume = avolume;
    squares[panes[i].area].weight = aweight;

    panes[i].veh = squares[panes[i].area].veh; // <--v-- todo deprecate
    panes[i].vstor = squares[panes[i].area].vstor;
    panes[i].size = panes[i].items.size();

    // sort the stuff
    switch(panes[i].sortby) {
        case SORTBY_NONE:
            if ( i != isinventory ) {
                std::sort( panes[i].items.begin(), panes[i].items.end(), advanced_inv_sorter(SORTBY_NONE) );
            }
            break;
        default:
            std::sort( panes[i].items.begin(), panes[i].items.end(), advanced_inv_sorter( panes[i].sortby ) );
            break;
    }


}

void advanced_inventory::redraw_pane( int i )
{
    std::string sortnames[8] = { "-none-", _("none"), _("name"), _("weight"), _("volume"), _("charges"), _("category"), "-" };
    // calculate the offset.
    getsquare(panes[i].area, panes[i].offx, panes[i].offy, panes[i].area_string, squares);

    if( recalc || panes[i].recalc == true ) {
        recalc_pane(i);
    }
    panes[i].redraw = false;
    // paginate (not sure why)
    panes[i].max_page = (int)ceil(panes[i].size / (itemsPerPage + 0.0)); //(int)ceil(panes[i].size/20.0);
    panes[i].max_index = panes[i].page == (-1 + panes[i].max_page) ? ((panes[i].size % itemsPerPage) == 0 ? itemsPerPage : panes[i].size % itemsPerPage) : itemsPerPage;
    // check if things are out of bound
    panes[i].index = (panes[i].index >= panes[i].max_index) ? panes[i].max_index - 1 : panes[i].index;


    panes[i].page = panes[i].max_page == 0 ? 0 : ( panes[i].page >= panes[i].max_page ? panes[i].max_page - 1 : panes[i].page);

    if( panes[i].sortby == SORTBY_CATEGORY && panes[i].items.size() > 0 ) {
        int lpos = panes[i].index + (panes[i].page * itemsPerPage);
        if ( lpos < panes[i].items.size() && panes[i].items[lpos].volume == -8 ) {
            panes[i].index += ( panes[i].index + 1 >= itemsPerPage ? -1 : 1 );
        }
    }
    // draw the stuff
    werase(panes[i].window);

    print_items( panes[i], (src == i) );

    int sel = -1;
    if ( panes[i].size > 0 ) {
        sel = panes[i].items[panes[i].index].area;
    }

    advanced_inv_print_header(squares, panes[i], sel );
    // todo move --v to --^
    mvwprintz(panes[i].window, 1, 2, src == i ? c_cyan : c_ltgray, "%s", panes[i].area_string.c_str());
    mvwprintz(panes[i].window, 2, 2, src == i ? c_green : c_dkgray , "%s", squares[panes[i].area].desc.c_str() );

    if ( i == src ) {
        if(panes[src].max_page > 1 ) {
            mvwprintz(panes[src].window, 4, 2, c_ltblue, _("[<] page %d of %d [>]"), panes[src].page + 1, panes[src].max_page);
        }
    }
    ////////////
    if ( src == i ) {
        wattron(panes[i].window, c_cyan);
    }
    wborder(panes[i].window, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
    mvwprintw(panes[i].window, 0, 3, _("< [s]ort: %s >"), sortnames[ ( panes[i].sortby <= 6 ? panes[i].sortby : 0 ) ].c_str() );
    int max = MAX_ITEM_IN_SQUARE;
    if ( panes[i].area == isall ) {
        max *= 9;
    }
    int fmtw = 7 + ( panes[i].size > 99 ? 3 : panes[i].size > 9 ? 2 : 1 ) + ( max > 99 ? 3 : max > 9 ? 2 : 1 );
    mvwprintw(panes[i].window, 0 , (w_width / 2) - fmtw, "< %d/%d >", panes[i].size, max );
    const char * fprefix = _("[F]ilter");
    if ( ! filter_edit ) {
        if ( panes[i].filter.size() > 0 ) {
            mvwprintw(panes[i].window, getmaxy(panes[i].window) - 1, 2, "< %s: %s >", fprefix, panes[i].filter.c_str() );
        } else {
            mvwprintw(panes[i].window, getmaxy(panes[i].window) - 1, 2, "< %s >", fprefix );
        }
    }
    if ( src == i ) {
        wattroff(panes[i].window, c_white);
    }
    if ( ! filter_edit && panes[i].filter.size() > 0 ) {
        mvwprintz(panes[i].window, getmaxy(panes[i].window) - 1, 6 + strlen(fprefix), c_white, "%s", panes[i].filter.c_str() );
    }

}


void advanced_inventory::display(player * pp)
{
    init(pp);

    player & u=*p;
    map & m = g->m;

    u.inv.sort();
    u.inv.restack((&g->u));

    std::string sortnames[8] = { "-none-", _("none"), _("name"), _("weight"), _("volume"), _("charges"), _("category"), "-" };

    WINDOW *head = newwin(head_height,w_width, headstart, colstart);
    WINDOW *left_window = newwin(w_height,w_width/2, headstart+head_height,colstart);
    WINDOW *right_window = newwin(w_height,w_width/2, headstart+head_height,colstart+w_width/2);

    itemsPerPage=getmaxy(left_window)-ADVINVOFS;

    panes[left].window = left_window;
    panes[right].window = right_window;

    inCategoryMode = false;

    std::vector<int> category_index_start;
    category_index_start.reserve(NUM_SORTBY);

    while(!exit) {
        dest = (src == left ? right : left);
        // recalc and redraw
        if ( recalc ) redraw = true;
        for (int i = 0; i < 2; i++) {
            if ( panes[i].recalc ) panes[i].redraw = true;
        }

        if ( recalc ) redraw = true; // global recalc = global redraw

        // any redraw = redraw everything except opposite
        if(redraw || panes[0].redraw || panes[1].redraw ) {
            for (int i = 0; i < 2; i++) {
                if ( redraw || panes[i].redraw ) {
                    redraw_pane( i );
                    wrefresh(panes[i].window);
                }
            }
            recalc = false;
            if (redraw) {
                werase(head);
                draw_border(head);
                int line = 1;
                if( checkshowmsg || showmsg ) {
                    for (int i = g->messages.size() - 1; i >= 0 && line < 4; i--) {
                        std::string mes = g->messages[i].message;
                        if (g->messages[i].count > 1) {
                            std::stringstream mesSS;
                            mesSS << mes << " x " << g->messages[i].count;
                            mes = mesSS.str();
                        }
                        nc_color col = c_dkgray;
                        if (int(g->messages[i].turn) >= g->curmes) {
                            col = c_ltred;
                            showmsg=true;
                        } else {
                            col = c_ltgray;
                        }
                        if ( showmsg ) {
                            mvwprintz(head, line, 2, col, mes.c_str());
                        }
                        line++;
                    }
                }
                if ( ! showmsg ) {
                    mvwprintz(head, 0, w_width - utf8_width(_("< [?] show log >")) - 1,
                              c_white, _("< [?] show log >"));
                    mvwprintz(head, 1, 2, c_white,
                              _("hjkl or arrow keys to move cursor, [m]ove item between panes ([M]: all)"));
                    mvwprintz(head, 2, 2, c_white,
                              _("1-9 (or GHJKLYUBNID) to select square for active tab, 0 for inventory, D for dragged item"));
                    mvwprintz(head, 3, 2, c_white,
                              _("[e]xamine item, [s]ort display, toggle auto[p]ickup, [q]uit."));
                    if (panes[src].sortby == SORTBY_CATEGORY) {
                        nc_color highlight_color = inCategoryMode ? c_white_red : h_ltgray;
                        mvwprintz(head, 3, 3 + utf8_width(_("[e]xamine item, [s]ort display, toggle auto[p]ickup, [q]uit.")),
                                  highlight_color, _("[space] toggles selection modes"));
                    }
                } else {
                    mvwprintz(head, 0, w_width - utf8_width(_("< [?] show help >")) - 1,
                              c_white, _("< [?] show help >"));
                }
                wrefresh(head);
            }
            redraw = false;
        }

        int list_pos = panes[src].index + (panes[src].page * itemsPerPage);
        int item_pos = panes[src].size > 0 ? panes[src].items[list_pos].idx : 0;

        int changex = -1;
        int changey = 0;
        bool donothing = false;

        int c = lastCh ? lastCh : getch();
        lastCh = 0;
        int changeSquare;

        if(c == 'i') {
            c = (char)'0';
        }

        if(c == 'a') {
            c = (char)'a';
        }

        if(c == 'D') {
            c = (char)'D';
        }

        changeSquare = getsquare((char)c, panes[src].offx, panes[src].offy,
                                 panes[src].area_string, squares);

        category_index_start.clear();

        // Finds the index of the first item in each category.
        for (int current_item_index = 0; current_item_index < panes[src].items.size();
             ++current_item_index) {
             // Found a category header.
            if (panes[src].items[current_item_index].volume == -8) {
                category_index_start.push_back(current_item_index + 1);
            }
        }

        if (' ' == c) {
            inCategoryMode = !inCategoryMode;
            redraw = true; // We redraw to force the color change of the highlighted line and header text.
        }
        else if(changeSquare != -1) {
             // do nthing
            if(panes[left].area == changeSquare || panes[right].area == changeSquare ||
               isDirectionalDragged(panes[left].area, changeSquare) ||
               isDirectionalDragged(panes[right].area, changeSquare)) {
                lastCh = (int)popup_getkey(_("same square!"));
                if(lastCh == 'q' || lastCh == KEY_ESCAPE || lastCh == ' ' ) lastCh = 0;
            } else if(squares[changeSquare].canputitems) {
                panes[src].area = changeSquare;
                panes[src].page = 0;
                panes[src].index = 0;
            } else {
                popup(_("You can't put items there"));
            }
            recalc = true;
        } else if('m' == c || 'M' == c || '\n' == c || 'p' == c ) {
            // If the active screen has no item.
            if( panes[src].size == 0 ) {
                continue;
            } else if ( item_pos == -8 ) {
                continue; // category header
            }
            bool moveall = ('M' == c || '\n' == c );
            int destarea = panes[dest].area;
            if ( panes[dest].area == isall || 'p' == c ) {
                bool valid=false;
                uimenu m; /* using new uimenu class */
                m.text=_("Select destination");
                m.pad_left=9; /* free space for advanced_inv_menu_square */

                for(int i=1; i < 10; i++) {
                    std::string prefix = string_format("%2d/%d",
                            squares[i].size, MAX_ITEM_IN_SQUARE);
                    if (squares[i].size >= MAX_ITEM_IN_SQUARE) {
                        prefix += _(" (FULL)");
                    }
                    m.entries.push_back( uimenu_entry( /* std::vector<uimenu_entry> */
                        i, /* return value */
                        (squares[i].canputitems && i != panes[src].area), /* enabled */
                        i+48, /* hotkey */
                        prefix + " " +
                          squares[i].name + " " +
                          ( squares[i].vstor >= 0 ? squares[i].veh->name : "" ) /* entry text */
                    ) );
                }

                m.selected=uistate.adv_inv_last_popup_dest-1; // selected keyed to uimenu.entries, which starts at 0;
                m.show(); // generate and show window.
                while ( m.ret == UIMENU_INVALID && m.keypress != 'q' && m.keypress != KEY_ESCAPE ) {
                    advanced_inv_menu_square(squares, &m ); // render a fancy ascii grid at the left of the menu
                    m.query(false); // query, but don't loop
                }
                if ( m.ret >= 0 && m.ret <= 9 ) { // is it a square?
                    if ( m.ret == panes[src].area ) { // should never happen, but sanity checks keep developers sane.
                        popup(_("Can't move stuff to the same place."));
                    } else if ( ! squares[m.ret].canputitems ) { // this was also disabled in it's uimenu_entry
                        popup(_("Invalid. Like the menu said."));
                    } else {
                        destarea = m.ret;
                        valid=true;
                        uistate.adv_inv_last_popup_dest=m.ret;
                    }
                }
                if ( ! valid ) continue;
            }
            // from inventory
            if(panes[src].area == isinventory) {
                int max = (squares[destarea].max_size - squares[destarea].size);
                int free_volume = 1000 * ( squares[ destarea ].vstor >= 0 ?
                                           squares[ destarea ].veh->free_volume( squares[ destarea ].vstor ) :
                                           m.free_volume ( squares[ destarea ].x, squares[ destarea ].y ) );
                const std::list<item>& stack = u.inv.const_stack(item_pos);
                const item* it = &stack.front();

                int amount = 1;
                int volume = it->precise_unit_volume();
                bool askamount = false;
                if ( stack.size() > 1) {
                    amount = stack.size();
                    askamount = true;
                } else if ( it->count_by_charges() ) {
                    amount = it->charges;
                    askamount = true;
                }

                if ( volume > 0 && volume * amount > free_volume ) {
                    int volmax = int( free_volume / volume );
                    if ( volmax == 0 ) {
                        popup(_("Destination area is full. Remove some items first."));
                        continue;
                    }
                    if ( stack.size() > 1) {
                        max = ( volmax < max ? volmax : max );
                    } else if ( it->count_by_charges() ) {
                        max = volmax;
                    }
                } else if ( it->count_by_charges() ) {
                    max = amount;
                }
                if ( max == 0 ) {
                    popup(_("Destination area has too many items. Remove some first."));
                    continue;
                }
                if ( askamount && ( amount > max || !moveall ) ) {
                    std::string popupmsg=_("How many do you want to move? (0 to cancel)");
                    if(amount > max) {
                        popupmsg=string_format(_("Destination can only hold %d! Move how many? (0 to cancel) "), max);
                    }
                    // fixme / todo make popup take numbers only (m = accept, q = cancel)
                    amount = helper::to_int(
                        string_input_popup( popupmsg, 20,
                             helper::to_string(
                                 ( amount > max ? max : amount )
                             ), "", "", -1, true//input only digits
                        )
                    );
                }
                recalc=true;
                if(stack.size() > 1) { // if the item is stacked
                    if ( amount != 0 && amount <= stack.size() ) {
                        amount = amount > max ? max : amount;
                        std::list<item> moving_items = u.inv.reduce_stack(item_pos, amount);
                        bool chargeback=false;
                        int moved=0;
                        for(std::list<item>::iterator iter = moving_items.begin();
                            iter != moving_items.end();
                            ++iter)
                        {
                          if ( chargeback == true ) {
                                u.i_add(*iter);
                          } else {
                            if(squares[destarea].vstor >= 0) {
                                if(squares[destarea].veh->add_item(squares[destarea].vstor,*iter) == false) {
                                    // testme
                                    u.i_add(*iter);
                                    popup(_("Destination full. %d / %d moved. Please report a bug if items have vanished."),moved,amount);
                                    chargeback=true;
                                }
                            } else {
                                if(m.add_item_or_charges(squares[destarea].x, squares[destarea].y, *iter, 0) == false) {
                                    // testme
                                    u.i_add(*iter);
                                    popup(_("Destination full. %d / %d moved. Please report a bug if items have vanished."),moved,amount);
                                    chargeback=true;
                                }
                            }
                            moved++;
                          }
                        }
                        if ( moved != 0 ) u.moves -= 100;
                    }
                } else if(it->count_by_charges()) {
                    if(amount != 0 && amount <= it->charges ) {
                        item moving_item = u.inv.reduce_charges(item_pos, amount);
                        if (squares[destarea].vstor>=0) {
                            if(squares[destarea].veh->add_item(squares[destarea].vstor,moving_item) == false) {
                                // fixme add item back
                                u.i_add(moving_item);
                                popup(_("Destination full. Please report a bug if items have vanished."));
                                continue;
                            }
                        } else {
                            if ( m.add_item_or_charges(squares[destarea].x, squares[destarea].y, moving_item, 0) == false ) {
                                // fixme add item back
                                u.i_add(moving_item);
                                popup(_("Destination full. Please report a bug if items have vanished."));
                                continue;
                            }
                        }
                        u.moves -= 100;
                    }
                } else {
                    item moving_item = u.inv.remove_item(item_pos);
                    if(squares[destarea].vstor>=0) {
                        if(squares[destarea].veh->add_item(squares[destarea].vstor, moving_item) == false) {
                           // fixme add item back (test)
                           u.i_add(moving_item);
                           popup(_("Destination full. Please report a bug if items have vanished."));
                           continue;
                        }
                    } else {
                        if(m.add_item_or_charges(squares[destarea].x, squares[destarea].y, moving_item) == false) {
                           // fixme add item back (test)
                           u.i_add(moving_item);
                           popup(_("Destination full. Please report a bug if items have vanished."));
                           continue;
                        }
                    }
                    u.moves -= 100;
                }
// from map / vstor
            } else {
                int s;
                if(panes[src].area == isall) {
                    s = panes[src].items[list_pos].area;
                    // todo: phase out these vars? ---v // temp_fudge pending tests/cleanup
                    panes[src].offx = squares[s].offx;
                    panes[src].offy = squares[s].offy;
                    panes[src].vstor = squares[s].vstor;
                    panes[src].veh = squares[s].veh;
                    recalc = true;
                } else {
                    s = panes[src].area;
                }
                if ( s == destarea ) {
                    popup(_("Source area is the same as destination (%s)."),squares[destarea].name.c_str());
                    continue;
                }
                item *it = panes[src].items[list_pos].it;

/*                std::vector<item> src_items = squares[s].vstor >= 0 ?
                    squares[s].veh->parts[squares[s].vstor].items :
                    m.i_at(squares[s].x,squares[s].y);
                if(src_items[item_pos].made_of(LIQUID))
*/
                if ( it->made_of(LIQUID) )
                {
                    popup(_("You can't pick up a liquid."));
                    continue;
                }
                else // from veh/map
                {
                    int trycharges = -1;
                    if ( destarea == isinventory ) // if destination is inventory
                    {
                        if(squares[destarea].size >= MAX_ITEM_IN_SQUARE) {
                            popup(_("Too many items."));
                            continue;
                        }
                        int tryvolume = it->volume();
                        int tryweight = it->weight();
                        int amount = 1;
                        if ( it->count_by_charges() && it->charges > 1 ) {
                           amount = it->charges;
                           int unitvolume = it->precise_unit_volume();
                           int unitweight = ( tryweight * 1000 ) / it->charges;
                           int max_vol = u.volume_capacity() - u.volume_carried();
                           int max_weight = ( u.weight_capacity() * 4 ) - u.weight_carried();
                           max_vol *= 1000;
                           max_weight *= 1000;
                           int max = amount;
                           if ( unitvolume > 0 && unitvolume * amount > max_vol ) {
                              max = int( max_vol / unitvolume );
                           }
                           if ( unitweight > 0 && unitweight * amount > max_weight ) {
                              max = int( max_weight / unitweight );
                           }
                           // popup("uvol: %d amt: %d mvol: %d mamt: %d", unitvolume, amount, max_vol, max);
                           if ( max != 0 ) {
                                std::string popupmsg=_("How many do you want to move? (0 to cancel)");
                                if(amount > max) {
                                    popupmsg=string_format(_("Destination can only hold %d! Move how many? (0 to cancel) "), max);
                                    moveall = false;
                                }
                                // fixme / todo make popup take numbers only (m = accept, q = cancel)
                                if ( !moveall ) {
                                    amount = helper::to_int(
                                        string_input_popup( popupmsg, 20,
                                             helper::to_string(
                                                 ( amount > max ? max : amount )
                                             ), "", "", -1, true//input only digits
                                        )
                                    );
                                    if ( amount > max ) amount = max;
                                } else {
                                    amount = max;
                                }
                                if ( amount != it->charges ) {
                                    tryvolume = ( unitvolume * amount ) / 1000;
                                    tryweight = ( unitweight * amount ) / 1000;
                                    trycharges = amount;
                                }
                                if ( trycharges == 0 ) continue;
                            } else {
                                continue;
                            }
                        }
                        // ...not even going to think about checking for stack
                        // at this time...
                        if(!u.can_pickVolume(tryvolume))
                        {
                            popup(_("There's no room in your inventory."));
                            continue;
                        }
                        else if(!u.can_pickWeight(tryweight, false))
                        {
                            popup(_("This is too heavy!"));
                            continue;
                        }
                    }

                    recalc=true;

                    item new_item = (*it);

                    if ( trycharges > 0 ) {
                        new_item.charges = trycharges;
                    }
                    if(destarea == isinventory) {
                        u.inv.assign_empty_invlet(new_item);
                        u.i_add(new_item);
                        u.moves -= 100;
                    } else if (squares[destarea].vstor >= 0) {
                        if( squares[destarea].veh->add_item( squares[destarea].vstor, new_item ) == false) {
                            popup(_("Destination area is full. Remove some items first"));
                            continue;
                        }
                    } else {
                        if ( m.add_item_or_charges(squares[destarea].x, squares[destarea].y, new_item, 0 ) == false ) {
                            popup(_("Destination area is full. Remove some items first"));
                            continue;
                        }
                    }
                    if( trycharges > 0 ) {
                        it->charges -= trycharges;
                    } else {
                        if(panes[src].vstor>=0) {
                            panes[src].veh->remove_item (panes[src].vstor, item_pos);
                        } else {
                            m.i_rem(u.posx+panes[src].offx,u.posy+panes[src].offy, item_pos);
                        }
                    }
                }
            }
        } else if('?' == c) {
            showmsg=(!showmsg);
            checkshowmsg=false;
            redraw=true;
        } else if('s' == c) {
            redraw=true;
            uimenu sm; /* using new uimenu class */
            sm.text=_("Sort by... ");
            sm.entries.push_back(uimenu_entry(SORTBY_NONE, true, 'u', _("Unsorted (recently added first)") ));
            sm.entries.push_back(uimenu_entry(SORTBY_NAME, true, 'n', sortnames[SORTBY_NAME]));
            sm.entries.push_back(uimenu_entry(SORTBY_WEIGHT, true, 'w', sortnames[SORTBY_WEIGHT]));
            sm.entries.push_back(uimenu_entry(SORTBY_VOLUME, true, 'v', sortnames[SORTBY_VOLUME]));
            sm.entries.push_back(uimenu_entry(SORTBY_CHARGES, true, 'x', sortnames[SORTBY_CHARGES]));
            sm.entries.push_back(uimenu_entry(SORTBY_CATEGORY, true, 'c', sortnames[SORTBY_CATEGORY]));
            sm.selected=panes[src].sortby-1; /* pre-select current sort. uimenu.selected is entries[index] (starting at 0), not return value */
            sm.query(); /* calculate key and window variables, generate window, and loop until we get a valid answer */
            if(sm.ret < 1) continue; /* didn't get a valid answer =[ */
            panes[src].sortby = sm.ret;

            if ( src == left ) {
                uistate.adv_inv_leftsort=sm.ret;
            } else {
                uistate.adv_inv_rightsort=sm.ret;
            }
            recalc = true;
        } else if( 'f' == c || '.' == c || '/' == c) {
            long key = 0;
            int spos = -1;
            std::string filter=panes[src].filter;
            filter_edit = true;

            do {
                mvwprintz(panes[src].window, getmaxy(panes[src].window) - 1, 2, c_cyan, "< ");
                mvwprintz(panes[src].window, getmaxy(panes[src].window) - 1, (w_width/2) - 3, c_cyan, " >");
                filter=string_input_win(panes[src].window, panes[src].filter, 256, 4, w_height - 1, (w_width/2) - 4, false, key, spos, "", 4, getmaxy(panes[src].window) - 1 );
                if ( filter != panes[src].filter ) {
                    panes[src].filtercache.clear();
                    panes[src].filter=filter;
                    recalc_pane(src);
                    redraw_pane(src);
                }
            } while(key != '\n' && key != KEY_ESCAPE);
            filter_edit = false;
            redraw = true;
        } else if('p' == c) {
            if(panes[src].size == 0) {
                continue;
            } else if ( item_pos == -8 ) {
                continue; // category header
            }
            if ( panes[src].items[list_pos].autopickup == true ) {
                removePickupRule(panes[src].items[list_pos].name);
                panes[src].items[list_pos].autopickup=false;
            } else {
                addPickupRule(panes[src].items[list_pos].name);
                panes[src].items[list_pos].autopickup=true;
            }
            redraw = true;
        } else if('e' == c) {
            if(panes[src].size == 0) {
                continue;
            } else if ( item_pos == -8 ) {
                continue; // category header
            }
            item *it = panes[src].items[list_pos].it;
            int ret=0;
            if(panes[src].area == isinventory ) {
                ret = g->inventory_item_menu( item_pos, colstart + ( src == left ? w_width / 2 : 0 ),
                                              w_width / 2, (src == right ? 1 : -1) );
                recalc = true;
                checkshowmsg = true;
            } else {
                std::vector<iteminfo> vThisItem, vDummy;
                it->info(true, &vThisItem);
                int rightWidth = w_width / 2;
                vThisItem.push_back(iteminfo(_("DESCRIPTION"), "\n"));
                vThisItem.push_back(iteminfo(_("DESCRIPTION"),
                                             center_text(_("[up / page up] previous"),
                                                         rightWidth - 4)));
                vThisItem.push_back(iteminfo(_("DESCRIPTION"),
                                             center_text(_("[down / page down] next"),
                                                         rightWidth - 4)));
                ret = compare_split_screen_popup(colstart + ( src == left ? w_width/2 : 0 ),
                                                 rightWidth, 0, it->tname(), vThisItem, vDummy );
            }
            if ( ret == KEY_NPAGE || ret == KEY_DOWN ) {
                changey += 1;
                lastCh='e';
            } else if ( ret == KEY_PPAGE || ret == KEY_UP ) {
                changey += -1;
                lastCh='e';
            } else {
                lastCh = 0; redraw = true;
            };
        } else if( 'q' == c || KEY_ESCAPE == c) {
            exit = true;
        } else if('>' == c || KEY_NPAGE == c) {
            panes[src].page++;
            if( panes[src].page >= panes[src].max_page ) panes[src].page = 0;
            redraw = true;
        } else if('<' == c || KEY_PPAGE == c) {
            panes[src].page--;
            if( panes[src].page < 0 ) {
                panes[src].page = panes[src].max_page;
            }
            redraw = true;
        } else {
            switch(c) {
                case 'j':
                case KEY_DOWN:
                    changey = 1;
                    break;
                case 'k':
                case KEY_UP:
                    changey = -1;
                    break;
                case 'h':
                case KEY_LEFT:
                    changex = 0;
                    break;
                case 'l':
                case KEY_RIGHT:
                    changex = 1;
                    break;
                case '\t':
                    changex = dest;
                    break;
                default :
                    donothing = true;
                    break;
            }
        }
        if(!donothing) {
          if ( changey != 0 ) {
            for ( int l=2; l > 0; l-- ) {
              int new_index = panes[src].index;

              if (panes[src].sortby == SORTBY_CATEGORY &&
                  category_index_start.size() > 0 && inCategoryMode) {
                int prev_cat = 0, next_cat = 0, selected_cat = 0;

                for (int curr_cat = 0; curr_cat < category_index_start.size(); ++curr_cat) {
                    int next_cat_start = curr_cat + 1 < category_index_start.size() ?
                        curr_cat + 1 : panes[src].items.size() - 1;
                    int actual_index = panes[src].index + panes[src].page * itemsPerPage;

                    if (actual_index >= category_index_start[curr_cat] &&
                        actual_index <= category_index_start[next_cat_start]) {
                        selected_cat = curr_cat;

                        prev_cat = (curr_cat - 1) >= 0 ? curr_cat - 1 :
                            category_index_start.size() - 1;
                        prev_cat = category_index_start[selected_cat] < actual_index ?
                            selected_cat : prev_cat;

                        next_cat = (curr_cat + 1) < category_index_start.size() ? curr_cat + 1 : 0;
                    }
                }

                if (changey > 0) {
                    panes[src].page = category_index_start[next_cat] / itemsPerPage;
                    new_index = category_index_start[next_cat] % itemsPerPage;
                } else {
                    panes[src].page = category_index_start[prev_cat] / itemsPerPage;
                    new_index = category_index_start[prev_cat] % itemsPerPage;

                    panes[src].max_index = panes[src].page < panes[src].max_page - 1 ?
                        itemsPerPage : panes[src].items.size() % itemsPerPage;
                }
              } else {
                  new_index = panes[src].index + changey;
              }

              panes[src].index = new_index;
              if ( panes[src].index < 0 ) {
                  panes[src].page--;
                  if( panes[src].page < 0 ) {
                    panes[src].page = panes[src].max_page-1;
                    panes[src].index = panes[src].items.size() - 1 - ( panes[src].page * itemsPerPage );
                  } else {
                    panes[src].index = itemsPerPage; // corrected at the start of next iteration
                  }
              } else if ( panes[src].index >= panes[src].max_index ) {
                  panes[src].page++;
                  if( panes[src].page >= panes[src].max_page ) panes[src].page = 0;
                  panes[src].index = 0;
              }
              int lpos=panes[src].index + (panes[src].page * itemsPerPage);
              if ( lpos < panes[src].items.size() && panes[src].items[lpos].volume != -8 ) {
                  l=0;
              }

            }
            panes[src].redraw = true;
          }
          if ( changex >= 0 ) {
            src = changex;
            redraw = true;
          }
        }
    }

    uistate.adv_inv_last_coords.x = u.posx;
    uistate.adv_inv_last_coords.y = u.posy;
    uistate.adv_inv_leftarea = panes[left].area;
    uistate.adv_inv_rightarea = panes[right].area;
    uistate.adv_inv_leftindex = panes[left].index;
    uistate.adv_inv_leftpage = panes[left].page;
    uistate.adv_inv_rightindex = panes[right].index;
    uistate.adv_inv_rightpage = panes[right].page;

    uistate.adv_inv_leftfilter = panes[left].filter;
    uistate.adv_inv_rightfilter = panes[right].filter;

    werase(head);
    werase(panes[left].window);
    werase(panes[right].window);
    delwin(head);
    delwin(panes[left].window);
    delwin(panes[right].window);
    g->refresh_all();
}
