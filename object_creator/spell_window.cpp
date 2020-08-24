#include "spell_window.h"

#include "bodypart.h"
#include "format.h"
#include "json.h"
#include "magic.h"

#include <sstream>

static spell_type default_spell_type()
{
    spell_type ret;
    ret.sound_type = sounds::sound_t::combat;
    ret.sound_description = to_translation( "an explosion" );
    ret.sound_variant = "default";
    ret.message = to_translation( "You cast %s!" );
    ret.skill = skill_id( "spellcraft" );
    ret.spell_class = trait_id( "NONE" );
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
    id_label.setDisabled( true );
    id_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    id_label.show();

    name_label.setParent( this );
    name_label.setText( QString( "name" ) );
    name_label.resize( default_text_box_size );
    name_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    name_label.setDisabled( true );
    name_label.show();

    description_label.setParent( this );
    description_label.setText( QString( "description" ) );
    description_label.resize( default_text_box_size );
    description_label.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    description_label.setDisabled( true );
    description_label.show();
    row += 3;

    effect_label.setParent( this );
    effect_label.setText( QString( "spell effect" ) );
    effect_label.resize( default_text_box_size );
    effect_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    effect_label.setDisabled( true );
    effect_label.show();

    effect_str_label.setParent( this );
    effect_str_label.setText( QString( "effect string" ) );
    effect_str_label.resize( default_text_box_size );
    effect_str_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    effect_str_label.setDisabled( true );
    effect_str_label.show();

    valid_targets_label.setParent( this );
    valid_targets_label.setText( QString( "valid_targets" ) );
    valid_targets_label.resize( default_text_box_size );
    valid_targets_label.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    valid_targets_label.setDisabled( true );
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
    effect_str_box.move( QPoint( col * default_text_box_width, row++ *default_text_box_height ) );
    effect_str_box.setToolTip( QString(
                                   _( "Additional data related to the spell effect.  See MAGIC.md for details." ) ) );
    effect_str_box.show();
    QObject::connect( &effect_str_box, &QLineEdit::textChanged,
    [&]() {
        editable_spell.effect_str = effect_str_box.text().toStdString();
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
    energy_cost_label.setDisabled( true );
    energy_cost_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    energy_cost_label.show();

    damage_label.setParent( this );
    damage_label.setText( QString( "damage" ) );
    damage_label.resize( default_text_box_size );
    damage_label.setDisabled( true );
    damage_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    damage_label.show();

    range_label.setParent( this );
    range_label.setText( QString( "range" ) );
    range_label.resize( default_text_box_size );
    range_label.setDisabled( true );
    range_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    range_label.show();

    aoe_label.setParent( this );
    aoe_label.setText( QString( "aoe" ) );
    aoe_label.resize( default_text_box_size );
    aoe_label.setDisabled( true );
    aoe_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    aoe_label.show();

    dot_label.setParent( this );
    dot_label.setText( QString( "dot" ) );
    dot_label.resize( default_text_box_size );
    dot_label.setDisabled( true );
    dot_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    dot_label.show();

    pierce_label.setParent( this );
    pierce_label.setText( QString( "pierce" ) );
    pierce_label.resize( default_text_box_size );
    pierce_label.setDisabled( true );
    pierce_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    pierce_label.show();

    casting_time_label.setParent( this );
    casting_time_label.setText( QString( "casting time" ) );
    casting_time_label.resize( default_text_box_size );
    casting_time_label.setDisabled( true );
    casting_time_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    casting_time_label.show();

    energy_source_label.setParent( this );
    energy_source_label.setText( QString( "energy source" ) );
    energy_source_label.resize( default_text_box_size );
    energy_source_label.setDisabled( true );
    energy_source_label.move( QPoint( col * default_text_box_width, row++ *default_text_box_height ) );
    energy_source_label.show();

    dmg_type_label.setParent( this );
    dmg_type_label.setText( QString( "damage type" ) );
    dmg_type_label.resize( default_text_box_size );
    dmg_type_label.setDisabled( true );
    dmg_type_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    dmg_type_label.show();

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
            final_energy_cost_box.setValue( editable_spell.base_casting_time );
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
                               row++ *default_text_box_height ) );
    dmg_type_box.setToolTip( QString( _( "The casting time of the spell at level 0" ) ) );
    dmg_type_box.show();
    QStringList damage_types;
    for( int i = 0; i < static_cast<int>( damage_type::NUM_DT ); i++ ) {
        damage_types.append( QString( io::enum_to_string( static_cast<damage_type>( i ) ).c_str() ) );
    }
    dmg_type_box.addItems( damage_types );
    dmg_type_box.setCurrentIndex( static_cast<int>( damage_type::DT_NONE ) );
    QObject::connect( &dmg_type_box, &QComboBox::currentTextChanged,
    [&]() {
        const damage_type tp = static_cast<damage_type>( dmg_type_box.currentIndex() );
        editable_spell.dmg_type = tp;
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
    affected_bps_label.setDisabled( true );
    affected_bps_label.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    affected_bps_label.show();
    row += 4;

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
    row += 4;

    max_row = std::max( max_row, row );
    max_col = std::max( max_col, col );
    this->resize( QSize( ( max_col + 1 ) * default_text_box_width,
                         ( max_row ) * default_text_box_height ) );
}

void creator::spell_window::write_json()
{
    std::ostringstream stream;
    JsonOut jo( stream );
    jo.write( editable_spell );

    std::istringstream in_stream( stream.str() );
    JsonIn jsin( in_stream );

    formatter::format( jsin, jo );

    QString output_json{ stream.str().c_str() };

    spell_json.setText( output_json );
}
