#include "dual_list_box.h"

creator::dual_list_box::dual_list_box( const QStringList &items )
{
    this->items = items;

    addWidget( &included_box, 0, 0 );
    addWidget( &excluded_box, 2, 0 );

    addWidget( &include_all_button, 1, 0 );
    addWidget( &include_sel_button, 1, 1 );
    addWidget( &exclude_sel_button, 1, 2 );
    addWidget( &exclude_all_button, 1, 3 );

    // initialize the list as all excluded first
    exclude_all();

    QObject::connect( &include_all_button, &QPushButton::click, this, &dual_list_box::include_all );
    QObject::connect( &exclude_all_button, &QPushButton::click, this, &dual_list_box::exclude_all );
    QObject::connect( &include_sel_button, &QPushButton::click, this, &dual_list_box::include_selected );
    QObject::connect( &exclude_sel_button, &QPushButton::click, this, &dual_list_box::exclude_selected );

    include_all_button.setText( QString( "<<" ) );
    exclude_all_button.setText( QString( ">>" ) );
    include_sel_button.setText( QString( "<" ) );
    exclude_sel_button.setText( QString( ">" ) );
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
