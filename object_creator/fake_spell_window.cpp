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

    int row = 0;
    int col = 0;
    int max_row = 0;
    int max_col = 0;

    id_label.setParent( this );
    id_label.setText( QString( "id" ) );
    id_label.resize( default_text_box_size );
    id_label.setDisabled( true );
    id_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    id_label.show();
    
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
    } );
    QObject::connect( &id_box, &QLineEdit::textChanged, this, &fake_spell_window::modified );
}
