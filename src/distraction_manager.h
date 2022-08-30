#pragma once
#ifndef CATA_SRC_DISTRACTION_MANAGER_H
#define CATA_SRC_DISTRACTION_MANAGER_H

namespace distraction_manager
{

class distraction_manager_gui
{
    public:
        void show();
};

} // namespace distraction_manager

distraction_manager::distraction_manager_gui &get_distraction_manager();

#endif // CATA_SRC_DISTRACTION_MANAGER_H
