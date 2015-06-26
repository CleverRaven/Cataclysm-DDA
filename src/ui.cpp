#include "ui.h"
#include "catacharset.h"
#include "output.h"
#include "debug.h"
#include "input.h"
#include "cursesdef.h"
#include "uistate.h"
#include "options.h"
#include "game.h"
#include "player.h"

#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include <iterator>
#include <cstring>

#ifdef debuguimenu
#define dprint(a,...)      mvprintw(a,0,__VA_ARGS__)
#else
#define dprint(a,...)      void()
#endif

//// ui_base ////////////////////////////////////////////////////////////// {{{1
ui_base::ui_base()
{
    ctxt = new input_context(ctxt_name);
    register_input_actions();
}

ui_base::~ui_base()
{
    delete ctxt;
}

void ui_base::modify_border(int do_border, nc_color border_color)
{
    this->do_border = do_border;
    this->border_color = border_color;
}

void ui_base::draw_border()
{
    auto *w = window.get();
    wattron(w, border_color);
    wborder(w, LINE_XOXO, LINE_XOXO, 
               LINE_OXOX, LINE_OXOX,
               LINE_OXXO, LINE_OOXX, 
               LINE_XXOO, LINE_XOOX);
    wattroff(w, border_color);
}

void ui_base::set_error(int code, const std::string &msg)
{
    error_code = code;
    error_msg = msg;
}

ui_input_code ui_base::handle_input(const std::string &action)
{
    return ui_continue;
}

std::string ui_base::get_input() const
{
    return "";
}

int ui_base::operator ()()
{
    return run();
}

int ui_base::run()
{
    while(true) {
        draw();
        finish();
        draw_border();
        auto code = handle_input(get_input());
        switch(code) {
            case ui_error: // ran into an issue
                debugmsg("ui_error [%d]: %s", error_code, error_msg.c_str());
                return -1;
            case ui_exit: // exit normally
                return return_code;
            case ui_continue: // continue displaying the ui
                break;
            default:    // unknown ui code
                debugmsg("Invalid ui_input_code given! [%d]", code);
                return -2;
        }
    }
    // control should not reach here
}

// set the size of the window
void ui_base::set_size(const point &s)
{
    size = s;
}

// set the position of the window
void ui_base::set_position(const point &p)
{
    pos = p;
}
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_functor /////////////////////////////////////////////////////////// {{{1
void ui_functor::add_function(const ui_callback &uc)
{
    functions[uc.name] = uc.func;
}

void ui_functor::rem_function(const ui_callback &uc)
{
    for(auto iter = functions.begin(); iter != functions.end(); ++iter) {
        if(iter->first == uc.name) {
            functions.erase(iter);
            return;
        }
    }
    // check your code homie! :-)
    assert(false);
}

void ui_functor::mod_function(const ui_callback &uc)
{
    // the function must be present! use add_function() to add a new one! :-)
    assert(functions.find(uc.name) != functions.end());
    functions[uc.name] = uc.func;
}
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_element /////////////////////////////////////////////////////////// {{{1
void ui_element::draw()
{
    finish();
}
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_scrollbar ///////////////////////////////////////////////////////// {{{1
void ui_scrollbar::draw()
{
    const auto &offset = pos;

    if(size.y >= num_items) {
        //scrollbar is not required
        bar_color = BORDER_COLOR;
    }

    //Clear previous scrollbar
    for(int i = offset.y; i < offset.y + size.y; i++) {
        mvwputch(window.get(), i, offset.x, bar_color, LINE_XOXO);
    }

    if(size.y >= num_items) {
        wrefresh(window.get());
        return;
    }

    if(num_items > 0) {
        mvwputch(window.get(), offset.y, offset.x, c_ltgreen, '^');
        mvwputch(window.get(), offset.y + size.y - 1, offset.x, c_ltgreen, 'v');

        int height = ((size.y - 2) * (size.y - 2)) / num_items;

        if (height < 2) {
            height = 2;
        }

        const int &line = this->get_line();
        int y = (line == 0) ? -1 :
            (line == (num_items - 1)) ? size.y - 3 - height : 
            (line * (size.y - 3 - height)) / num_items;
        for(int i = 0; i < height; ++i) {
            mvwputch(window.get(), i + size.y + 2 + y, offset.x, c_cyan_cyan, LINE_XOXO);
        }
    }

    wrefresh(window.get());
}
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_tabbed //////////////////////////////////////////////////////////// {{{1
ui_tabbed::ui_tabbed()
{
    labels.push_back("NULL");
}

ui_tabbed::ui_tabbed(const char *start, ...)
{
    va_list ap;
    if(start == nullptr || strlen(start) == 0) {
        labels.push_back("NULL");
        return;
    }
    labels.push_back(std::string(start));
    va_start(ap, start);
    while (const char *label = va_arg(ap, char *)) {
        labels.push_back(std::string(label));
    }
    va_end(ap);
}

const std::vector<std::string> &ui_tabbed::get_labels() const
{
    return labels;
}

template <typename T>
void ui_tabbed::set_labels(T v)
{
    labels = v;
}

// yucky duplicate code :-(
void ui_tabbed::set_labels(const char *start, ...)
{
    // free the previous labels
    labels.clear();
    va_list ap;
    if(start == nullptr || strlen(start) == 0) {
        labels.push_back("NULL");
        return;
    }
    labels.push_back(std::string(start));
    va_start(ap, start);
    while (const char *label = va_arg(ap, char *)) {
        labels.push_back(std::string(label));
    }
    va_end(ap);
}

size_t ui_tabbed::find_label(const std::string &name)
{
    int index = -1;
    for(int i = 0; i < labels.size(); ++i) {
        if(name == labels[i]) {
            index = i;
            break;
        }
    }
    // recheck your code friend! :-)
    assert(index != -1);
    return index;
}

void ui_tabbed::add_label(const std::string &name)
{
    labels.push_back(name);
}

void ui_tabbed::rem_label(const std::string &name)
{
    auto elem = labels.begin();
    std::advance(elem, find_label(name));
    labels.erase(elem);
}

void ui_tabbed::mod_label(const std::string &old_name, const std::string &new_name)
{
    labels[find_label(old_name)] = new_name;
}

// TODO: go through the window drawing code to assure sanity
void ui_tabbed::draw_tabs()
{
    auto *w = window.get();

    int win_width;
    win_width = getmaxx(w);

    // Draw the line under the tabs
    for (int x = 0; x < win_width; x++) {
        mvwputch(w, 2, x, c_white, LINE_OXOX);
    }

    int total_width = 0;
    for (auto &i : labels) {
        total_width += i.length() + 6;    // "< |four| >"
    }

    if (total_width > win_width) {
        //debugmsg("draw_tabs not given enough space! %s", labels[0]);
        return;
    }

    // Extra "buffer" space per each side of each tab
    double buffer_extra = (win_width - total_width) / (labels.size() * 2);
    int buffer = static_cast<int>(buffer_extra);
    // Set buffer_extra to (0, 1); the "extra" whitespace that builds up
    buffer_extra = buffer_extra - buffer;
    int xpos = 0;
    double savings = 0;

    for (size_t i = 0; i < labels.size(); i++) {
        int length = labels[i].length();
        xpos += buffer + 2;
        savings += buffer_extra;
        if (savings > 1) {
            savings--;
            xpos++;
        }
        mvwputch(w, 0, xpos, c_white, LINE_OXXO);
        mvwputch(w, 1, xpos, c_white, LINE_XOXO);
        mvwputch(w, 0, xpos + length + 1, c_white, LINE_OOXX);
        mvwputch(w, 1, xpos + length + 1, c_white, LINE_XOXO);
        if ((int)i == active_tab) {
            mvwputch(w, 1, xpos - 2, h_white, '<');
            mvwputch(w, 1, xpos + length + 3, h_white, '>');
            mvwputch(w, 2, xpos, c_white, LINE_XOOX);
            mvwputch(w, 2, xpos + length + 1, c_white, LINE_XXOO);
            mvwprintz(w, 1, xpos + 1, h_white, "%s", labels[i].c_str());
            for (int x = xpos + 1; x <= xpos + length; x++) {
                mvwputch(w, 0, x, c_white, LINE_OXOX);
                mvwputch(w, 2, x, c_black, 'x');
            }
        } else {
            mvwputch(w, 2, xpos, c_white, LINE_XXOX);
            mvwputch(w, 2, xpos + length + 1, c_white, LINE_XXOX);
            mvwprintz(w, 1, xpos + 1, c_white, "%s", labels[i].c_str());
            for (int x = xpos + 1; x <= xpos + length; x++) {
                mvwputch(w, 0, x, c_white, LINE_OXOX);
            }
        }
        xpos += length + 1 + buffer;
    }
}

void ui_tabbed::finish()
{
}
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_canvas //////////////////////////////////////////////////////////// {{{1
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_list ////////////////////////////////////////////////////////////// {{{1
size_t ui_list::find_entry(const std::string &name) const
{
    for(size_t i = 0; i < entries.size(); ++i) {
        if(entries[i].name == name) {
            return i;
        }
    }
    // entry must be valid!
    assert(false);
    return entries.size();
}

void ui_list::add_entry(const std::string &name, bool enabled, nc_color col, int ret)
{
    add_entry(ui_list_entry(name, enabled, col, ret));
}

void ui_list::add_entry(const ui_list_entry &entry)
{
    entries.push_back(entry);
}

void ui_list::rem_entry(const std::string &name)
{
    rem_entry(ui_list_entry(name));
}

void ui_list::rem_entry(const ui_list_entry &entry)
{
    auto iter = entries.begin();
    std::advance(iter, find_entry(entry.name));
    entries.erase(iter);
}

void ui_list::mod_entry(const std::string &name, bool enabled, nc_color col)
{
    mod_entry(ui_list_entry(name, enabled, col));
}

void ui_list::mod_entry(const ui_list_entry &entry)
{
    entries[find_entry(entry.name)] = entry;
}

ui_list_entry ui_list::get_entry(const std::string &name) const
{
    return entries[find_entry(name)];
}

void ui_list::clear()
{
    entries.clear();
}

/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_container ///////////////////////////////////////////////////////// {{{1
ui_container::ui_container()
{
}

ui_container::ui_container(const point &pos, const point &size)
{
    this->pos = pos;
    this->size = size;
    set_region_size(size);
}

ui_container::ui_container(const std::vector<ui_children> &children)
{
    auto size = calculate_size(children);
    set_size(size);
    set_region_size(size);
    for(auto &child : children) {
        add_element(child.name, child.element);
    }
}

void ui_container::set_region_size(const point &size)
{
    region.resize(size.x * size.y);
}

bool ui_container::add_region(const point &pos, const point &size)
{
    // check to see if any part of the region overlaps before setting anything
    if(!does_region_overlap(pos, size)) {
        // region is safe, set bits
        for(int y = pos.y; y < pos.y + size.y; ++y) {
            for(int x = pos.x; x < pos.x + size.x; ++x) {
                region[this->size.x * y + x] = true;
            }
        }
        return true;
    }
    return false;
}

void ui_container::rem_region(const point &pos, const point &size)
{
}

bool ui_container::does_region_overlap(const point &pos, const point &size)
{
    // points for each of the element's 4 corners
    const std::array<point, 4> c = {
        {pos.x, pos.y},
        {pos.x + size.x, pos.y},
        {pos.x, pos.y + size.y},
        {pos.x + size.x, pos.y + size.y}
    };
    auto is_inside = [&c](const quad &q) {
        bool ul = 
        return (q.x >= 
    };
    for(auto &q : region) {
    }
}

//bool ui_container::does_region_overlap(const point &pos, const point &size)
//{
//    // check the midpoint first, for any overlaps, to save on calculations
//    const int midpoint = ((pos.y + (size.y / 2)) * this->size.x) + (pos.x + (size.x / 2));
//    if(region[midpoint] == true) {
//        return true;
//    }
//    // calculate the points for each of the 4 corners
//    const std::array<int, 4> corners = {
//        pos.y * this->size.x,                                   // upper left
//        pos.y * this->size.x + pos.x + size.x,                  // upper right
//        (pos.y + size.y) * this->size.x,                        // lower left
//        (pos.y + size.y) * this->size.x + pos.x + size.x        // lower right
//    };
//    // verify that the 4 corners are not taken
//    for(auto &corner : corners) {
//        if(region[corner] == true) {
//            return true;
//        }
//    }
//    // defer calculation of quadrant midpoints and fourths to post corner check
//    const point one_fourth = {size.x / 4, size.y / 4};
//    const std::array<int, 4> quadrants = {
//        // quadrant 1
//        corners[0] + (one_fourth.y * this->size.x) + one_fourth.x,
//        // quadrant 2
//        corners[1] + (one_fourth.y * this->size.x) - one_fourth.x,
//        // quadrant 4
//        corners[2] - (one_fourth.y * this->size.x) + one_fourth.x,
//        // quadrant 3
//        corners[3] - (one_fourth.y * this->size.x) - one_fourth.x
//    };
//    // check each quadrant midpoint for a possible region
//    for(auto &quadrant : quadrants) {
//        if(region[quadrant] == true) {
//            return true;
//        }
//    }
//    // fallback to manually check each individual bit in the quadrant
//    for(int y = pos.y; y < pos.y + size.y; ++y) {
//        for(int x = pos.x; x < pos.x + size.x; ++x) {
//            if(region[this->size.x * y + x] == true) {
//                return true;
//            }
//        }
//    }
//    return false;
//}

point ui_container::calculate_size(const std::vector<ui_children> &children)
{
    // stores the calculated size
    point calc = {0, 0};
    for(auto &child : children) {
        if(child.element == nullptr) {
            continue;
        }
        auto p = child.element->get_position();
        auto s = child.element->get_size();
        // position + size
        point n(p.x + s.x, p.y + s.y);
        if(n.x > calc.x) {
            calc.x = n.x;
        }
        if(n.y > calc.y) {
            calc.y = n.y;
        }
    }
    return calc;
}

void ui_container::refresh()
{
}

void ui_container::add_element(const std::string &name, ui_element *element)
{
    children[name] = element;
}

void ui_container::rem_element(const std::string &name)
{
}
/////////////////////////////////////////////////////////////////////////// }}}1



//// work in progress ///////////////////////////////////////////////////// {{{1
////////////////////////////////////
int getfoldedwidth (std::vector<std::string> foldedstring)
{
    int ret = 0;
    for (auto &i : foldedstring) {
        int width = utf8_width(i.c_str());
        if ( width > ret ) {
            ret = width;
        }
    }
    return ret;
}

////////////////////////////////////
uimenu::uimenu()
{
    init();
}

// here we emulate the old int ret=menu(bool, "header", "option1", "option2", ...);
uimenu::uimenu(bool, const char * const mes, ...)
{
    init();
    va_list ap;
    va_start(ap, mes);
    int i = 0;
    while (char const *const tmp = va_arg(ap, char *)) {
        entries.push_back(uimenu_entry(i++, true, MENU_AUTOASSIGN, tmp ));
    }
    va_end(ap);
    query();
}

// exact usage as menu_vec
uimenu::uimenu(bool cancelable, const char *mes,
               const std::vector<std::string> options)
{
    init();
    if (options.empty()) {
        debugmsg("0-length menu (\"%s\")", mes);
        ret = -1;
    } else {
        text = mes;
        shift_retval = 1;
        return_invalid = cancelable;

        for (size_t i = 0; i < options.size(); i++) {
            entries.push_back(uimenu_entry(i, true, MENU_AUTOASSIGN, options[i] ));
        }
        query();
    }
}

uimenu::uimenu(bool cancelable, const char *mes,
               const std::vector<std::string> &options,
               const std::string &hotkeys_override)
{
    init();
    hotkeys = hotkeys_override;
    if (options.empty()) {
        debugmsg("0-length menu (\"%s\")", mes);
        ret = -1;
    } else {
        text = mes;
        shift_retval = 1;
        return_invalid = cancelable;

        for (size_t i = 0; i < options.size(); i++) {
            entries.push_back(uimenu_entry(i, true, MENU_AUTOASSIGN, options[i] ));
        }
        query();
    }
}

uimenu::uimenu(int startx, int width, int starty, std::string title,
               std::vector<uimenu_entry> ents)
{
    // another quick convenience coonstructor
    init();
    pos = {startx, starty};
    size.x = width;
    text = title;
    entries = ents;
    query();
    //dprint(2,"const: ret=%d pos.x=%d pos.y=%d size.x=%d size.y=%d, text=%s",ret,pos.x,pos.y,size.x,size.y, text.c_str() );
}

uimenu::uimenu(bool cancelable, int startx, int width, int starty, std::string title,
               std::vector<uimenu_entry> ents)
{
    // another quick convenience coonstructor
    init();
    return_invalid = cancelable;
    pos = {startx, starty};
    size.x = width;
    text = title;
    entries = ents;
    query();
    //dprint(2,"const: ret=%d pos.x=%d pos.y=%d size.x=%d size.y=%d, text=%s",ret,pos.x,pos.y,size.x,size.y, text.c_str() );
}

/*
 * Enables oneshot construction -> running -> exit
 */
uimenu::operator int() const
{
    int r = ret + shift_retval;
    return r;
}

/*
 * Sane defaults on initialization
 */
void uimenu::init()
{
    pos.x = MENU_AUTOASSIGN;            // starting position
    pos.y = MENU_AUTOASSIGN;            // -1 = auto center
    size.x = MENU_AUTOASSIGN;           // MENU_AUTOASSIGN = based on text width or max entry width, -2 = based on max entry, folds text
    size.y =
        MENU_AUTOASSIGN; // -1 = autocalculate based on number of entries + number of lines in text // fixme: scrolling list with offset
    ret = UIMENU_INVALID;  // return this unless a valid selection is made ( -1024 )
    text = "";             // header text, after (maybe) folding, populates:
    textformatted.clear(); // folded to textwidth
    textwidth = MENU_AUTOASSIGN; // if unset, folds according to size.x
    textalign = MENU_ALIGN_LEFT; // todo
    title = "";            // Makes use of the top border, no folding, sets min width if size.x is auto
    keypress = 0;          // last keypress from (int)getch()
    window.reset(new WINDOW); // our window
    keymap.clear();        // keymap[int] == index, for entries[index]
    selected = 0;          // current highlight, for entries[index]
    entries.clear();       // uimenu_entry(int returnval, bool enabled, int keycode, std::string text, ...todo submenu stuff)
    started = false;       // set to true when width and key calculations are done, and window is generated.
    pad_left = 0;          // make a blank space to the left
    pad_right = 0;         // or right
    desc_enabled = false;  // don't show option description by default
    desc_lines = 6;        // default number of lines for description
    border = true;         // todo: always true
    border_color = c_magenta; // border color
    text_color = c_ltgray;  // text color
    title_color = c_green;  // title color
    hilight_color = h_white; // highlight for up/down selection bar
    hotkey_color = c_ltgreen; // hotkey text to the right of menu entry's text
    disabled_color = c_dkgray; // disabled menu entry
    return_invalid = false;  // return 0-(int)invalidKeyCode
    hilight_full = true;     // render hilight_color background over the entire line (minus padding)
    hilight_disabled =
        false; // if false, hitting 'down' onto a disabled entry will advance downward to the first enabled entry
    shift_retval = 0;        // for legacy menu/vec_menu
    vshift = 0;              // scrolling menu offset
    vmax = 0;                // max entries area rows
    callback = NULL;         // * uimenu_callback
    filter = "";             // filter string. If "", show everything
    fentries.clear();        // fentries is the actual display after filtering, and maps displayed entry number to actual entry number
    fselected = 0;           // fentries[selected]
    filtering = true;        // enable list display filtering via '/' or '.'
    filtering_nocase = true; // ignore case when filtering
    max_entry_len = 0;       // does nothing but can be read
    max_desc_len = 0;        // for calculating space for descriptions

    scrollbar_auto =
        true;   // there is no force-on; true will only render scrollbar if entries > vertical height
    scrollbar_nopage_color =
        c_ltgray;    // color of '|' line for the entire area that isn't current page.
    scrollbar_page_color = c_cyan_cyan; // color of the '|' line for whatever's the current page.
    scrollbar_side = -1;     // -1 == choose left unless taken, then choose right

    last_fsize = -1;
    last_vshift = -1;
    hotkeys = DEFAULT_HOTKEYS;
}

/*
 * case insensitive string::find( string::findstr ). findstr must be lowercased
 */
bool lcmatch(const std::string &str, const std::string &findstr)
{
    std::string ret = "";
    ret.reserve( str.size() );
    transform( str.begin(), str.end(), std::back_inserter(ret), tolower );
    return ( (int)ret.find( findstr ) != -1 );
}

/*
 * repopulate filtered entries list (fentries) and set fselected accordingly
 */
void uimenu::filterlist()
{
    bool notfiltering = ( ! filtering || filter.size() < 1 );
    int num_entries = entries.size();
    bool nocase = (filtering_nocase == true); // todo: && is_all_lc( filter )
    std::string fstr = "";
    fstr.reserve(filter.size());
    if ( nocase ) {
        transform( filter.begin(), filter.end(), std::back_inserter(fstr), tolower );
    } else {
        fstr = filter;
    }
    fentries.clear();
    fselected = -1;
    int f = 0;
    for( int i = 0; i < num_entries; i++ ) {
        if( notfiltering || ( nocase == false && (int)entries[ i ].txt.find(filter) != -1 ) ||
            lcmatch(entries[i].txt, fstr ) ) {
            fentries.push_back( i );
            if ( i == selected ) {
                fselected = f;
            } else if ( i > selected && fselected == -1 ) {
                // Past the previously selected entry, which has been filtered out,
                // choose another nearby entry instead.
                fselected = f;
            }
            f++;
        }
    }
    if ( fselected == -1 ) {
        fselected = 0;
        vshift = 0;
        if ( fentries.empty() ) {
            selected = -1;
        } else {
            selected = fentries [ 0 ];
        }
    } else if (fselected < (int)fentries.size()) {
        selected = fentries[fselected];
    } else {
        fselected = selected = -1;
    }
    // scroll to top of screen if all remaining entries fit the screen.
    if ((int)fentries.size() <= vmax) {
        vshift = 0;
    }
}

/*
 * Call string_input_win / ui_element_input::input_filter and filter the entries list interactively
 */
std::string uimenu::inputfilter()
{
    std::string identifier = ""; // todo: uimenu.filter_identifier ?
    long key = 0;
    int spos = -1;
    mvwprintz(window.get(), size.y - 1, 2, border_color, "< ");
    mvwprintz(window.get(), size.y - 1, size.x - 3, border_color, " >");
    /*
    //debatable merit
        std::string origfilter = filter;
        int origselected = selected;
        int origfselected = fselected;
        int origvshift = vshift;
    */
    do {
        // filter=filter_input->query(filter, false);
        filter = string_input_win( window.get(), filter, 256, 4, size.y - 1, size.x - 4,
                                   false, key, spos, identifier, 4, size.y - 1 );
        // key = filter_input->keypress;
        if ( key != KEY_ESCAPE ) {
            if ( scrollby(0, key) == false ) {
                filterlist();
            }
            show();
        }
    } while(key != '\n' && key != KEY_ESCAPE);

    if ( key == KEY_ESCAPE ) {
        /*
        //perhaps as an option
                filter = origfilter;
                selected = origselected;
                fselected = origfselected;
                vshift = origvshift;
        */
        filterlist();
    }

    wattron(window.get(), border_color);
    for( int i = 1; i < size.x - 1; i++ ) {
        mvwaddch(window.get(), size.y - 1, i, LINE_OXOX);
    }
    wattroff(window.get(), border_color);

    return filter;
}

/*
 * Calculate sizes, populate arrays, initialize window
 */
void uimenu::setup()
{
    bool w_auto = (size.x == -1 || size.x == -2 );
    bool w_autofold = ( size.x == -2);

    if ( w_auto ) {
        size.x = 4;
        if ( !title.empty() ) {
            size.x = title.size() + 5;
        }
    }

    bool h_auto = (size.y == -1);
    if ( h_auto ) {
        size.y = 4;
    }

    if ( desc_enabled && !(w_auto && h_auto) ) {
        desc_enabled = false; // give up
        debugmsg("desc_enabled without w_auto and h_auto (h: %d, w: %d)", h_auto, w_auto);
    }

    max_entry_len = 0;
    max_desc_len = 0;
    std::vector<int> autoassign;
    int pad = pad_left + pad_right + 2;
    int descwidth_final = 0; // for description width guard
    for ( size_t i = 0; i < entries.size(); i++ ) {
        int txtwidth = utf8_width(remove_color_tags( entries[ i ].txt ).c_str());
        if ( txtwidth > max_entry_len ) {
            max_entry_len = txtwidth;
        }
        if(entries[ i ].enabled) {
            if( entries[ i ].hotkey > 0 ) {
                keymap[ entries[ i ].hotkey ] = i;
            } else if ( entries[ i ].hotkey == -1 && i < 100 ) {
                autoassign.push_back(i);
            }
            if ( entries[ i ].retval == -1 ) {
                entries[ i ].retval = i;
            }
            if ( w_auto && size.x < txtwidth + pad + 4 ) {
                size.x = txtwidth + pad + 4;
            }
        } else {
            if ( w_auto && size.x < txtwidth + pad + 4 ) {
                size.x = txtwidth + pad + 4;    // todo: or +5 if header
            }
        }
        if ( desc_enabled ) {
            // subtract one from desc_lines for the reminder of the text
            int descwidth = utf8_width(entries[i].desc.c_str()) / (desc_lines - 1);
            descwidth += 4; // 2x border + 2x ' ' pad
            if ( descwidth_final < descwidth ) {
                descwidth_final = descwidth;
            }
        }
        if ( entries[ i ].text_color == c_red_red ) {
            entries[ i ].text_color = text_color;
        }
        fentries.push_back( i );
    }
    size_t next_free_hotkey = 0;
    for( auto it = autoassign.begin(); it != autoassign.end() &&
         next_free_hotkey < hotkeys.size(); ++it ) {
        while( next_free_hotkey < hotkeys.size() ) {
            const int setkey = hotkeys[next_free_hotkey];
            next_free_hotkey++;
            if( keymap.count( setkey ) == 0 ) {
                entries[*it].hotkey = setkey;
                keymap[setkey] = *it;
                break;
            }
        }
    }

    if (desc_enabled) {
        if (descwidth_final > TERMX) {
            desc_enabled = false; // give up
            debugmsg("description would exceed terminal width (%d vs %d available)", descwidth_final, TERMX);
        } else if (descwidth_final > size.x) {
            size.x = descwidth_final;
        }
    }

    if (w_auto && size.x > TERMX) {
        size.x = TERMX;
    }

    if(!text.empty() ) {
        int twidth = utf8_width(remove_color_tags( text ).c_str());
        bool formattxt = true;
        int realtextwidth = 0;
        if ( textwidth == -1 ) {
            if ( w_autofold || !w_auto ) {
                realtextwidth = size.x - 4;
            } else {
                realtextwidth = twidth;
                if ( twidth + 4 > size.x ) {
                    if ( realtextwidth + 4 > TERMX ) {
                        realtextwidth = TERMX - 4;
                    }
                    textformatted = foldstring(text, realtextwidth);
                    formattxt = false;
                    realtextwidth = 10;
                    for (auto &l : textformatted) {
                        if ( utf8_width(l.c_str()) > realtextwidth ) {
                            realtextwidth = utf8_width(l.c_str());
                        }
                    }
                    if ( realtextwidth + 4 > size.x ) {
                        size.x = realtextwidth + 4;
                    }
                }
            }
        } else if ( textwidth != -1 ) {
            realtextwidth = textwidth;
        }
        if ( formattxt == true ) {
            textformatted = foldstring(text, realtextwidth);
        }
    }

    if (h_auto) {
        size.y = 3 + textformatted.size() + entries.size();
        if (desc_enabled) {
            int y_final = size.y + desc_lines + 1; // add one for border
            if (y_final > TERMY) {
                desc_enabled = false; // give up
                debugmsg("with description height would exceed terminal height (%d vs %d available)",
                         y_final, TERMY);
            } else {
                size.y = y_final;
            }
        }
    }

    if ( size.y > TERMY ) {
        size.y = TERMY;
    }

    vmax = entries.size();
    if ( vmax + 3 + (int)textformatted.size() > size.y ) {
        vmax = size.y - 3 - textformatted.size();
        if ( vmax < 1 ) {
            if (textformatted.empty()) {
                popup("Can't display menu options, 0 %d available screen rows are occupied\nThis is probably a bug.\n",
                      TERMY);
            } else {
                popup("Can't display menu options, %d %d available screen rows are occupied by\n'%s\n(snip)\n%s'\nThis is probably a bug.\n",
                      textformatted.size(), TERMY, textformatted[0].c_str(),
                      textformatted[ textformatted.size() - 1 ].c_str()
                     );
            }
        }
    }

    if (pos.x == -1) {
        pos.x = int((TERMX - size.x) / 2);
    }
    if (pos.y == -1) {
        pos.y = int((TERMY - size.y) / 2);
    }

    if ( scrollbar_side == -1 ) {
        scrollbar_side = ( pad_left > 0 ? 1 : 0 );
    }
    if ( (int)entries.size() <= vmax ) {
        scrollbar_auto = false;
    }
    window.reset(newwin(size.y, size.x, pos.y, pos.x));

    werase(window.get());
    draw_border(window.get(), border_color);
    if( !title.empty() ) {
        mvwprintz(window.get(), 0, 1, border_color, "< ");
        wprintz(window.get(), title_color, "%s", title.c_str() );
        wprintz(window.get(), border_color, " >");
    }
    fselected = selected;
    if(fselected < 0) {
        fselected = selected = 0;
    } else if(fselected >= static_cast<int>(entries.size())) {
        fselected = selected = static_cast<int>(entries.size()) - 1;
    }
    if(!entries.empty() && !entries[fselected].enabled) {
        for(size_t i = 0; i < entries.size(); ++i) {
            if(entries[i].enabled) {
                fselected = selected = i;
                break;
            }
        }
    }
    started = true;
}

void uimenu::apply_scrollbar()
{
    if ( ! scrollbar_auto ) {
        return;
    }
    if ( last_vshift != vshift || last_fsize != (int)fentries.size() ) {
        last_vshift = vshift;
        last_fsize = fentries.size();

        int sbside = ( scrollbar_side == 0 ? 0 : size.x );
        int estart = textformatted.size() + 1;

        if ( !fentries.empty() && vmax < (int)fentries.size() ) {
            wattron(window.get(), border_color);
            mvwaddch(window.get(), estart, sbside, '^');
            wattroff(window.get(), border_color);

            wattron(window.get(), scrollbar_nopage_color);
            for( int i = estart + 1; i < estart + vmax - 1; i++ ) {
                mvwaddch(window.get(), i, sbside, LINE_XOXO);
            }
            wattroff(window.get(), scrollbar_nopage_color);

            wattron(window.get(), border_color);
            mvwaddch(window.get(), estart + vmax - 1, sbside, 'v');
            wattroff(window.get(), border_color);

            int svmax = vmax - 2;
            int fentriessz = fentries.size() - vmax;
            int sbsize = (vmax * svmax) / fentries.size();
            if ( sbsize < 2 ) {
                sbsize = 2;
            }
            int svmaxsz = svmax - sbsize;
            int sbstart = ( vshift * svmaxsz ) / fentriessz;
            int sbend = sbstart + sbsize;

            wattron(window.get(), scrollbar_page_color);
            for ( int i = sbstart; i < sbend; i++ ) {
                mvwaddch(window.get(), i + estart + 1, sbside, LINE_XOXO);
            }
            wattroff(window.get(), scrollbar_page_color);

        } else {
            wattron(window.get(), border_color);
            for( int i = estart; i < estart + vmax; i++ ) {
                mvwaddch(window.get(), i, sbside, LINE_XOXO);
            }
            wattroff(window.get(), border_color);
        }
    }
}

/*
 * Generate and refresh output
 */
void uimenu::show()
{
    if (!started) {
        setup();
    }
    std::string padspaces = std::string(size.x - 2 - pad_left - pad_right, ' ');
    const int text_lines = textformatted.size();
    for ( int i = 0; i < text_lines; i++ ) {
        trim_and_print(window.get(), 1 + i, 2, getmaxx(window) - 4, text_color, "%s", textformatted[i].c_str());
    }

    mvwputch(window.get(), text_lines + 1, 0, border_color, LINE_XXXO);
    for ( int i = 1; i < size.x - 1; ++i) {
        mvwputch(window.get(), text_lines + 1, i, border_color, LINE_OXOX);
    }
    mvwputch(window.get(), text_lines + 1, size.x - 1, border_color, LINE_XOXX);

    int estart = text_lines + 2;

    calcStartPos( vshift, fselected, vmax, fentries.size() );

    for ( int fei = vshift, si = 0; si < vmax; fei++, si++ ) {
        if ( fei < (int)fentries.size() ) {
            int ei = fentries [ fei ];
            nc_color co = ( ei == selected ?
                            hilight_color :
                            ( entries[ ei ].enabled ?
                              entries[ ei ].text_color :
                              disabled_color )
                          );

            if ( hilight_full ) {
                mvwprintz(window.get(), estart + si, pad_left + 1, co , "%s", padspaces.c_str());
            }
            if(entries[ ei ].enabled && entries[ ei ].hotkey >= 33 && entries[ ei ].hotkey < 126 ) {
                mvwprintz( window.get(), estart + si, pad_left + 2, ( ei == selected ) ? hilight_color :
                           hotkey_color , "%c", entries[ ei ].hotkey );
            }
            if( padspaces.size() > 3 ) {
                // padspaces's length indicates the maximal width of the entry, it is used above to
                // activate the highlighting, it is used to override previous text there, but in both
                // cases printeing starts at pad_left+1, here it starts at pad_left+4, so 3 cells less
                // to be used.
                const auto entry = utf8_wrapper( entries[ ei ].txt );
                trim_and_print( window.get(), estart + si, pad_left + 4, size.x - 2 - pad_left - pad_right, co, "%s", entry.c_str() );
            }
            if ( !entries[ei].extratxt.txt.empty() ) {
                mvwprintz( window.get(), estart + si, pad_left + 1 + entries[ ei ].extratxt.left,
                           entries[ ei ].extratxt.color, "%s", entries[ ei ].extratxt.txt.c_str() );
            }
            if ( entries[ei].extratxt.sym != 0 ) {
                mvwputch ( window.get(), estart + si, pad_left + 1 + entries[ ei ].extratxt.left,
                           entries[ ei ].extratxt.color, entries[ ei ].extratxt.sym );
            }
            if ( callback != NULL && ei == selected ) {
                callback->select(ei, this);
            }
        } else {
            mvwprintz(window.get(), estart + si, pad_left + 1, c_ltgray , "%s", padspaces.c_str());
        }
    }

    if ( desc_enabled ) {
        // draw border
        mvwputch(window.get(), size.y - desc_lines - 2, 0, border_color, LINE_XXXO);
        for ( int i = 1; i < size.x - 1; ++i) {
            mvwputch(window.get(), size.y - desc_lines - 2, i, border_color, LINE_OXOX);
        }
        mvwputch(window.get(), size.y - desc_lines - 2, size.x - 1, border_color, LINE_XOXX);

        // clear previous desc the ugly way
        for ( int y = desc_lines + 1; y > 1; --y ) {
            for ( int x = 2; x < size.x - 2; ++x) {
                mvwputch(window.get(), size.y - y, x, text_color, " ");
            }
        }

        // draw description
        fold_and_print(window.get(), size.y - desc_lines - 1, 2, size.x - 4, text_color, entries[selected].desc.c_str());
    }

    if ( !filter.empty() ) {
        mvwprintz( window.get(), size.y - 1, 2, border_color, "< %s >", filter.c_str() );
        mvwprintz( window.get(), size.y - 1, 4, text_color, "%s", filter.c_str() );
    }
    apply_scrollbar();

    this->refresh(true);
}

/*
 * wrefresh + wrefresh callback's window
 */
void uimenu::refresh( bool refresh_callback )
{
    wrefresh(window.get());
    if ( refresh_callback && callback != NULL ) {
        callback->refresh(this);
    }
}

/*
 * redraw borders, which is required in some cases ( look_around() )
 */
void uimenu::redraw( bool redraw_callback )
{
    draw_border(window.get(), border_color);
    if( !title.empty() ) {
        mvwprintz(window.get(), 0, 1, border_color, "< ");
        wprintz(window.get(), title_color, "%s", title.c_str() );
        wprintz(window.get(), border_color, " >");
    }
    if ( !filter.empty() ) {
        mvwprintz(window.get(), size.y - 1, 2, border_color, "< %s >", filter.c_str() );
        mvwprintz(window.get(), size.y - 1, 4, text_color, "%s", filter.c_str());
    }
    (void)redraw_callback; // TODO
    /*
    // pending tests on if this is needed
        if ( redraw_callback && callback != NULL ) {
            callback->redraw(this);
        }
    */
}

/*
 * check for valid scrolling keypress and handle. return false if invalid keypress
 */
bool uimenu::scrollby(int scrollby, const int key)
{
    if ( key != 0 ) {
        if ( key == KEY_UP ) {
            scrollby = -1;
        } else if ( key == KEY_PPAGE ) {
            scrollby = (-vmax + 1);
        } else if ( key == KEY_DOWN ) {
            scrollby = 1;
        } else if ( key == KEY_NPAGE ) {
            scrollby = vmax - 1;
        } else {
            return false;
        }
    } else if ( scrollby == 0 ) {
        return false;
    }

    bool looparound = ( scrollby == -1 || scrollby == 1 );
    bool backwards = ( scrollby < 0 );

    fselected += scrollby;
    if ( ! looparound ) {
        if ( backwards && fselected < 0 ) {
            fselected = 0;
        } else if ( fselected >= (int)fentries.size() ) {
            fselected = fentries.size() - 1;
        }
    }

    int iter = ( hilight_disabled ? 1 : fentries.size() );

    if ( backwards ) {
        while ( iter > 0 ) {
            iter--;
            if( fselected < 0 ) {
                fselected = fentries.size() - 1;
            }
            if ( entries[ fentries [ fselected ] ].enabled == false ) {
                fselected--;
            } else {
                iter = 0;
            }
        }
    } else {
        while ( iter > 0 ) {
            iter--;
            if( fselected >= (int)fentries.size() ) {
                fselected = 0;
            }
            if ( entries[ fentries [ fselected ] ].enabled == false ) {
                fselected++;
            } else {
                iter = 0;
            }
        }
    }
    if( static_cast<size_t>( fselected ) < fentries.size() ) {
        selected = fentries [ fselected ];
    }
    return true;
}

/*
 * Handle input and update display
 */
void uimenu::query(bool loop)
{
    keypress = 0;
    if ( entries.empty() ) {
        return;
    }
    int startret = UIMENU_INVALID;
    ret = UIMENU_INVALID;
    bool keycallback = (callback != NULL );

    show();
    do {
        bool skiprefresh = false;
        bool skipkey = false;
        keypress = getch();

        if ( scrollby(0, keypress) == true ) {
            /* nothing */
        } else if ( filtering && ( keypress == '/' || keypress == '.' ) ) {
            inputfilter();
        } else if ( !fentries.empty() && ( keypress == '\n' || keypress == KEY_ENTER ||
                                           keymap.find(keypress) != keymap.end() ) ) {
            if ( keymap.find(keypress) != keymap.end() ) {
                selected = keymap[ keypress ];//fixme ?
            }
            if( entries[ selected ].enabled ) {
                ret = entries[ selected ].retval; // valid
            } else if ( return_invalid ) {
                ret = 0 - entries[ selected ].retval; // disabled
            }
        } else if ( keypress == KEY_ESCAPE && return_invalid) { //break loop with ESCAPE key
            break;
        } else {
            if ( keycallback ) {
                skipkey = callback->key( keypress, selected, this );
            }
            if ( ! skipkey && return_invalid ) {
                ret = -1;
            }
        }

        if ( skiprefresh == false ) {
            show();
        }
    } while ( loop && (ret == startret ) );
}

/*
 * cleanup
 */
uimenu::~uimenu()
{
    reset();
}

void uimenu::reset()
{
    if (window.get() != NULL) {
        werase(window.get());
        wrefresh(window.get());
        delwin(window.get());
        window.reset(new WINDOW);
    }

    init();
}

void uimenu::addentry(std::string str)
{
    entries.push_back(str);
}

void uimenu::addentry(int r, bool e, int k, std::string str)
{
    entries.push_back(uimenu_entry(r, e, k, str));
}

void uimenu::addentry(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    const std::string text = vstring_format(format, ap);
    va_end(ap);
    entries.push_back(uimenu_entry(text));
}

void uimenu::addentry(int r, bool e, int k, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    const std::string text = vstring_format(format, ap);
    va_end(ap);
    entries.push_back(uimenu_entry(r, e, k, text));
}

void uimenu::addentry_desc(std::string str, std::string desc)
{
    entries.push_back(uimenu_entry(str, desc));
}

void uimenu::addentry_desc(int r, bool e, int k, std::string str, std::string desc)
{
    entries.push_back(uimenu_entry(r, e, k, str, desc));
}

void uimenu::settext(std::string str)
{
    text = str;
}

void uimenu::settext(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    text = vstring_format(format, ap);
    va_end(ap);
}

pointmenu_cb::pointmenu_cb( const std::vector< tripoint > &pts ) : points( pts )
{
    last = INT_MIN;
    last_view = g->u.view_offset;
}

void pointmenu_cb::select( int /*num*/, uimenu * /*menu*/ ) {
    g->u.view_offset = last_view;
}

void pointmenu_cb::refresh( uimenu *menu ) {
    if( last == menu->selected ) {
        return;
    }
    if( menu->selected < 0 || menu->selected >= (int)points.size() ) {
        last = menu->selected;
        g->u.view_offset = {0, 0, 0};
        g->draw_ter();
        menu->redraw( false ); // show() won't redraw borders
        menu->show();
        return;
    }

    last = menu->selected;
    const tripoint &center = points[menu->selected];
    g->u.view_offset = center - g->u.pos();
    g->u.view_offset.z = 0; // TODO: Remove this line when it's safe
    g->draw_trail_to_square( g->u.view_offset, true);
    menu->redraw( false );
    menu->show();
}
/////////////////////////////////////////////////////////////////////////// }}}1
