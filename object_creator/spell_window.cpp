#include "spell_window.h"
#include "collapsing_widget.h"

#include <algorithm>
#include "bodypart.h"
#include "field_type.h"
#include "format.h"
#include "json.h"
#include "magic.h"
#include "monstergenerator.h"
#include "mutation.h"
#include "skill.h"

#include "QtWidgets/qheaderview.h"
#include <QtWidgets/QScrollArea>

#include <sstream>

static spell_type default_spell_type()
{
    spell_type ret;
    ret.sound_type = sounds::sound_t::combat;
    ret.sound_description = to_translation( "an explosion." );
    ret.sound_variant = "default";
    ret.message = to_translation( "You cast %s!" );
    ret.skill = skill_id( "spellcraft" );
    ret.spell_class = trait_id( "NONE" );
    ret.effect_name = "area_pull";
    ret.spell_area = spell_shape::blast;
    ret.min_damage.min.dbl_val = 0;
    ret.max_damage.min.dbl_val = 0;
    ret.damage_increment.min.dbl_val = 0.0f;
    ret.min_accuracy.min.dbl_val = 20;
    ret.accuracy_increment.min.dbl_val = 0.0f;
    ret.max_accuracy.min.dbl_val = 20;
    ret.min_range.min.dbl_val = 0;
    ret.max_range.min.dbl_val = 0;
    ret.range_increment.min.dbl_val = 0.0f;
    ret.min_aoe.min.dbl_val = 0;
    ret.max_aoe.min.dbl_val = 0;
    ret.aoe_increment.min.dbl_val = 0.0f;
    ret.min_dot.min.dbl_val = 0;
    ret.max_dot.min.dbl_val = 0;
    ret.dot_increment.min.dbl_val = 0.0f;
    ret.min_duration.min.dbl_val = 0;
    ret.max_duration.min.dbl_val = 0;
    ret.duration_increment.min.dbl_val = 0.0f;
    ret.min_pierce.min.dbl_val = 0;
    ret.max_pierce.min.dbl_val = 0;
    ret.pierce_increment.min.dbl_val = 0.0f;
    ret.base_energy_cost.min.dbl_val = 0;
    ret.final_energy_cost.min.dbl_val = 0;
    ret.base_energy_cost.min.dbl_val = 0;
    ret.energy_increment.min.dbl_val = 0.0f;
    ret.difficulty.min.dbl_val = 0;
    ret.max_level.min.dbl_val = 0;
    ret.base_casting_time.min.dbl_val = 0;
    ret.final_casting_time.min.dbl_val = 0;
    ret.base_casting_time.min.dbl_val = 0;
    ret.casting_time_increment.min.dbl_val = 0.0f;
    ret.field_chance.min.dbl_val = 1;
    ret.max_field_intensity.min.dbl_val = 0;
    ret.min_field_intensity.min.dbl_val = 0;
    ret.field_intensity_increment.min.dbl_val = 0.0f;
    ret.field_intensity_variance.min.dbl_val = 0.0f;
    return ret;
}

creator::spell_window::spell_window( QWidget *parent, Qt::WindowFlags flags )
    : QWidget ( parent, flags )
{
    editable_spell = default_spell_type();


    QHBoxLayout* mainRow = new QHBoxLayout;
    QVBoxLayout* mainColumn1 = new QVBoxLayout;
    QVBoxLayout* mainColumn2 = new QVBoxLayout;
    QVBoxLayout* mainColumn3 = new QVBoxLayout;

    this->setLayout( mainRow );
    mainRow->addLayout( mainColumn1, 0 );
    mainRow->addLayout( mainColumn2, 1 );
    mainRow->addLayout( mainColumn3, 2 );

    //Add a stretchfactor so that the first column takes 15%, the second 55% and the third 30% of the width
    mainRow->setStretchFactor( mainColumn1, 15 );
    mainRow->setStretchFactor( mainColumn2, 55 );
    mainRow->setStretchFactor( mainColumn3, 30 );


    // =========================================================================================
    // first column of boxes (just the spell list)
    spell_items_box.setToolTip( QString(
                                    _( "Various spells, select one to see the details" ) ) );

    for( const spell_type &sp_t : spell_type::get_all() ) {
        QListWidgetItem *new_item = new QListWidgetItem(
            QString( sp_t.id.c_str() ) );
        spell_items_box.addItem( new_item );
    }
    //When the user selects a spell from the spell list, populate the fields in the form.
    QObject::connect( &spell_items_box, &QListWidget::itemSelectionChanged,
    [&]() {
        spell_window::populate_fields();
    } );

    //Add the spell list to the first column
    mainColumn1->addWidget( &spell_items_box );


    // =========================================================================================
    //Second column of boxes (the form)

    //declare a new scrollarea
    QScrollArea *scrollArea = new QScrollArea( this );
    scrollArea->setMinimumSize( QSize( 430, 600 ) );
    scrollArea->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
    scrollArea->setWidgetResizable( true );

    //Create a frame and add it to the scrollarea
    QFrame *spell_form_frame = new QFrame();
    spell_form_frame->setFrameShape(QFrame::Box);
    spell_form_frame->setFrameShadow(QFrame::Raised);
    QVBoxLayout *spell_form_layout = new QVBoxLayout(spell_form_frame);
    scrollArea->setWidget(spell_form_frame);
    mainColumn2->addWidget(scrollArea);


    // ========================= Basic info ======================================

    id_label.setText( QString( "id" ) );
    id_box.setToolTip( QString( _( "The id of the spell" ) ) );
    QObject::connect( &id_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.id = spell_id( id_box.text().toStdString() );
        write_json();
    } );

    name_label.setText( QString( "name" ) );
    name_box.setToolTip( QString( _( "The name of the spell" ) ) );
    QObject::connect( &name_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.name = translation::no_translation( name_box.text().toStdString() );
        write_json();
    } );

    description_label.setText( QString( "description" ) );
    description_box.setToolTip( QString( _( "The description of the spell" ) ) );
    description_box.setMaximumHeight( 75 );
    description_box.setMinimumHeight( 75 );
    QObject::connect( &description_box, &QPlainTextEdit::textChanged,
    [&]() {
        editable_spell.description = translation::no_translation(
                                         description_box.toPlainText().toStdString() );
        write_json();
    } );

    spell_class_label.setText( QString( "spell class" ) );
    spell_class_box.setToolTip( QString(
                                    _( "The trait required to learn this spell; if the player has a conflicting trait you cannot learn the spell. You gain the trait when you learn the spell." ) ) );
    QStringList all_traits;
    all_traits.append( QString( "NONE" ) );
    for( const mutation_branch &trait : mutation_branch::get_all() ) {
        all_traits.append( QString( trait.id.c_str() ) );
    }
    spell_class_box.addItems( all_traits );
    QObject::connect( &spell_class_box, &QComboBox::currentTextChanged,
    [&]() {
        editable_spell.spell_class = trait_id( spell_class_box.currentText().toStdString() );
        write_json();
    } );

    skill_label.setText( QString( "spell skill" ) );
    skill_box.setToolTip( QString(
                              _( "Uses this skill to calculate spell failure chance." ) ) );
    QStringList all_skills;
    all_skills.append( QString( "NONE" ) );
    for( const Skill &sk : Skill::skills ) {
        all_skills.append( QString( sk.ident().c_str() ) );
    }
    skill_box.addItems( all_skills );
    QObject::connect( &skill_box, &QComboBox::currentTextChanged,
    [&]() {
        editable_spell.skill = skill_id( skill_box.currentText().toStdString() );
        write_json();
    } );
    skill_box.setCurrentText( QString( editable_spell.skill.c_str() ) );

    difficulty_label.setText( QString( "difficulty" ) );
    difficulty_box.setToolTip( QString(
                                   _( "The difficulty of the spell. This affects spell failure chance." ) ) );
    difficulty_box.setMaximum( INT_MAX );
    difficulty_box.setMinimum( 0 );
    QObject::connect( &difficulty_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.difficulty.min.dbl_val = difficulty_box.value();
        write_json();
    } );
    
    max_level_label.setText( QString( "max level" ) );
    max_level_box.setToolTip( QString(
                                  _( "The max level of the spell. Spell level affects a large variety of things, including spell failure chance, damage, etc." ) ) );
    max_level_box.setMaximum( 80 );
    max_level_box.setMinimum( 0 );
    QObject::connect( &max_level_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_level.min.dbl_val = max_level_box.value();
        write_json();
    } );

    //create a new gridlayout to hold the basic info widgets
    QGridLayout *basic_info_layout = new QGridLayout( scrollArea );
    basic_info_layout->addWidget( &id_label, 0, 0 );
    basic_info_layout->addWidget( &id_box, 0, 1 );
    basic_info_layout->addWidget( &name_label, 1, 0 );
    basic_info_layout->addWidget( &name_box, 1, 1 );
    basic_info_layout->addWidget( &description_label, 2, 0 );
    basic_info_layout->addWidget( &description_box, 2, 1 );
    basic_info_layout->addWidget( &spell_class_label, 3, 0 );
    basic_info_layout->addWidget( &spell_class_box, 3, 1 );
    basic_info_layout->addWidget( &skill_label, 4, 0 );
    basic_info_layout->addWidget( &skill_box, 4, 1 );
    basic_info_layout->addWidget( &difficulty_label, 5, 0 );
    basic_info_layout->addWidget( &difficulty_box, 5, 1 );
    basic_info_layout->addWidget( &max_level_label, 6, 0 );
    basic_info_layout->addWidget( &max_level_box, 6, 1 );


    //Add a collapsible widget to show/hide the form elements
    collapsing_widget *basic_info_group = new collapsing_widget( scrollArea, "Basic info", *basic_info_layout );
    spell_form_layout->addWidget( basic_info_group );
    basic_info_group->adjustSize(); //Makes sure the description box gets space to display all its text

    // ========================= Spell effect ======================================

    effect_label.setText( QString( "spell effect" ) );
    effect_box.setParent( this );
    effect_box.setToolTip( QString(
                               _( "The type of effect the spell can have.  See MAGIC.md for details." ) ) );
    QStringList spell_effects;
    for( const auto &spell_effect_pair : spell_effect::effect_map ) {
        spell_effects.append( QString( spell_effect_pair.first.c_str() ) );
    }
    effect_box.addItems( spell_effects );
    QObject::connect( &effect_box, &QComboBox::currentTextChanged,
    [&]() {
        // no need to look up the actual functor, we aren't going to be using that here.
        editable_spell.effect_name = effect_box.currentText().toStdString();
        write_json();
    } );

    effect_str_label.setText( QString( "effect string" ) );
    effect_str_box.setToolTip( QString(
                                   _( "Additional data related to the spell effect.  See MAGIC.md for details." ) ) );
    QObject::connect( &effect_str_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.effect_str = effect_str_box.text().toStdString();
        write_json();
    } );

    shape_label.setText( QString( "spell shape" ) );
    shape_box.setToolTip( QString(
                                   _( "The shape of the spell" ) ) );
    QStringList spell_shapes;
    for( const auto &spell_shape_pair : spell_effect::shape_map ) {
        spell_shapes.append( QString( io::enum_to_string<spell_shape>( spell_shape_pair.first ).c_str() ) );
    }
    shape_box.addItems( spell_shapes );
    QObject::connect( &shape_box, &QComboBox::currentTextChanged,
    [&]() {
        // no need to look up the actual functor, we aren't going to be using that here.
        const std::string shape_string = shape_box.currentText().toStdString();
        for( int i = 0; i < static_cast<int>( spell_shape::num_shapes ); i++ ) {
            const spell_shape cur_shape = static_cast<spell_shape>( i );
            if( io::enum_to_string<spell_shape>( cur_shape ) == shape_string ) {
                editable_spell.spell_area = cur_shape;
                break;
            }
        }
        write_json();
    } );


    //create a new gridlayout to hold the widgets for the spell effects
    QGridLayout *spell_effect_layout = new QGridLayout( scrollArea );
    spell_effect_layout->addWidget( &effect_label, 0, 0 );
    spell_effect_layout->addWidget( &effect_box, 0, 1 );
    spell_effect_layout->addWidget( &effect_str_label, 1, 0 );
    spell_effect_layout->addWidget( &effect_str_box, 1, 1 );
    spell_effect_layout->addWidget( &shape_label, 2, 0 );
    spell_effect_layout->addWidget( &shape_box, 2, 1 );

    //Add a collapsible widget to show/hide the form elements
    collapsing_widget *spell_effect_group = new collapsing_widget( scrollArea, "Spell effect", *spell_effect_layout );
    spell_form_layout->addWidget( spell_effect_group );

    // ========================= Field Properties ======================================

    field_id_label.setText( QString( "field id" ) );
    field_id_box.setToolTip( QString( _( "id of the field that should spawn" ) ) );
    QStringList all_field_types;
    all_field_types.append( QString( "NONE" ) );
    for( const field_type &fd_type : field_types::get_all() ) {
        all_field_types.append( QString( fd_type.id.c_str() ) );
    }
    field_id_box.addItems( all_field_types );
    QObject::connect( &field_id_box, &QComboBox::currentTextChanged,
    [&]() {
        std::string selected = field_id_box.currentText().toStdString();
        field_type_id field;
        if( selected == "NONE" ) {
            editable_spell.field = std::nullopt;
        } else {
            editable_spell.field = field_type_id( selected );
        }
        write_json();
    } );

    field_chance_label.setText( QString( "field chance" ) );
    field_chance_box.setToolTip( QString(
                                     _( "the chance one_in( field_chance ) that the field spawns at a tripoint in the area of the spell. 0 and 1 are 100% chance" ) ) );
    QObject::connect( &field_chance_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.field_chance.min.dbl_val = field_chance_box.value();
        write_json();
    } );

    min_field_intensity_label.setText( QString( "min field intensity" ) );
    min_field_intensity_box.setToolTip( QString( _( "Minimum field intensity" ) ) );
    QObject::connect( &min_field_intensity_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.min_field_intensity.min.dbl_val = min_field_intensity_box.value();
        max_field_intensity_box.setValue( std::max( max_field_intensity_box.value(),
                                          static_cast<int>(editable_spell.min_field_intensity.min.dbl_val.value() ) ) );
        editable_spell.max_field_intensity.min.dbl_val = max_field_intensity_box.value();
        write_json();
    } );

    field_intensity_increment_label.setText( QString( "intensity increment" ) );
    field_intensity_increment_box.setToolTip( QString( _( "Field intensity increment" ) ) );
    field_intensity_increment_box.setMinimum( INT_MIN );
    field_intensity_increment_box.setMaximum( INT_MAX );
    field_intensity_increment_box.setSingleStep( 0.1 );
    QObject::connect( &field_intensity_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.field_intensity_increment.min.dbl_val = field_intensity_increment_box.value();
        write_json();
    } );

    max_field_intensity_label.setText( QString( "max field intensity" ) );
    max_field_intensity_box.setToolTip( QString( _( "Field intensity increment" ) ) );
    QObject::connect( &max_field_intensity_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_field_intensity.min.dbl_val = max_field_intensity_box.value();
        min_field_intensity_box.setValue( std::min( min_field_intensity_box.value(),
                                          max_field_intensity_box.value() ) );
        editable_spell.min_field_intensity.min.dbl_val = min_field_intensity_box.value();
        write_json();
    } );

    field_intensity_variance_label.setText( QString( "intensity variance" ) );
    field_intensity_variance_box.setToolTip( QString(
                _( "field intensity added to the map is +- ( 1 + field_intensity_variance ) * field_intensity" ) ) );
    QObject::connect( &field_intensity_variance_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.field_intensity_variance.min.dbl_val = field_intensity_variance_box.value();
        write_json();
    } );

    //create a new gridlayout to hold the widgets for the spell effects
    QGridLayout *field_properties_layout = new QGridLayout( scrollArea );
    field_properties_layout->addWidget( &field_id_label, 0, 0 );
    field_properties_layout->addWidget( &field_id_box, 0, 1 );
    field_properties_layout->addWidget( &field_chance_label, 1, 0 );
    field_properties_layout->addWidget( &field_chance_box, 1, 1 );
    field_properties_layout->addWidget( &min_field_intensity_label, 2, 0 );
    field_properties_layout->addWidget( &min_field_intensity_box, 2, 1 );
    field_properties_layout->addWidget( &field_intensity_increment_label, 3, 0 );
    field_properties_layout->addWidget( &field_intensity_increment_box, 3, 1 );
    field_properties_layout->addWidget( &max_field_intensity_label, 4, 0 );
    field_properties_layout->addWidget( &max_field_intensity_box, 4, 1 );
    field_properties_layout->addWidget( &field_intensity_variance_label, 5, 0 );
    field_properties_layout->addWidget( &field_intensity_variance_box, 5, 1 );


    //Add a collapsible widget to show/hide the form elements
    collapsing_widget *field_properties_group = new collapsing_widget( scrollArea, "Field properties", *field_properties_layout );
    spell_form_layout->addWidget( field_properties_group );

    // ========================= Spell power ======================================

    energy_cost_label.setText( QString( "energy cost" ) );
    base_energy_cost_box.setToolTip( QString( _( "The energy cost of the spell at level 0" ) ) );
    base_energy_cost_box.setMaximum( INT_MAX );
    base_energy_cost_box.setMinimum( 0 );
    QObject::connect( &base_energy_cost_box, &QSpinBox::textChanged,
    [&]() {
        base_energy_cost_box_textchanged();
    } );

    energy_increment_box.setToolTip( QString(
                                         _( "Amount of energy cost increased per level of the spell" ) ) );
    energy_increment_box.setMaximum( INT_MAX );
    energy_increment_box.setMinimum( INT_MIN );
    QObject::connect( &energy_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.energy_increment.min.dbl_val = energy_increment_box.value();
        write_json();
    } );

    final_energy_cost_box.setToolTip( QString( _( "The maximum energy cost the spell can achieve" ) ) );
    final_energy_cost_box.setMinimum( 0 );
    final_energy_cost_box.setMaximum( INT_MAX );
    QObject::connect( &final_energy_cost_box, &QSpinBox::textChanged,
    [&]() {
        final_energy_cost_box_textchanged();
    } );

    damage_label.setText( QString( "damage" ) );
    min_damage_box.setToolTip( QString( _( "The damage of the spell at level 0" ) ) );
    min_damage_box.setMaximum( INT_MAX );
    min_damage_box.setMinimum( INT_MIN );
    QObject::connect( &min_damage_box, &QSpinBox::textChanged,
    [&]() {
        min_damage_box_textchanged();
    } );

    damage_increment_box.setToolTip( QString(
                                         _( "Amount of damage increased per level of the spell" ) ) );
    damage_increment_box.setMaximum( INT_MAX );
    damage_increment_box.setMinimum( INT_MIN );
    QObject::connect( &damage_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.damage_increment.min.dbl_val = damage_increment_box.value();
        write_json();
    } );

    max_damage_box.setToolTip( QString( _( "The maximum damage the spell can achieve" ) ) );
    max_damage_box.setMaximum( INT_MAX );
    max_damage_box.setMinimum( INT_MIN );
    QObject::connect( &max_damage_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_damage.min.dbl_val = max_damage_box.value();
        min_damage_box.setValue( std::min( min_damage_box.value(), max_damage_box.value() ) );
        editable_spell.min_damage.min.dbl_val = min_damage_box.value();
        write_json();
    } );

    range_label.setText( QString( "range" ) );
    min_range_box.setToolTip( QString( _( "The range of the spell at level 0" ) ) );
    min_range_box.setMaximum( INT_MAX );
    min_range_box.setMinimum( INT_MIN );
    QObject::connect( &min_range_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.min_range.min.dbl_val = min_range_box.value();
        max_range_box.setValue( std::max( max_range_box.value(), static_cast<int>( editable_spell.min_range.min.dbl_val.value() ) ) );
        editable_spell.max_range.min.dbl_val = max_range_box.value();
        write_json();
    } );

    range_increment_box.setToolTip( QString(
                                        _( "Amount of range increased per level of the spell" ) ) );
    range_increment_box.setMaximum( INT_MAX );
    range_increment_box.setMinimum( INT_MIN );
    range_increment_box.setMinimum( 0 );
    QObject::connect( &range_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.range_increment.min.dbl_val = range_increment_box.value();
        write_json();
    } );

    max_range_box.setToolTip( QString( _( "The maximum range the spell can achieve" ) ) );
    max_range_box.setMinimum( 0 );
    QObject::connect( &max_range_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_range.min.dbl_val = max_range_box.value();
        min_range_box.setValue( std::min( min_range_box.value(), max_range_box.value() ) );
        editable_spell.min_range.min.dbl_val = min_range_box.value();
        write_json();
    } );

    aoe_label.setText( QString( "aoe" ) );
    min_aoe_box.setToolTip( QString( _( "The aoe of the spell at level 0" ) ) );
    min_aoe_box.setMaximum( INT_MAX );
    min_aoe_box.setMinimum( INT_MIN );
    QObject::connect( &min_aoe_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.min_aoe.min.dbl_val = min_aoe_box.value();
        max_aoe_box.setValue( std::max( max_aoe_box.value(), static_cast<int>( editable_spell.min_aoe.min.dbl_val.value() ) ) );
        editable_spell.max_aoe.min.dbl_val = max_aoe_box.value();
        write_json();
    } );

    aoe_increment_box.setToolTip( QString( _( "Amount of aoe increased per level of the spell" ) ) );
    aoe_increment_box.setMaximum( INT_MAX );
    aoe_increment_box.setMinimum( INT_MIN );
    aoe_increment_box.setMinimum( 0 );
    QObject::connect( &aoe_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.aoe_increment.min.dbl_val = aoe_increment_box.value();
        write_json();
    } );

    max_aoe_box.setToolTip( QString( _( "The maximum aoe the spell can achieve" ) ) );
    max_aoe_box.setMinimum( 0 );
    QObject::connect( &max_aoe_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_aoe.min.dbl_val = max_aoe_box.value();
        min_aoe_box.setValue( std::min( min_aoe_box.value(), max_aoe_box.value() ) );
        editable_spell.min_aoe.min.dbl_val = min_aoe_box.value();
        write_json();
    } );

    dot_label.setText( QString( "dot" ) );
    min_dot_box.setToolTip( QString( _( "The dot of the spell at level 0" ) ) );
    min_dot_box.setMaximum( INT_MAX );
    min_dot_box.setMinimum( INT_MIN );
    QObject::connect( &min_dot_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.min_dot.min.dbl_val = min_dot_box.value();
        max_dot_box.setValue( std::max( max_dot_box.value(), static_cast<int>( editable_spell.min_dot.min.dbl_val.value() ) ) );
        editable_spell.max_dot.min.dbl_val = max_dot_box.value();
        write_json();
    } );

    dot_increment_box.setToolTip( QString( _( "Amount of dot increased per level of the spell" ) ) );
    dot_increment_box.setMaximum( INT_MAX );
    dot_increment_box.setMinimum( INT_MIN );
    QObject::connect( &dot_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.dot_increment.min.dbl_val = dot_increment_box.value();
        write_json();
    } );

    max_dot_box.setToolTip( QString( _( "The maximum dot the spell can achieve" ) ) );
    max_dot_box.setMaximum( INT_MAX );
    max_dot_box.setMinimum( INT_MIN );
    QObject::connect( &max_dot_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_dot.min.dbl_val = max_dot_box.value();
        min_dot_box.setValue( std::min( min_dot_box.value(), max_dot_box.value() ) );
        editable_spell.min_dot.min.dbl_val = min_dot_box.value();
        write_json();
    } );

    pierce_label.setText( QString( "pierce" ) );
    min_pierce_box.setToolTip( QString( _( "The pierce of the spell at level 0" ) ) );
    // disabled for now as it does nothing in game
    min_pierce_box.setDisabled( true );
    QObject::connect( &min_pierce_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.min_pierce.min.dbl_val = min_pierce_box.value();
        max_pierce_box.setValue( std::max( max_pierce_box.value(), static_cast<int>( editable_spell.min_pierce.min.dbl_val.value() ) ) );
        editable_spell.max_pierce.min.dbl_val = max_pierce_box.value();
        write_json();
    } );

    pierce_increment_box.setToolTip( QString(
                                         _( "Amount of pierce increased per level of the spell" ) ) );
    pierce_increment_box.setMaximum( INT_MAX );
    pierce_increment_box.setMinimum( INT_MIN );
    // disabled for now as it does nothing in game
    pierce_increment_box.setDisabled( true );
    QObject::connect( &pierce_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.pierce_increment.min.dbl_val = pierce_increment_box.value();
        write_json();
    } );

    max_pierce_box.setToolTip( QString( _( "The maximum pierce the spell can achieve" ) ) );
    max_pierce_box.setMinimum( 0 );
    max_pierce_box.setMaximum( INT_MAX );
    // disabled for now as it does nothing in game
    max_pierce_box.setDisabled( true );
    QObject::connect( &max_pierce_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_pierce.min.dbl_val = max_pierce_box.value();
        min_pierce_box.setValue( std::min( min_pierce_box.value(), max_pierce_box.value() ) );
        editable_spell.min_pierce.min.dbl_val = min_pierce_box.value();
        write_json();
    } );

    casting_time_label.setText( QString( "casting time" ) );
    base_casting_time_box.setToolTip( QString( _( "The casting time of the spell at level 0" ) ) );
    base_casting_time_box.setMaximum( INT_MAX );
    base_casting_time_box.setMinimum( 0 );
    QObject::connect( &base_casting_time_box, &QSpinBox::textChanged,
    [&]() {
        base_casting_time_box_textchanged();
    } );
    casting_time_increment_box.setToolTip( QString(
            _( "Amount of casting time increased per level of the spell" ) ) );
    casting_time_increment_box.setMaximum( INT_MAX );
    casting_time_increment_box.setMinimum( INT_MIN );
    QObject::connect( &casting_time_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.casting_time_increment.min.dbl_val = casting_time_increment_box.value();
        write_json();
    } );

    final_casting_time_box.setToolTip( QString(
                                           _( "The maximum casting time the spell can achieve" ) ) );
    final_casting_time_box.setMinimum( 0 );
    final_casting_time_box.setMaximum( INT_MAX );
    QObject::connect( &final_casting_time_box, &QSpinBox::textChanged,
    [&]() {
        final_casting_time_box_textchanged();
    } );

    duration_label.setText( QString( "duration" ) );
    min_duration_box.setToolTip( QString( _( "The duration of the spell at level 0" ) ) );
    min_duration_box.setMaximum( INT_MAX );
    QObject::connect( &min_duration_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.min_duration.min.dbl_val = min_duration_box.value();
        max_duration_box.setValue( std::max( max_duration_box.value(), static_cast<int>( editable_spell.min_duration.min.dbl_val.value() ) ) );
        editable_spell.min_duration.min.dbl_val = max_duration_box.value();
        write_json();
    } );

    duration_increment_box.setToolTip( QString(
                                           _( "Duration increased per level of the spell" ) ) );
    duration_increment_box.setMaximum( INT_MAX );
    duration_increment_box.setMinimum( INT_MIN );
    QObject::connect( &duration_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.duration_increment.min.dbl_val = duration_increment_box.value();
        write_json();
    } );

    max_duration_box.setToolTip( QString( _( "The maximum duration the spell can achieve" ) ) );
    max_duration_box.setMaximum( INT_MAX );
    max_duration_box.setMinimum( INT_MIN );
    QObject::connect( &max_duration_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_duration.min.dbl_val = max_duration_box.value();
        min_duration_box.setValue( std::min( min_duration_box.value(), max_duration_box.value() ) );
        editable_spell.min_duration.min.dbl_val = min_duration_box.value();
        write_json();
    } );

    //create a new gridlayout to hold the widgets for the spell power
    QGridLayout *spell_power_layout = new QGridLayout( scrollArea );
    spell_power_layout->addWidget( &energy_cost_label, 0, 0 );
    spell_power_layout->addWidget( &base_energy_cost_box, 0, 1 );
    spell_power_layout->addWidget( &energy_increment_box, 0, 2 );
    spell_power_layout->addWidget( &final_energy_cost_box, 0, 3 );
    spell_power_layout->addWidget( &damage_label, 1, 0 );
    spell_power_layout->addWidget( &min_damage_box, 1, 1 );
    spell_power_layout->addWidget( &damage_increment_box, 1, 2 );
    spell_power_layout->addWidget( &max_damage_box, 1, 3 );
    spell_power_layout->addWidget( &range_label, 2, 0 );
    spell_power_layout->addWidget( &min_range_box, 2, 1 );
    spell_power_layout->addWidget( &range_increment_box, 2, 2 );
    spell_power_layout->addWidget( &max_range_box, 2, 3 );
    spell_power_layout->addWidget( &aoe_label, 3, 0 );
    spell_power_layout->addWidget( &min_aoe_box, 3, 1 );
    spell_power_layout->addWidget( &aoe_increment_box, 3, 2 );
    spell_power_layout->addWidget( &max_aoe_box, 3, 3 );
    spell_power_layout->addWidget( &dot_label, 4, 0 );
    spell_power_layout->addWidget( &min_dot_box, 4, 1 );
    spell_power_layout->addWidget( &dot_increment_box, 4, 2 );
    spell_power_layout->addWidget( &max_dot_box, 4, 3 );
    spell_power_layout->addWidget( &pierce_label, 5, 0 );
    spell_power_layout->addWidget( &min_pierce_box, 5, 1 );
    spell_power_layout->addWidget( &pierce_increment_box, 5, 2 );
    spell_power_layout->addWidget( &max_pierce_box, 5, 3 );
    spell_power_layout->addWidget( &casting_time_label, 6, 0 );
    spell_power_layout->addWidget( &base_casting_time_box, 6, 1 );
    spell_power_layout->addWidget( &casting_time_increment_box, 6, 2 );
    spell_power_layout->addWidget( &final_casting_time_box, 6, 3 );
    spell_power_layout->addWidget( &duration_label, 7, 0 );
    spell_power_layout->addWidget( &min_duration_box, 7, 1 );
    spell_power_layout->addWidget( &duration_increment_box, 7, 2 );
    spell_power_layout->addWidget( &max_duration_box, 7, 3 );


    //Add a collapsible widget to show/hide the form elements
    collapsing_widget *spell_power_group = new collapsing_widget( scrollArea, "Spell power", *spell_power_layout );
    spell_form_layout->addWidget( spell_power_group );


    // ========================= Spell targets ======================================

    valid_targets_label.setText( QString( "valid_targets" ) );
    valid_targets_box.resize( QSize( 600, 600 ) );
    for( int i = 0; i < static_cast<int>( spell_target::num_spell_targets ); i++ ) {
        QListWidgetItem *new_item = new QListWidgetItem(
            QString( io::enum_to_string( static_cast<spell_target>( i ) ).c_str() ) );
        new_item->setCheckState( Qt::Unchecked );
        valid_targets_box.addItem( new_item );
    }
    QObject::connect( &valid_targets_box, &QListWidget::itemChanged,
    [&]() {
        for( int i = 0; i < static_cast<int>( spell_target::num_spell_targets ); i++ ) {
            editable_spell.valid_targets.set( static_cast<spell_target>( i ),
                                              valid_targets_box.item( i )->checkState() == Qt::CheckState::Checked );
        }
        write_json();
    } );


    affected_bps_label.setText( QString( "affected body parts" ) );
    affected_bps_box.resize( QSize( 600, 600 ) );
    affected_bps_box.setToolTip( QString(
                                     _( "Body parts affected by the spell's effects." ) ) );
    for( const body_part_type &bp : body_part_type::get_all() ) {
        QListWidgetItem *new_item = new QListWidgetItem( QString( bp.id.c_str() ) );
        new_item->setCheckState( Qt::Unchecked );
        affected_bps_box.addItem( new_item );
    }
    QObject::connect( &affected_bps_box, &QListWidget::itemChanged,
    [&]() {
        body_part_set temp;
        for( int row = 0; row < affected_bps_box.count(); row++ ) {
            QListWidgetItem *widget = affected_bps_box.item( row );
            if( widget != nullptr && widget->checkState() == Qt::CheckState::Checked ) {
                temp.set( bodypart_str_id( widget->text().toStdString() ) );
            }
        }
        editable_spell.affected_bps = temp;
        write_json();
    } );

    QStringList all_mtypes;
    for( const mtype &mon : MonsterGenerator::generator().get_all_mtypes() ) {
        all_mtypes.append( QString( mon.id.str().c_str() ) );
    }

    targeted_monster_ids_box.initialize( all_mtypes );
    targeted_monster_ids_box.resize( QSize( 600, 600 ) );
    QObject::connect( &targeted_monster_ids_box, &dual_list_box::pressed, [&]() {
        const QStringList mon_ids = targeted_monster_ids_box.get_included();
        editable_spell.targeted_monster_ids.clear();
        for( const QString &id : mon_ids ) {
            editable_spell.targeted_monster_ids.emplace( mtype_id( id.toStdString() ) );
        }
        write_json();
    } );

    //create a new gridlayout to hold the widgets for the spell targets
    QGridLayout *spell_targets_layout = new QGridLayout( scrollArea );
    spell_targets_layout->addWidget( &valid_targets_label, 0, 0 );
    spell_targets_layout->addWidget( &valid_targets_box, 0, 1 );
    spell_targets_layout->addWidget( &affected_bps_label, 1, 0 );
    spell_targets_layout->addWidget( &affected_bps_box, 1, 1 );
    spell_targets_layout->addWidget( &targeted_monster_ids_box, 2, 0, 1, 2 );


    //Add a collapsible widget to show/hide the form elements
    collapsing_widget *spell_targets_group = new collapsing_widget( scrollArea, "Spell targets", *spell_targets_layout );
    spell_form_layout->addWidget( spell_targets_group );


    // ========================= Spell sounds ======================================



    sound_description_label.setText( QString( "Sound Description" ) );
    sound_description_box.setText( QString( editable_spell.sound_description.translated().c_str() ) );
    QObject::connect( &sound_description_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.sound_description = to_translation( sound_description_box.text().toStdString() );
        write_json();
    } );

    sound_type_label.setText( QString( "Sound Type" ) );
    sound_type_box.setToolTip( QString(
                                     _( "The type of sound for this spell" ) ) );
    QStringList sound_types;
    for( int i = 0; i < static_cast<int>( sounds::sound_t::LAST ); i++ ) {
        sound_types.append( QString( io::enum_to_string( static_cast<sounds::sound_t>( i ) ).c_str() ) );
    }
    sound_type_box.addItems( sound_types );
    sound_type_box.setCurrentIndex( static_cast<int>( sounds::sound_t::combat ) );
    QObject::connect( &sound_type_box, &QComboBox::currentTextChanged,
    [&]() {
        const sounds::sound_t tp = static_cast<sounds::sound_t>( sound_type_box.currentIndex() );
        editable_spell.sound_type = tp;
        write_json();
    } );

    sound_id_label.setText( QString( "Sound ID" ) );
    sound_id_box.setText( QString( editable_spell.sound_id.c_str() ) );
    QObject::connect( &sound_id_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.sound_id = sound_id_box.text().toStdString();
        write_json();
    } );

    sound_variant_label.setText( QString( "Sound Variant" ) );
    sound_variant_box.setText( QString( editable_spell.sound_variant.c_str() ) );
    QObject::connect( &sound_variant_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.sound_variant = sound_variant_box.text().toStdString();
        write_json();
    } );

    sound_ambient_label.setText( QString( "Sound Ambient" ) );
    sound_ambient_box.setChecked( editable_spell.sound_ambient );
    QObject::connect( &sound_ambient_box, &QCheckBox::stateChanged,
    [&]() {
        editable_spell.sound_ambient = sound_ambient_box.checkState();
        write_json();
    } );

    //create a new gridlayout to hold the widgets for the spell targets
    QGridLayout *spell_sound_layout = new QGridLayout( scrollArea );
    spell_sound_layout->addWidget( &sound_description_label, 0, 0 );
    spell_sound_layout->addWidget( &sound_description_box, 0, 1 );
    spell_sound_layout->addWidget( &sound_type_label, 1, 0 );
    spell_sound_layout->addWidget( &sound_type_box, 1, 1 );
    spell_sound_layout->addWidget( &sound_id_label, 2, 0 );
    spell_sound_layout->addWidget( &sound_id_box, 2, 1 );
    spell_sound_layout->addWidget( &sound_variant_label, 3, 0 );
    spell_sound_layout->addWidget( &sound_variant_box, 3, 1 );
    spell_sound_layout->addWidget( &sound_ambient_label, 4, 0 );
    spell_sound_layout->addWidget( &sound_ambient_box, 4, 1 );
    


    //Add a collapsible widget to show/hide the form elements
    collapsing_widget *spell_sounds_group = new collapsing_widget( scrollArea, "Spell sound", *spell_sound_layout );
    spell_form_layout->addWidget( spell_sounds_group );


    // ========================= Spell misc fields ======================================


    energy_source_label.setText( QString( "energy source" ) );
    energy_source_box.setToolTip( QString( _( "The energy source" ) ) );
    QStringList energy_types;
    for( int i = 0; i < static_cast<int>( magic_energy_type::last ); i++ ) {
        energy_types.append( QString( io::enum_to_string( static_cast<magic_energy_type>( i ) ).c_str() ) );
    }
    energy_source_box.addItems( energy_types );
    energy_source_box.setCurrentIndex( static_cast<int>( magic_energy_type::none ) );
    QObject::connect( &energy_source_box, &QComboBox::currentTextChanged,
    [&]() {
        const magic_energy_type tp = static_cast<magic_energy_type>( energy_source_box.currentIndex() );
        editable_spell.energy_source = tp;
        write_json();
    } );

    dmg_type_label.setText( QString( "damage type" ) );
    dmg_type_box.setToolTip( QString( _( "The casting time of the spell at level 0" ) ) );
    QStringList damage_types;
    damage_types.append( QString( damage_type_id::NULL_ID().c_str() ) );
    for( const damage_type &dt : damage_type::get_all() ) {
        damage_types.append( QString( dt.id.c_str() ) );
    }
    dmg_type_box.addItems( damage_types );
    dmg_type_box.setCurrentIndex( 0 );
    QObject::connect( &dmg_type_box, &QComboBox::currentTextChanged,
    [&]() {
        editable_spell.dmg_type = damage_type_id( dmg_type_box.currentText().toStdString() );
        write_json();
    } );

    spell_message_label.setText( QString( "spell message" ) );
    spell_message_box.setToolTip( QString(
                                      _( "The message that displays in the sidebar when the spell is cast. You can use %s to stand in for the name of the spell." ) ) );
    spell_message_box.setText( QString( editable_spell.message.translated().c_str() ) );
    QObject::connect( &spell_message_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.message = to_translation( spell_message_box.text().toStdString() );
        write_json();
    } );

    components_label.setText( QString( "spell components" ) );
    components_box.setToolTip( QString(
                                   _( "The components required in order to cast the spell. Leave empty for no components." ) ) );
    QStringList all_requirements;
    all_requirements.append( QString( "NONE" ) );
    for( const requirement_data &req : requirement_data::get_all() ) {
        all_requirements.append( QString( req.id().c_str() ) );
    }
    components_box.addItems( all_requirements );
    QObject::connect( &components_box, &QComboBox::currentTextChanged,
    [&]() {
        std::string selected = components_box.currentText().toStdString();
        requirement_id rq_components;
        if( selected == "NONE" ) {
            editable_spell.spell_components = rq_components;
        } else {
            editable_spell.spell_components = requirement_id( selected );
        }
        write_json();
    } );


    spell_flags_box.resize( QSize( 100 * 2, 20 * 10 ) );
    spell_flags_box.setToolTip( QString(
                                    _( "Various spell flags.  Please see MAGIC.md, flags.json, and JSON_INFO.md for details." ) ) );
    for( int i = 0; i < static_cast<int>( spell_flag::LAST ); i++ ) {
        QListWidgetItem *new_item = new QListWidgetItem(
            QString( io::enum_to_string( static_cast<spell_flag>( i ) ).c_str() ) );
        new_item->setCheckState( Qt::Unchecked );
        spell_flags_box.addItem( new_item );
    }
    QObject::connect( &spell_flags_box, &QListWidget::itemChanged,
    [&]() {
        for( int i = 0; i < static_cast<int>( spell_flag::LAST ); i++ ) {
            editable_spell.spell_tags.set( static_cast<spell_flag>( i ),
                                           spell_flags_box.item( i )->checkState() == Qt::CheckState::Checked );
        }
        write_json();
    } );

    // learn_spells_box.resize( QSize( 100 * 2, 20 * 6 ) );
    learn_spells_box.insertColumn( 0 );
    learn_spells_box.insertColumn( 0 );
    learn_spells_box.insertRow( 0 );
    learn_spells_box.setHorizontalHeaderLabels( QStringList{ "Spell Id", "Level" } );
    learn_spells_box.verticalHeader()->hide();
    learn_spells_box.horizontalHeader()->resizeSection( 0, 200 * 1.45 );
    // learn_spells_box.horizontalHeader()->resizeSection( 1, 100 / 2.0 );
    learn_spells_box.setSelectionBehavior( QAbstractItemView::SelectionBehavior::SelectItems );
    learn_spells_box.setSelectionMode( QAbstractItemView::SelectionMode::SingleSelection );

    QObject::connect( &learn_spells_box, &QTableWidget::cellChanged, [&]() {
        {
            if( learn_spells_box.selectedItems().length() > 0 ) {
                QTableWidgetItem *cur_widget = learn_spells_box.selectedItems().first();
                const int cur_row = learn_spells_box.row( cur_widget );
                QAbstractItemModel *model = learn_spells_box.model();
                QVariant id_text = model->data( model->index( cur_row, 0 ), Qt::DisplayRole );
                QVariant lvl_text = model->data( model->index( cur_row, 1 ), Qt::DisplayRole );

                const bool cur_row_empty = id_text.toString().isEmpty() && lvl_text.toString().isEmpty();
                const bool last_row = cur_row == learn_spells_box.rowCount() - 1;
                if( last_row && !cur_row_empty ) {
                    learn_spells_box.insertRow( learn_spells_box.rowCount() );
                } else if( !last_row && cur_row_empty ) {
                    learn_spells_box.removeRow( cur_row );
                }
            }
        }

        editable_spell.learn_spells.clear();
        for( int row = 0; row < learn_spells_box.rowCount() - 1; row++ ) {
            QAbstractItemModel *model = learn_spells_box.model();
            QVariant id_text = model->data( model->index( row, 0 ), Qt::DisplayRole );
            QVariant lvl_text = model->data( model->index( row, 1 ), Qt::DisplayRole );
            editable_spell.learn_spells[id_text.toString().toStdString()] = lvl_text.toInt();
        }

        write_json();
    } );

    additional_spells_box.setText( QString( "Additional Spells" ) );
    additional_spells_box.setMaximumHeight( 100 );
    additional_spells_box.set_spells( editable_spell.additional_spells );
    QObject::connect( &additional_spells_box, &fake_spell_listbox::modified,
    [&]() {
        editable_spell.additional_spells = additional_spells_box.get_spells();
        write_json();
    } );

    //create a new gridlayout to hold the widgets for the misc fields
    QGridLayout *spell_misc_layout = new QGridLayout( scrollArea );
    spell_misc_layout->addWidget( &energy_source_label, 0, 0 );
    spell_misc_layout->addWidget( &energy_source_box, 0, 1 );
    spell_misc_layout->addWidget( &dmg_type_label, 1, 0 );
    spell_misc_layout->addWidget( &dmg_type_box, 1, 1 );
    spell_misc_layout->addWidget( &spell_message_label, 2, 0 );
    spell_misc_layout->addWidget( &spell_message_box, 2, 1 );
    spell_misc_layout->addWidget( &components_label, 3, 0 );
    spell_misc_layout->addWidget( &components_box, 3, 1 );
    spell_misc_layout->addWidget( &spell_flags_box, 4, 0, 1, 2 );
    spell_misc_layout->addWidget( &learn_spells_box, 5, 0, 1, 2 );
    spell_misc_layout->addWidget( &additional_spells_box, 6, 0, 1, 2 );


    //Add a collapsible widget to show/hide the form elements
    collapsing_widget *spell_misc_group = new collapsing_widget( scrollArea, "Misc", *spell_misc_layout );
    spell_form_layout->addWidget( spell_misc_group );

    // =========================================================================================
    // third column (just json output)

    spell_json.setParent( this );
    spell_json.setReadOnly( true );

    mainColumn3->addWidget( &spell_json );
}

//When the final_casting_time_box text changes, update the base casting time box
//First check if the energy increment has a value
//if the energy increment is positive, the base casting time should be the minimum of the two
//if the energy increment is negative, the base casting time should be the maximum of the two
void creator::spell_window::final_casting_time_box_textchanged()
{
    editable_spell.final_casting_time.min.dbl_val = final_casting_time_box.value();
    //check if the casting time increment has a value
    if(editable_spell.casting_time_increment.min.dbl_val.has_value()) {
        if( editable_spell.casting_time_increment.min.dbl_val.value() > 0.0f) {
            base_casting_time_box.setValue( std::min( base_casting_time_box.value(),
                                            static_cast<int>( editable_spell.final_casting_time.min.dbl_val.value() ) ) );
        } else if( editable_spell.casting_time_increment.min.dbl_val.value() < 0.0f ) {
            base_casting_time_box.setValue( std::max( base_casting_time_box.value(),
                                            static_cast<int>( editable_spell.final_casting_time.min.dbl_val.value() ) ) );
        } else {
            base_casting_time_box.setValue( editable_spell.final_casting_time.min.dbl_val.value() );
        }
    }
    editable_spell.base_casting_time.min.dbl_val = base_casting_time_box.value();
    write_json();
}


void creator::spell_window::base_energy_cost_box_textchanged()
{
    editable_spell.base_energy_cost.min.dbl_val = base_energy_cost_box.value();
    if( editable_spell.energy_increment.min.dbl_val.has_value() ) {
        if( editable_spell.energy_increment.min.dbl_val.value() > 0.0f ) {
            final_energy_cost_box.setValue( std::max( final_energy_cost_box.value(),
                                            static_cast<int>( editable_spell.base_energy_cost.min.dbl_val.value() ) ) );
        } else if( editable_spell.energy_increment.min.dbl_val.value() < 0.0f ) {
            final_energy_cost_box.setValue( std::min( final_energy_cost_box.value(),
                                            static_cast<int>( editable_spell.base_energy_cost.min.dbl_val.value() ) ) );
        } else {
            final_energy_cost_box.setValue( editable_spell.base_energy_cost.min.dbl_val.value() );
        }
    }
    editable_spell.final_energy_cost.min.dbl_val = final_energy_cost_box.value();
    write_json();
}

void creator::spell_window::base_casting_time_box_textchanged()
{
    editable_spell.base_casting_time.min.dbl_val = base_casting_time_box.value();
    if(editable_spell.casting_time_increment.min.dbl_val.has_value()) {
        if( editable_spell.casting_time_increment.min.dbl_val.value() > 0.0f ) {
            final_casting_time_box.setValue( std::max( final_casting_time_box.value(),
                                             static_cast<int>( editable_spell.base_casting_time.min.dbl_val.value() ) ) );
        } else if( editable_spell.energy_increment.min.dbl_val.value() < 0.0f ) {
            final_casting_time_box.setValue( std::min( final_casting_time_box.value(),
                                             static_cast<int>( editable_spell.base_casting_time.min.dbl_val.value() ) ) );
        } else {
            final_casting_time_box.setValue( editable_spell.base_casting_time.min.dbl_val.value() );
        }
        editable_spell.final_casting_time.min.dbl_val = final_casting_time_box.value();
    }
    write_json();
}


void creator::spell_window::min_damage_box_textchanged()
{
    editable_spell.min_damage.min.dbl_val = min_damage_box.value();
    if(editable_spell.damage_increment.min.dbl_val.has_value()) {
        if( editable_spell.damage_increment.min.dbl_val.value() > 0.0f ) {
            max_damage_box.setValue( std::max( max_damage_box.value(), 
                                        static_cast<int>( editable_spell.min_damage.min.dbl_val.value() ) ) );
        } else if( editable_spell.damage_increment.min.dbl_val.value() < 0.0f ) {
            max_damage_box.setValue( std::min( max_damage_box.value(), 
                                        static_cast<int>( editable_spell.min_damage.min.dbl_val.value() ) ) );
        } else {
            max_damage_box.setValue( editable_spell.min_damage.min.dbl_val.value() );
        }
    }
    editable_spell.max_damage.min.dbl_val = max_damage_box.value();
    write_json();
}

/*
* This enforces rules applied to the energy cost increment values
* If the energy cost increment is positive, the max energy cost should be greater than the min energy cost
* If the energy cost increment is negative, the max energy cost should be less than the min energy cost
* If the energy cost increment is 0, the max energy cost should be equal to the min energy cost
*/
void creator::spell_window::final_energy_cost_box_textchanged()
{
    editable_spell.final_energy_cost.min.dbl_val = final_energy_cost_box.value();
    if( editable_spell.energy_increment.min.dbl_val.has_value() ) {
        if( editable_spell.energy_increment.min.dbl_val.value() > 0.0f) {
            base_energy_cost_box.setValue( std::min( base_energy_cost_box.value(),
                                        static_cast<int>( editable_spell.final_energy_cost.min.dbl_val.value() ) ) );
        } else if( editable_spell.energy_increment.min.dbl_val.value() < 0.0f ) {
            base_energy_cost_box.setValue( std::max( base_energy_cost_box.value(),
                                        static_cast<int>( editable_spell.final_energy_cost.min.dbl_val.value() ) ) );
        } else {
            base_energy_cost_box.setValue( editable_spell.final_energy_cost.min.dbl_val.value() );
        }
    }
    editable_spell.base_energy_cost.min.dbl_val = base_energy_cost_box.value();
    write_json();
}

static std::string check_errors( const spell_type &sp )
{
    std::string errors;
    const auto add_newline = []( std::string & error ) {
        if( !error.empty() ) {
            error += '\n';
        }
    };
    if( sp.id.is_empty() ) {
        errors = "Spell id is empty";
    }
    if( sp.name.empty() ) {
        add_newline( errors );
        errors += "Spell name is empty";
    }
    if( sp.description.empty() ) {
        add_newline( errors );
        errors += "Spell description is empty";
    }
    bool has_targets = false;
    for( int i = 0; i < static_cast<int>( spell_target::num_spell_targets ); i++ ) {
        if( sp.valid_targets.test( static_cast<spell_target>( i ) ) ) {
            has_targets = true;
            break;
        }
    }
    if( !has_targets ) {
        add_newline( errors );
        errors += "Spell has no valid targets";
    }
    return errors;
}

void creator::spell_window::write_json()
{
    const std::string errors = check_errors( editable_spell );

    if( !errors.empty() ) {
        spell_json.setText( QString( errors.c_str() ) );
        return;
    }

    std::ostringstream stream;
    JsonOut jo( stream );
    jo.write( std::vector<spell_type> { editable_spell } );

    std::istringstream in_stream( stream.str() );
    TextJsonIn jsin( in_stream );

    std::ostringstream window_out;
    JsonOut window_jo( window_out, true );

    formatter::format( jsin, window_jo );

    QString output_json{ window_out.str().c_str() };

    spell_json.setText( output_json );
}

void creator::spell_window::populate_fields()
{
    QListWidgetItem *editItem = spell_items_box.currentItem();
    const QString &s = editItem->text();

    for( const spell_type &sp_t : spell_type::get_all() ) {
        if( sp_t.id.c_str() == s ) {

            editable_spell.targeted_monster_ids =
                sp_t.targeted_monster_ids; //Need to set this first to output the right json later

            id_box.setText( QString( sp_t.id.c_str() ) );
            name_box.setText( QString( sp_t.name.translated().c_str() ) );
            description_box.setPlainText( QString( sp_t.description.translated().c_str() ) );

            int index = effect_box.findText( sp_t.effect_name.c_str() );
            if( index != -1 ) {
                effect_box.setCurrentIndex( index );
            }
            effect_str_box.setText( QString( sp_t.effect_str.c_str() ) );
            index = shape_box.findText( QString( io::enum_to_string<spell_shape>( sp_t.spell_area ).c_str() ) );
            if( index != -1 ) {
                shape_box.setCurrentIndex( index );
            }

            for( int i = 0; i < static_cast<int>( spell_target::num_spell_targets ); i++ ) {
                if( sp_t.valid_targets.test( static_cast<spell_target>( i ) ) ) {
                    valid_targets_box.item( i )->setCheckState( Qt::Checked );
                } else {
                    valid_targets_box.item( i )->setCheckState( Qt::Unchecked );
                }
            }

            for( int i = 0; i < static_cast<int>( spell_flag::LAST ); i++ ) {
                if( sp_t.spell_tags.test( static_cast<spell_flag>( i ) ) ) {
                    spell_flags_box.item( i )->setCheckState( Qt::Checked );
                } else {
                    spell_flags_box.item( i )->setCheckState( Qt::Unchecked );
                }
            }


            editable_spell.learn_spells.clear();
            learn_spells_box.clearContents();
            learn_spells_box.setRowCount( 1 );
            std::map<std::string, int> l_spells = sp_t.learn_spells;
            if( !l_spells.empty() ) {
                for( std::map<std::string, int>::iterator it = l_spells.begin();
                     it != l_spells.end(); ++it ) {
                    learn_spells_box.insertRow( 0 );
                    int learn_at_level = it->second;
                    const std::string learn_spell_id = it->first;

                    QTableWidgetItem *item = new QTableWidgetItem();
                    item->setText( QString( learn_spell_id.c_str() ) );
                    learn_spells_box.setCurrentItem( item );
                    learn_spells_box.setItem( 0, 0, item );

                    item = new QTableWidgetItem();
                    item->setText( QString( std::to_string( learn_at_level ).c_str() ) );
                    learn_spells_box.setCurrentItem( item );
                    learn_spells_box.setItem( 0, 1, item );
                }
            }


            base_energy_cost_box.setValue( sp_t.base_energy_cost.min.dbl_val.value() );
            energy_increment_box.setValue( sp_t.energy_increment.min.dbl_val.value() );
            final_energy_cost_box.setValue( sp_t.final_energy_cost.min.dbl_val.value() );
            if(sp_t.min_range.min.is_constant()) {
                min_range_box.setValue( sp_t.min_range.min.dbl_val.value() );
            } else {
                min_range_box.setValue( 0 );
            }
            range_increment_box.setValue( sp_t.range_increment.min.dbl_val.value() );
            if(sp_t.max_range.min.is_constant()) {
                max_range_box.setValue( sp_t.max_range.min.dbl_val.value() );
            } else {
                max_range_box.setValue( 0 );
            }
            
            if(sp_t.min_damage.min.is_constant()) {
                min_damage_box.setValue( sp_t.min_damage.min.dbl_val.value() );
            } else {
                min_damage_box.setValue( 0 );
            }
            damage_increment_box.setValue( sp_t.damage_increment.min.dbl_val.value() );
            if(sp_t.max_damage.min.is_constant()) {
                max_damage_box.setValue( sp_t.max_damage.min.dbl_val.value() );
            } else {
                max_damage_box.setValue( 0 );
            }
            if(sp_t.min_aoe.min.is_constant()) {
                min_aoe_box.setValue( sp_t.min_aoe.min.dbl_val.value() );
            } else {
                min_aoe_box.setValue( 0 );
            }
            aoe_increment_box.setValue( sp_t.aoe_increment.min.dbl_val.value() );
            if(sp_t.max_aoe.min.is_constant()) {
                max_aoe_box.setValue( sp_t.max_aoe.min.dbl_val.value() );
            } else {
                max_aoe_box.setValue( 0 );
            }
            min_dot_box.setValue( sp_t.min_dot.min.dbl_val.value() );
            dot_increment_box.setValue( sp_t.dot_increment.min.dbl_val.value() );
            max_dot_box.setValue( sp_t.max_dot.min.dbl_val.value() );
            if(sp_t.min_duration.min.dbl_val.has_value()) {
                min_duration_box.setValue( sp_t.min_duration.min.dbl_val.value() );
            } else {
                min_duration_box.setValue( 0 );
            }
            duration_increment_box.setValue( sp_t.duration_increment.min.dbl_val.value() );
            if( sp_t.max_duration.min.dbl_val.has_value() ) {
                max_duration_box.setValue( sp_t.max_duration.min.dbl_val.value() );
            } else {
                max_duration_box.setValue( 0 );
            }
            base_casting_time_box.setValue( sp_t.base_casting_time.min.dbl_val.value() );
            casting_time_increment_box.setValue( sp_t.casting_time_increment.min.dbl_val.value() );
            final_casting_time_box.setValue( sp_t.final_casting_time.min.dbl_val.value() );



            index = energy_source_box.findText( QString( io::enum_to_string<magic_energy_type>
                                                ( sp_t.energy_source ).c_str() ) );
            if( index != -1 ) {
                energy_source_box.setCurrentIndex( index );
            }

            index = dmg_type_box.findText( QString( sp_t.dmg_type.is_null() ? "NONE" : sp_t.dmg_type->name.translated().c_str() ) );
            if( index != -1 ) {
                dmg_type_box.setCurrentIndex( index );
            }


            std::string cls = sp_t.spell_class.c_str();
            if( cls != "" ) {
                index = spell_class_box.findText( QString( cls.c_str() ) );
                if( index != -1 ) {
                    spell_class_box.setCurrentIndex( index );
                }
            } else {
                index = spell_class_box.findText( QString( "NONE" ) );
                if( index != -1 ) {
                    spell_class_box.setCurrentIndex( index );
                }
            }


            difficulty_box.setValue( sp_t.difficulty.min.dbl_val.value() );
            max_level_box.setValue( sp_t.max_level.min.dbl_val.value() );
            spell_message_box.setText( QString( sp_t.message.translated().c_str() ) );


            std::string cmp = sp_t.spell_components.c_str();
            if( cmp != "" ) {
                index = components_box.findText( QString( cmp.c_str() ) );
                if( index != -1 ) {
                    components_box.setCurrentIndex( index );
                }
            } else {
                index = components_box.findText( QString( "NONE" ) );
                if( index != -1 ) {
                    components_box.setCurrentIndex( index );
                }
            }

            std::string skill = sp_t.skill.c_str();
            if( skill != "" ) {
                index = skill_box.findText( QString( skill.c_str() ) );
                if( index != -1 ) {
                    skill_box.setCurrentIndex( index );
                }
            } else {
                index = skill_box.findText( QString( "NONE" ) );
                if( index != -1 ) {
                    skill_box.setCurrentIndex( index );
                }
            }

            field_id_box.setCurrentIndex( field_id_box.findText( QString( "NONE" ) ) );
            if( sp_t.field ) {
                QString field = sp_t.field->id().c_str();
                index = field_id_box.findText( field );
                if( index != -1 ) {
                    field_id_box.setCurrentIndex( index );
                }
            }

            field_chance_box.setValue( sp_t.field_chance.min.dbl_val.value() );
            field_intensity_increment_box.setValue( sp_t.field_intensity_increment.min.dbl_val.value() );
            min_field_intensity_box.setValue( sp_t.min_field_intensity.min.dbl_val.value() );
            max_field_intensity_box.setValue( sp_t.max_field_intensity.min.dbl_val.value() );
            field_intensity_variance_box.setValue( sp_t.field_intensity_variance.min.dbl_val.value() );



            for( int row = 0; row < affected_bps_box.count(); row++ ) {
                QListWidgetItem *editItem = affected_bps_box.item( row );
                bodypart_str_id bpid = bodypart_str_id( editItem->text().toStdString() );
                if( sp_t.affected_bps.test( bpid ) ) {
                    editItem->setCheckState( Qt::Checked );
                } else {
                    editItem->setCheckState( Qt::Unchecked );
                }
            }


            sound_description_box.setText( QString( sp_t.sound_description.translated().c_str() ) );
            index = sound_type_box.findText( QString( io::enum_to_string( sp_t.sound_type ).c_str() ) );
            if( index != -1 ) {
                sound_type_box.setCurrentIndex( index );
            }
            sound_id_box.setText( QString( sp_t.sound_id.c_str() ) );
            sound_variant_box.setText( QString( sp_t.sound_variant.c_str() ) );
            sound_ambient_box.setCheckState( Qt::Unchecked );
            if( sp_t.sound_ambient ) {
                sound_ambient_box.setCheckState( Qt::Checked );
            }


            QStringList mon_ids;
            for( const mtype_id &mon_id : sp_t.targeted_monster_ids ) {
                mon_ids.append( mon_id->id.c_str() );
            }
            targeted_monster_ids_box.set_included( mon_ids );

            break;
        }
    }
}
