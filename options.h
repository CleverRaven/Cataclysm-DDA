#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm> //atoi

class cOpt
{
    public:
        //Default constructor
        cOpt() {
            sType = "VOID";
            iPage = -1;
        };

        //string constructor
        cOpt(const int iPageIn, const std::string sMenuTextIn, const std::string sTooltipIn, const std::string sItemsIn, std::string sDefaultIn) {
            iPage = iPageIn;
            sMenuText = sMenuTextIn;
            sTooltip = sTooltipIn;
            sType = "string";

            std::stringstream ssTemp(sItemsIn);
            std::string sItem;
            while (std::getline(ssTemp, sItem, ',')) {
                vItems.push_back(sItem);
            }

            if (getItemPos(sDefaultIn) != -1) {
                sDefaultIn = vItems[0];
            }

            sDefault = sDefaultIn;
            sSet = sDefaultIn;
        };

        //bool constructor
        cOpt(const int iPageIn, const std::string sMenuTextIn, const std::string sTooltipIn, const bool bDefaultIn) {
            iPage = iPageIn;
            sMenuText = sMenuTextIn;
            sTooltip = sTooltipIn;
            sType = "bool";

            bDefault = bDefaultIn;
            bSet = bDefaultIn;
        };

        //int constructor
        cOpt(const int iPageIn, const std::string sMenuTextIn, const std::string sTooltipIn, const int iMinIn, int iMaxIn, int iDefaultIn) {
            iPage = iPageIn;
            sMenuText = sMenuTextIn;
            sTooltip = sTooltipIn;
            sType = "int";

            if (iMinIn > iMaxIn) {
                iMaxIn = iMinIn;
            }

            iMin = iMinIn;
            iMax = iMaxIn;

            if (iDefaultIn < iMinIn || iDefaultIn > iMaxIn) {
                iDefaultIn = iMinIn ;
            }

            iDefault = iDefaultIn;
            iSet = iDefaultIn;
        };

        //Default deconstructor
        ~cOpt() {};

        //helper functions
        int getPage() {
            return iPage;
        };

        std::string getMenuText() {
            return sMenuText;
        };

        std::string getTooltip() {
            return sTooltip;
        };

        std::string getType() {
            return sType;
        };

        std::string getValue() {
            if (sType == "string") {
                return sSet;

            } else if (sType == "bool") {
                return (bSet) ? "True" : "False";

            } else if (sType == "int") {
                std::stringstream ssTemp;
                ssTemp << iSet;
                return ssTemp.str();
            }

            return "";
        };

        std::string getDefaultText() {
            if (sType == "string") {
                std::string sItems = "";
                for (int i = 0; i < vItems.size(); i++) {
                    if (sItems != "") {
                        sItems += ", ";
                    }
                    sItems += vItems[i];
                }
                return sDefault + " - Values: " + sItems;

            } else if (sType == "bool") {
                return (bDefault) ? "True" : "False";

            } else if (sType == "int") {
                std::stringstream ssTemp;
                ssTemp << iDefault << " - Min: " << iMin << ", Max: " << iMax;
                return ssTemp.str();
            }

            return "";
        };

        int getItemPos(const std::string sSearch) {
            if (sType == "string") {
                for (int i = 0; i < vItems.size(); i++) {
                    if (vItems[i] == sSearch) {
                        return i;
                    }
                }
            }

            return -1;
        };

        //set to next item
        void setNext() {
            if (sType == "string") {
                int iNext = getItemPos(sSet)+1;
                if (iNext >= vItems.size()) {
                    iNext = 0;
                }

                sSet = vItems[iNext];

            } else if (sType == "bool") {
                bSet = !bSet;

            } else if (sType == "int") {
                iSet++;
                if (iSet > iMax) {
                    iSet = iMin;
                }
            }
        };

        //set to prev item
        void setPrev() {
            if (sType == "string") {
                int iPrev = getItemPos(sSet)-1;
                if (iPrev < 0) {
                    iPrev = vItems.size()-1;
                }

                sSet = vItems[iPrev];

            } else if (sType == "bool") {
                bSet = !bSet;

            } else if (sType == "int") {
                iSet--;
                if (iSet < iMin) {
                    iSet = iMax;
                }
            }
        };

        //set value
        void setValue(std::string sSetIn) {
            if (sType == "string") {
                if (getItemPos(sSetIn) != -1) {
                    sSet = sSetIn;
                }

            } else if (sType == "bool") {
                bSet = (sSetIn == "True" || sSetIn == "true" || sSetIn == "T" || sSetIn == "t");

            } else if (sType == "int") {
                iSet = atoi(sSetIn.c_str());
            }
        };

        //Set default class behaviour to int
        operator int() const {
            if (sType == "string") {
                return (sSet != "" && sSet == sDefault) ? 1 : 0;

            } else if (sType == "bool") {
                return (bSet) ? 1 : 0;

            } else if (sType == "int") {
                return iSet;
            }

            return 0;
        };

        // if (class == "string")
        bool operator==(const std::string sCompare) const {
            if ( sType == "string" && sSet == sCompare ) {
                return true;
            }

            return false;
        };

        // if (class != "string")
        bool operator!=(const std::string sCompare) const {
            return !(*this == sCompare);
        };

    private:
        int iPage;
        std::string sMenuText;
        std::string sTooltip;
        std::string sType;

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
};

extern std::map<std::string, cOpt> OPTIONS;

void initOptions();
void load_options();
void save_options();

#endif
