#include "bodygraph.h"
#include "cata_imgui.h"
#include "character.h"
#include "input_context.h"
#include "options.h"
#include "uilist.h"

class body_status_ui;

const int BODY_STATUS_UI_TAB_COUNT = 3;

enum class body_status_tab {
    BS_TAB_SUMMARY,
    BS_TAB_MEDICAL,
    BS_TAB_SORT_ARMOR,
    BS_TAB_last
};

template<>
struct enum_traits<body_status_tab> {
    static constexpr body_status_tab first = body_status_tab::BS_TAB_SUMMARY;
    static constexpr body_status_tab last = body_status_tab::BS_TAB_last;
};

enum class medical_tab_mode {
    TAB_LIMBS,
    TAB_SUMMARY,
    last
};

class medical_ui : public cataimgui::window
{
    public:
        explicit medical_ui( Character *guy ) : cataimgui::window( _( "Medical" ),
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav ) {
            you = guy;
        };

        bool execute();
        static std::string get_limb_effects( const Character &you, const bodypart_id &part );
        static std::string get_limb_wounds( const Character &you, const bodypart_id &part );
        static std::string get_limb_scores_and_modifiers( const Character &you,
                const bodypart_id &part );
        static std::string get_modified_limb_name( const Character &you,
                const bodypart_id &part );

        //static void draw_limbs_list(const std::vector<bodypart_id>& bodyparts);
        static void summary_tab( const Character &you );
        static void draw_limbs_details( const Character &you, const bodypart_id &bp );
        static void limb_tab( const std::vector<bodypart_id> &bodyparts );

        void draw_hint_section();

        std::string last_action;
    protected:

        void draw_controls() override;
        cataimgui::bounds get_bounds() override {

            const float window_width = std::clamp( float( str_width_to_pixels( EVEN_MINIMUM_TERM_WIDTH ) ),
                                                   ImGui::GetMainViewport()->Size.x / 2,
                                                   ImGui::GetMainViewport()->Size.x );
            const float window_height = std::clamp( float( str_height_to_pixels( EVEN_MINIMUM_TERM_HEIGHT ) ),
                                                    ImGui::GetMainViewport()->Size.y / 2,
                                                    ImGui::GetMainViewport()->Size.y );

            const cataimgui::bounds bounds{ -1.f, -1.f, window_width, window_height };
            return bounds;
        }

    private:
        Character *you;
        bodypart_id bp;
        input_context ctxt;
        cataimgui::scroll s = cataimgui::scroll::none;
        medical_tab_mode selected_tab = medical_tab_mode::TAB_LIMBS;

        static float get_table_column_width() {
            return ImGui::GetWindowSize().x / 2;
        }
};

// only for handling additional inputs, draws nothing
class body_status_callback : public uilist_callback
{
        body_status_ui *ui_parent;
    public:
        body_status_callback() = default;
        explicit body_status_callback( body_status_ui *parent ) : ui_parent( parent ) {};
        // handles any non-uilist (character_creator) inputs
        bool key( const input_context &ctxt, const input_event &event, int, uilist *menu ) override;
        // this override stores and resets uilist's return value so that the uilist doesn't redraw
        void confirm( uilist *menu ) override;
        void select( uilist *menu ) override;
        void refresh( uilist *menu ) override;
};

using bodygraph_part_tuple =
    std::tuple<bodypart_id, const sub_body_part_type *, const bodygraph_part *, bool>;

// For each tuple:
//   0 = bodypart or parent of subpart
//   1 = subpart (if viewing a sub part)
//   2 = bodygraph data for this part
//   3 = part is present on character
struct bodygraph_part_info {
    bodypart_id bp_id;
    sub_bodypart_str_id sbp_id;
    const bodygraph_part *bg_part;
    bool present_on_character;

    bool operator==( const bodygraph_part_info &other ) const {
        return bp_id == other.bp_id &&
               sbp_id == other.sbp_id &&
               bg_part == other.bg_part &&
               present_on_character == other.present_on_character;
    }
    bool operator!=( const bodygraph_part_info &other ) const {
        return !( *this == other );
    }
    // returns sub-bodypart ID string if present or bodypart ID string otherwise
    std::string get_part() const;
    bodygraph_part_info( bodypart_id bp_id, sub_bodypart_str_id sbp_id,
                         const bodygraph_part *bg_part, bool present_on_character ) :
        bp_id( bp_id ), sbp_id( sbp_id ), bg_part( bg_part ),
        present_on_character( present_on_character ) {};
};

// cache for draw_worn_items_by_limb()
struct armor_text_entry {
    std::optional<std::string> header;

    std::string item_name;
    std::string encumbrance;
    std::string storage;
    std::string hidden;
};

struct body_status_uistate {
    body_status_uistate() = default;
    explicit body_status_uistate( Character &displayed_character, bodygraph_id bg_id );

    Character &displayed_character;

    body_status_tab selected_tab = body_status_tab::BS_TAB_SUMMARY;
    body_status_tab switched_tab = body_status_tab::BS_TAB_last;

    std::string button_action;

    // this differs from `partlist` in that it doesn't correspond to the bodygraph
    std::vector<bodypart_id> all_bodyparts;
    int selected_index = 0;
    int selected_index_set = -1;
    bodypart_id selected_bodypart = bodypart_str_id::NULL_ID();
    item_location selected_armor = item_location::nowhere;
    // if present, a bodypart was selected in the SUMMARY menu and subparts are being displayed
    std::optional<bodypart_id> parent_bodypart = std::nullopt;

    // formerly `bodygraph_display` fields
    bodygraph_id displayed_graph_id;
    bodygraph_var display_mode = bodygraph_var::hp;
    std::vector<bodygraph_part_info> partlist;
    bodygraph_info info;
    // limb summary info text block
    std::vector<std::string> info_txt;
    // some of these should be removed
    int info_width = 0;
    int top_info = 0;
    int blink = 0;
    bool symbol_blinking = false;

    // `all_bodyparts`, but combines parts with opposites into one
    cata::flat_set<bodypart_id> all_bodyparts_compact;
    std::vector<armor_text_entry> worn_items_by_limb_txt;
    // currently selected limb category;
    // only add clothing to list if it covers the given bodypart; if NULL, ignore
    bodypart_id sort_armor_category = bodypart_str_id::NULL_ID();
    int sort_armor_category_index = 0;
    // copy of `outfit::worn` filtered by coverage of `sort_armor_category`
    std::vector<std::list<item>::iterator> worn_subset;
    std::vector<bodypart_id> sort_armor_categories;

    // formerly `bodygraph_display` functions
    void prepare_partlist();
    void prepare_infolist();
    void prepare_infotext( bool reset_pos );
    const bodygraph_part_info &get_selected_partlist() const;

    void update_sorted_armor();
    void prepare_bodyparts_compact();
    void prepare_worn_items_by_limb();
    item_location get_character_worn( int index ) const;

    void init();

    void set_description_scroll();
    void update_blink();
    std::string prepare_bodygraph_text( std::optional<std::string> bodypart_or_subpart_id );

    bool bodygraph_collapsed = false;
    bool scrolled_up = false;
    bool scrolled_down = false;
    bool quit_menu = false;
};

/* handles three different body status windows:
* 1. the tabbed window, for displaying SUMMARY, CLOTHING, MEDICAL lists
* 2. the info window, for displaying information about the tabbed window
* 3. the bodygraph window, for visually displaying the desired custom information
*/
class body_status_ui
{
    protected:
        std::array<std::shared_ptr<uilist>, BODY_STATUS_UI_TAB_COUNT> bs_uilist;
        std::array<input_context, BODY_STATUS_UI_TAB_COUNT> bs_inputs;
        body_status_callback bs_callback;
        shared_ptr_fast<uilist_impl> bs_uilist_current;

    public:
        // public?
        Character &displayed_character;
        body_status_uistate bs_uistate;
        bool reset_list_entries = false;
        bool reset_window_size = false;

        body_status_ui( Character &disp_character ) : displayed_character( disp_character ),
            bs_uistate( disp_character, bodygraph_id::NULL_ID() ) {
            bs_callback = body_status_callback( this );
        };

        //TODO: interface this block with character creator
        // setup new uilist if necessary, assumes current tab has been set
        void setup_new_uilist();
        // update uilist_entries for current tab, but do not modify uilist_entry count
        void update_uilist_entries();
        void upon_switching_tab();
        void update_uilist_position( ImVec2 new_position );
        void setup_input_context( input_context &cc_ictxt );
        // @return action was handled
        bool handle_action( const std::string &action );
        std::shared_ptr<uilist> get_current_tab_uilist();
        std::shared_ptr<uilist> get_tab_uilist( body_status_tab tab );
        void set_current_tab_uilist( const std::shared_ptr<uilist> &new_uilist );
        input_context &get_current_tab_input();
        void set_current_tab_input( const input_context &new_input );

        void display();
};

class body_status_ui_impl : public cataimgui::window
{
        body_status_ui *ui_parent;
    public:
        explicit body_status_ui_impl( body_status_ui *parent );

        // description is drawn entirely based on the selected item in the list; varies per tab
        void draw_description();

        // entirely independent of selected item in the list, though may default unless provided with state data?
        void draw_bodygraph();

        // @param subpart_graph - if true, use subpart instead of bodypart
        void draw_graph_imgui( std::optional<std::string> bodypart_or_subpart_id );
        void draw_info_imgui( const std::vector<std::string> info_txt ) const;
        void draw_worn_items_by_limb();
    protected:
        void draw_controls();
        cataimgui::bounds get_bounds() override;
};
