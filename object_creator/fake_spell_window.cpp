#include "fake_spell_window.h"

#include "bodypart.h"
#include "format.h"
#include "json.h"

#include "QtWidgets/qcombobox.h"

creator::fake_spell_window::fake_spell_window( QWidget *parent, Qt::WindowFlags flags )
    : QMainWindow( parent, flags )
{
    const int default_text_box_height = 20;
    const int default_text_box_width = 100;
    const QSize default_text_box_size( default_text_box_width, default_text_box_height );
    const QSize half_width_text_box( default_text_box_width / 2, default_text_box_height );

    int row = 0;
    int col = 0;
    int max_row = 0;

    id_label.setParent( this );
    id_label.setText( QString( "id" ) );
    id_label.resize( default_text_box_size );
    id_label.setDisabled( true );
    id_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    id_label.show();

    max_level_label.setParent( this );
    max_level_label.setText( QString( "maximum level" ) );
    max_level_label.resize( default_text_box_size );
    max_level_label.setDisabled( true );
    max_level_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    max_level_label.show();

    min_level_label.setParent( this );
    min_level_label.setText( QString( "minimum level" ) );
    min_level_label.resize( default_text_box_size );
    min_level_label.setDisabled( true );
    min_level_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    min_level_label.show();

    once_in_label.setParent( this );
    once_in_label.setText( QString( "once in" ) );
    once_in_label.resize( default_text_box_size );
    once_in_label.setDisabled( true );
    once_in_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    once_in_label.setToolTip( QString(
                                  _( "The frequency this spell activates when part of an intermittent enchantment.  RNG-based." ) ) );
    once_in_label.show();

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
        if( editable_spell.id.is_valid() ) {
            max_level_box.setMaximum( editable_spell.id->max_level.min.dbl_val.value() );
            min_level_box.setMaximum( editable_spell.id->max_level.min.dbl_val.value() );
        }
    } );
    QObject::connect( &id_box, &QLineEdit::textChanged, this, &fake_spell_window::modified );

    max_level_box.setParent( this );
    max_level_box.resize( default_text_box_size );
    max_level_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    max_level_box.setToolTip( QString(
                                  _( "The max level this fake_spell can achieve.  Set to -1 for max == max_level of spell." ) ) );
    max_level_box.setMinimum( -1 );
    max_level_box.show();

    min_level_box.setParent( this );
    min_level_box.resize( default_text_box_size );
    min_level_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    min_level_box.setToolTip( QString(
                                  _( "The min level of this fake_spell.  Max level takes precedence." ) ) );
    min_level_box.setMinimum( 0 );
    min_level_box.show();

    once_in_duration_box.setParent( this );
    once_in_duration_box.resize( half_width_text_box );
    once_in_duration_box.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    once_in_duration_box.setToolTip( QString(
                                         _( "The min level of this fake_spell.  Max level takes precedence." ) ) );
    once_in_duration_box.setMinimum( 1 );
    once_in_duration_box.show();
    const auto update_spell_trigger_duration = [&]() {
        time_duration unit = 1_turns;
        for( const std::pair<std::string, time_duration> &unit_pair : time_duration::units ) {
            if( unit_pair.first == once_in_units_box.currentText().toStdString() ) {
                unit = unit_pair.second;
            }
        }
        editable_spell.trigger_once_in = to_turns<int>( unit * once_in_duration_box.value() );
    };
    QObject::connect( &once_in_duration_box, qOverload<int>( &QSpinBox::valueChanged ),
                      update_spell_trigger_duration );

    once_in_units_box.setParent( this );
    once_in_units_box.resize( half_width_text_box );
    once_in_units_box.move( QPoint( col * default_text_box_width + default_text_box_width / 2,
                                    row++ * default_text_box_height ) );
    once_in_units_box.setToolTip( QString( _( "The unit to be used for the once_in frequency." ) ) );
    for( const std::pair<std::string, time_duration> &unit_pair : time_duration::units ) {
        // intentionally limiting the available units to single letter abbreviations
        if( unit_pair.first.length() == 1 ) {
            once_in_units_box.addItem( QString( unit_pair.first.c_str() ) );
        }
    }
    once_in_units_box.show();
    QObject::connect( &once_in_units_box, qOverload<int>( &QComboBox::currentIndexChanged ),
                      update_spell_trigger_duration );
}
