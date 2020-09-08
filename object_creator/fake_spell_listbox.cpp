#include "fake_spell_listbox.h"

#include "bodypart.h"
#include "format.h"
#include "json.h"


creator::fake_spell_listbox::fake_spell_listbox( QWidget *parent )
    : QListWidget( parent )
{
    const int default_text_box_height = 20;
    const int default_text_box_width = 100;
    const QSize default_text_box_size( default_text_box_width, default_text_box_height );

    int row = 0;
    int col = 0;
    int max_row = 0;
    int max_col = 0;

    QListWidget::resize( QSize( default_text_box_width * 2, default_text_box_height * 4 ) );

    id_label.setParent( this );
    id_label.resize( default_text_box_size );
    id_label.show();
    row++;

    add_spell_button.setParent( this );
    add_spell_button.setText( QString( "Add fake_spell" ) );
    add_spell_button.resize( default_text_box_size );
    add_spell_button.move( QPoint( default_text_box_width * col, default_text_box_height * row ) );
    add_spell_button.show();
    QObject::connect( &add_spell_button, &QPushButton::clicked,
        [&]() {
        windows.push_back( new fake_spell_window() );
        windows.back()->hide();
        QObject::connect( windows.back(), &fake_spell_window::modified, this, &fake_spell_listbox::modified );
        fake_spell_list_box.addItem( QString( "NEW (edit)" ) );
    } );

    // =========================================================================================
    // second column of boxes
    max_row = std::max( row, max_row );
    row = 0;
    col = 1;

    fake_spell_list_box.setParent( this );
    fake_spell_list_box.resize( QSize( default_text_box_width, default_text_box_height * 4 ) );
    fake_spell_list_box.move( QPoint( default_text_box_width * col, default_text_box_height * row ) );
    fake_spell_list_box.show();
    QObject::connect( &fake_spell_list_box, qOverload<int>( &QComboBox::currentIndexChanged ),
        [&]() {
        for( fake_spell_window *win : windows ) {
            win->hide();
        }
        const int cur_index = fake_spell_list_box.currentIndex();
        windows.at( cur_index )->show();
        fake_spell_list_box.setCurrentText( windows.at( cur_index )->get_fake_spell().id.c_str() );
    } );
    row += 4;
}

void creator::fake_spell_listbox::set_spells( const std::vector<fake_spell> &spells )
{
    windows.clear();
    for( const fake_spell &sp : spells ) {
        windows.push_back( new fake_spell_window() );
        windows.back()->set_fake_spell( sp );
        fake_spell_list_box.addItem( QString( sp.id.c_str() ) );
    }
}

std::vector<fake_spell> creator::fake_spell_listbox::get_spells() const
{
    std::vector<fake_spell> spells;
    for( const fake_spell_window *win : windows ) {
        spells.push_back( win->get_fake_spell() );
    }
    return spells;
}
