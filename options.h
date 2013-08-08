#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <string>
#include <map>
#include <algorithm>

class cOpt
{
    public:
        //Default constructor
        cOpt() {
            sType = "VOID";
            sSet = "";
            bSet = false;
            dSet = 0;
        };

        //string constructor
        cOpt(std::string sCategory, std::string sMenuTextIn, std::string sTooltipIn, std::string sItemsIn, std::string sDefaultIn) {
            sMenuText = sMenuTextIn;
            sTooltip = sTooltipIn;
            sType = "string";

            std::stringstream ssTemp(sItemsIn);
            std::string sItem;
            while (std::getline(ssTemp, sItem, ',')) {
                mItems[sItem] = true;
            }

            if (!mItems[sDefaultIn]) {
                //debugmsg(("Default value '" + sDefaultIn + "' from: " + sMenuTextIn + " is not matching: " + sItemsIn).c_str());
            }

            sDefault = sDefaultIn;
            sSet = sDefaultIn;
        };

        //bool constructor
        cOpt(std::string sCategory, std::string sMenuTextIn, std::string sTooltipIn, bool bDefaultIn) {
            sMenuText = sMenuTextIn;
            sTooltip = sTooltipIn;
            sType = "bool";

            bDefault = bDefaultIn;
            bSet = bDefaultIn;
        };

        //double constructor
        cOpt(std::string sCategory, std::string sMenuTextIn, std::string sTooltipIn, double dMinIn, double dMaxIn, double dDefaultIn) {
            sMenuText = sMenuTextIn;
            sTooltip = sTooltipIn;
            sType = "double";

            if (dMinIn > dMaxIn) {
                //debugmsg(("Min value from: " + sMenuTextIn + " has to be higher than its Max value.").c_str());
            }

            dMin = dMinIn;
            dMax = dMaxIn;

            if (dDefaultIn < dMinIn || dDefaultIn > dMaxIn) {
                //debugmsg(("Default value from: " + sMenuTextIn + " has to be in between its Min and Max value.").c_str());
            }

            dDefault = dDefaultIn;
            dSet = dDefaultIn;
        };

        //Default deconstructor
        ~cOpt() {};

        //set value
        void set(std::string sSetIn) {
            if (sType == "string") {
                if (mItems[sSetIn]) {
                    sSet = sSetIn;
                } else {
                    //debugmsg(("Can't set '" + sSetIn + "' in: " + sMenuText).c_str());
                }

            } else if (sType == "bool") {
                bSet = (sSetIn == "True" || sSetIn == "true" || sSetIn == "T" || sSetIn == "t");

            } else if (sType == "double") {
                dSet = atoi(sSetIn.c_str());
            }
        };

        // if (class == "string")
        bool operator==(const std::string sCompare) const {
            //debugmsg("bool operator==(const std::string sCompare) const {");
            //debugmsg((sSet + " == " + sCompare).c_str());
            if ( sType == "string" && sSet == sCompare ) {
                return true;
            }

            return false;
        };

        // if (class != "string")
        bool operator!=(const std::string sCompare) const {
            return !(*this == sCompare);
        };

/* //Somehow those two override the string compare operators
        // if (class == bool)
        bool operator==(const bool bCompare) const {
            debugmsg("bool operator==(const bool bCompare) const {");
            if ( sType == "bool" && bSet == bCompare ) {
                return true;
            }

            return false;
        };

        // if (class != "bool")
        bool operator!=(const bool bCompare) const {
            return !(*this == bCompare);
        };
*/
        // if (class == "double")
        bool operator==(const double dCompare) const {
            //debugmsg("bool operator==(const double dCompare) const {");
            if ( sType == "double" && dSet == dCompare ) {
                return true;
            }

            return false;
        };

        // if (class != "double")
        bool operator!=(const double dCompare) const {
            return !(*this == dCompare);
        };

        // if (class == "double")
        bool operator<=(const double dCompare) const {
            //debugmsg("bool operator<=(const double dCompare) const {");
            if ( sType == "double" && dSet <= dCompare ) {
                return true;
            }

            return false;
        };

        // if (class >= "double")
        bool operator>=(const double dCompare) const {
            //debugmsg("bool operator>=(const double dCompare) const {");
            if ( sType == "double" && dSet >= dCompare ) {
                return true;
            }

            return false;
        };

        // handle if (class)    //without explicit this would handle <= >= and probably others as well
        explicit operator bool() const {
            //debugmsg("operator bool() const {");
            if (sType == "string") {
                return (sSet != "" && sSet == sDefault);

            } else if (sType == "bool") {
                return bSet;

            } else if (sType == "double") {
                return dSet;
            }

            return false;
        };

        operator int() const {
            //debugmsg("operator int() const {");
            if (sType == "string") {
                return 0;

            } else if (sType == "bool") {
                return (bSet) ? 1 : 0;

            } else if (sType == "double") {
                return (int)dSet;
            }

            return 0;
        };

    private:
        std::string sMenuText;
        std::string sTooltip;
        std::string sType;

        //sType == "string"
        std::string sSet;
        std::map<std::string, bool> mItems;
        std::string sDefault;

        //sType == "double"
        double dSet;
        double dMin;
        double dMax;
        double dDefault;

        //sType == "bool"
        bool bSet;
        bool bDefault;
};

extern std::map<std::string, cOpt> OPTIONS;

void load_options();
void save_options();

void initOptions();

/*
void create_default_options();
std::string options_header();

std::string option_string(option_key key);
std::string option_name(option_key key);
std::string option_desc(option_key key);
*/

#endif
