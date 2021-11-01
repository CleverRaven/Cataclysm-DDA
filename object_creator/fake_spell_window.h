#ifndef CATA_OBJECT_CREATOR_FAKE_SPELL_WINDOW_H
#define CATA_OBJECT_CREATOR_FAKE_SPELL_WINDOW_H

#include "magic.h"

#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qlistwidget.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qspinbox.h>

namespace creator
{
class fake_spell_window : public QMainWindow
{
        Q_OBJECT
    public:
        fake_spell_window( QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags() );

        void show() {
            QWidget::show();
        }

        void hide() {
            QWidget::hide();
        }

        const fake_spell &get_fake_spell() const {
            return editable_spell;
        };

        void set_fake_spell( const fake_spell &sp ) {
            editable_spell = sp;
        }

        void set_update_func( const std::function<void( void )> &func ) {
            update_func = func;
        }

        // the index of this window in the windows vector in fake_spell_listbox
        int index = 0;
    Q_SIGNALS:
        // emits a signal if any attached data is modified
        void modified();
    private:
        std::function<void( void )> update_func = []() {};

        fake_spell editable_spell;

        QLineEdit error_window;

        QLabel id_label;
        QLabel max_level_label;
        QLabel min_level_label;
        QLabel once_in_label;

        QLineEdit id_box;
        QSpinBox max_level_box;
        QSpinBox min_level_box;
        QSpinBox once_in_duration_box;
        QComboBox once_in_units_box;
};
}

#endif
