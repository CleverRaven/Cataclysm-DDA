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
            sPage = "";
        };

        //string constructor
        cOpt(const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn, const std::string sItemsIn, std::string sDefaultIn) {
            sPage = sPageIn;
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
        cOpt(const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn, const bool bDefaultIn) {
            sPage = sPageIn;
            sMenuText = sMenuTextIn;
            sTooltip = sTooltipIn;
            sType = "bool";

            bDefault = bDefaultIn;
            bSet = bDefaultIn;
        };

        //int constructor
        cOpt(const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn, const int iMinIn, int iMaxIn, int iDefaultIn) {
            sPage = sPageIn;
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

        //float constructor
        cOpt(const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn,
             const float fMinIn, float fMaxIn, float fDefaultIn, float fStepIn) {
            sPage = sPageIn;
            sMenuText = sMenuTextIn;
            sTooltip = sTooltipIn;
            sType = "float";

            if (fMinIn > fMaxIn) {
                fMaxIn = fMinIn;
            }

            fMin = fMinIn;
            fMax = fMaxIn;
            fStep = fStepIn;

            if (fDefaultIn < fMinIn || fDefaultIn > fMaxIn) {
                fDefaultIn = fMinIn ;
            }

            fDefault = fDefaultIn;
            fSet = fDefaultIn;
        };

        //Default deconstructor
        ~cOpt() {};

        //helper functions
        std::string getPage() {
            return sPage;
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

            } else if (sType == "float") {
                std::stringstream ssTemp;
                ssTemp.precision(1);
                ssTemp << std::fixed << fSet;
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

            } else if (sType == "float") {
                std::stringstream ssTemp;
                ssTemp << fDefault << " - Min: " << fMin << ", Max: " << fMax;
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

            } else if (sType == "float") {
                fSet += fStep;
                if (fSet > fMax) {
                    fSet = fMin;
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

            } else if (sType == "float") {
                fSet -= fStep;
                if (fSet < fMin) {
                    fSet = fMax;
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

                if ( iSet < iMin || iSet > iMax ) {
                    iSet = iDefault;
                }

            } else if (sType == "float") {
                fSet = atof(sSetIn.c_str());

                if ( fSet < fMin || fSet > fMax ) {
                    fSet = fDefault;
                }
            }
        };

        //Set default class behaviour to float
        operator float() const {
            if (sType == "string") {
                return (sSet != "" && sSet == sDefault) ? 1.0 : 0.0;

            } else if (sType == "bool") {
                return (bSet) ? 1.0 : 0.0;

            } else if (sType == "int") {
                return (float)iSet;

            } else if (sType == "float") {
                return fSet;
            }

            return 0.0;
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
        std::string sPage;
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

        //sType == "float"
        float fSet;
        float fMin;
        float fMax;
        float fDefault;
        float fStep;
};

extern std::map<std::string, cOpt> OPTIONS;
extern std::map<std::string, cOpt> ACTIVE_WORLD_OPTIONS;
extern bool awo_populated;

void initOptions();
void load_options();
void save_options();
void save_world_options(std::string world, std::map<std::string, cOpt> world_ops);

#endif
