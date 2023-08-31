#ifndef CATA_OBJECT_CREATOR_MOD_SELECTION_WINDOW_H
#define CATA_OBJECT_CREATOR_MOD_SELECTION_WINDOW_H

//Include Qframe, QLabel and QSpinBox for the mod selection window
#include <QtWidgets/qframe.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qspinbox.h>
#include <QtCore/QSettings>

#include "dual_list_box.h"
#include "mod_manager.h"
#include "worldfactory.h"


namespace creator
{
class mod_selection_window : public QWidget
{


    public:
        mod_selection_window( QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags() );

        void show() {
            QWidget::show();
        }

        void hide() {
            QWidget::hide();
        }

    private:
        dual_list_box mods_box;
        QSettings* settings;
};
}

#endif // CATA_OBJECT_CREATOR_MOD_SELECTION_WINDOW_H
