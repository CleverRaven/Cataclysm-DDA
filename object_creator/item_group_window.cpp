#include "item_group_window.h"

#include <algorithm>
#include "format.h"
#include "json.h"
#include "item_group.h"
#include "item.h"
#include "item_factory.h"

#include "QtWidgets/qheaderview.h"

#include <sstream>


creator::item_group_window::item_group_window( QWidget *parent, Qt::WindowFlags flags )
    : QMainWindow( parent, flags )
{
    const int default_text_box_height = 20;
    const int default_text_box_width = 100;
    const QSize default_text_box_size( default_text_box_width, default_text_box_height );

    int row = 0;
    int col = 0;
    int max_row = 0;
    int max_col = 0;

    item_group_json.resize( QSize( 800, 600 ) );
    item_group_json.setReadOnly( true );


    // =========================================================================================
    // first column of boxes
    id_label.setParent( this );
    id_label.setText( QString( "id" ) );
    id_label.resize(default_text_box_size);
    id_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    id_label.show();

    item_search_label.setParent( this );
    item_search_label.setText( QString( "Search items" ) );
    item_search_label.resize( default_text_box_size );
    item_search_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    item_search_label.show();

    item_list_total_box.setParent( this );
    item_list_total_box.resize( QSize( default_text_box_width * 2, default_text_box_height * 10 ) );
    item_list_total_box.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    item_list_total_box.setToolTip( QString( _( "All items" ) ) );
    item_list_total_box.show();
    QObject::connect( &item_list_total_box, &QListWidget::itemDoubleClicked, this, 
            &item_group_window::selected_item_doubleclicked );
    row += 10;
    item_list_populate_filtered();

    group_search_label.setParent( this );
    group_search_label.setText( QString( "Search groups" ) );
    group_search_label.resize( default_text_box_size );
    group_search_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    group_search_label.show();

    group_list_total_box.setParent( this );
    group_list_total_box.resize( QSize( default_text_box_width * 2, default_text_box_height * 10 ) );
    group_list_total_box.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    group_list_total_box.setToolTip( QString( _( "All groups" ) ) );
    group_list_total_box.show();
    group_list_populate_filtered();
    QObject::connect( &group_list_total_box, &QListWidget::itemDoubleClicked, this,
            &item_group_window::selected_group_doubleclicked);
    row += 10;


    // =========================================================================================
    // second column of boxes
    row = 0;
    col++;

    id_box.setParent( this );
    id_box.resize( default_text_box_size );
    id_box.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    id_box.setToolTip( QString( _( "The id of the item_group" ) ) );
    id_box.setText( "tools_home" );
    id_box.show();
    QObject::connect( &id_box, &QLineEdit::textChanged, [&]() { write_json(); } );

    item_search_box.setParent( this );
    item_search_box.resize( default_text_box_size );
    item_search_box.move( QPoint(col * default_text_box_width, row++ * default_text_box_height ) );
    item_search_box.setToolTip( QString( _( "Enter text and press return to search for items" ) ) );
    item_search_box.show();
    QObject::connect( &item_search_box, &QLineEdit::returnPressed, this, 
            &item_group_window::items_search_return_pressed );

    row += 10;
    group_search_box.setParent( this );
    group_search_box.resize( default_text_box_size );
    group_search_box.move( QPoint(col * default_text_box_width, row++ * default_text_box_height ) );
    group_search_box.setToolTip( QString( _( "Enter text and press return to search for groups" ) ) );
    group_search_box.show();
    QObject::connect( &group_search_box, &QLineEdit::returnPressed, this,
            &item_group_window::group_search_return_pressed );
    row += 11;


    // =========================================================================================
    // third column of boxes
    row = 0;
    col++;


    entries_box.setParent( this );
    entries_box.resize( QSize( default_text_box_width * 4, default_text_box_height * 23 ) );
    entries_box.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    entries_box.insertColumn( 0 );
    entries_box.insertColumn( 0 );
    entries_box.insertColumn( 0 );
    entries_box.insertColumn( 0 );
    entries_box.insertRow( 0 );
    entries_box.setHorizontalHeaderLabels( QStringList{ "X", "GI", "Item", "Prob" } );
    entries_box.verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    entries_box.verticalHeader()->setDefaultSectionSize(12);
    entries_box.verticalHeader()->hide();
    entries_box.horizontalHeader()->resizeSection( 0, default_text_box_width * 0.25 );
    entries_box.horizontalHeader()->resizeSection( 1, default_text_box_width * 0.25 );
    entries_box.horizontalHeader()->resizeSection( 2, default_text_box_width * 2.50 );
    entries_box.horizontalHeader()->resizeSection( 3, default_text_box_width * 0.50 );
    entries_box.setSelectionBehavior( QAbstractItemView::SelectionBehavior::SelectItems );
    entries_box.setSelectionMode( QAbstractItemView::SelectionMode::SingleSelection );
    entries_box.show();
    QObject::connect( &entries_box, &QTableWidget::cellChanged, [&]() { write_json(); } );


    // =========================================================================================
    // Finalize

    row += 24;
    col += 3;
    max_row = std::max( max_row, row );
    max_col = std::max( max_col, col );
    this->resize( QSize( ( max_col + 1 ) * default_text_box_width,
                         ( max_row ) * default_text_box_height ) );
}

void creator::item_group_window::write_json()
{
    std::ostringstream stream;
    JsonOut jo( stream );

    jo.start_object();

    jo.member( "type", "item_group" );
    jo.member( "id", id_box.text().toStdString() );
    jo.member( "subtype", "distribution" );

    if(entries_box.rowCount() > 0){
        jo.member( "entries" );
        jo.start_array();
        for( int row = 0; row < entries_box.rowCount() - 0; row++ ) {
            jo.start_object();
            QAbstractItemModel* model = entries_box.model();
            QVariant group_item = model->data( model->index( row, 1 ), Qt::DisplayRole );
            QVariant item_text = model->data( model->index( row, 2 ), Qt::DisplayRole );
            QVariant prob_text = model->data( model->index( row, 3 ), Qt::DisplayRole );
            if( group_item.toString().toStdString() == "G" ){
                jo.member( "group", item_text.toString().toStdString() );
            } else {
                jo.member( "item", item_text.toString().toStdString() );
            }
            jo.member( "prob", prob_text.toInt() );
            jo.end_object();
        }
        jo.end_array();
    }
    jo.end_object();

    std::istringstream in_stream( stream.str() );
    JsonIn jsin( in_stream );

    std::ostringstream window_out;
    JsonOut window_jo( window_out, true );

    formatter::format( jsin, window_jo );

    QString output_json{ window_out.str().c_str() };

    item_group_json.setText( output_json );
}

void creator::item_group_window::deleteEntriesLine()
{
    QWidget* w = qobject_cast<QWidget*>(sender()->parent());

    if (w) {
        int row = entries_box.indexAt(w->pos()).row();
        entries_box.removeRow(row);
        if (entries_box.rowCount() < 1) {
            entries_box.insertRow(0);
        }
    }
    write_json();
}

void creator::item_group_window::items_search_return_pressed()
{
    std::string searchQuery = item_search_box.text().toStdString();
    item_list_populate_filtered( searchQuery );
}

void creator::item_group_window::group_search_return_pressed()
{
    std::string searchQuery = group_search_box.text().toStdString();
    group_list_populate_filtered( searchQuery );
}

void creator::item_group_window::group_list_populate_filtered( std::string searchQuery )
{
    std::string groupID;
    group_list_total_box.clear();
    if ( searchQuery == "" ) {
        for (const item_group_id i : item_controller.get()->get_all_group_names()) {
            groupID = i.c_str();
            QListWidgetItem* new_item = new QListWidgetItem( QString( groupID.c_str() ) );
            group_list_total_box.addItem( new_item );
        }
    } else {
        for( const item_group_id i : item_controller.get()->get_all_group_names() ) {
            groupID = i.c_str();
            if( groupID.find( searchQuery ) != std::string::npos ) {
                QListWidgetItem* new_item = new QListWidgetItem( QString( groupID.c_str() ) );
                group_list_total_box.addItem( new_item );
            }
        }
    }
}

void creator::item_group_window::item_list_populate_filtered( std::string searchQuery )
{
    std::string itemID;
    item_list_total_box.clear();
    if ( searchQuery == "" ) {
        for (const itype* i : item_controller->all()) {
            item tmpItem(i, calendar::turn_zero);
            QListWidgetItem* new_item = new QListWidgetItem( QString( tmpItem.typeId().c_str() ) );
            set_item_tooltip( new_item, tmpItem );
            item_list_total_box.addItem( new_item );
        }
    } else {
        for ( const itype* i : item_controller->all() ) {
            item tmpItem(i, calendar::turn_zero);
            itemID = tmpItem.typeId().c_str();
            if( itemID.find( searchQuery ) != std::string::npos ) {
                QListWidgetItem* new_item = new QListWidgetItem(
                    QString(tmpItem.typeId().c_str() ) );
                set_item_tooltip( new_item, tmpItem );
                item_list_total_box.addItem( new_item );
            }
        }
    }
}

void creator::item_group_window::set_item_tooltip(QListWidgetItem* new_item, item tmpItem)
{
    std::string tooltip = "id: ";
    tooltip += tmpItem.typeId().c_str();
    tooltip += "\nname: ";
    tooltip += tmpItem.tname().c_str();
    new_item->setToolTip( QString( _( tooltip.c_str() ) ) );
}

void creator::item_group_window::entries_add_item( QListWidgetItem* cur_widget, bool group )
{
    const int last_row = entries_box.rowCount() - 1;
    QAbstractItemModel* model = entries_box.model();
    QVariant item_text = model->data( model->index( last_row, 1 ), Qt::DisplayRole );
    QVariant prob_text = model->data( model->index( last_row, 2 ), Qt::DisplayRole );

    const bool last_row_empty = item_text.toString().isEmpty() && prob_text.toString().isEmpty();
    if( !last_row_empty ) {
        entries_box.insertRow( entries_box.rowCount() );
    }

    QWidget* pWidget = new QWidget();
    QPushButton* btnitem = new QPushButton();
    btnitem->setText( "X" );
    connect( btnitem, &QPushButton::clicked, this, &item_group_window::deleteEntriesLine );
    QHBoxLayout* pLayout = new QHBoxLayout( pWidget );
    pLayout->addWidget( btnitem );
    pLayout->setAlignment( Qt::AlignCenter );
    pLayout->setContentsMargins( 0, 0, 0, 0 );
    pWidget->setLayout( pLayout );
    entries_box.setCellWidget( entries_box.rowCount() - 1, 0, pWidget );

    QTableWidgetItem* item = new QTableWidgetItem();
    if (group) {
        item->setText( QString( "G" ) );
    } else {
        item->setText( QString( "I" ) );
    }
    entries_box.setItem(entries_box.rowCount() - 1, 1, item);

    item = new QTableWidgetItem();
    item->setText(cur_widget->text());
    entries_box.setItem(entries_box.rowCount() - 1, 2, item);

    item = new QTableWidgetItem();
    item->setText( QString( "100" ) );
    entries_box.setItem( entries_box.rowCount() - 1, 3, item );

    delete cur_widget;

    write_json();
}

void creator::item_group_window::selected_group_doubleclicked()
{
    if (group_list_total_box.selectedItems().length() > 0) {
        QListWidgetItem* cur_widget = group_list_total_box.selectedItems().first();
        entries_add_item(cur_widget, true);
    }
}

void creator::item_group_window::selected_item_doubleclicked()
{
    if (item_list_total_box.selectedItems().length() > 0) {
        QListWidgetItem* cur_widget = item_list_total_box.selectedItems().first();
        entries_add_item(cur_widget, false);
    }
}
