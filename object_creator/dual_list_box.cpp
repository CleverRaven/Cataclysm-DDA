#include "dual_list_box.h"

void creator::dual_list_box::initialize( const QStringList &items, const QSize &default_text_box_size )
{
    this->items = items;

    const double button_width = 0.2;
    const double box_width = 0.9;
    const QSize box_size( box_width * default_text_box_size.width(), 4 * default_text_box_size.height() );
    const QSize button_size( button_width * default_text_box_size.width(), default_text_box_size.height() );

    included_box.resize( box_size );
    excluded_box.resize( box_size );

    include_all_button.resize( button_size );
    exclude_all_button.resize( button_size );
    include_sel_button.resize( button_size );
    exclude_sel_button.resize( button_size );

    included_box.setParent( this );
    excluded_box.setParent( this );

    include_all_button.setParent( this );
    exclude_all_button.setParent( this );
    include_sel_button.setParent( this );
    exclude_sel_button.setParent( this );

    QGridLayout *listbox_layout = new QGridLayout();
    setLayout( listbox_layout );
    
    listbox_layout->addWidget( &included_box, 0, 0 );
    listbox_layout->addWidget( &excluded_box, 0, 2 );

    listbox_layout->addWidget( &include_all_button, 0, 1 );
    listbox_layout->addWidget( &include_sel_button, 1, 1 );
    listbox_layout->addWidget( &exclude_sel_button, 2, 1 );
    listbox_layout->addWidget( &exclude_all_button, 3, 1 );

    // initialize the list as all excluded first
    exclude_all();

    QObject::connect( &include_all_button, &QPushButton::click, this, &dual_list_box::include_all );
    QObject::connect( &exclude_all_button, &QPushButton::click, this, &dual_list_box::exclude_all );
    QObject::connect( &include_sel_button, &QPushButton::click, this,
        &dual_list_box::include_selected );
    QObject::connect( &exclude_sel_button, &QPushButton::click, this,
        &dual_list_box::exclude_selected );

    include_all_button.setText( QString( "<<" ) );
    exclude_all_button.setText( QString( ">>" ) );
    include_sel_button.setText( QString( "<" ) );
    exclude_sel_button.setText( QString( ">" ) );

    QObject::connect( &include_all_button, &QPushButton::click, this, &dual_list_box::click );
    QObject::connect( &exclude_all_button, &QPushButton::click, this, &dual_list_box::click );
    QObject::connect( &include_sel_button, &QPushButton::click, this, &dual_list_box::click );
    QObject::connect( &exclude_sel_button, &QPushButton::click, this, &dual_list_box::click );

    included_box.show();
    excluded_box.show();

    include_all_button.show();
    exclude_all_button.show();
    include_sel_button.show();
    exclude_sel_button.show();
}

void creator::dual_list_box::include_all()
{
    excluded_box.clear();
    included_box.clear();
    included_box.addItems( items );
}

void creator::dual_list_box::exclude_all()
{
    excluded_box.clear();
    included_box.clear();
    excluded_box.addItems( items );
}

void creator::dual_list_box::include_selected()
{
    const QString selected = excluded_box.currentText();
    included_box.addItem( selected );
    excluded_box.removeItem( excluded_box.currentIndex() );
}

void creator::dual_list_box::exclude_selected()
{
    const QString selected = included_box.currentText();
    excluded_box.addItem( selected );
    included_box.removeItem( included_box.currentIndex() );
}

QStringList creator::dual_list_box::get_included() const
{
    QStringList ret;
    for( int i = 0; i < included_box.count(); i++ ) {
        ret.append( included_box.itemText( i ) );
    }
    return ret;
}
