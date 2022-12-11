#include "dual_list_box.h"

void creator::dual_list_box::initialize( const QStringList &items )
{
    this->items = items;

    included_box.setParent( this );
    button_widget.setParent( this );
    excluded_box.setParent( this );

    include_all_button.setParent( &button_widget );
    exclude_all_button.setParent( &button_widget );
    include_sel_button.setParent( &button_widget );
    exclude_sel_button.setParent( &button_widget );

    QVBoxLayout *button_widget_layout = new QVBoxLayout();
    button_widget.setLayout( button_widget_layout );

    button_widget_layout->addWidget( &include_all_button );
    button_widget_layout->addWidget( &include_sel_button );
    button_widget_layout->addWidget( &exclude_sel_button );
    button_widget_layout->addWidget( &exclude_all_button );

    QHBoxLayout *listbox_layout = new QHBoxLayout();
    listbox_layout->setSpacing( 0 );
    listbox_layout->setMargin( 0 );
    setLayout( listbox_layout );

    listbox_layout->addWidget( &included_box );
    listbox_layout->addWidget( &button_widget );
    listbox_layout->addWidget( &excluded_box );

    // initialize the list as all excluded first
    exclude_all();

    QObject::connect( &include_all_button, &QPushButton::pressed, this, &dual_list_box::include_all );
    QObject::connect( &exclude_all_button, &QPushButton::pressed, this, &dual_list_box::exclude_all );
    QObject::connect( &include_sel_button, &QPushButton::pressed, this,
                      &dual_list_box::include_selected );
    QObject::connect( &exclude_sel_button, &QPushButton::pressed, this,
                      &dual_list_box::exclude_selected );

    include_all_button.setText( QString( "<<" ) );
    exclude_all_button.setText( QString( ">>" ) );
    include_sel_button.setText( QString( "<" ) );
    exclude_sel_button.setText( QString( ">" ) );

    QObject::connect( &include_all_button, &QPushButton::pressed, this, &dual_list_box::pressed );
    QObject::connect( &exclude_all_button, &QPushButton::pressed, this, &dual_list_box::pressed );
    QObject::connect( &include_sel_button, &QPushButton::pressed, this, &dual_list_box::pressed );
    QObject::connect( &exclude_sel_button, &QPushButton::pressed, this, &dual_list_box::pressed );

    included_box.show();
    excluded_box.show();

    include_all_button.show();
    exclude_all_button.show();
    include_sel_button.show();
    exclude_sel_button.show();

    included_box.setSelectionBehavior( QAbstractItemView::SelectionBehavior::SelectItems );
    included_box.setSelectionMode( QAbstractItemView::SelectionMode::SingleSelection );
    button_widget_layout->setSizeConstraint( QLayout::SizeConstraint::SetMaximumSize );
    listbox_layout->setSizeConstraint( QLayout::SizeConstraint::SetMaximumSize );
}

void creator::dual_list_box::resize( const QSize &size )
{
    const double button_width = 0.1;
    const double box_width = 0.45;

    included_box.setMaximumHeight( size.height() );
    included_box.setMaximumWidth( size.width() * box_width );

    button_widget.setMaximumHeight( size.height() );
    button_widget.setMaximumWidth( size.width() * button_width );

    excluded_box.setMaximumHeight( size.height() );
    excluded_box.setMaximumWidth( size.width() * box_width );
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
    const QList<QListWidgetItem *> selected_items = excluded_box.selectedItems();
    if( selected_items.isEmpty() ) {
        return;
    }
    const QString selected = selected_items.first()->text();
    included_box.addItem( selected );

    int index = 0;
    QStringList excluded;
    for( int i = 0; i < excluded_box.count(); i++ ) {
        const QString excluded_single = excluded_box.item( i )->text();
        if( excluded_single != selected ) {
            excluded.append( excluded_single );
        } else {
            index = i;
        }
    }
    excluded_box.clear();
    excluded_box.addItems( excluded );
    if( excluded.isEmpty() ) {
        return;
    }
    if( index > excluded_box.count() - 1 ) {
        index = excluded_box.count() - 1;
    }
    excluded_box.item( index )->setSelected( true );
}

void creator::dual_list_box::exclude_selected()
{
    const QList<QListWidgetItem *> selected_items = included_box.selectedItems();
    if( selected_items.isEmpty() ) {
        return;
    }
    const QString selected = selected_items.first()->text();
    excluded_box.addItem( selected );

    int index = 0;
    QStringList included;
    for( int i = 0; i < included_box.count(); i++ ) {
        const QString included_single = included_box.item( i )->text();
        if( included_single != selected ) {
            included.append( included_single );
        } else {
            index = i;
        }
    }
    included_box.clear();
    included_box.addItems( included );
    if( included.isEmpty() ) {
        return;
    }
    if( index > included_box.count() - 1 ) {
        index = included_box.count() - 1;
    }
    included_box.item( index )->setSelected( true );
}

QStringList creator::dual_list_box::get_included() const
{
    QStringList ret;
    for( int i = 0; i < included_box.count(); i++ ) {
        ret.append( included_box.item( i )->text() );
    }
    return ret;
}

void creator::dual_list_box::set_included( const QStringList ret )
{
    if( included_box.count() > 0 ) {
        exclude_all();
    }
    for( int y = 0; y < ret.count(); y++ ) {

        for( int i = 0; i < excluded_box.count(); i++ ) {
            const QString excluded_single = excluded_box.item( i )->text();
            if( excluded_single == ret[y] ) {
                excluded_box.item( i )->setSelected( true );
                break;
            }
        }
        include_selected();
    }
}
