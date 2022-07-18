#include "item_group_window.h"

#include "format.h"
#include "item_factory.h"

#include "QtWidgets/qheaderview.h"
#include <QtCore/QCoreApplication>
#include <QtWidgets/QSpinBox>


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
    scrollArea->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
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
        itemGroupEntry* ent = dynamic_cast<creator::itemGroupEntry*>( i );
        if ( ent != nullptr ) {
            ent->get_json( jo );
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
    creator::distributionCollection* dis = new creator::distributionCollection( this, this );
    dis->set_depth( 0 );
    QVBoxLayout* b = static_cast<QVBoxLayout*>( entries_box->layout() );
    b->insertWidget( b->count() - 1, dis ); //Add before the stretch element
    dis->set_bg_color();
}



creator::itemGroupEntry::itemGroupEntry( QWidget* parent, QString entryText, bool group ) : QFrame( parent )
{
    //setMinimumSize( QSize( 250, 60 ) );
    //setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding) );

    if ( group ) {
        setObjectName("group");
        setStyleSheet( "background-color:rgb(198,149,133)" );
    } else {
        setObjectName("item");
        setStyleSheet( "background-color:rgb(247,236,232)" );
    }
    setMaximumHeight( 60 );
    QHBoxLayout* entryLayout = new QHBoxLayout;

    QPushButton* btnDeleteThis = new QPushButton;
    btnDeleteThis->setText("X");
    btnDeleteThis->setMaximumSize( QSize( 24, 24 ) );
    btnDeleteThis->setStyleSheet("background-color:rgb(206,99,108)");
    connect(btnDeleteThis, &QPushButton::clicked, this, &itemGroupEntry::delete_self);

    title_label = new QLabel;
    title_label->setText( entryText );

    QLabel* prob_label = new QLabel;
    prob_label->setText( QString( "Prob:" ) );
    prob_label->setMaximumSize( QSize( 24, 60 ) );

    prob = new QSpinBox;
    prob->setRange( 0, 100 );
    prob->setValue( 100 );
    prob->setMaximumSize( QSize( 24, 60 ) );
    connect( prob, QOverload<int>::of( &QSpinBox::valueChanged ),
        [=](int i) { change_notify_parent(); } );

    entryLayout->addWidget( btnDeleteThis );
    entryLayout->addWidget( title_label );
    entryLayout->addWidget( prob_label );
    entryLayout->addWidget( prob );

    setLayout( entryLayout );
}


void creator::itemGroupEntry::change_notify_parent() {
    QEvent* myEvent = new QEvent( item_group_changed::eventType );
    QCoreApplication::sendEvent( this->parent(), myEvent );
}



QSize creator::itemGroupEntry::sizeHint() const
{
    return QSize( 300, 60 );
}
QSize creator::itemGroupEntry::minimumSizeHint() const
{
    return QSize( 250, 45 );
}

void creator::itemGroupEntry::delete_self() {
    QObject* myParent = this->parent();
    setParent( nullptr );
    QEvent* myEvent = new QEvent( item_group_changed::eventType );
    QCoreApplication::sendEvent( myParent, myEvent );
    deleteLater();
}


void creator::itemGroupEntry::get_json( JsonOut &jo ) {

    jo.start_object();
    jo.member("item", "test_item" );
    jo.member("prob", "100" );
    jo.end_object();

    //for( int row = 0; row < this->rowCount() - 0; row++ ) {
    //    QTableWidgetItem* item = this->item(row, 1);
    //    if( item == nullptr ) {
    //        break;
    //    }
    //    jo.start_object();
    //    QAbstractItemModel* model = this->model();
    //    QVariant item_text = model->data( model->index( row, 1 ), Qt::DisplayRole );
    //    QVariant prob_text = model->data( model->index( row, 2 ), Qt::DisplayRole );

    //    //If the backgroundcolor is yellow, it's a group. Otherwise it's an item
    //    QBrush item_color = item->backgroundColor();
    //    if( item_color == Qt::yellow ){
    //        jo.member( "group", item_text.toString().toStdString() );
    //    } else {
    //        jo.member( "item", item_text.toString().toStdString() );
    //    }
    //    jo.member( "prob", prob_text.toInt() );
    //    jo.end_object();
    //}
}

creator::distributionCollection::distributionCollection( QWidget* parent, 
                        item_group_window* top_parent ){
    top_parent_widget = top_parent;

    setObjectName( "distributionCollection" );
    //setMinimumSize( QSize( 250, 100 ) );
    setAcceptDrops( true );
    setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding) );

    verticalBox = new QVBoxLayout;
    verticalBox->setSizeConstraint( QLayout::SetMinAndMaxSize );
    //verticalBox->setSizeConstraint( QLayout::SetFixedSize );
    //verticalBox->setSizeConstraint( QLayout::SetNoConstraint );
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

    QPushButton* btnDeleteThis = new QPushButton;
    btnDeleteThis->setText( "X" );
    btnDeleteThis->setMaximumSize( QSize( 25, 25 ) );
    btnDeleteThis->setStyleSheet( "background-color:rgb(206,99,108)" );
    connect( btnDeleteThis, &QPushButton::clicked, this, &distributionCollection::delete_self );

    buttonsTopBar_Layout->addWidget( title_label );
    buttonsTopBar_Layout->addWidget( btnCollection );
    buttonsTopBar_Layout->addWidget( btnDistribution );
    buttonsTopBar_Layout->addWidget( btnDeleteThis );

    buttonsTopBar->setLayout( buttonsTopBar_Layout );
    verticalBox->addWidget( buttonsTopBar );
    verticalBox->setAlignment( buttonsTopBar, Qt::AlignTop );
    setLayout( verticalBox );
}


void creator::distributionCollection::delete_self() {
    //if( depth > 0 ) {
    //    distributionCollection* parent = 
    //        dynamic_cast<creator::distributionCollection*>( this->parent() );
    //    if ( parent != nullptr ) {
    //        setParent( nullptr );
    //        //It's no longer a child of the parent, so the parent needs the size updated
    //        parent->update_size();
    //    }
    //}
    setParent( nullptr );
    change_notify_top_parent();
    deleteLater();
}

void creator::distributionCollection::set_depth( int d ) {
    depth = d;
}

QSize creator::distributionCollection::sizeHint() const
{
    return QSize( 300, 250 );
}
QSize creator::distributionCollection::minimumSizeHint() const
{
    return QSize( 250, 200 );
}

void creator::distributionCollection::update_size() {
    //QObjectList entriesChildren = this->children();
    ////TODO: Find a better way to grow widgets based on the size of their children
    ////Right now it gets 60 for the toolbar and 250 for the other children
    //int length = entriesChildren.length(), height;
    //if (length > 1) {
    //    height = 250 * entriesChildren.length() - 2;
    //}
    //else {
    //    height = 250;
    //}
    //height += 60;
    //setMinimumSize( QSize( 250, height) );
    //this->adjustSize();

    ////Now update the parent size
    //if( depth > 0 ) {
    //    distributionCollection* parent = 
    //        dynamic_cast<creator::distributionCollection*>( this->parent() );
    //    if ( parent != nullptr ) {
    //        parent->update_size();
    //    }
    //}
}


void creator::distributionCollection::add_distribution() {
    creator::distributionCollection* dis = new creator::distributionCollection( this, top_parent_widget );
    dis->set_depth( depth+1 );
    dis->set_bg_color();
    layout()->addWidget( dis );
}

void creator::distributionCollection::set_bg_color() {
    QString colors[8] = { "252,252,252", "244,247,252", "219,228,249", "202,216,249", 
                            "184,204,249", "169,194,252", "165,185,252", "136,171,252" };
    if( depth <= 7 ){
        setStyleSheet( "background-color:rgb("+ colors[depth] + ")" );
    } else {
        setStyleSheet( "background-color:rgb(252,252,252)" );
    }
}




bool creator::distributionCollection::event( QEvent* event )
{
    if( event->type() == item_group_changed::eventType ) {
        change_notify_top_parent();
        return true;
    }
    //call the event method of the base class for the events that aren't handled
    return QFrame::event( event );
}


void creator::distributionCollection::add_entry( QString entryText, bool group ) {
    creator::itemGroupEntry* itemGroupEntry = new creator::itemGroupEntry( this, entryText, group );
    layout()->addWidget( itemGroupEntry );
    //update_size();
}



//Notify the item_group_window that the item group has changed
void creator::distributionCollection::change_notify_top_parent()
{
    QEvent* myEvent = new QEvent( item_group_changed::eventType );
    QCoreApplication::sendEvent( top_parent_widget, myEvent );
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
        itemGroupEntry* ent = dynamic_cast<creator::itemGroupEntry*>( i );
        if( ent != nullptr ) {
            ent->get_json( jo );
        }
        distributionCollection* dis = dynamic_cast<creator::distributionCollection*>( i );
        if( dis != nullptr ) {
            dis->get_json( jo );
        }
    }
    jo.end_array();
    jo.end_object();
}


void creator::distributionCollection::dragEnterEvent( QDragEnterEvent* event )
{
    if ( event->mimeData()->hasFormat( "text/plain" ) ){
        event->acceptProposedAction();
    }
}

void creator::distributionCollection::dragMoveEvent( QDragMoveEvent* event )
{
    event->acceptProposedAction();
}


void creator::distributionCollection::dropEvent( QDropEvent* event )
{
    QString itemText = event->mimeData()->text();
    QObject* sourceListOfItemsOrGroups = event->source();
    //If the property 'items' is true, it's the list of items. Otherwise it's the list of groups.
    if( sourceListOfItemsOrGroups->property( "items" ).toBool() ) {
        add_entry( itemText, false );
    } else {
        add_entry( itemText, true );
    }

    //change_notify_top_parent();
    event->acceptProposedAction();
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
