
#include "string_editor_window.h"

#include "input.h"
#include "output.h"
#include "string_formatter.h"
#include "ui.h"
#include "ui_manager.h"
#include "cursesdef.h"
#include "wcwidth.h"

string_editor_window::string_editor_window(catacurses::window& win, std::string text) {
    //_text = text;
    _win = win;
    _maxx = getmaxx(win);
    _maxy = getmaxy(win);
    _utext = utf8_wrapper(text);
    _foldedtext = foldstring(_utext.str(), _maxx - 1);

}

std::pair<int, int> string_editor_window::get_line_and_position(std::string text, std::vector<std::string> foldedtext, int position) {
    int counter = 0;
    int lineposition;
    int line;
    if (foldedtext.empty()) return std::make_pair(0, 0);
    for (int i = 0; i < foldedtext.size(); i++) {
        utf8_wrapper linetext(foldedtext[i]);
        int temp = linetext.display_width();
        int size = foldedtext[i].size();
        //foldstring, cuts " " away, so it is possible to get a hughe disconect between folded and unfolded string.
        temp += (foldedtext[i].back() != ' ') ? 1 : 0; //|| _foldedtext[i].back() == '\n' AHHHHHHHH vielleicht kann ich das white noice am ende messen.
        //if (_foldedtext[i].back() == ' ' && static_cast<int>(_foldedtext[i].size()) > 1) temp += (_foldedtext[i].substr(_foldedtext[i].size() - 1 , 1 ) == " ") ? 1 : 0;
        /*int temp2 = -1;
        int offset = 0;
        std::string subtext = "";
        do {
            temp2++;
            offset = counter + temp + temp2;
            subtext = _utext.substr(offset, 1);
        } while (offset <=_position && (subtext == " "|| subtext == "\n"));

        temp += temp2;*/


        if (counter + temp > position) {
            //_utext.substr(_position)
            lineposition = position - counter;
            line = i;
            return std::make_pair(line, lineposition);
        }
        else {
            counter += temp;
        }
    }
    return std::make_pair(static_cast<int>(foldedtext.size()), 0);
}


int string_editor_window::get_position(int y, int x) {
    int position = 0;
    int yi = 0;
    int xi = 0;
    int offset = 0;
    //auto _utext = utf8_wrapper(text);
    utf8_wrapper uline;

    /*if (y >=_foldedtext.size()) y = _foldedtext.size();
    if (_foldedtext[yi].size() <= x) x = _foldedtext[yi].size() ;*/
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    do {
        uline = utf8_wrapper(_foldedtext[yi]);
        //uint32_t tst = UTF8_getch(uline.substr(xi, 1));
        //mk_wcwidth(tst) == 0
        if (_foldedtext[yi].size() <= 0) {
            if (y == yi && x == xi) break;
            if (y < yi) {
                position--;
                break;
            }

            position++;
            yi++;
            xi = 0;

        }
        else {
            if ((uline.substr(xi, 1).str() != _utext.substr(position, 1).str())) {
                position++;
            }
            else {
                if (y == yi && x == xi) break;
                if (y < yi) {
                    position--;
                    break;
                }

                position++;
                if (xi == uline.size() - 1) {
                    yi++;
                    xi = 0;
                }
                else {
                    xi++;
                }
            }
        }



    } while (position < _utext.size() && yi < _foldedtext.size());
    if (position < 0) position = 0;
    return position;

}

void string_editor_window::print_editor() {

    /*const int maxx = getmaxx(win);
    const int maxy = getmaxy(win);*/
    //utf8_wrapper _utext(_utext);
    const int middelofpage = _maxy / 2;
    //const auto _foldedtext = _foldedtext;//foldstring(_utext, _maxx-1);
    /*if (_position >= _utext.size()) _position = 0;
    if (_position < 0) _position = _utext.size() - 1;*/

    auto line_position = std::make_pair(_yposition, _xposition);


    int topoflist = 0;
    int bottomoflist = std::min(topoflist + _maxy, static_cast<int>(_foldedtext.size()));
    if (_maxy <= _foldedtext.size()) {
        if (line_position.first > middelofpage) {
            topoflist = line_position.first - middelofpage;
            bottomoflist = topoflist + _maxy;
        }
        if (line_position.first + middelofpage >= _foldedtext.size()) {
            bottomoflist = static_cast<int>(_foldedtext.size());
            topoflist = bottomoflist - _maxy;
        }
    }

    for (int i = topoflist; i < bottomoflist; i++) {
        int y = i - topoflist;
        trim_and_print(_win, point(1, y), _maxx, c_white, _foldedtext[i]);
        if (i == line_position.first) {
            std::string c_cursor = " ";
            if (_position < _utext.size()) {
                utf8_wrapper cursor = _utext.substr(_position, 1);
                size_t a = _position;
                //while (a > 0 && cursor.display_width() == 0) {
                //    // A combination code point, move back to the earliest
                //    // non-combination code point
                //    a--;
                //    cursor = _utext.substr(a, _position - a + 1);
                //}
                if (*cursor.c_str() != '\n') c_cursor = cursor.str();
            }


            mvwprintz(_win, point(line_position.second + 1, y), h_white, "%s", c_cursor);
        }
    }
    if (_foldedtext.size() > _maxy) {
        scrollbar sbar;
        sbar.content_size(_foldedtext.size());
        sbar.viewport_pos(topoflist);
        sbar.viewport_size(_maxy);
        sbar.apply(_win);
    }
}


bool string_editor_window::canceled() const {
    return _canceled;
}

bool string_editor_window::confirmed() const {
    return _confirmed;
}

bool string_editor_window::handled() const {
    return _handled;
}

void string_editor_window::create_context()
{
    ctxt_ptr = std::make_unique<input_context>("STRING_INPUT", keyboard_mode::keychar);
    ctxt = ctxt_ptr.get();
    ctxt->register_action("ANY_INPUT");
}



void string_editor_window::coursour_left(int n) {
    for (int i = 0; i < n; i++) {
        if (_xposition > 0) {
            _xposition--;
        }
        else {
            if (_yposition > 0) {
                _yposition -= 1;
            }
            else {
                _yposition = _foldedtext.size() - 1;
            }
            _xposition = utf8_wrapper(_foldedtext[_yposition]).size() - 1;
        }
    }
}

void string_editor_window::coursour_right(int n) {
    for (int i = 0; i < n; i++) {
        if (utf8_wrapper(_foldedtext[_yposition]).size() - 1 > _xposition) {
            _xposition++;
        }
        else {
            if (_yposition < _foldedtext.size() - 1) {
                _yposition += 1;
            }
            else {
                _yposition = 0;
            }
            _xposition = 0;
        }
    }
}

void string_editor_window::coursour_up(int n) {
    for (int i = 0; i < n; i++) {
        if (_yposition > 0) {
            _yposition -= 1;
        }
        else {
            _yposition = _foldedtext.size() - 1;
        }
        auto size = utf8_wrapper(_foldedtext[_yposition]).size();
        if (size <= _xposition) _xposition = size - 1;
        if (_xposition < 0) _xposition = 0;
    }
}

void string_editor_window::coursour_down(int n) {
    for (int i = 0; i < n; i++) {
        if (_yposition < _foldedtext.size() - 1) {
            _yposition += 1;
        }
        else {
            _yposition = 0;
        }
        auto size = utf8_wrapper(_foldedtext[_yposition]).size();
        if (size <= _xposition) _xposition = size - 1;
        if (_xposition < 0) _xposition = 0;
    }
}

const std::string& string_editor_window::query_string(const bool loop)
{

    if (!ctxt) {
        create_context();
    }

    //utf8_wrapper _utext(_text);
    utf8_wrapper edit(ctxt->get_edittext());
    if (_position == -1) {
        _position = _utext.length();
    }
    //const int scrmax = std::max(0, _endx - _startx);

    //std::unique_ptr<ui_adaptor> ui;


    int ch = 0;

    _canceled = false;
    _confirmed = false;
    do {
        /*if (_position < 0) {
            _position = 0;
        }
        if (_position > _utext.size()) _position = _utext.size();*/
        //_text = _utext.str();
        _foldedtext = foldstring(_utext.str(), _maxx - 1);
        auto line_position = get_line_and_position(_utext.str(), _foldedtext, _position);
        _xposition = line_position.second;
        _yposition = line_position.first;
        //_position = get_position(_text, _foldedtext, _yposition, _xposition);

        /*if (ui) {
            ui_manager::redraw();
        }
        else {*/
        werase(_win);
        print_editor();
        wnoutrefresh(_win);
        //}

        /*if (draw_only) {
            return _utext.str();
        }*/

        const std::string action = ctxt->handle_input();
        const input_event ev = ctxt->get_raw_input();
        ch = ev.type == input_event_t::keyboard_char ? ev.get_first_input() : 0;
        _handled = true;

        if (callbacks[ch]) {
            if (callbacks[ch]()) {
                continue;
            }
        }

        if (_ignore_custom_actions && action != "ANY_INPUT") {
            _handled = false;
            continue;
        }

        if (ch == KEY_ESCAPE) {
#if defined(__ANDROID__)
            if (get_option<bool>("ANDROID_AUTO_KEYBOARD")) {
                SDL_StopTextInput();
            }
#endif
            //_text.clear();
            //_position = -1;
            //_canceled = true;

            return _utext.str();
        }
        /*else if (ch == '\n') {

        }*/
        else if (ch == KEY_UP) {
            _position -= _maxx - 1;
            if (_position < 0) _position = _utext.size();

            /*coursour_up();
            _position = get_position(_yposition,_xposition);*/
        }
        else if (ch == KEY_DOWN) {
            _position += _maxx - 1;
            if (_position > static_cast<int>(_utext.size())) _position = 0;

            /*coursour_down();
            _position = get_position(_yposition, _xposition);*/
        }
        else if (ch == KEY_NPAGE || ch == KEY_PPAGE || ch == KEY_BTAB || ch == '\t') {
            _handled = false;
        }
        else if (ch == KEY_RIGHT) {
            if (_position + 1 <= static_cast<int>(_utext.size())) { //=
                _position++;
            }
            else {
                _position = 0;
            }
            //coursour_right(); 

        }
        else if (ch == KEY_LEFT) {
            if (_position > 0) {
                _position--;
            }
            else {
                _position = _utext.size();
            }
            //coursour_left();

        }
        else if (ch == 0x15) {                      // ctrl-u: delete all the things
            _position = 0;
            _utext.erase(0);
        }
        else if (ch == KEY_BACKSPACE) {
            if (_position > 0 && _position <= static_cast<int>(_utext.size())) {
                _position--;
                _utext.erase(_position, 1);
                //coursour_left();
            }
        }
        else if (ch == KEY_HOME) {
            _yposition = 0;
            _xposition = 0;
            _position = 0;
        }
        else if (ch == KEY_END) {
            _position = _utext.size();
            /*_yposition = _foldedtext.size() - 1;
            _xposition = _foldedtext[_yposition].size() - 1;*/
        }
        else if (ch == KEY_DC) {
            if (_position < static_cast<int>(_utext.size())) {
                _utext.erase(_position, 1);
            }
        }
        else if (ch == 0x16 || ch == KEY_F(2) || !ev.text.empty() || ch == KEY_ENTER || ch == '\n') {
            // ctrl-v, f2, or _utext input
            // bail out early if already at length limit
            //if (_max_length <= 0 || _utext.display_width() < static_cast<size_t>(_max_length)) {
            std::string entered;
            if (ch == 0x16) {
#if defined(TILES)
                if (edit.empty()) {
                    char* const clip = SDL_GetClipboardText();
                    if (clip) {
                        entered = clip;
                        SDL_free(clip);
                    }
                }
#endif
            }
            else if (ch == KEY_F(2)) {
                if (edit.empty()) {
                    entered = get_input_string_from_file();
                }
            }
            else if (ch == KEY_ENTER || ch == '\n') {
                entered = "\n";

            }
            else {
                entered = ev.text;
            }
            if (!entered.empty()) {
                utf8_wrapper insertion;
                const char* str = entered.c_str();
                int len = entered.length();

                while (len > 0) {
                    const uint32_t ch = UTF8_getch(&str, &len);
                    if (ch != '\r') { //ch != '\n' &&


                        insertion.append(utf8_wrapper(utf32_to_utf8(ch)));


                    }
                }
                _utext.insert(_position, insertion);

                ////there was a Bug if " \n" was in the _utext. This is a workaround. 
                //if (*insertion.c_str() == '\n' || *insertion.c_str() == ' ') {
                //    _utext.replace_all(utf8_wrapper("  "), utf8_wrapper(" "));
                //    _utext.replace_all(utf8_wrapper(" \n"), utf8_wrapper("\n"));
                //}

                _position += insertion.length();
                //coursour_right(insertion.length());
                edit = utf8_wrapper();
                ctxt->set_edittext(std::string());

            }
            //}
        }
        else if (ev.edit_refresh) {
            edit = utf8_wrapper(ev.edit);
            ctxt->set_edittext(ev.edit);
        }
        else {
            _handled = false;
        }
    } while (loop);

    return _utext.str();
}
