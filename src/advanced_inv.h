#ifndef _ADVANCED_INV_H_
#define _ADVANCED_INV_H_
#include "output.h"
#include "game.h"
#include <string>
struct advanced_inv_area {
    int id;
    int hscreenx;
    int hscreeny;
    int offx;
    int offy;
    int x;
    int y;
    std::string name;
    std::string shortname;
    bool canputitems;
    vehicle *veh;
    int vstor;
    int size;
    std::string desc;
    int volume, weight;
    int max_size, max_volume;
};

// see item_factory.h
class item_category;

// for printing items in environment
struct advanced_inv_listitem {
    int idx;
    int area;
    item *it;
    std::string name;
    std::string name_without_prefix;
    bool autopickup;
    int stacks;
    int volume;
    int weight;
    const item_category *cat;
};

class advanced_inventory_pane {
  public:
    int pos;
    int area, offx, offy, size, vstor;  // quick lookup later
    int index, max_page, max_index, page;
    std::string area_string;
    int sortby;
    int issrc;
    vehicle *veh;
    WINDOW *window;
    std::vector<advanced_inv_listitem> items;
    int numcats;
    std::string filter;
    bool recalc;
    bool redraw;
    std::map<std::string, bool> filtercache;
    advanced_inventory_pane() {
        offx = 0; offy = 0; size = 0; vstor = -1;
        index = 0; max_page = 0; max_index = 0; page = 0;
        area_string =  _("Initializing...");
        sortby = 1; issrc = 0; veh = NULL; window = NULL;
        items.clear();
        numcats = 0; filter="";
        filtercache.clear();
    }
};

class advanced_inventory
{
    public:
        player * p;

        const int head_height;
        const int min_w_height;
        const int min_w_width;
        const int max_w_width;
        const int left;
        const int right;
        const int isinventory;
        const int isall;
        const int isdrag;

    bool checkshowmsg;
    bool showmsg;
    bool inCategoryMode;

    int itemsPerPage;
    int w_height;
    int w_width;

    int headstart;
    int colstart;

//    itemsPerPage=getmaxy(left_window)-ADVINVOFS;
    // todo: awaiting ui::menu // last_tmpdest=-1;
    bool exit;// = false;
    bool redraw;// = true;
    bool recalc;// = true;
    int lastCh;// = 0;

    int src;// = left; // the active screen , 0 for left , 1 for right.
    int dest;// = right;
    bool examineScroll;// = false;
    bool filter_edit;

        advanced_inventory_pane panes[2];
        advanced_inv_area squares[12];

        advanced_inventory() :
            head_height(5),
            min_w_height(10),
            min_w_width(FULL_SCREEN_WIDTH),
            max_w_width(120),
            left(0),
            right(1),
            isinventory(0),
            isall(10),
            isdrag(11) {
        }
        void display(player *pp);
        void print_items(advanced_inventory_pane &pane, bool active);
        void recalc_pane(int i);
        void redraw_pane(int i);
        void init(player *pp);
    private:
};

#endif
