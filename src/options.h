#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm> //atoi

typedef enum { COPT_NO_HIDE,
               COPT_SDL_HIDE,
               COPT_CURSES_HIDE,
               COPT_POSIX_CURSES_HIDE
             } copt_hide_t;

class regional_settings;
class options_data
{
        friend class regional_settings;
    public:
        void add_retry(const std::string &var, const std::string &val);
        void add_value(const std::string &myoption, const std::string &myval, std::string myvaltxt = "" );
        options_data();
    private:
        void enable_json(const std::string &var);
        std::map<std::string, std::string> post_json_verify;
};

class cOpt
{
        friend class options_data;
    public:
        //Default constructor
        cOpt();

        //string constructor
        cOpt(const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn,
             const std::string sItemsIn, std::string sDefaultIn, copt_hide_t opt_hide);

        //bool constructor
        cOpt(const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn,
             const bool bDefaultIn, copt_hide_t opt_hide);

        //int constructor
        cOpt(const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn,
             const int iMinIn, int iMaxIn, int iDefaultIn, copt_hide_t opt_hide);

        //float constructor
        cOpt(const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn,
             const float fMinIn, float fMaxIn, float fDefaultIn, float fStepIn, copt_hide_t opt_hide);

        //Default deconstructor
        ~cOpt() {};

        void setSortPos(const std::string sPageIn);

        //helper functions
        int getSortPos();

        /**
         * Option should be hidden in current build.
         * @return true if option should be hidden, false if not.
         */
        bool is_hidden();

        std::string getPage();
        std::string getMenuText();
        std::string getTooltip();
        std::string getType();

        std::string getValue();
        std::string getValueName();
        std::string getDefaultText(const bool bTranslated = true);

        int getItemPos(const std::string sSearch);

        //set to next item
        void setNext();
        //set to prev item
        void setPrev();
        //set value
        void setValue(std::string sSetIn);
        void setValue(float fSetIn);

        //Set default class behaviour to float
        operator float() const;
        //allow (explicit) boolean conversions
        explicit operator bool() const;
        // if (class == "string")
        bool operator==(const std::string sCompare) const;
        // if (class != "string")
        bool operator!=(const std::string sCompare) const;

    private:
        std::string sPage;
        std::string sMenuText;
        std::string sTooltip;
        std::string sType;

        copt_hide_t hide;
        int iSortPos;

        //sType == "string"
        std::string sSet;
        std::vector<std::string> vItems;
        std::string sDefault;

        //sType == "bool"
        bool bSet;
        bool bDefault;

        //sType == "int"
        int iSet;
        int iMin;
        int iMax;
        int iDefault;

        //sType == "float"
        float fSet;
        float fMin;
        float fMax;
        float fDefault;
        float fStep;
};

extern std::unordered_map<std::string, cOpt> OPTIONS;
extern std::unordered_map<std::string, cOpt> ACTIVE_WORLD_OPTIONS;
extern std::map<int, std::vector<std::string> > mPageItems;
extern int iWorldOptPage;

extern options_data optionsdata;
void initOptions();
void load_options();
void save_options(bool ingame = false);
void show_options(bool ingame = false);

bool use_narrow_sidebar(); // short-circuits to on if terminal is too small

std::string get_tileset_names(std::string dir_path);

#endif
