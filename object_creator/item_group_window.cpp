#include "item_group_window.h"

#include "format.h"
#include "item_factory.h"
#include "iteminfo_query.h"
#include "output.h"
#include <regex>

#include "QtWidgets/qheaderview.h"
#include <QtCore/QCoreApplication>

creator::item_group_window::item_group_window( QWidget *parent, Qt::WindowFlags flags )
    : QMainWindow( parent, flags )
{
    QWidget* wid = new QWidget( this );
    this->setCentralWidget( wid );
    this->setAcceptDrops( true );

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
    QObject::connect( id_box, &QLineEdit::textChanged, [&]() { write_json(); } );

    QString tooltipText = "In a Collection each entry is chosen independently from the other\n";
    tooltipText += "entries. Therefore, the probability associated with each entry is absolute,";
    tooltipText += "\nin the range of 0...1. In the json files it is implemented as a percentage";
    tooltipText += "\n(with values from 0 to 100). \n\nA Distribution is a weighted list.";
    tooltipText += "\nExactly one entry is chosen from it.The probability of each entry is";
    tooltipText += "\nrelative to the probability of the other entries. A probability";
    tooltipText += "\nof 0 (or negative) means it is never chosen.";

    QLabel* subtype_label = new QLabel( "Subtype" );
    subtype = new QComboBox;
    subtype->setToolTip( tooltipText );
    subtype->addItems( QStringList{ "none", "collection", "distribution" } );
    connect( subtype, QOverload<int>::of( &QComboBox::currentIndexChanged ),
        [=]( int index ) { write_json(); } );

    item_search_label = new QLabel("Search items");
    item_search_box = new QLineEdit;
    item_search_box->setToolTip( QString( _( "Enter text and press return to search for items" ) ) );
    QObject::connect( item_search_box, &QLineEdit::returnPressed, this,
        &item_group_window::items_search_return_pressed );

    QGridLayout* basicInfoLayout = new QGridLayout();
    basicInfoLayout->addWidget( id_label, 0, 0 );
    basicInfoLayout->addWidget( id_box, 0, 1 );
    basicInfoLayout->addWidget( subtype_label, 1, 0 );
    basicInfoLayout->addWidget( subtype, 1, 1 );
    basicInfoLayout->addWidget( item_search_label, 2, 0 );
    basicInfoLayout->addWidget( item_search_box, 2, 1 );
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
    QObject::connect( group_search_box, &QLineEdit::returnPressed, this,
        &item_group_window::group_search_return_pressed );

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


    group_container = new nested_group_container( scrollArea, this );
    scrollArea->setWidget( group_container );
    mainColumn2->addWidget( scrollArea );


    // =========================================================================================
    // Finalize

    this->resize( QSize( 1024, 800 ) );
}

void creator::item_group_window::write_json()
{
    std::ostringstream stream;
    JsonOut jo( stream );

    jo.start_object();

    jo.member( "type", "item_group" );
    jo.member( "id", id_box->text().toStdString() );
    std::string sub = subtype->currentText().toStdString();
    if( sub != "none" ) {
        jo.member( "subtype", subtype->currentText().toStdString() );
    }

    jo.member( "entries" );
    jo.start_array();
    QObjectList entriesChildren = group_container->children();
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
    TextJsonIn jsin( in_stream );

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
            //Inline groups get an ID assigned. We don't add those to the list
            //Inline groups' ID contains a ' ' so we filter for that
            if( groupID.find( " " ) == std::string::npos ) {
                QListWidgetItem* new_item = new QListWidgetItem( QString( groupID.c_str() ) );
                set_group_tooltip( new_item, i );
                group_list_total_box->addItem( new_item );
            }
        }
    } else {
        for( const item_group_id i : item_controller.get()->get_all_group_names() ) {
            groupID = i.c_str();
            if( groupID.find( searchQuery ) != std::string::npos ) {
                QListWidgetItem* new_item = new QListWidgetItem( QString( groupID.c_str() ) );
                set_group_tooltip( new_item, i );
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
            QListWidgetItem* new_item = new QListWidgetItem( QString( i->get_id().c_str() ) );
            set_item_tooltip( new_item, i );
            item_list_total_box->addItem( new_item );
        }
    } else {
        for( const itype* i : item_controller->all() ) {
            itemID = i->get_id().c_str();
            if( itemID.find( searchQuery ) != std::string::npos ) {
                QListWidgetItem* new_item = new QListWidgetItem( QString( itemID.c_str() ) );
                set_item_tooltip( new_item, i );
                item_list_total_box->addItem( new_item );
            }
        }
    }
}

void creator::item_group_window::set_group_tooltip( QListWidgetItem* new_item, 
                                                    const item_group_id tmpGroupID )
{
    std::string tooltip = "items: ";
    for ( const itype* type : item_group::every_possible_item_from( tmpGroupID ) ) {
        tooltip += "\n";
        tooltip += type->get_id().c_str();
    }
    new_item->setToolTip( QString( _( tooltip.c_str() ) ) );
}

void creator::item_group_window::set_item_tooltip( QListWidgetItem* new_item, 
                                                    const itype* tmpItype )
{
    std::string tooltip = "id: ";
    tooltip += tmpItype->get_id().c_str();
    tooltip += "\nname: ";
    tooltip += tmpItype->nname( 1 );
    tooltip += "\ntype: ";
    tooltip += tmpItype->get_item_type_string();

    std::vector<iteminfo_parts> vol_weight = { iteminfo_parts::BASE_VOLUME, 
                                            iteminfo_parts::BASE_WEIGHT };
    item tempItem( tmpItype->get_id() );

    std::vector<iteminfo> info_v;
    const iteminfo_query query_v( vol_weight );
    tempItem.info( info_v, &query_v, 1 );
    std::string info = "\ninfo: " + format_item_info( info_v, {} );

    //We get the info, but for some items additional info is added below --
    //We remove the extra info we don't need and also remove any color tags
    std::size_t found = info.find("--");
    if( found != std::string::npos ) {
        info.erase( found );
    }
    std::regex tags( "<[^>]*>" );
    std::string remove{};

    tooltip += std::regex_replace( info, tags, remove );
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



creator::distributionCollection::distributionCollection( bool isCollection, 
                            QWidget* parent, item_group_window* top_parent ){
    top_parent_widget = top_parent;

    setObjectName( "distributionCollection" );
    setAcceptDrops( true );
    setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum ) );
    
    verticalBox = new QVBoxLayout;
    verticalBox->setSizeConstraint( QLayout::SetMinAndMaxSize );
    QFrame* buttonsTopBar = new QFrame;
    buttonsTopBar->setStyleSheet( "background-color:rgb(217,179,255)" );
    buttonsTopBar->setMaximumHeight( 60 );
    QHBoxLayout* buttonsTopBar_Layout = new QHBoxLayout;

    entryType = new QComboBox;
    entryType->addItems( QStringList{ "collection", "distribution" } );
    entryType->setToolTip( QString( _( "Change the type to a collection or a distribution" ) ) );
    connect( entryType, QOverload<int>::of( &QComboBox::currentIndexChanged ),
        [=]( int index ) { change_notify_top_parent(); } );
    if( isCollection ) {
        entryType->setCurrentIndex( 0 );
    } else {
        entryType->setCurrentIndex( 1 );
    }
    

    QPushButton* btnCollection = new QPushButton;
    btnCollection->setText( "+Collection" );
    btnCollection->setToolTip( QString( _( "Add a new collection" ) ) );
    connect( btnCollection, &QPushButton::clicked, this, &distributionCollection::add_collection );
    QPushButton* btnDistribution = new QPushButton;
    btnDistribution->setText( "+Distribution" );
    btnDistribution->setToolTip( QString( _( "Add a new distribution" ) ) );
    connect( btnDistribution, &QPushButton::clicked, this, &distributionCollection::add_distribution );

    QLabel* prob_label = new QLabel;
    prob_label->setText( QString( "Prob:" ) );
    prob_label->setMinimumSize( QSize( 30, 24 ) );
    prob_label->setMaximumSize( QSize( 35, 24 ) );

    prob = new QSpinBox;
    prob->setRange( 0, 100 );
    prob->setMinimumSize( QSize( 45, 24 ) );
    prob->setMaximumSize( QSize( 50, 24 ) );
    QString tooltipText = "A probability of 0 (or negative) means the entry is never chosen;\n";
    tooltipText += "a probability of 100 % means it's always chosen. The default is 100,\n";
    tooltipText += "because it's the most useful value. A default of 0 would mean the entry\n";
    tooltipText += "could be removed anyway. Setting this to 0 or 100 will remove the";
    tooltipText += " property from json";
    prob->setToolTip( tooltipText );
    connect( prob, QOverload<int>::of( &QSpinBox::valueChanged ),
        [=]( int i ) { change_notify_top_parent(); } );

    QPushButton* btnDeleteThis = new QPushButton;
    btnDeleteThis->setText( "X" );
    btnDeleteThis->setMaximumSize( QSize( 25, 25 ) );
    btnDeleteThis->setStyleSheet( "background-color:rgb(206,99,108)" );
    btnDeleteThis->setToolTip( QString( _( "Delete this" ) ) );
    connect( btnDeleteThis, &QPushButton::clicked, this, &distributionCollection::delete_self );

    buttonsTopBar_Layout->addWidget( entryType );
    buttonsTopBar_Layout->addWidget( btnCollection );
    buttonsTopBar_Layout->addWidget( btnDistribution );
    buttonsTopBar_Layout->addWidget( prob_label );
    buttonsTopBar_Layout->addWidget( prob );
    buttonsTopBar_Layout->addWidget( btnDeleteThis );

    buttonsTopBar->setLayout( buttonsTopBar_Layout );
    verticalBox->addWidget( buttonsTopBar );
    verticalBox->setAlignment( buttonsTopBar, Qt::AlignTop );
    setLayout( verticalBox );
    setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
    setLineWidth( 2 );
}

void creator::distributionCollection::delete_self() {
    setParent( nullptr );
    change_notify_top_parent();
    deleteLater();
}

void creator::distributionCollection::set_depth( int d ) {
    depth = d;
}

QSize creator::distributionCollection::sizeHint() const
{
    return QSize( 300, 100 );
}

QSize creator::distributionCollection::minimumSizeHint() const
{
    return QSize( 250, 80 );
}

void creator::distributionCollection::add_distribution() {
    creator::distributionCollection* dis = new creator::distributionCollection( false, 
                                            this, top_parent_widget );
    dis->set_depth( depth+1 );
    dis->set_bg_color();
    layout()->addWidget( dis );
    change_notify_top_parent();
}

void creator::distributionCollection::add_collection() {
    creator::distributionCollection* dis = new creator::distributionCollection( true, 
                                            this, top_parent_widget );
    dis->set_depth( depth+1 );
    dis->set_bg_color();
    layout()->addWidget( dis );
    change_notify_top_parent();
}

void creator::distributionCollection::set_bg_color() {
    QString colors[8] = { "204,217,255", "179,198,255", "153,179,255", "128,159,255", 
                            "102,140,255", "77,121,255", "51,102,255", "26,83,255" };
    if( depth <= 7 ){
        setStyleSheet( "background-color:rgb("+ colors[depth] + ")" );
    } else {
        setStyleSheet( "background-color:rgb(252,252,252)" );
    }
}

void creator::distributionCollection::add_entry( QString entryText, bool group ) {
    creator::itemGroupEntry* itemGroupEntry = new creator::itemGroupEntry( this, entryText, 
                                                                group, top_parent_widget);
    layout()->addWidget( itemGroupEntry );
    change_notify_top_parent();
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
    jo.member( entryType->currentText().toStdString() );
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
    int pr = prob->value();
    if( pr ) {
        //Only add if it's less then 100 since 100 is the default
        if ( pr < 100 ) {
            jo.member( "prob", pr );
        }
    }
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
    event->acceptProposedAction();
}

creator::nested_group_container::nested_group_container( QWidget* parent, 
                        item_group_window* top_parent ) : QFrame( parent )
{
    top_parent_widget = top_parent;
    verticalBox = new QVBoxLayout;

    setLayout( verticalBox );
    setStyleSheet( "background-color:rgb(139,104,168)" );
    setAcceptDrops( true );

    QFrame* entriesTopBar = new QFrame;
    entriesTopBar->setStyleSheet( "background-color:rgb(217,179,255)" );
    entriesTopBar->setMaximumHeight( 60 );
    QHBoxLayout* entriesTopBar_Layout = new QHBoxLayout;

    QLabel* entries_label = new QLabel;
    entries_label->setText( QString( "Entries:" ) );

    QPushButton* btnCollection = new QPushButton;
    btnCollection->setText( "+Collection" );
    btnCollection->setToolTip( QString( _( "Add a new collection" ) ) );
    connect( btnCollection, &QPushButton::clicked, this, &nested_group_container::add_collection );
    QPushButton* btnDistribution = new QPushButton;
    btnDistribution->setText( "+Distribution" );
    btnDistribution->setToolTip( QString( _( "Add a new distribution" ) ) );
    connect( btnDistribution, &QPushButton::clicked, this, &nested_group_container::add_distribution );

    entriesTopBar_Layout->addWidget( entries_label );
    entriesTopBar_Layout->addWidget( btnCollection );
    entriesTopBar_Layout->addWidget( btnDistribution );

    entriesTopBar->setLayout( entriesTopBar_Layout );
    verticalBox->addWidget( entriesTopBar );
    verticalBox->setAlignment( entriesTopBar, Qt::AlignTop );
    verticalBox->addStretch();
}

//Notify the item_group_window that the item group has changed
void creator::nested_group_container::change_notify_top_parent()
{
    QEvent* myEvent = new QEvent( item_group_changed::eventType );
    QCoreApplication::sendEvent( top_parent_widget, myEvent );
}

void creator::nested_group_container::add_distribution() {
    creator::distributionCollection* dis = new creator::distributionCollection( false, 
                                        this, top_parent_widget );
    dis->set_depth( 0 );
    QVBoxLayout* b = static_cast<QVBoxLayout*>( this->layout() );
    b->insertWidget( b->count() - 1, dis ); //Add before the stretch element
    dis->set_bg_color();
    change_notify_top_parent();
}

void creator::nested_group_container::add_collection() {
    creator::distributionCollection* col = new creator::distributionCollection( true, 
                                        this, top_parent_widget );
    col->set_depth( 0 );
    QVBoxLayout* b = static_cast<QVBoxLayout*>( this->layout() );
    b->insertWidget( b->count() - 1, col ); //Add before the stretch element
    col->set_bg_color();
    change_notify_top_parent();
}

void creator::nested_group_container::add_entry( QString entryText, bool group ) {
    creator::itemGroupEntry* itemGroupEntry = new creator::itemGroupEntry( this, entryText,
        group, top_parent_widget );
    QVBoxLayout* b = static_cast<QVBoxLayout*>( this->layout() );
    b->insertWidget( b->count() - 1, itemGroupEntry ); //Add before the stretch element
    change_notify_top_parent();
}

void creator::nested_group_container::dragEnterEvent( QDragEnterEvent* event )
{
    if ( event->mimeData()->hasFormat( "text/plain" ) ){
        event->acceptProposedAction();
    }
}

void creator::nested_group_container::dragMoveEvent( QDragMoveEvent* event )
{
    event->acceptProposedAction();
}

void creator::nested_group_container::dropEvent( QDropEvent* event )
{
    QString itemText = event->mimeData()->text();
    QObject* sourceListOfItemsOrGroups = event->source();
    //If the property 'items' is true, it's the list of items. Otherwise it's the list of groups.
    if( sourceListOfItemsOrGroups->property( "items" ).toBool() ) {
        add_entry( itemText, false );
    } else {
        add_entry( itemText, true );
    }
    event->acceptProposedAction();
}

creator::itemGroupEntry::itemGroupEntry( QWidget* parent, QString entryText, bool group, 
                            item_group_window* top_parent ) : QFrame( parent )
{
    top_parent_widget = top_parent;

    if ( group ) {
        setObjectName( "group" );
        setStyleSheet( "background-color:rgb(255,204,153)" );
    } else {
        setObjectName( "item" );
        setStyleSheet( "background-color:rgb(247,236,232)" );
    }
    setMaximumHeight( 60 ); 
    QHBoxLayout* entryLayout = new QHBoxLayout;

    title_label = new QLabel;
    title_label->setText( entryText );
    title_label->setStyleSheet( "font: 10pt;" );

    QLabel* prob_label = new QLabel;
    prob_label->setText( QString( "Prob:" ) );
    prob_label->setMinimumSize( QSize( 30, 24 ) );
    prob_label->setMaximumSize( QSize( 35, 24 ) );

    prob = new QSpinBox;
    prob->setRange( 0, 100 );
    prob->setValue( 100 );
    QString tooltipText = "A probability of 0 (or negative) means the entry is never chosen;\n";
    tooltipText += "a probability of 100 % means it's always chosen. The default is 100,\n";
    tooltipText += "because it's the most useful value. A default of 0 would mean the entry\n";
    tooltipText += "could be removed anyway. Setting this to 0 or 100 will remove the";
    tooltipText += "property from json";
    prob->setToolTip( tooltipText );
    prob->setMinimumSize( QSize( 45, 24 ) );
    prob->setMaximumSize( QSize( 50, 24 ) );
    connect( prob, QOverload<int>::of( &QSpinBox::valueChanged ),
        [=]( int i ) { change_notify_top_parent(); } );

    QLabel* charges_label = new QLabel;
    charges_label->setText( QString( "Charges:" ) );
    charges_label->setMinimumSize( QSize( 42, 24 ) );
    charges_label->setMaximumSize( QSize( 47, 24) );

    charges_min = new QSpinBox;
    charges_min->setRange( 0, INT_MAX );
    charges_min->setMinimumSize( QSize( 45, 24 ) );
    charges_min->setMaximumSize( QSize( 50, 24 ) );
    tooltipText = "The minimum amount of charges. Only shows up in JSON if max has ";
    tooltipText += "a greater value then 0";
    charges_min->setToolTip( tooltipText );
    connect( charges_min, QOverload<int>::of( &QSpinBox::valueChanged ),
        [=]( int i ) { change_notify_top_parent(); } );

    charges_max = new QSpinBox;
    charges_max->setRange( 0, INT_MAX );
    charges_max->setMinimumSize( QSize( 45, 24 ) );
    charges_max->setMaximumSize( QSize( 50, 24 ) );
    tooltipText = "The maximum amount of charges. Only shows up in JSON if the value ";
    tooltipText += "is greater then 0";
    charges_max->setToolTip( tooltipText );
    connect( charges_max, QOverload<int>::of( &QSpinBox::valueChanged ),
        [=]( int i ) { change_notify_top_parent(); } );

    QPushButton* btnDeleteThis = new QPushButton;
    btnDeleteThis->setText( "X" );
    btnDeleteThis->setMaximumSize( QSize( 24, 24 ) );
    btnDeleteThis->setStyleSheet( "background-color:rgb(206,99,108)" );
    btnDeleteThis->setToolTip( QString( _( "Delete this entry from the list" ) ) );
    connect( btnDeleteThis, &QPushButton::clicked, this, &itemGroupEntry::delete_self );

    entryLayout->addWidget( title_label );
    entryLayout->addWidget( prob_label );
    entryLayout->addWidget( prob );
    entryLayout->addWidget( charges_label );
    entryLayout->addWidget( charges_min );
    entryLayout->addWidget( charges_max );
    entryLayout->addWidget( btnDeleteThis );

    setLayout( entryLayout );
}

void creator::itemGroupEntry::change_notify_top_parent() {
    QEvent* myEvent = new QEvent( item_group_changed::eventType );
    QCoreApplication::sendEvent( top_parent_widget, myEvent );
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
    change_notify_top_parent();
    deleteLater();
}

void creator::itemGroupEntry::get_json( JsonOut &jo ) {

    jo.start_object();
    if( this->objectName() == "item" ) {
        jo.member( "item", title_label->text().toStdString() );
    } else {
        jo.member( "group", title_label->text().toStdString() );
    }
    int pr = prob->value(); //If prob is 0, we omit prob entirely
    if( pr ) {
        //Only add if it's less then 100 since 100 is the default
        if( pr < 100 ) {
            jo.member( "prob", pr );
        }
    }
    pr = charges_max->value(); //If charges-max is 0, we omit charges entirely
    if( pr ) {
        jo.member( "charges-min", charges_min->value() );
        jo.member( "charges-max", pr );
    }
    jo.end_object();
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
