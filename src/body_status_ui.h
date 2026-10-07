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
    explicit medical_ui(Character* guy) : cataimgui::window(_("Medical"),
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav) {
        you = guy;
    };

    bool execute();
    std::string get_limb_effects(const bodypart_id& part) const;
    std::string get_limb_wounds(const bodypart_id& part) const;
    std::string get_limb_scores_and_modifiers(const bodypart_id& part) const;
    std::string get_modified_limb_name(const bodypart_id& part) const;

    void draw_limbs_list(const std::vector<bodypart_id>& bodyparts);
    void summary_tab() const;
    void limb_tab(const std::vector<bodypart_id>& bodyparts);

    void draw_hint_section();

    std::string last_action;
protected:

    void draw_controls() override;
    cataimgui::bounds get_bounds() override {

        const float window_width = std::clamp(float(str_width_to_pixels(EVEN_MINIMUM_TERM_WIDTH)),
            ImGui::GetMainViewport()->Size.x / 2,
            ImGui::GetMainViewport()->Size.x);
        const float window_height = std::clamp(float(str_height_to_pixels(EVEN_MINIMUM_TERM_HEIGHT)),
            ImGui::GetMainViewport()->Size.y / 2,
            ImGui::GetMainViewport()->Size.y);

        const cataimgui::bounds bounds{ -1.f, -1.f, window_width, window_height };
        return bounds;
    }

private:
    Character* you;
    bodypart_id bp;
    input_context ctxt;
    cataimgui::scroll s = cataimgui::scroll::none;
    medical_tab_mode selected_tab = medical_tab_mode::TAB_LIMBS;

    float get_table_column_width() const {
        return ImGui::GetWindowSize().x / 2;
    }
};

// only for handling additional inputs, draws nothing
class body_status_callback : public uilist_callback
{
    body_status_ui* ui_parent;
public:
    body_status_callback() = default;
    explicit body_status_callback(body_status_ui* parent) : ui_parent(parent) {};
    // handles any non-uilist (character_creator) inputs
    bool key(const input_context& ctxt, const input_event& event, int, uilist* menu) override;
    // this override stores and resets uilist's return value so that the uilist doesn't redraw
    void confirm(uilist* menu) override;
    void select(uilist* menu) override;
    void refresh(uilist* menu) override;
};

struct body_status_uistate {
    body_status_uistate() = default;
    explicit body_status_uistate(std::shared_ptr<Character> displayed_character, bodygraph_id bg_id );

    std::shared_ptr<Character> displayed_character;

    body_status_tab selected_tab = body_status_tab::BS_TAB_SUMMARY;
    body_status_tab switched_tab = body_status_tab::BS_TAB_last;
    
    std::vector<bodypart_id> all_bodyparts;
    int selected_bodypart_index;
    bodygraph_display bodygraph_display_state;

    std::vector<item_location> character_worn;
    // only add clothing to list if it covers the given bodypart; if NULL, ignore
    bodypart_id filter_worn_by = bodypart_str_id::NULL_ID();

    void init();

    // based on selected part
    void update_bodygraph_display();
    
    bool scrolled_up = false;
    bool scrolled_down = false;
    bool quit_to_main_menu = false;
};
