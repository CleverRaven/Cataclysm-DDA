#include "item_group_window.h"

#include "format.h"
#include "item_factory.h"

#include "QtWidgets/qheaderview.h"
#include <QtCore/QCoreApplication>


creator::item_group_window::item_group_window( QWidget *parent, Qt::WindowFlags flags )
    : QMainWindow( parent, flags )
{
    QWidget* wid = new QWidget( this );
    this->setCentralWidget( wid );

    QHBoxLayout* mainRow = new QHBoxLayout;
    QVBoxLayout* mainColumn1 = new QVBoxLayout;
    QVBoxLayout* mainColumn2 = new QVBoxLayout;

    wid->setLayout( mainRow );
    mainRow->addLayout( mainColumn1, 0 );
    mainRow->addLayout( mainColumn2, 1 );

    item_group_json.resize( QSize( 800, 600 ) );
    item_group_json.setReadOnly( true );


    // =========================================================================================
    // first column of boxes


    id_label = new QLabel( "id" );
    id_box = new QLineEdit( "tools_home" );
    id_box->setToolTip( QString( _( "The id of the item_group" ) ) );
    QObject::connect(id_box, &QLineEdit::textChanged, [&]() { write_json(); });


    item_search_label = new QLabel("Search items");
    item_search_box = new QLineEdit;
    item_search_box->setToolTip( QString( _( "Enter text and press return to search for items" ) ) );
    QObject::connect( item_search_box, &QLineEdit::returnPressed, this,
        &item_group_window::items_search_return_pressed );


    QGridLayout* basicInfoLayout = new QGridLayout();
    basicInfoLayout->addWidget( id_label, 0, 0 );
    basicInfoLayout->addWidget( id_box, 0, 1 );
    basicInfoLayout->addWidget( item_search_label, 1, 0 );
    basicInfoLayout->addWidget( item_search_box, 1, 1 );
    mainColumn1->addLayout( basicInfoLayout );

    item_list_total_box = new ListWidget_Drag;
    item_list_total_box->setMinimumSize( QSize( 200, 200 ) );
    item_list_total_box->setDragEnabled( true );
    item_list_total_box->setToolTip( QString( _( "All items" ) ) );
    //Only the item list has 'items' set to true so it distinguishes itself from the group listbox
    //This is used when adding an item to entrieslist and color the bg based on this property
    item_list_total_box->setProperty( "items", true );
    item_list_populate_filtered();
    mainColumn1->addWidget( item_list_total_box );


    group_search_label = new QLabel( "Search groups" );
    group_search_box = new QLineEdit;
    group_search_box->setToolTip( QString( _( "Enter text and press return to search for groups" ) ) );
    QObject::connect(group_search_box, &QLineEdit::returnPressed, this,
        &item_group_window::group_search_return_pressed);

    QHBoxLayout* group_searchLayout = new QHBoxLayout();
    group_searchLayout->addWidget( group_search_label );
    group_searchLayout->addWidget( group_search_box );
    mainColumn1->addLayout( group_searchLayout );


    group_list_total_box = new ListWidget_Drag;
    group_list_total_box->setMinimumSize( QSize( 200, 200 ) );
    group_list_total_box->setDragEnabled( true );
    group_list_total_box->setToolTip( QString( _( "All groups" ) ) );
    group_list_populate_filtered();
    mainColumn1->addWidget( group_list_total_box );

    // =========================================================================================
    // second column

    scrollArea = new QScrollArea;
    scrollArea->setMinimumSize( QSize( 430, 600 ) );
    QSizePolicy mysizepolicy;
    mysizepolicy.setHorizontalPolicy( QSizePolicy::Expanding );
    mysizepolicy.setVerticalPolicy( QSizePolicy::Expanding );
    scrollArea->setSizePolicy( mysizepolicy );
    scrollArea->setWidgetResizable( true );

    verticalBox = new QVBoxLayout;

    entries_box = new QFrame( scrollArea );
    entries_box->setLayout( verticalBox );
    entries_box->setStyleSheet( "background-color:rgb(139,104,168)" );
    scrollArea->setWidget( entries_box );

    QFrame* entriesTopBar = new QFrame;
    entriesTopBar->setStyleSheet( "background-color:rgb(142,229,188)" );
    entriesTopBar->setMaximumHeight( 60 );
    QHBoxLayout* entriesTopBar_Layout = new QHBoxLayout;

    QLabel* entries_label = new QLabel;
    entries_label->setText( QString( "Entries:" ) );

    QPushButton* btnCollection = new QPushButton;
    btnCollection->setText( "+Collection" );
    QPushButton* btnDistribution = new QPushButton;
    btnDistribution->setText( "+Distribution" );
    connect( btnDistribution, &QPushButton::clicked, this, &item_group_window::add_distribution );
    QPushButton* btnItemList = new QPushButton;
    btnItemList->setText( "+Item list" );

    entriesTopBar_Layout->addWidget( entries_label );
    entriesTopBar_Layout->addWidget( btnCollection );
    entriesTopBar_Layout->addWidget( btnDistribution );
    entriesTopBar_Layout->addWidget( btnItemList );

    entriesTopBar->setLayout( entriesTopBar_Layout );
    verticalBox->addWidget( entriesTopBar );
    verticalBox->setSizeConstraint( QLayout::SetNoConstraint );
    verticalBox->setAlignment( entriesTopBar, Qt::AlignTop );
    verticalBox->addStretch();
    
    mainColumn2->addWidget( scrollArea );


    // =========================================================================================
    // Finalize

    this->resize( QSize( 800, 800 ) );
}

void creator::item_group_window::write_json()
{
    std::ostringstream stream;
    JsonOut jo( stream );

    jo.start_object();

    jo.member( "type", "item_group" );
    jo.member( "id", id_box->text().toStdString() );
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
    std::string searchQuery = item_search_box->text().toStdString();
    item_list_populate_filtered( searchQuery );
}

void creator::item_group_window::group_search_return_pressed()
{
    std::string searchQuery = group_search_box->text().toStdString();
    group_list_populate_filtered( searchQuery );
}

void creator::item_group_window::group_list_populate_filtered( std::string searchQuery )
{
    std::string groupID;
    group_list_total_box->clear();
    if( searchQuery == "" ) {
        for( const item_group_id i : item_controller.get()->get_all_group_names() ) {
            groupID = i.c_str();
            QListWidgetItem* new_item = new QListWidgetItem( QString( groupID.c_str() ) );
            group_list_total_box->addItem( new_item );
        }
    } else {
        for( const item_group_id i : item_controller.get()->get_all_group_names() ) {
            groupID = i.c_str();
            if( groupID.find( searchQuery ) != std::string::npos ) {
                QListWidgetItem* new_item = new QListWidgetItem( QString( groupID.c_str() ) );
                group_list_total_box->addItem( new_item );
            }
        }
    }
}

void creator::item_group_window::item_list_populate_filtered( std::string searchQuery )
{
    std::string itemID;
    item_list_total_box->clear();
    if( searchQuery == "" ) {
        for( const itype* i : item_controller->all() ) {
            item tmpItem(i, calendar::turn_zero);
            QListWidgetItem* new_item = new QListWidgetItem( QString( tmpItem.typeId().c_str() ) );
            set_item_tooltip( new_item, tmpItem );
            item_list_total_box->addItem( new_item );
        }
    } else {
        for( const itype* i : item_controller->all() ) {
            item tmpItem(i, calendar::turn_zero);
            itemID = tmpItem.typeId().c_str();
            if( itemID.find( searchQuery ) != std::string::npos ) {
                QListWidgetItem* new_item = new QListWidgetItem(
                    QString(tmpItem.typeId().c_str() ) );
                set_item_tooltip( new_item, tmpItem );
                item_list_total_box->addItem( new_item );
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

void creator::item_group_window::add_distribution() {
    creator::distributionCollection* dis = new creator::distributionCollection;
    QVBoxLayout* b = static_cast<QVBoxLayout*>( entries_box->layout() );
    b->insertWidget( b->count() - 1, dis ); //Add before the stretch element
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

creator::entriesList::entriesList( QWidget* parent ) : QTableWidget( parent )
{
    setMinimumSize( 200, 200 );
    setAcceptDrops( true );
    //setParent( parent );
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
    QObject::connect( this, &QTableWidget::cellChanged, [&]() { change_notify_top_parent(); });
}

creator::distributionCollection::distributionCollection( QWidget* parent ){

    setObjectName( "distributionCollection" );
    setMinimumSize( QSize( 250, 250 ) );
    setStyleSheet( "background-color:rgb(104,160,186)" );

    verticalBox = new QVBoxLayout;
    QFrame* buttonsTopBar = new QFrame;
    buttonsTopBar->setStyleSheet( "background-color:rgb(142,229,188)" );
    buttonsTopBar->setMaximumHeight( 60 );
    QHBoxLayout* buttonsTopBar_Layout = new QHBoxLayout;

    QLabel* title_label = new QLabel;
    title_label->setText( QString( "Distribution:" ) );

    QPushButton* btnCollection = new QPushButton;
    btnCollection->setText( "+Collection" );
    QPushButton* btnDistribution = new QPushButton;
    btnDistribution->setText( "+Distribution" );
    connect( btnDistribution, &QPushButton::clicked, this, &distributionCollection::add_distribution );
    btnItemList = new QPushButton;
    btnItemList->setText( "+Item list" );
    connect( btnItemList, &QPushButton::clicked, this, &distributionCollection::add_entries_list );

    QPushButton* btnDeleteThis = new QPushButton;
    btnDeleteThis->setText( "X" );
    btnDeleteThis->setStyleSheet( "background-color:rgb(206,99,108)" );
    connect( btnDeleteThis, &QPushButton::clicked, this, &distributionCollection::delete_self );

    buttonsTopBar_Layout->addWidget( title_label );
    buttonsTopBar_Layout->addWidget( btnCollection );
    buttonsTopBar_Layout->addWidget( btnDistribution );
    buttonsTopBar_Layout->addWidget( btnItemList );
    buttonsTopBar_Layout->addWidget( btnDeleteThis );

    buttonsTopBar->setLayout( buttonsTopBar_Layout );
    verticalBox->addWidget( buttonsTopBar );
    verticalBox->setAlignment( buttonsTopBar, Qt::AlignTop );
    setLayout( verticalBox );
}


void creator::distributionCollection::delete_self() {
    setParent( nullptr );
    deleteLater();
}


void creator::distributionCollection::add_distribution() {
    creator::distributionCollection* dis = new creator::distributionCollection;
    layout()->addWidget( dis );
}


void creator::distributionCollection::add_entries_list() {
    creator::entriesList* entries_list = new creator::entriesList;
    layout()->addWidget( entries_list );
    btnItemList->setDisabled( true );
}



void creator::distributionCollection::get_json( JsonOut& jo ) {
    QObjectList entriesChildren = this->children();
    if( entriesChildren.length() < 1 ) {
        return;
    }
    jo.start_object();
    jo.member( "distribution" );
    jo.start_array();
    for ( QObject* i : entriesChildren ) {
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

void creator::entriesList::deleteEntriesLine()
{
    QWidget* w = qobject_cast<QWidget*>( sender()->parent() );
    if( w ) {
        int row = this->indexAt( w->pos() ).row();
        this->removeRow( row );
        if ( this->rowCount() < 1 ) {
            this->insertRow ( 0 );
        }
        change_notify_top_parent();
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
    if( sourceListOfItemsOrGroups->property( "items" ).toBool() ) {
        entriesList::add_item( itemText, false );
    } else {
        entriesList::add_item( itemText, true );
    }

    change_notify_top_parent();
    event->acceptProposedAction();
}


//Notify the item_group_window that the item group has changed
void creator::entriesList::change_notify_top_parent()
{
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
}


QEvent::Type creator::item_group_changed::eventType = QEvent::User;
QEvent::Type creator::item_group_changed::registeredType()
{
    if( eventType == QEvent::None ) {
        int generatedType = QEvent::registerEventType();
        eventType = static_cast<QEvent::Type>( generatedType );
    }
    return eventType;
}
