#include "list_view.h"

#include "cata_utility.h"
#include "output.h"

template<typename T>
list_view<T>::~list_view() = default;
template<typename T>
list_view<T>::list_view( input_context &ctxt ) : allow_selecting( true )
{
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
}

template<typename T>
void list_view<T>::calc_startpos_with_categories( int &start_pos, int &max_size, const int height )
{
    std::string last_pre_cat = get_element_category( selected_index );
    std::string last_post_cat = last_pre_cat;

    if( max_size < height ) {
        max_size++;
    }

    int start_offset = 0;
    int lines_pre = 1;
    int lines_post = 1;
    bool is_pre = false;
    int pre_counter = 1;
    int post_counter = 1;
    while( lines_pre + lines_post < max_size ) {
        int &counter = is_pre ? pre_counter : post_counter;
        int &lines = is_pre ? lines_pre : lines_post;
        std::string &last_cat = is_pre ? last_pre_cat : last_post_cat;
        int next_index = selected_index + ( is_pre ? -counter : counter );

        bool end_pre = selected_index - pre_counter < 0;
        bool end_post = selected_index + post_counter >= static_cast<int>( list_elements.size() );

        if( end_pre && end_post ) {
            // shouldn't reach here, but adjust max_size just to be safe
            max_size = lines_pre + lines_post;
            break;
        }

        if( next_index >= 0 && next_index < static_cast<int>( list_elements.size() ) ) {
            std::string cat = get_element_category( next_index );
            if( !cat.empty() && cat != last_cat ) {
                last_cat = cat;
                lines++;

                if( max_size < height ) {
                    max_size++;
                }

                if( lines_pre + lines_post >= max_size ) {
                    if( is_pre ) {
                        max_size--;
                    }
                    break;
                }
            }

            if( is_pre ) {
                start_offset++;
            }

            lines++;
        }

        counter++;

        is_pre = !end_pre && ( end_post || lines_post > lines_pre );
    }

    start_pos = selected_index - start_offset;
}

template<typename T>
void list_view<T>::draw( const catacurses::window &w, const point &p, const int width,
                         const int height, const bool )
{
    if( list_elements.empty() ) {
        return;
    }

    int start_pos = 0;
    int max_size = std::min<int>( height, list_elements.size() );
    if( draw_categories ) {
        calc_startpos_with_categories( start_pos, max_size, height );
    } else {
        calcStartPos( start_pos, selected_index, height, list_elements.size() );
    }
    std::string last_category;
    int cat_count = 0;
    for( int i = 0; i + cat_count < max_size; i++ ) {
        // todo: check index
        ui_element *element = list_elements[start_pos + i];
        if( element ) {
            if( draw_categories ) {
                std::string cat = get_element_category( start_pos + i );
                if( !cat.empty() && ( cat != last_category ) ) {
                    last_category = cat;
                    mvwprintz( w, p + point( 1, i + cat_count ), c_magenta, cat );
                    cat_count++;
                    if( i + cat_count >= max_size ) {
                        break;
                    }
                }
            }
            element->draw( w, p + point( 1, i + cat_count ), width, 1, element == get_selected_element() );
        }
    }
}

template<typename T>
T *list_view<T>::get_selected_element()
{
    if( allow_selecting && !list_elements.empty() ) {
        return list_elements[selected_index];
    }
    return nullptr;
}

template<typename T>
int list_view<T>::get_selected_index()
{
    if( !allow_selecting ) {
        return -1;
    }

    return selected_index;
}

template<typename T>
int list_view<T>::get_selected_position()
{
    if( !allow_selecting ) {
        return -1;
    }

    return selected_index + ( draw_categories ? std::distance( categories.begin(),
                              categories.upper_bound( selected_index ) ) - 1  : 0 );
}

template<typename T>
int list_view<T>::get_count()
{
    return list_elements.size();
}

template<typename T>
int list_view<T>::get_count_with_categories()
{
    return list_elements.size() + ( draw_categories ? categories.size() - 1 : 0 );
}

template<typename T>
void list_view<T>::select_next()
{
    if( !allow_selecting || list_elements.empty() ) {
        return;
    }

    selected_index++;

    if( selected_index == static_cast<int>( list_elements.size() ) ) {
        selected_index = 0;
    }
}

template<typename T>
void list_view<T>::select_prev()
{
    if( !allow_selecting || list_elements.empty() ) {
        return;
    }
    if( selected_index == 0 ) {
        selected_index = list_elements.size();
    }

    selected_index--;
}

template<typename T>
void list_view<T>::select_page_up()
{
    if( !allow_selecting || list_elements.empty() ) {
        return;
    }

    if( selected_index == 0 ) {
        selected_index = list_elements.size() - 1;
    } else if( selected_index - page_scroll < 0 ) {
        selected_index = 0;
    } else {
        selected_index -= page_scroll;
    }
}

template<typename T>
void list_view<T>::select_page_down()
{
    if( !allow_selecting || list_elements.empty() ) {
        return;
    }

    const int size = list_elements.size();

    if( selected_index == size - 1 ) {
        selected_index = 0;
    } else if( selected_index + page_scroll >= size ) {
        selected_index = size - 1;
    } else {
        selected_index += page_scroll;
    }
}

template<typename T>
void list_view<T>::set_elements( std::vector<T *> new_list )
{
    selected_index = 0;
    list_elements = new_list;
    calculate_categories();
}

template<typename T>
bool list_view<T>::handle_input( const std::string &action )
{
    if( allow_selecting ) {
        if( action == "UP" ) {
            select_prev();
            return true;
        } else if( action == "DOWN" ) {
            select_next();
            return true;
        } else if( action == "PAGE_UP" ) {
            select_page_up();
            return true;
        } else if( action == "PAGE_DOWN" ) {
            select_page_down();
            return true;
        }
    }
    return false;
}

template<typename T>
bool list_view<T>::toggle_draw_categories()
{
    draw_categories = !draw_categories;
    return draw_categories;
}

template<typename T>
void list_view<T>::calculate_categories()
{
    categories.clear();

    std::string last_cat;
    for( int i = 0; i < static_cast<int>( list_elements.size() ); i++ ) {
        std::string cat = list_elements[i]->get_category();
        if( !cat.empty() && cat != last_cat ) {
            last_cat = cat;
            categories[i] = cat;
        }
    }
}

template<typename T>
std::string list_view<T>::get_element_category( const int index )
{
    if( !categories.empty() ) {
        auto it = categories.upper_bound( index );
        if( it != categories.begin() ) {
            it--;
        }
        return it->second;
    }

    return std::string();
}

template<typename T>
bool list_view<T>::is_drawing_categories()
{
    return draw_categories;
}
