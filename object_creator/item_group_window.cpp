#include "item_group_window.h"

#include "format.h"
#include "item_factory.h"

#include "QtWidgets/qheaderview.h"
#include <QtCore/QCoreApplication>


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
    item_list_total_box.setDragEnabled( true );
    item_list_total_box.setToolTip( QString( _( "All items" ) ) );
    //Only the item list has 'items' set to true so it distinguishes itself from the group listbox
    //This is used when adding an item to entrieslist and color the bg based on this property
    item_list_total_box.setProperty( "items", true );
    item_list_total_box.show();
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
    group_list_total_box.setDragEnabled( true );
    group_list_total_box.setToolTip( QString( _( "All groups" ) ) );
    group_list_total_box.show();
    group_list_populate_filtered();
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

    entries_box = new QFrame( this );
    entries_box->resize( QSize( 400, 200 ) );
    entries_box->move( QPoint( col * default_text_box_width, row * default_text_box_height ) );

    creator::distributionCollection* dis = new creator::distributionCollection( entries_box );

    // =========================================================================================
    // Finalize

    row += 26;
    col += 4;
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

    jo.member( "entries" );
    jo.start_array();
    QObjectList entriesChildren = entries_box->children();
    for ( QObject* i : entriesChildren ) {
        entriesList* lst = dynamic_cast<creator::entriesList*>( i );
        if ( lst != nullptr ) {
            lst->get_json( jo );
        }
        distributionCollection* dis = dynamic_cast<creator::distributionCollection*>( i );
        if ( dis != nullptr ) {
            dis->get_json( jo );
        }
    }
    jo.end_array();
    jo.end_object();

    std::istringstream in_stream( stream.str() );
    JsonIn jsin( in_stream );

    std::ostringstream window_out;
    JsonOut window_jo( window_out, true );

    formatter::format( jsin, window_jo );

    QString output_json{ window_out.str().c_str() };

    item_group_json.setText( output_json );
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
    if( searchQuery == "" ) {
        for( const item_group_id i : item_controller.get()->get_all_group_names() ) {
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
    if( searchQuery == "" ) {
        for( const itype* i : item_controller->all() ) {
            item tmpItem(i, calendar::turn_zero);
            QListWidgetItem* new_item = new QListWidgetItem( QString( tmpItem.typeId().c_str() ) );
            set_item_tooltip( new_item, tmpItem );
            item_list_total_box.addItem( new_item );
        }
    } else {
        for( const itype* i : item_controller->all() ) {
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

void creator::item_group_window::set_item_tooltip( QListWidgetItem* new_item, item tmpItem )
{
    std::string tooltip = "id: ";
    tooltip += tmpItem.typeId().c_str();
    tooltip += "\nname: ";
    tooltip += tmpItem.tname().c_str();
    new_item->setToolTip( QString( _( tooltip.c_str() ) ) );
}

bool creator::item_group_window::event( QEvent* event )
{
    if( event->type() == item_group_changed::eventType ) {
        write_json();
        return true;
    }
    //call the event method of the base class for the events that aren't handled
    return QMainWindow::event( event );
}


void creator::entriesList::add_item( QString itemText, bool group)
{
    const int last_row = this->rowCount() - 1;
    QAbstractItemModel* model = this->model();
    QVariant item_text = model->data( model->index( last_row, 1 ), Qt::DisplayRole );
    QVariant prob_text = model->data( model->index( last_row, 2 ), Qt::DisplayRole );

    const bool last_row_empty = item_text.toString().isEmpty() && prob_text.toString().isEmpty();
    if ( !last_row_empty ) {
        this->insertRow( this->rowCount() );
    }

    QWidget* pWidget = new QWidget();
    QPushButton* btnitem = new QPushButton();
    btnitem->setText( "X" );
    connect( btnitem, &QPushButton::clicked, this, &entriesList::deleteEntriesLine );
    QHBoxLayout* pLayout = new QHBoxLayout( pWidget );
    pLayout->addWidget( btnitem );
    pLayout->setAlignment( Qt::AlignCenter );
    pLayout->setContentsMargins( 0, 0, 0, 0 );
    pWidget->setLayout( pLayout );
    this->setCellWidget( this->rowCount() - 1, 0, pWidget );

    QTableWidgetItem* item = new QTableWidgetItem();
    item->setText( itemText );
    if ( group ) {
        item->setBackgroundColor( Qt::yellow );
    }
    this->setItem( this->rowCount() - 1, 1, item );

    item = new QTableWidgetItem();
    item->setText( QString( "100" ) );
    this->setItem( this->rowCount() - 1, 2, item );
}


void creator::entriesList::get_json( JsonOut &jo ) {
    if( this->rowCount() < 1) {
        return;
    }
    for( int row = 0; row < this->rowCount() - 0; row++ ) {
        jo.start_object();
        QAbstractItemModel* model = this->model();
        QVariant item_text = model->data( model->index( row, 1 ), Qt::DisplayRole );
        QVariant prob_text = model->data( model->index( row, 2 ), Qt::DisplayRole );

        //If the backgroundcolor is yellow, it's a group. Otherwise it's an item
        QTableWidgetItem* item = this->item( row, 1 );
        QBrush item_color = item->backgroundColor();
        if( item_color == Qt::yellow ){
            jo.member( "group", item_text.toString().toStdString() );
        } else {
            jo.member( "item", item_text.toString().toStdString() );
        }
        jo.member( "prob", prob_text.toInt() );
        jo.end_object();
    }
}


void creator::distributionCollection::get_json( JsonOut &jo ) {
    QObjectList entriesChildren = this->children();
    if( entriesChildren.length() < 1 ) {
        return;
    }
    jo.start_object();
    jo.member( "distribution" );
    jo.start_array();
    for( QObject* i : entriesChildren ) {
        entriesList* lst = dynamic_cast<creator::entriesList*>( i );
        if( lst != nullptr ) {
            lst->get_json( jo );
        }
        distributionCollection* dis = dynamic_cast<creator::distributionCollection*>( i );
        if( dis != nullptr ) {
            dis->get_json( jo );
        }
    }
    jo.end_array();
    jo.end_object();
}

creator::entriesList::entriesList( QWidget* parent ) : QTableWidget( parent )
{
    setMinimumSize( 200, 200 );
    setAcceptDrops( true );
    setParent( parent );
    resize( QSize( 400, 200 ) );
    setAcceptDrops( true );
    insertColumn( 0) ;
    insertColumn( 0 );
    insertColumn( 0 );
    insertRow( 0 );
    setHorizontalHeaderLabels( QStringList{ "X", "Item", "Prob" } );
    verticalHeader()->setSectionResizeMode( QHeaderView::Fixed );
    verticalHeader()->setDefaultSectionSize( 12 );
    verticalHeader()->hide();
    horizontalHeader()->resizeSection( 0, 50 );
    horizontalHeader()->resizeSection( 1, 250 );
    horizontalHeader()->resizeSection( 2, 50 );
    setSelectionBehavior( QAbstractItemView::SelectionBehavior::SelectItems );
    setSelectionMode( QAbstractItemView::SelectionMode::SingleSelection );
    show();


}

creator::distributionCollection::distributionCollection( QWidget* parent ){
    this->setObjectName( "distributionCollection" );
    this->setParent( parent );
    setMinimumSize( 200, 200 );
    resize( QSize( 400, 200 ) );

    const int margin_top = 30;
    const int margin_left = 30;
    creator::entriesList* entries_list = new creator::entriesList( this );
    entries_list->move( QPoint( margin_left, margin_top ) );
}

void creator::entriesList::deleteEntriesLine()
{
    this->setObjectName( "entriesList" );
    QWidget* w = qobject_cast<QWidget*>( sender()->parent() );
    if( w ) {
        int row = this->indexAt( w->pos() ).row();
        this->removeRow( row );
        if ( this->rowCount() < 1 ) {
            this->insertRow ( 0 );
        }
    }
}

void creator::entriesList::dragEnterEvent( QDragEnterEvent* event )
{
    if ( event->mimeData()->hasFormat( "text/plain" ) ){
        event->acceptProposedAction();
    }
}

void creator::entriesList::dragMoveEvent( QDragMoveEvent* event )
{
    event->acceptProposedAction();
}


void creator::entriesList::dropEvent( QDropEvent* event )
{
    QString itemText = event->mimeData()->text();
    QObject* sourceListOfItemsOrGroups = event->source();
    //If the property 'items' is true, it's the list of items. Otherwise it's the list of groups. 
    //The added item in add_item is marked as an item if "items" is true or a group if false
    if( sourceListOfItemsOrGroups->property( "items" ).toBool() ) {
        entriesList::add_item( itemText, false );
    } else {
        entriesList::add_item( itemText, true );
    }

    //Notify the item_group_window that the item group has changed
    QObject* parent = this->parent();
    while( parent != nullptr ) {
        if( dynamic_cast<creator::item_group_window*>( parent ) == nullptr ) {
            parent = parent->parent();
        } else {
            QEvent* myEvent = new QEvent( item_group_changed::eventType );
            QCoreApplication::sendEvent( parent, myEvent );
            break;
        }
    }
    event->acceptProposedAction();
}


QEvent::Type creator::item_group_changed::eventType = QEvent::User;
QEvent::Type creator::item_group_changed::registeredType()
{
    if( eventType == QEvent::None ) {
        int generatedType = QEvent::registerEventType();
        eventType = static_cast<QEvent::Type>(generatedType);
    }
    return eventType;
}
