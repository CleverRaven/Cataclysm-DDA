#ifndef CATA_OBJECT_CREATOR_SPELL_WINDOW_H
#define CATA_OBJECT_CREATOR_SPELL_WINDOW_H

#include "dual_list_box.h"
#include "fake_spell_listbox.h"
#include "magic.h"

#include <QtWidgets/qcheckbox.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qgridlayout.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qlistwidget.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qplaintextedit.h>
#include <QtWidgets/qspinbox.h>
#include <QtWidgets/qtablewidget.h>

namespace creator
{
class spell_window : public QWidget
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
        QTextEdit spell_json;

        void write_json();
        void populate_fields();

        QLabel id_label;
        QLineEdit id_box;

        QLabel name_label;
        QLineEdit name_box;

        QLabel description_label;
        QPlainTextEdit description_box;

        QLabel valid_targets_label;
        QListWidget valid_targets_box;

        QLabel energy_cost_label;
        QSpinBox base_energy_cost_box;
        void base_energy_cost_box_textchanged();
        QDoubleSpinBox energy_increment_box;
        QSpinBox final_energy_cost_box;
        void final_energy_cost_box_textchanged();

        QLabel damage_label;
        QSpinBox min_damage_box;
        void min_damage_box_textchanged();
        QDoubleSpinBox damage_increment_box;
        QSpinBox max_damage_box;

        QLabel range_label;
        QSpinBox min_range_box;
        QDoubleSpinBox range_increment_box;
        QSpinBox max_range_box;

        QLabel aoe_label;
        QSpinBox min_aoe_box;
        QDoubleSpinBox aoe_increment_box;
        QSpinBox max_aoe_box;

        QLabel dot_label;
        QSpinBox min_dot_box;
        QDoubleSpinBox dot_increment_box;
        QSpinBox max_dot_box;

        QLabel pierce_label;
        QSpinBox min_pierce_box;
        QDoubleSpinBox pierce_increment_box;
        QSpinBox max_pierce_box;

        QLabel casting_time_label;
        QSpinBox base_casting_time_box;
        void base_casting_time_box_textchanged();
        QDoubleSpinBox casting_time_increment_box;
        QSpinBox final_casting_time_box;
        void final_casting_time_box_textchanged();

        QLabel duration_label;
        QSpinBox min_duration_box;
        QDoubleSpinBox duration_increment_box;
        QSpinBox max_duration_box;

        QLabel spell_flags_label;
        QListWidget spell_flags_box;

        QLabel spell_items_label;
        QListWidget spell_items_box;

        QLabel energy_source_label;
        QComboBox energy_source_box;

        QLabel dmg_type_label;
        QComboBox dmg_type_box;

        QLabel spell_class_label;
        QComboBox spell_class_box;

        QLabel difficulty_label;
        QSpinBox difficulty_box;

        QLabel max_level_label;
        QSpinBox max_level_box;

        QLabel spell_message_label;
        QLineEdit spell_message_box;

        QLabel components_label;
        QComboBox components_box;

        QLabel skill_label;
        QComboBox skill_box;

        QLabel field_id_label;
        QComboBox field_id_box;

        QLabel field_chance_label;
        QSpinBox field_chance_box;

        QLabel min_field_intensity_label;
        QSpinBox min_field_intensity_box;

        QLabel field_intensity_increment_label;
        QDoubleSpinBox field_intensity_increment_box;

        QLabel max_field_intensity_label;
        QSpinBox max_field_intensity_box;

        QLabel field_intensity_variance_label;
        QSpinBox field_intensity_variance_box;

        QLabel affected_bps_label;
        QListWidget affected_bps_box;

        QLabel effect_label;
        QComboBox effect_box;

        QLabel effect_str_label;
        QLineEdit effect_str_box;

        QLabel shape_label;
        QComboBox shape_box;

        QLabel sound_description_label;
        QLineEdit sound_description_box;

        QLabel sound_type_label;
        QComboBox sound_type_box;

        QLabel sound_id_label;
        QLineEdit sound_id_box;

        QLabel sound_variant_label;
        QLineEdit sound_variant_box;

        QLabel sound_ambient_label;
        QCheckBox sound_ambient_box;

        fake_spell_listbox additional_spells_box;

        QLabel targeted_monster_ids_label;
        dual_list_box targeted_monster_ids_box;

        QTableWidget learn_spells_box;
};
}

#endif
