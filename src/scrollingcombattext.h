#ifndef _SCROLLINGCOMBATTEXT_H_
#define _SCROLLINGCOMBATTEXT_H_

#include "color.h"
#include <string>
#include <vector>

class scrollingcombattext {
    private:

    public:
        const int iMaxSteps;

        scrollingcombattext() : iMaxSteps(5) {};
        ~scrollingcombattext() {};

        class cSCT {
            private:
                int iPosX;
                int iPosY;
                int iDir;
                int iStep;
                std::string sText;
                nc_color cColor;

            public:
                cSCT(const int p_iPosX, const int p_iPosY, const int p_iDir, const std::string p_sText, const nc_color p_cColor) {
                    iPosX = p_iPosX;
                    iPosY = p_iPosY;
                    iDir = p_iDir;
                    iStep = 0;
                    sText = p_sText;
                    cColor = p_cColor;
                };
                ~cSCT() {};

                int getNextStep();
                int getPosX();
                int getPosY();
                int getDir();
                std::string getText();
                nc_color getColor();
        };

        std::vector<cSCT> vSCT;

        void add(const int p_iPosX, const int p_iPosY, const int p_iDir, const std::string p_sText, const nc_color p_cColor);
        void advanceStep();
};

extern scrollingcombattext SCT;

#endif
