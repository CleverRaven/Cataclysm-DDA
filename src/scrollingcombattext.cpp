
#include "scrollingcombattext.h"

scrollingcombattext SCT;

int scrollingcombattext::cSCT::getNextStep() {
    return ++iStep;
}

int scrollingcombattext::cSCT::getPosX() {
    return iPosX;
}

int scrollingcombattext::cSCT::getPosY() {
    return iPosY;
}

int scrollingcombattext::cSCT::getDir() {
    return iDir;
}

std::string scrollingcombattext::cSCT::getText() {
    return sText;
}

nc_color scrollingcombattext::cSCT::getColor() {
    return cColor;
}

void scrollingcombattext::add(const int p_iPosX, const int p_iPosY, const int p_iDir, const std::string p_sText, const nc_color p_cColor) {
    vSCT.push_back(cSCT(p_iPosX, p_iPosY, p_iDir, p_sText, p_cColor));
}

void scrollingcombattext::advanceStep() {
    std::vector<cSCT>::iterator iter = vSCT.begin();

    while (iter != vSCT.end()) {
        if (iter->getNextStep() >= this->iMaxSteps) {
            iter = vSCT.erase(iter);
        } else {
            ++iter;
        }
    }
}
