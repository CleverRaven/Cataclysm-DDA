#include "spell_window.h"

#include "bodypart.h"
#include "field_type.h"
#include "format.h"
#include "json.h"
#include "magic.h"
#include "monstergenerator.h"
#include "mutation.h"
#include "skill.h"

#include "QtWidgets/qheaderview.h"

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
    return ret;
}

creator::spell_window::spell_window( QWidget *parent, Qt::WindowFlags flags )
    : QMainWindow( parent, flags )
{
    editable_spell = default_spell_type();

    const int default_text_box_height = 20;
    const int default_text_box_width = 100;
    const QSize default_text_box_size( default_text_box_width, default_text_box_height );

    int row = 0;
    int col = 0;
    int max_row = 0;
    int max_col = 0;

    spell_json.resize( QSize( 800, 600 ) );
    spell_json.setReadOnly( true );

    id_label.setParent( this );
    id_label.setText( QString( "id" ) );
    id_label.resize( default_text_box_size );
    id_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    id_label.show();

    name_label.setParent( this );
    name_label.setText( QString( "name" ) );
    name_label.resize( default_text_box_size );
    name_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    name_label.show();

    description_label.setParent( this );
    description_label.setText( QString( "description" ) );
    description_label.resize( default_text_box_size );
    description_label.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    description_label.show();
    row += 3;

    effect_label.setParent( this );
    effect_label.setText( QString( "spell effect" ) );
    effect_label.resize( default_text_box_size );
    effect_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    effect_label.show();

    effect_str_label.setParent( this );
    effect_str_label.setText( QString( "effect string" ) );
    effect_str_label.resize( default_text_box_size );
    effect_str_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    effect_str_label.show();

    shape_label.setParent( this );
    shape_label.setText( QString( "spell shape" ) );
    shape_label.resize( default_text_box_size );
    shape_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    shape_label.show();

    valid_targets_label.setParent( this );
    valid_targets_label.setText( QString( "valid_targets" ) );
    valid_targets_label.resize( default_text_box_size );
    valid_targets_label.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    valid_targets_label.show();
    row += static_cast<int>( spell_target::num_spell_targets );

    spell_flags_box.setParent( this );
    spell_flags_box.resize( QSize( default_text_box_width * 2, default_text_box_height * 10 ) );
    spell_flags_box.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    spell_flags_box.setToolTip( QString(
                                    _( "Various spell flags.  Please see MAGIC.md, flags.json, and JSON_INFO.md for details." ) ) );
    spell_flags_box.show();
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
    row += 10;

    learn_spells_box.setParent( this );
    learn_spells_box.resize( QSize( default_text_box_width * 2, default_text_box_height * 6 ) );
    learn_spells_box.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    learn_spells_box.insertColumn( 0 );
    learn_spells_box.insertColumn( 0 );
    learn_spells_box.insertRow( 0 );
    learn_spells_box.setHorizontalHeaderLabels( QStringList{ "Spell Id", "Level" } );
    learn_spells_box.verticalHeader()->hide();
    learn_spells_box.horizontalHeader()->resizeSection( 0, default_text_box_width * 1.45 );
    learn_spells_box.horizontalHeader()->resizeSection( 1, default_text_box_width / 2.0 );
    learn_spells_box.setSelectionBehavior( QAbstractItemView::SelectionBehavior::SelectItems );
    learn_spells_box.setSelectionMode( QAbstractItemView::SelectionMode::SingleSelection );
    learn_spells_box.show();

    QObject::connect( &learn_spells_box, &QTableWidget::cellChanged, [&]() {
        {
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

        editable_spell.learn_spells.clear();
        for( int row = 0; row < learn_spells_box.rowCount() - 1; row++ ) {
            QAbstractItemModel *model = learn_spells_box.model();
            QVariant id_text = model->data( model->index( row, 0 ), Qt::DisplayRole );
            QVariant lvl_text = model->data( model->index( row, 1 ), Qt::DisplayRole );

            editable_spell.learn_spells[id_text.toString().toStdString()] = lvl_text.toInt();
        }

        write_json();
    } );

    row += 6;

    // =========================================================================================
    // second column of boxes
    max_row = std::max( row, max_row );
    row = 0;
    col = 1;

    id_box.setParent( this );
    id_box.resize( default_text_box_size );
    id_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    id_box.setToolTip( QString( _( "The id of the spell" ) ) );
    id_box.show();
    QObject::connect( &id_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.id = spell_id( id_box.text().toStdString() );
        write_json();
    } );

    name_box.setParent( this );
    name_box.resize( default_text_box_size );
    name_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    name_box.show();
    QObject::connect( &name_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.name = translation::no_translation( name_box.text().toStdString() );
        write_json();
    } );

    description_box.setParent( this );
    description_box.resize( QSize( default_text_box_width, default_text_box_height * 3 ) );
    description_box.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    description_box.show();
    QObject::connect( &description_box, &QPlainTextEdit::textChanged,
    [&]() {
        editable_spell.description = translation::no_translation(
                                         description_box.toPlainText().toStdString() );
        write_json();
    } );
    row += 3;

    effect_box.setParent( this );
    effect_box.resize( default_text_box_size );
    effect_box.move( QPoint( col * default_text_box_width,
                             row++ * default_text_box_height ) );
    effect_box.setToolTip( QString(
                               _( "The type of effect the spell can have.  See MAGIC.md for details." ) ) );
    effect_box.show();
    QStringList spell_effects;
    for( const auto spell_effect_pair : spell_effect::effect_map ) {
        spell_effects.append( QString( spell_effect_pair.first.c_str() ) );
    }
    effect_box.addItems( spell_effects );
    QObject::connect( &effect_box, &QComboBox::currentTextChanged,
    [&]() {
        // no need to look up the actual functor, we aren't going to be using that here.
        editable_spell.effect_name = effect_box.currentText().toStdString();
        write_json();
    } );

    effect_str_box.setParent( this );
    effect_str_box.resize( default_text_box_size );
    effect_str_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    effect_str_box.setToolTip( QString(
                                   _( "Additional data related to the spell effect.  See MAGIC.md for details." ) ) );
    effect_str_box.show();
    QObject::connect( &effect_str_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.effect_str = effect_str_box.text().toStdString();
        write_json();
    } );

    shape_box.setParent( this );
    shape_box.resize( default_text_box_size );
    shape_box.move( QPoint( col * default_text_box_width,
                            row++ * default_text_box_height ) );
    QStringList spell_shapes;
    for( const auto spell_shape_pair : spell_effect::shape_map ) {
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

    valid_targets_box.setParent( this );
    valid_targets_box.resize( QSize( default_text_box_width, default_text_box_height *
                                     static_cast<int>( spell_target::num_spell_targets ) ) );
    valid_targets_box.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    valid_targets_box.show();
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
    row += static_cast<int>( spell_target::num_spell_targets );
    // spell flags

    row += 10;
    // =========================================================================================
    // third column of boxes
    max_row = std::max( max_row, row );
    row = 0;
    col++;

    energy_cost_label.setParent( this );
    energy_cost_label.setText( QString( "energy cost" ) );
    energy_cost_label.resize( default_text_box_size );
    energy_cost_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    energy_cost_label.show();

    damage_label.setParent( this );
    damage_label.setText( QString( "damage" ) );
    damage_label.resize( default_text_box_size );
    damage_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    damage_label.show();

    range_label.setParent( this );
    range_label.setText( QString( "range" ) );
    range_label.resize( default_text_box_size );
    range_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    range_label.show();

    aoe_label.setParent( this );
    aoe_label.setText( QString( "aoe" ) );
    aoe_label.resize( default_text_box_size );
    aoe_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    aoe_label.show();

    dot_label.setParent( this );
    dot_label.setText( QString( "dot" ) );
    dot_label.resize( default_text_box_size );
    dot_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    dot_label.show();

    pierce_label.setParent( this );
    pierce_label.setText( QString( "pierce" ) );
    pierce_label.resize( default_text_box_size );
    pierce_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    pierce_label.show();

    casting_time_label.setParent( this );
    casting_time_label.setText( QString( "casting time" ) );
    casting_time_label.resize( default_text_box_size );
    casting_time_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    casting_time_label.show();

    energy_source_label.setParent( this );
    energy_source_label.setText( QString( "energy source" ) );
    energy_source_label.resize( default_text_box_size );
    energy_source_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    energy_source_label.show();

    dmg_type_label.setParent( this );
    dmg_type_label.setText( QString( "damage type" ) );
    dmg_type_label.resize( default_text_box_size );
    dmg_type_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    dmg_type_label.show();

    spell_class_label.setParent( this );
    spell_class_label.setText( QString( "spell class" ) );
    spell_class_label.resize( default_text_box_size );
    spell_class_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    spell_class_label.show();

    difficulty_label.setParent( this );
    difficulty_label.setText( QString( "difficulty" ) );
    difficulty_label.resize( default_text_box_size );
    difficulty_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    difficulty_label.show();

    max_level_label.setParent( this );
    max_level_label.setText( QString( "max level" ) );
    max_level_label.resize( default_text_box_size );
    max_level_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    max_level_label.show();

    spell_message_label.setParent( this );
    spell_message_label.setText( QString( "spell message" ) );
    spell_message_label.resize( default_text_box_size );
    spell_message_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    spell_message_label.show();

    components_label.setParent( this );
    components_label.setText( QString( "spell components" ) );
    components_label.resize( default_text_box_size );
    components_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    components_label.show();

    skill_label.setParent( this );
    skill_label.setText( QString( "spell skill" ) );
    skill_label.resize( default_text_box_size );
    skill_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    skill_label.show();

    field_id_label.setParent( this );
    field_id_label.setText( QString( "field id" ) );
    field_id_label.resize( default_text_box_size );
    field_id_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    field_id_label.show();

    field_chance_label.setParent( this );
    field_chance_label.setText( QString( "field chance" ) );
    field_chance_label.resize( default_text_box_size );
    field_chance_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    field_chance_label.show();

    min_field_intensity_label.setParent( this );
    min_field_intensity_label.setText( QString( "min field intensity" ) );
    min_field_intensity_label.resize( default_text_box_size );
    min_field_intensity_label.move( QPoint( col * default_text_box_width,
                                            row++ * default_text_box_height ) );
    min_field_intensity_label.show();

    field_intensity_increment_label.setParent( this );
    field_intensity_increment_label.setText( QString( "intensity increment" ) );
    field_intensity_increment_label.resize( default_text_box_size );
    field_intensity_increment_label.move( QPoint( col * default_text_box_width,
                                          row++ * default_text_box_height ) );
    field_intensity_increment_label.show();

    max_field_intensity_label.setParent( this );
    max_field_intensity_label.setText( QString( "max field intensity" ) );
    max_field_intensity_label.resize( default_text_box_size );
    max_field_intensity_label.move( QPoint( col * default_text_box_width,
                                            row++ * default_text_box_height ) );
    max_field_intensity_label.show();

    field_intensity_variance_label.setParent( this );
    field_intensity_variance_label.setText( QString( "intensity variance" ) );
    field_intensity_variance_label.resize( default_text_box_size );
    field_intensity_variance_label.move( QPoint( col * default_text_box_width,
                                         row++ * default_text_box_height ) );
    field_intensity_variance_label.show();

    QStringList all_mtypes;
    for( const mtype &mon : MonsterGenerator::generator().get_all_mtypes() ) {
        all_mtypes.append( mon.id.c_str() );
    }

    targeted_monster_ids_box.initialize( all_mtypes );
    targeted_monster_ids_box.resize( QSize( default_text_box_width * 4, default_text_box_height * 6 ) );
    targeted_monster_ids_box.setParent( this );
    targeted_monster_ids_box.move( QPoint( col * default_text_box_width,
                                           row * default_text_box_height ) );
    targeted_monster_ids_box.show();
    QObject::connect( &targeted_monster_ids_box, &dual_list_box::pressed, [&]() {
        const QStringList mon_ids = targeted_monster_ids_box.get_included();
        editable_spell.targeted_monster_ids.clear();
        for( const QString &id : mon_ids ) {
            editable_spell.targeted_monster_ids.emplace( mtype_id( id.toStdString() ) );
        }
        write_json();
    } );
    row += 6;

    // =========================================================================================
    // fourth column of boxes
    max_row = std::max( max_row, row );
    row = 0;
    col++;

    base_energy_cost_box.setParent( this );
    base_energy_cost_box.resize( default_text_box_size );
    base_energy_cost_box.move( QPoint( col * default_text_box_width,
                                       row++ * default_text_box_height ) );
    base_energy_cost_box.setToolTip( QString( _( "The energy cost of the spell at level 0" ) ) );
    base_energy_cost_box.setMaximum( INT_MAX );
    base_energy_cost_box.setMinimum( 0 );
    base_energy_cost_box.show();
    QObject::connect( &base_energy_cost_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.base_energy_cost = base_energy_cost_box.value();
        if( editable_spell.energy_increment > 0.0f ) {
            final_energy_cost_box.setValue( std::max( final_energy_cost_box.value(),
                                            editable_spell.base_energy_cost ) );
        } else if( editable_spell.energy_increment < 0.0f ) {
            final_energy_cost_box.setValue( std::min( final_energy_cost_box.value(),
                                            editable_spell.base_energy_cost ) );
        } else {
            final_energy_cost_box.setValue( editable_spell.base_energy_cost );
        }
        editable_spell.final_energy_cost = final_energy_cost_box.value();
        write_json();
    } );

    min_damage_box.setParent( this );
    min_damage_box.resize( default_text_box_size );
    min_damage_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    min_damage_box.setToolTip( QString( _( "The damage of the spell at level 0" ) ) );
    min_damage_box.setMaximum( INT_MAX );
    min_damage_box.setMinimum( INT_MIN );
    min_damage_box.show();
    QObject::connect( &min_damage_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.min_damage = min_damage_box.value();
        if( editable_spell.damage_increment > 0.0f ) {
            max_damage_box.setValue( std::max( max_damage_box.value(), editable_spell.min_damage ) );
        } else if( editable_spell.damage_increment < 0.0f ) {
            max_damage_box.setValue( std::min( max_damage_box.value(), editable_spell.min_damage ) );
        } else {
            max_damage_box.setValue( editable_spell.min_damage );
        }
        editable_spell.max_damage = max_damage_box.value();
        write_json();
    } );

    min_range_box.setParent( this );
    min_range_box.resize( default_text_box_size );
    min_range_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    min_range_box.setToolTip( QString( _( "The range of the spell at level 0" ) ) );
    min_range_box.setMaximum( INT_MAX );
    min_range_box.setMinimum( INT_MIN );
    min_range_box.show();
    QObject::connect( &min_range_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.min_range = min_range_box.value();
        max_range_box.setValue( std::max( max_range_box.value(), editable_spell.min_range ) );
        editable_spell.max_range = max_range_box.value();
        write_json();
    } );

    min_aoe_box.setParent( this );
    min_aoe_box.resize( default_text_box_size );
    min_aoe_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    min_aoe_box.setToolTip( QString( _( "The aoe of the spell at level 0" ) ) );
    min_aoe_box.setMaximum( INT_MAX );
    min_aoe_box.setMinimum( INT_MIN );
    min_aoe_box.show();
    QObject::connect( &min_aoe_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.min_aoe = min_aoe_box.value();
        max_aoe_box.setValue( std::max( max_aoe_box.value(), editable_spell.min_aoe ) );
        editable_spell.max_aoe = max_aoe_box.value();
        write_json();
    } );

    min_dot_box.setParent( this );
    min_dot_box.resize( default_text_box_size );
    min_dot_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    min_dot_box.setToolTip( QString( _( "The dot of the spell at level 0" ) ) );
    min_dot_box.setMaximum( INT_MAX );
    min_dot_box.setMinimum( INT_MIN );
    min_dot_box.show();
    QObject::connect( &min_dot_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.min_dot = min_dot_box.value();
        max_dot_box.setValue( std::max( max_dot_box.value(), editable_spell.min_dot ) );
        editable_spell.max_dot = max_dot_box.value();
        write_json();
    } );

    min_pierce_box.setParent( this );
    min_pierce_box.resize( default_text_box_size );
    min_pierce_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    min_pierce_box.setToolTip( QString( _( "The pierce of the spell at level 0" ) ) );
    // disabled for now as it does nothing in game
    min_pierce_box.setDisabled( true );
    min_pierce_box.show();
    QObject::connect( &min_pierce_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.min_pierce = min_pierce_box.value();
        max_pierce_box.setValue( std::max( max_pierce_box.value(), editable_spell.min_pierce ) );
        editable_spell.max_pierce = max_pierce_box.value();
        write_json();
    } );

    base_casting_time_box.setParent( this );
    base_casting_time_box.resize( default_text_box_size );
    base_casting_time_box.move( QPoint( col * default_text_box_width,
                                        row++ * default_text_box_height ) );
    base_casting_time_box.setToolTip( QString( _( "The casting time of the spell at level 0" ) ) );
    base_casting_time_box.setMaximum( INT_MAX );
    base_casting_time_box.setMinimum( 0 );
    base_casting_time_box.show();
    QObject::connect( &base_casting_time_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.base_casting_time = base_casting_time_box.value();
        if( editable_spell.casting_time_increment > 0.0f ) {
            final_casting_time_box.setValue( std::max( final_casting_time_box.value(),
                                             editable_spell.base_casting_time ) );
        } else if( editable_spell.energy_increment < 0.0f ) {
            final_casting_time_box.setValue( std::min( final_casting_time_box.value(),
                                             editable_spell.base_casting_time ) );
        } else {
            final_casting_time_box.setValue( editable_spell.base_casting_time );
        }
        editable_spell.final_casting_time = final_casting_time_box.value();
        write_json();
    } );

    energy_source_box.setParent( this );
    energy_source_box.resize( default_text_box_size );
    energy_source_box.move( QPoint( col * default_text_box_width,
                                    row++ * default_text_box_height ) );
    energy_source_box.setToolTip( QString( _( "The casting time of the spell at level 0" ) ) );
    energy_source_box.show();
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

    dmg_type_box.setParent( this );
    dmg_type_box.resize( default_text_box_size );
    dmg_type_box.move( QPoint( col * default_text_box_width,
                               row++ * default_text_box_height ) );
    dmg_type_box.setToolTip( QString( _( "The casting time of the spell at level 0" ) ) );
    dmg_type_box.show();
    QStringList damage_types;
    for( int i = 0; i < static_cast<int>( damage_type::NUM ); i++ ) {
        damage_types.append( QString( io::enum_to_string( static_cast<damage_type>( i ) ).c_str() ) );
    }
    dmg_type_box.addItems( damage_types );
    dmg_type_box.setCurrentIndex( static_cast<int>( damage_type::NONE ) );
    QObject::connect( &dmg_type_box, &QComboBox::currentTextChanged,
    [&]() {
        const damage_type tp = static_cast<damage_type>( dmg_type_box.currentIndex() );
        editable_spell.dmg_type = tp;
        write_json();
    } );

    spell_class_box.setParent( this );
    spell_class_box.resize( default_text_box_size );
    spell_class_box.move( QPoint( col * default_text_box_width,
                                  row++ * default_text_box_height ) );
    spell_class_box.setToolTip( QString(
                                    _( "The trait required to learn this spell; if the player has a conflicting trait you cannot learn the spell. You gain the trait when you learn the spell." ) ) );
    spell_class_box.show();
    QStringList all_traits;
    for( const mutation_branch &trait : mutation_branch::get_all() ) {
        all_traits.append( QString( trait.id.c_str() ) );
    }
    spell_class_box.addItems( all_traits );
    QObject::connect( &spell_class_box, &QComboBox::currentTextChanged,
    [&]() {
        editable_spell.spell_class = trait_id( spell_class_box.currentText().toStdString() );
        write_json();
    } );

    difficulty_box.setParent( this );
    difficulty_box.resize( default_text_box_size );
    difficulty_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    difficulty_box.setToolTip( QString(
                                   _( "The difficulty of the spell. This affects spell failure chance." ) ) );
    difficulty_box.setMaximum( INT_MAX );
    difficulty_box.setMinimum( 0 );
    difficulty_box.show();
    QObject::connect( &difficulty_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.difficulty = difficulty_box.value();
        write_json();
    } );

    max_level_box.setParent( this );
    max_level_box.resize( default_text_box_size );
    max_level_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    max_level_box.setToolTip( QString(
                                  _( "The max level of the spell. Spell level affects a large variety of things, including spell failure chance, damage, etc." ) ) );
    max_level_box.setMaximum( 80 );
    max_level_box.setMinimum( 0 );
    max_level_box.show();
    QObject::connect( &max_level_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_level = max_level_box.value();
        write_json();
    } );

    spell_message_box.setParent( this );
    spell_message_box.resize( default_text_box_size );
    spell_message_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    spell_message_box.setToolTip( QString(
                                      _( "The message that displays in the sidebar when the spell is cast. You can use %s to stand in for the name of the spell." ) ) );
    spell_message_box.setText( QString( editable_spell.message.translated().c_str() ) );
    spell_message_box.show();
    QObject::connect( &spell_message_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.message = to_translation( spell_message_box.text().toStdString() );
        write_json();
    } );

    components_box.setParent( this );
    components_box.resize( default_text_box_size );
    components_box.move( QPoint( col * default_text_box_width,
                                 row++ * default_text_box_height ) );
    components_box.setToolTip( QString(
                                   _( "The components required in order to cast the spell. Leave empty for no components." ) ) );
    components_box.show();
    QStringList all_requirements;
    for( const requirement_data &req : requirement_data::get_all() ) {
        all_requirements.append( QString( req.id().c_str() ) );
    }
    components_box.addItems( all_requirements );
    QObject::connect( &components_box, &QComboBox::currentTextChanged,
    [&]() {
        editable_spell.spell_components = requirement_id( components_box.currentText().toStdString() );
        write_json();
    } );

    skill_box.setParent( this );
    skill_box.resize( default_text_box_size );
    skill_box.move( QPoint( col * default_text_box_width,
                            row++ * default_text_box_height ) );
    skill_box.setToolTip( QString(
                              _( "Uses this skill to calculate spell failure chance." ) ) );
    skill_box.show();
    QStringList all_skills;
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

    field_id_box.setParent( this );
    field_id_box.resize( default_text_box_size );
    field_id_box.move( QPoint( col * default_text_box_width,
                               row++ *default_text_box_height ) );
    field_id_box.show();
    QStringList all_field_types;
    for( const field_type &fd_type : field_types::get_all() ) {
        all_field_types.append( QString( fd_type.id.c_str() ) );
    }
    field_id_box.addItems( all_field_types );
    QObject::connect( &field_id_box, &QComboBox::currentTextChanged,
    [&]() {
        editable_spell.field = field_type_id( field_id_box.currentText().toStdString() );
        write_json();
    } );

    field_chance_box.setParent( this );
    field_chance_box.resize( default_text_box_size );
    field_chance_box.move( QPoint( col * default_text_box_width,
                                   row++ *default_text_box_height ) );
    field_chance_box.show();
    field_chance_box.setToolTip( QString(
                                     _( "the chance one_in( field_chance ) that the field spawns at a tripoint in the area of the spell. 0 and 1 are 100% chance" ) ) );
    QObject::connect( &field_chance_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.field_chance = field_chance_box.value();
        write_json();
    } );

    min_field_intensity_box.setParent( this );
    min_field_intensity_box.resize( default_text_box_size );
    min_field_intensity_box.move( QPoint( col * default_text_box_width,
                                          row++ * default_text_box_height ) );
    min_field_intensity_box.show();
    QObject::connect( &min_field_intensity_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.min_field_intensity = min_field_intensity_box.value();
        max_field_intensity_box.setValue( std::max( max_field_intensity_box.value(),
                                          editable_spell.min_field_intensity ) );
        editable_spell.max_field_intensity = max_field_intensity_box.value();
        write_json();
    } );

    field_intensity_increment_box.setParent( this );
    field_intensity_increment_box.resize( default_text_box_size );
    field_intensity_increment_box.move( QPoint( col * default_text_box_width,
                                        row++ *default_text_box_height ) );
    field_intensity_increment_box.show();
    field_intensity_increment_box.setMinimum( INT_MIN );
    field_intensity_increment_box.setMaximum( INT_MAX );
    field_intensity_increment_box.setSingleStep( 0.1 );
    QObject::connect( &field_intensity_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.field_intensity_increment = field_intensity_increment_box.value();
        write_json();
    } );

    max_field_intensity_box.setParent( this );
    max_field_intensity_box.resize( default_text_box_size );
    max_field_intensity_box.move( QPoint( col * default_text_box_width,
                                          row++ *default_text_box_height ) );
    max_field_intensity_box.show();
    QObject::connect( &max_field_intensity_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_field_intensity = max_field_intensity_box.value();
        min_field_intensity_box.setValue( std::min( min_field_intensity_box.value(),
                                          max_field_intensity_box.value() ) );
        editable_spell.min_field_intensity = min_field_intensity_box.value();
        write_json();
    } );

    field_intensity_variance_box.setParent( this );
    field_intensity_variance_box.resize( default_text_box_size );
    field_intensity_variance_box.move( QPoint( col * default_text_box_width,
                                       row++ * default_text_box_height ) );
    field_intensity_variance_box.show();
    field_intensity_variance_box.setToolTip( QString(
                _( "field intensity added to the map is +- ( 1 + field_intensity_variance ) * field_intensity" ) ) );
    QObject::connect( &field_intensity_variance_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.field_intensity_variance = field_intensity_variance_box.value();
        write_json();
    } );

    // =========================================================================================
    // fifth column of boxes
    max_row = std::max( max_row, row );
    row = 0;
    col++;

    energy_increment_box.setParent( this );
    energy_increment_box.resize( default_text_box_size );
    energy_increment_box.move( QPoint( col * default_text_box_width,
                                       row++ * default_text_box_height ) );
    energy_increment_box.setToolTip( QString(
                                         _( "Amount of energy cost increased per level of the spell" ) ) );
    energy_increment_box.setMaximum( INT_MAX );
    energy_increment_box.setMinimum( INT_MIN );
    energy_increment_box.show();
    QObject::connect( &energy_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.energy_increment = energy_increment_box.value();
        write_json();
    } );

    damage_increment_box.setParent( this );
    damage_increment_box.resize( default_text_box_size );
    damage_increment_box.move( QPoint( col * default_text_box_width,
                                       row++ * default_text_box_height ) );
    damage_increment_box.setToolTip( QString(
                                         _( "Amount of damage increased per level of the spell" ) ) );
    damage_increment_box.setMaximum( INT_MAX );
    damage_increment_box.setMinimum( INT_MIN );
    damage_increment_box.show();
    QObject::connect( &damage_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.damage_increment = damage_increment_box.value();
        write_json();
    } );

    range_increment_box.setParent( this );
    range_increment_box.resize( default_text_box_size );
    range_increment_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    range_increment_box.setToolTip( QString(
                                        _( "Amount of range increased per level of the spell" ) ) );
    range_increment_box.setMaximum( INT_MAX );
    range_increment_box.setMinimum( INT_MIN );
    range_increment_box.setMinimum( 0 );
    range_increment_box.show();
    QObject::connect( &range_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.range_increment = range_increment_box.value();
        write_json();
    } );

    aoe_increment_box.setParent( this );
    aoe_increment_box.resize( default_text_box_size );
    aoe_increment_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    aoe_increment_box.setToolTip( QString( _( "Amount of aoe increased per level of the spell" ) ) );
    aoe_increment_box.setMaximum( INT_MAX );
    aoe_increment_box.setMinimum( INT_MIN );
    aoe_increment_box.setMinimum( 0 );
    aoe_increment_box.show();
    QObject::connect( &aoe_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.aoe_increment = aoe_increment_box.value();
        write_json();
    } );

    dot_increment_box.setParent( this );
    dot_increment_box.resize( default_text_box_size );
    dot_increment_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    dot_increment_box.setToolTip( QString( _( "Amount of dot increased per level of the spell" ) ) );
    dot_increment_box.setMaximum( INT_MAX );
    dot_increment_box.setMinimum( INT_MIN );
    dot_increment_box.show();
    QObject::connect( &dot_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.dot_increment = dot_increment_box.value();
        write_json();
    } );

    pierce_increment_box.setParent( this );
    pierce_increment_box.resize( default_text_box_size );
    pierce_increment_box.move( QPoint( col * default_text_box_width,
                                       row++ * default_text_box_height ) );
    pierce_increment_box.setToolTip( QString(
                                         _( "Amount of pierce increased per level of the spell" ) ) );
    pierce_increment_box.setMaximum( INT_MAX );
    pierce_increment_box.setMinimum( INT_MIN );
    // disabled for now as it does nothing in game
    pierce_increment_box.setDisabled( true );
    pierce_increment_box.show();
    QObject::connect( &pierce_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.pierce_increment = pierce_increment_box.value();
        write_json();
    } );

    casting_time_increment_box.setParent( this );
    casting_time_increment_box.resize( default_text_box_size );
    casting_time_increment_box.move( QPoint( col * default_text_box_width,
                                     row++ * default_text_box_height ) );
    casting_time_increment_box.setToolTip( QString(
            _( "Amount of casting time increased per level of the spell" ) ) );
    casting_time_increment_box.setMaximum( INT_MAX );
    casting_time_increment_box.setMinimum( INT_MIN );
    casting_time_increment_box.show();
    QObject::connect( &casting_time_increment_box, &QDoubleSpinBox::textChanged,
    [&]() {
        editable_spell.casting_time_increment = casting_time_increment_box.value();
        write_json();
    } );

    affected_bps_label.setParent( this );
    affected_bps_label.setText( QString( "affected body parts" ) );
    affected_bps_label.resize( default_text_box_size );
    affected_bps_label.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    affected_bps_label.show();
    row += 4;

    additional_spells_box.setParent( this );
    additional_spells_box.setText( QString( "Additional Spells" ) );
    additional_spells_box.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    additional_spells_box.set_spells( editable_spell.additional_spells );
    QObject::connect( &additional_spells_box, &fake_spell_listbox::modified,
    [&]() {
        editable_spell.additional_spells = additional_spells_box.get_spells();
        write_json();
    } );
    row += 4;

    sound_description_label.setParent( this );
    sound_description_label.setText( QString( "Sound Description" ) );
    sound_description_label.resize( default_text_box_size );
    sound_description_label.move( QPoint( col * default_text_box_width,
                                          row++ * default_text_box_height ) );
    sound_description_label.show();

    sound_type_label.setParent( this );
    sound_type_label.setText( QString( "Sound Type" ) );
    sound_type_label.resize( default_text_box_size );
    sound_type_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    sound_type_label.show();

    sound_id_label.setParent( this );
    sound_id_label.setText( QString( "Sound ID" ) );
    sound_id_label.resize( default_text_box_size );
    sound_id_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    sound_id_label.show();

    sound_variant_label.setParent( this );
    sound_variant_label.setText( QString( "Sound Variant" ) );
    sound_variant_label.resize( default_text_box_size );
    sound_variant_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    sound_variant_label.show();

    sound_ambient_label.setParent( this );
    sound_ambient_label.setText( QString( "Sound Ambient" ) );
    sound_ambient_label.resize( default_text_box_size );
    sound_ambient_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    sound_ambient_label.show();

    // =========================================================================================
    // sixth column of boxes
    max_row = std::max( max_row, row );
    row = 0;
    col++;

    final_energy_cost_box.setParent( this );
    final_energy_cost_box.resize( default_text_box_size );
    final_energy_cost_box.move( QPoint( col * default_text_box_width,
                                        row++ * default_text_box_height ) );
    final_energy_cost_box.setToolTip( QString( _( "The maximum energy cost the spell can achieve" ) ) );
    final_energy_cost_box.setMinimum( 0 );
    final_energy_cost_box.setMaximum( INT_MAX );
    final_energy_cost_box.show();
    QObject::connect( &final_energy_cost_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.final_energy_cost = final_energy_cost_box.value();
        if( editable_spell.energy_increment > 0.0f ) {
            base_energy_cost_box.setValue( std::min( base_energy_cost_box.value(),
                                           editable_spell.final_energy_cost ) );
        } else if( editable_spell.energy_increment < 0.0f ) {
            base_energy_cost_box.setValue( std::max( base_energy_cost_box.value(),
                                           editable_spell.final_energy_cost ) );
        } else {
            base_energy_cost_box.setValue( editable_spell.final_energy_cost );
        }
        editable_spell.base_energy_cost = base_energy_cost_box.value();
        write_json();
    } );

    max_damage_box.setParent( this );
    max_damage_box.resize( default_text_box_size );
    max_damage_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    max_damage_box.setToolTip( QString( _( "The maximum damage the spell can achieve" ) ) );
    max_damage_box.setMaximum( INT_MAX );
    max_damage_box.setMinimum( INT_MIN );
    max_damage_box.show();
    QObject::connect( &max_damage_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_damage = max_damage_box.value();
        min_damage_box.setValue( std::min( min_damage_box.value(), max_damage_box.value() ) );
        editable_spell.min_damage = min_damage_box.value();
        write_json();
    } );

    max_range_box.setParent( this );
    max_range_box.resize( default_text_box_size );
    max_range_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    max_range_box.setToolTip( QString( _( "The maximum range the spell can achieve" ) ) );
    max_range_box.setMinimum( 0 );
    max_range_box.show();
    QObject::connect( &max_range_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_range = max_range_box.value();
        min_range_box.setValue( std::min( min_range_box.value(), max_range_box.value() ) );
        editable_spell.min_range = min_range_box.value();
        write_json();
    } );

    max_aoe_box.setParent( this );
    max_aoe_box.resize( default_text_box_size );
    max_aoe_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    max_aoe_box.setToolTip( QString( _( "The maximum aoe the spell can achieve" ) ) );
    max_aoe_box.setMinimum( 0 );
    max_aoe_box.show();
    QObject::connect( &max_aoe_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_aoe = max_aoe_box.value();
        min_aoe_box.setValue( std::min( min_aoe_box.value(), max_aoe_box.value() ) );
        editable_spell.min_aoe = min_aoe_box.value();
        write_json();
    } );

    max_dot_box.setParent( this );
    max_dot_box.resize( default_text_box_size );
    max_dot_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    max_dot_box.setToolTip( QString( _( "The maximum dot the spell can achieve" ) ) );
    max_dot_box.setMaximum( INT_MAX );
    max_dot_box.setMinimum( INT_MIN );
    max_dot_box.show();
    QObject::connect( &max_dot_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_dot = max_dot_box.value();
        min_dot_box.setValue( std::min( min_dot_box.value(), max_dot_box.value() ) );
        editable_spell.min_dot = min_dot_box.value();
        write_json();
    } );

    max_pierce_box.setParent( this );
    max_pierce_box.resize( default_text_box_size );
    max_pierce_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    max_pierce_box.setToolTip( QString( _( "The maximum pierce the spell can achieve" ) ) );
    max_pierce_box.setMinimum( 0 );
    max_pierce_box.setMaximum( INT_MAX );
    // disabled for now as it does nothing in game
    max_pierce_box.setDisabled( true );
    max_pierce_box.show();
    QObject::connect( &max_pierce_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.max_pierce = max_pierce_box.value();
        min_pierce_box.setValue( std::min( min_pierce_box.value(), max_pierce_box.value() ) );
        editable_spell.min_pierce = min_pierce_box.value();
        write_json();
    } );

    final_casting_time_box.setParent( this );
    final_casting_time_box.resize( default_text_box_size );
    final_casting_time_box.move( QPoint( col * default_text_box_width,
                                         row++ * default_text_box_height ) );
    final_casting_time_box.setToolTip( QString(
                                           _( "The maximum casting time the spell can achieve" ) ) );
    final_casting_time_box.setMinimum( 0 );
    final_casting_time_box.setMaximum( INT_MAX );
    final_casting_time_box.show();
    QObject::connect( &final_casting_time_box, &QSpinBox::textChanged,
    [&]() {
        editable_spell.final_casting_time = final_casting_time_box.value();
        if( editable_spell.energy_increment > 0.0f ) {
            base_casting_time_box.setValue( std::min( base_casting_time_box.value(),
                                            editable_spell.final_casting_time ) );
        } else if( editable_spell.energy_increment < 0.0f ) {
            base_casting_time_box.setValue( std::max( base_casting_time_box.value(),
                                            editable_spell.final_casting_time ) );
        } else {
            base_casting_time_box.setValue( editable_spell.final_casting_time );
        }
        editable_spell.base_casting_time = base_casting_time_box.value();
        write_json();
    } );

    affected_bps_box.setParent( this );
    affected_bps_box.resize( QSize( default_text_box_width, default_text_box_height * 4 ) );
    affected_bps_box.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    affected_bps_box.setToolTip( QString(
                                     _( "Body parts affected by the spell's effects." ) ) );
    affected_bps_box.show();
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
    row += 8;

    sound_description_box.setParent( this );
    sound_description_box.resize( default_text_box_size );
    sound_description_box.move( QPoint( col * default_text_box_width,
                                        row++ * default_text_box_height ) );
    sound_description_box.show();
    sound_description_box.setText( QString( editable_spell.sound_description.translated().c_str() ) );
    QObject::connect( &sound_description_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.sound_description = to_translation( sound_description_box.text().toStdString() );
        write_json();
    } );

    sound_type_box.setParent( this );
    sound_type_box.resize( default_text_box_size );
    sound_type_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    sound_type_box.show();
    QStringList sound_types;
    for( int i = 0; i < static_cast<int>( sounds::sound_t::_LAST ); i++ ) {
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

    sound_id_box.setParent( this );
    sound_id_box.resize( default_text_box_size );
    sound_id_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    sound_id_box.show();
    sound_id_box.setText( QString( editable_spell.sound_id.c_str() ) );
    QObject::connect( &sound_id_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.sound_id = sound_id_box.text().toStdString();
        write_json();
    } );

    sound_variant_box.setParent( this );
    sound_variant_box.resize( default_text_box_size );
    sound_variant_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    sound_variant_box.show();
    sound_variant_box.setText( QString( editable_spell.sound_variant.c_str() ) );
    QObject::connect( &sound_variant_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.sound_variant = sound_variant_box.text().toStdString();
        write_json();
    } );

    sound_ambient_box.setParent( this );
    sound_ambient_box.resize( default_text_box_size );
    sound_ambient_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    sound_ambient_box.show();
    sound_ambient_box.setChecked( editable_spell.sound_ambient );
    QObject::connect( &sound_ambient_box, &QCheckBox::stateChanged,
    [&]() {
        editable_spell.sound_ambient = sound_ambient_box.checkState();
        write_json();
    } );

    max_row = std::max( max_row, row );
    max_col = std::max( max_col, col );
    this->resize( QSize( ( max_col + 1 ) * default_text_box_width,
                         ( max_row ) * default_text_box_height ) );
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
    JsonIn jsin( in_stream );

    std::ostringstream window_out;
    JsonOut window_jo( window_out, true );

    formatter::format( jsin, window_jo );

    QString output_json{ window_out.str().c_str() };

    spell_json.setText( output_json );
}
