#ifndef CATA_OBJECT_CREATOR_SPELL_WINDOW_H
#define CATA_OBJECT_CREATOR_SPELL_WINDOW_H

#include "magic.h"

#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qlistwidget.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qplaintextedit.h>
#include <QtWidgets/qspinbox.h>

namespace creator
{
class spell_window : public QMainWindow
{
    public:
        spell_window( QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags() );

        void show() {
            spell_json.show();
            QWidget::show();
        }

        void hide() {
            spell_json.hide();
            QWidget::hide();
        }
    private:
        spell_type editable_spell;
        QLineEdit spell_json;
        QLineEdit error_window;

        void write_json();

        QLineEdit id_label;
        QLineEdit id_box;

        QLineEdit name_label;
        QLineEdit name_box;

        QLineEdit description_label;
        QPlainTextEdit description_box;

        QLineEdit valid_targets_label;
        QListWidget valid_targets_box;

        QLineEdit energy_cost_label;
        QSpinBox base_energy_cost_box;
        QDoubleSpinBox energy_increment_box;
        QSpinBox final_energy_cost_box;

        QLineEdit damage_label;
        QSpinBox min_damage_box;
        QDoubleSpinBox damage_increment_box;
        QSpinBox max_damage_box;

        QLineEdit range_label;
        QSpinBox min_range_box;
        QDoubleSpinBox range_increment_box;
        QSpinBox max_range_box;

        QLineEdit aoe_label;
        QSpinBox min_aoe_box;
        QDoubleSpinBox aoe_increment_box;
        QSpinBox max_aoe_box;

        QLineEdit dot_label;
        QSpinBox min_dot_box;
        QDoubleSpinBox dot_increment_box;
        QSpinBox max_dot_box;

        QLineEdit pierce_label;
        QSpinBox min_pierce_box;
        QDoubleSpinBox pierce_increment_box;
        QSpinBox max_pierce_box;

        QLineEdit casting_time_label;
        QSpinBox base_casting_time_box;
        QDoubleSpinBox casting_time_increment_box;
        QSpinBox final_casting_time_box;

        QLineEdit spell_flags_label;
        QListWidget spell_flags_box;

        QLineEdit energy_source_label;
        QComboBox energy_source_box;

        QLineEdit dmg_type_label;
        QComboBox dmg_type_box;

        QLineEdit affected_bps_label;
        QListWidget affected_bps_box;

        QLineEdit effect_label;
        QComboBox effect_box;

        QLineEdit effect_str_label;
        QLineEdit effect_str_box;
};
}

#endif
