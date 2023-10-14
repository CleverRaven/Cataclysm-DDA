#include "item_group_window.h"

#include "format.h"
#include "item_factory.h"
#include "iteminfo_query.h"
#include "output.h"
#include <regex>

#include "QtWidgets/qheaderview.h"
#include <QtCore/QCoreApplication>

creator::item_group_window::item_group_window( QWidget *parent, Qt::WindowFlags flags )
    : QWidget ( parent, flags )
{

    this->setAcceptDrops( true );

    QHBoxLayout* mainRow = new QHBoxLayout;
    QVBoxLayout* mainColumn1 = new QVBoxLayout;
    QVBoxLayout* mainColumn2 = new QVBoxLayout;
    QVBoxLayout* mainColumn3 = new QVBoxLayout;

    this->setLayout( mainRow );
    mainRow->addLayout( mainColumn1, 0 );
    mainRow->addLayout( mainColumn2, 1 );
    mainRow->addLayout( mainColumn3, 2 );

    // =========================================================================================
    // first column of boxes


    QString tooltipText = "The ID of the item_group. For vanilla, this has to be unique";
    tooltipText += "\nfor mods, if you use an ID that's already present in vanilla,";
    tooltipText += "\nit will overwrite that item_group, unless the 'extend' property is used.";
    tooltipText += "\nFor mods, if you create an unique item_group ID, it will act like";
    tooltipText += "\nany other item_group of course.";

    id_label = new QLabel( "Id" );
    id_box = new QLineEdit( "tools_home" );
    id_box->setToolTip( tooltipText );
    QObject::connect( id_box, &QLineEdit::textChanged, [&]() { write_json(); } );


    tooltipText = "Add a comment to this item_group. This has no function in-game.";
    tooltipText += "\nThe only purpose is to let other developers know something about ";
    tooltipText += "\nthis item_group.";

    comment_label = new QLabel( "Comment" );
    comment_box = new QLineEdit( "" );
    comment_box->setToolTip( tooltipText );
    QObject::connect( comment_box, &QLineEdit::textChanged, [&]() { write_json(); } );

    ammo_frame = new simple_property_widget( this, QString( "ammo" ), 
                                            property_type::NUMBER, this );
    tooltipText = "specifies the percent chance that the entries will spawn fully loaded(if it";
    tooltipText += "\nneeds a magazine, it will be added for you). Defaults to 0 if unspecified.";
    ammo_frame->setToolTip( tooltipText );

    magazine_frame = new simple_property_widget( this, QString( "magazine" ), 
                                            property_type::NUMBER, this );
    tooltipText = "specifies the percent chance that the entries will spawn with a";
    tooltipText += "\nmagazine. Defaults to 0 if unspecified.";
    magazine_frame->setToolTip( tooltipText );

    QLabel* subtype_label = new QLabel( "Subtype" );
    subtype = new QComboBox;
    tooltipText = "In a Collection each entry is chosen independently from the other\n";
    tooltipText += "entries. Therefore, the probability associated with each entry is absolute,";
    tooltipText += "\nin the range of 0...1. In the json files it is implemented as a percentage";
    tooltipText += "\n(with values from 0 to 100). \n\nA Distribution is a weighted list.";
    tooltipText += "\nExactly one entry is chosen from it.The probability of each entry is";
    tooltipText += "\nrelative to the probability of the other entries. A probability";
    tooltipText += "\nof 0 (or negative) means it is never chosen.";
    subtype->setToolTip( tooltipText );
    subtype->addItems( QStringList{ "none", "collection", "distribution" } );
    connect( subtype, QOverload<int>::of( &QComboBox::currentIndexChanged ),
        [=]( ) { write_json(); } );

    QLabel* containerItem_label = new QLabel;
    containerItem_label->setText( QString( "Container-item:" ) );

    containerItem = new QLineEdit;
    tooltipText = "causes all the items of the group to spawn in a container,";
    tooltipText += "\nrather than as separate top-level items. If the items might";
    tooltipText += "\nnot all fit in the container, you must specify how to deal";
    tooltipText += "\nwith the overflow by setting on_overflow to either discard";
    tooltipText += "\nto discard items at random until they fit, or spill to have";
    tooltipText += "\nthe excess items be spawned alongside the container.";
    containerItem->setToolTip( tooltipText );
    QObject::connect( containerItem, &QLineEdit::textChanged, [&]() { write_json(); } );

    QLabel* overflow_label = new QLabel( "Overflow" );
    overflow = new QComboBox;
    tooltipText = "Discard:";
    tooltipText += "\nDiscard items at random until they fit";
    tooltipText += "\nSpill:";
    tooltipText += "\nHave the excess items be spawned alongside the container.";
    tooltipText += "\n\nWill only show up in JSON if container-item is set.";
    overflow->setToolTip( tooltipText );
    overflow->addItems( QStringList{ "discard", "spill" } );
    connect( overflow, QOverload<int>::of( &QComboBox::currentIndexChanged ),
        [=]( ) { write_json(); } );

    item_search_label = new QLabel("Search items");
    item_search_box = new QLineEdit;
    item_search_box->setToolTip( QString( _( "Enter text and press return to search for items" ) ) );
    QObject::connect( item_search_box, &QLineEdit::returnPressed, this,
        &item_group_window::items_search_return_pressed );

    QGridLayout* basicInfoLayout = new QGridLayout();
    basicInfoLayout->addWidget( id_label, 0, 0 );
    basicInfoLayout->addWidget( id_box, 0, 1 );
    basicInfoLayout->addWidget( comment_label, 1, 0 );
    basicInfoLayout->addWidget( comment_box, 1, 1 );
    basicInfoLayout->addWidget( ammo_frame, 2, 0, 1, 2 );
    basicInfoLayout->addWidget( magazine_frame, 3, 0, 1, 2 );
    basicInfoLayout->addWidget( subtype_label, 4, 0 );
    basicInfoLayout->addWidget( subtype, 4, 1 );
    basicInfoLayout->addWidget( containerItem_label, 5, 0 );
    basicInfoLayout->addWidget( containerItem, 5, 1 );
    basicInfoLayout->addWidget( overflow_label, 6, 0 );
    basicInfoLayout->addWidget( overflow, 6, 1 );
    basicInfoLayout->addWidget( item_search_label, 7, 0 );
    basicInfoLayout->addWidget( item_search_box, 7, 1 );
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
    // third column

    item_group_json.setMinimumSize( QSize( 400, 300 ) );
    item_group_json.setMaximumWidth(500);
    item_group_json.setReadOnly( true );
    mainColumn3->addWidget( &item_group_json );

    // =========================================================================================
    // Finalize

    this->resize( QSize( 1280, 800 ) );
}

void creator::item_group_window::write_json()
{
    std::ostringstream stream;
    JsonOut jo( stream );

    jo.start_object();
    jo.member( "type", "item_group" );
    jo.member( "id", id_box->text().toStdString() );
    if( comment_box->text().size() > 0 ) {
        jo.member( "//", comment_box->text().toStdString() );
    }
    this->ammo_frame->get_json( jo );
    this->magazine_frame->get_json( jo );
    

    std::string sub = subtype->currentText().toStdString();
    if( sub != "none" ) {
        jo.member( "subtype", subtype->currentText().toStdString() );
    }
    if( containerItem->text().size() > 0 ) {
        jo.member( "container-item", containerItem->text().toStdString() );
        jo.member( "overflow", overflow->currentText().toStdString() );
    }

    group_container->get_json( jo );
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
        for( const item_group_id &i : item_controller.get()->get_all_group_names() ) {
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
        for( const item_group_id &i : item_controller.get()->get_all_group_names() ) {
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

    if( event->type() == item_group_changed::eventType || 
        event->type() == property_changed::eventType ) {
        write_json();
        return true;
    }
    //call the event method of the base class for the events that aren't handled
    return QWidget::event( event );
}



creator::distributionCollection::distributionCollection( bool isCollection, 
                                            item_group_window* top_parent ){
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
        [=]( ) { change_notify_top_parent(); } );
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
        [=]( ) { change_notify_top_parent(); } );

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
                                            top_parent_widget );
    dis->set_depth( depth+1 );
    dis->set_bg_color();
    layout()->addWidget( dis );
    change_notify_top_parent();
}

void creator::distributionCollection::add_collection() {
    creator::distributionCollection* dis = new creator::distributionCollection( true, 
                                            top_parent_widget );
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
                                        top_parent_widget );
    dis->set_depth( 0 );
    QVBoxLayout* b = static_cast<QVBoxLayout*>( this->layout() );
    b->insertWidget( b->count() - 1, dis ); //Add before the stretch element
    dis->set_bg_color();
    change_notify_top_parent();
}

void creator::nested_group_container::add_collection() {
    creator::distributionCollection* col = new creator::distributionCollection( true, 
                                        top_parent_widget );
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

void creator::nested_group_container::get_json( JsonOut &jo ) {
    jo.member( "entries" );
    jo.start_array();
    QObjectList entriesChildren = this->children();
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
    QHBoxLayout* entryLayout = new QHBoxLayout;
    entryLayout->setMargin( 0 );
    entryLayout->setContentsMargins( 4,4,4,4 );

    title_label = new QLabel;
    title_label->setText( entryText );
    title_label->setToolTip( entryText );
    title_label->setStyleSheet( "font: 10pt;" );

    //This layout will contain all the properties and wrap the widgets as needed
    flowLayout = new FlowLayout;
    flowLayout->setMargin( 0 );
    flowLayout->setContentsMargins( 0,4,0,0 );



    prob_frame = new simple_property_widget( this, QString( "prob" ), 
                                            property_type::NUMBER, this );
    QString tooltipText = "A probability of 0 (or negative) means the entry is never chosen;";
    tooltipText += "\na probability of 100 % means it's always chosen. The default is 100,";
    tooltipText += "\nbecause it's the most useful value. A default of 0 would mean the entry";
    tooltipText += "\ncould be removed anyway. Setting this to 0 or 100 will remove the";
    tooltipText += "\nproperty from json";
    prob_frame->setToolTip( tooltipText );
    prob_frame->allow_hiding( true );


    count_frame = new simple_property_widget( this, QString( "count" ), 
                                            property_type::MINMAX, this );
    count_frame->hide();
    count_frame->allow_hiding( true );
    tooltipText = "Count:";
    tooltipText += "\nMakes the item spawn repeat, each time creating a new item.";
    tooltipText += "\n\ncount-min:";
    tooltipText += "\nThe game will repeat the item spawn at least this many times";
    tooltipText += "\nOnly shows up in JSON if the value is greater then 0";
    tooltipText += "\nSetting this equal to count-max will set the JSON value to count: <number>";
    tooltipText += "\nIf only count-min is set, the JSON value will simply be count: <number>";
    tooltipText += "\n\ncount-max:";
    tooltipText += "\nThe game will repeat the item spawn up to this many times";
    tooltipText += "\nOnly shows up in JSON if the value is greater then 0";
    tooltipText += "\nSetting this equal to count-min will set the JSON value to count: <number>";
    tooltipText += "\nIf only count-max is set, the JSON value will simply be count: <number>";
    count_frame->setToolTip( tooltipText );

    charges_frame = new simple_property_widget( this, QString( "charges" ), 
                                            property_type::MINMAX, this );
    charges_frame->hide();
    charges_frame->allow_hiding( true );
    tooltipText = "The minimum amount of charges. Only shows up in JSON if max has ";
    tooltipText += "\na greater value then 0";
    charges_frame->setToolTip( tooltipText );

    damage_frame = new simple_property_widget( this, QString( "damage" ), 
                                            property_type::MINMAX, this );
    damage_frame->hide();
    damage_frame->allow_hiding( true );
    tooltipText = "The amount of damage this item has taken. 0 means undamaged, 3 means maximum damage.";
    tooltipText += "\nIf both min and max are set, a random damage value between those two is selected.";
    damage_frame->set_minmax_limits( 0, 3, 0, 3 );
    damage_frame->setToolTip( tooltipText );

    contentsItem_frame = new simple_property_widget( this, QString( "contents-item" ), 
                                            property_type::LINEEDIT, this );
    contentsItem_frame->hide();
    contentsItem_frame->allow_hiding( true );
    tooltipText = "is added as contents of the created item. It is not checked if they can be";
    tooltipText += "\nput into the item. This allows water, that contains a book, that contains";
    tooltipText += "\na steel frame, that contains a corpse.";
    contentsItem_frame->setToolTip( tooltipText );

    contentsGroup_frame = new simple_property_widget( this, QString( "contents-group" ), 
                                            property_type::LINEEDIT, this );
    contentsGroup_frame->hide();
    contentsGroup_frame->allow_hiding( true );
    tooltipText = "is added as contents of the created item. It is not checked if they can be";
    tooltipText += "\nput into the item. This allows water, that contains a book, that contains";
    tooltipText += "\na steel frame, that contains a corpse.";
    contentsGroup_frame->setToolTip( tooltipText );

    containerItem_frame = new simple_property_widget( this, QString( "container-item" ), 
                                            property_type::LINEEDIT, this );
    containerItem_frame->hide();
    containerItem_frame->allow_hiding( true );
    tooltipText = "The container that the item spawns in.";
    tooltipText += "\nWill override the item's default container.";
    containerItem_frame->setToolTip( tooltipText );

    ammoItem_frame = new simple_property_widget( this, QString( "ammo-item" ), 
                                            property_type::LINEEDIT, this );
    ammoItem_frame->hide();
    ammoItem_frame->allow_hiding( true );
    tooltipText = "Can be specified for guns and magazines in the entries ";
    tooltipText += "\narray to use a non-default ammo type.";
    ammoItem_frame->setToolTip( tooltipText );

    entryWrapper_frame = new simple_property_widget( this, QString( "entry-wrapper" ), 
                                            property_type::LINEEDIT, this );
    entryWrapper_frame->hide();
    entryWrapper_frame->allow_hiding( true );
    tooltipText = "Used for spawning lots of non-stackable items inside a container. ";
    tooltipText += "\nInstead of creating a dedicated itemgroup for that, you can use this field to ";
    tooltipText += "\ndefine that inside an entry. Note that you may want to set ";
    tooltipText += "\ncontainer-item to null to override the item's default container.";
    entryWrapper_frame->setToolTip( tooltipText );

    sealed_frame = new simple_property_widget( this, QString( "sealed" ), 
                                            property_type::BOOLEAN, this );
    sealed_frame->hide();
    sealed_frame->allow_hiding( true );
    tooltipText = "If true, a container will be sealed when the item spawns. Default is true.";
    sealed_frame->setToolTip( tooltipText );

    if( !group ) {
        variant_frame = new simple_property_widget( this, QString( "variant" ), 
                                                property_type::LINEEDIT, this );
        variant_frame->hide();
        variant_frame->allow_hiding( true );
        variant_frame->setToolTip( "A valid itype variant id for this item." );
    }

    add_property = new QComboBox;
    tooltipText = "Add new property to this entry";
    add_property->setToolTip( tooltipText );
    //Variant only applies to items, not groups
    if ( group ) {
        add_property->addItems( QStringList{ "Add property", "count", "charges", "damage",
                                                    "contents-item", "contents-group" } );
    } else {
        add_property->addItems( QStringList{ "Add property", "count", "charges", "damage",
                            "contents-item", "contents-group", "container-item", "ammo-item", 
                            "entry-wrapper", "sealed", "variant" } );
    }
    add_property->setMaximumWidth( 145 );
    connect( add_property, QOverload<int>::of( &QComboBox::activated ), 
                                    [=](){ add_property_changed(); }) ;

    QPushButton* btnDeleteThis = new QPushButton;
    btnDeleteThis->setText( "X" );
    btnDeleteThis->setMaximumSize( QSize( 24, 24 ) );
    btnDeleteThis->setStyleSheet( "background-color:rgb(206,99,108)" );
    btnDeleteThis->setToolTip( QString( _( "Delete this entry from the list" ) ) );
    connect( btnDeleteThis, &QPushButton::clicked, this, &itemGroupEntry::delete_self );

    entryLayout->addWidget( title_label );
    flowLayout->addWidget( prob_frame );
    flowLayout->addWidget( count_frame );
    flowLayout->addWidget( charges_frame );
    flowLayout->addWidget( damage_frame );
    flowLayout->addWidget( contentsItem_frame );
    flowLayout->addWidget( contentsGroup_frame );
    flowLayout->addWidget( containerItem_frame );
    flowLayout->addWidget( ammoItem_frame );
    flowLayout->addWidget( entryWrapper_frame );
    flowLayout->addWidget( sealed_frame );

    //Variant only applies to items, not groups
    if ( !group ) {
        flowLayout->addWidget( variant_frame );
    } 
    entryLayout->addLayout( flowLayout );
    entryLayout->addWidget( add_property );
    entryLayout->addWidget( btnDeleteThis );

    entryLayout->setStretchFactor( title_label, 1 );
    entryLayout->setStretchFactor( flowLayout, 2 );
    entryLayout->setStretchFactor( add_property, 1  );
    entryLayout->setStretchFactor( btnDeleteThis, 1  );

    setLayout( entryLayout );
}


bool creator::itemGroupEntry::event( QEvent* event )
{
    //The event is a property_changed when one the itemGroupEntry's properties change
    if( event->type() == property_changed::eventType ) {
        QObjectList entriesChildren = this->children();
        for ( QObject* i : entriesChildren ) {
            simple_property_widget* ent = dynamic_cast<creator::simple_property_widget*>( i );
            if ( ent != nullptr ) {
                QString property = ent->get_propertyName();
                //If the property is hidden and not in the add_property combobox, 
                //add it to the combobox
                if( ent->isHidden()  && add_property->findText( property ) == -1 ){
                    add_property->addItem( property ) ;
                }
            }
        }
        change_notify_top_parent();
        return true;
    }
    //call the event method of the base class for the events that aren't handled
    return QFrame::event( event );
}

void creator::itemGroupEntry::change_notify_top_parent() {
    QEvent* myEvent = new QEvent( item_group_changed::eventType );
    QCoreApplication::sendEvent( top_parent_widget, myEvent );
}

void creator::itemGroupEntry::add_property_changed() {
    std::string prop = add_property->currentText().toStdString();
    if ( prop == "" || prop == "Add property" ){
        return;
    } else if ( prop == "prob" ){
        prob_frame->show();
    } else if ( prop == "count" ){
        count_frame->show();
    } else if ( prop == "charges" ){
        charges_frame->show();
    } else if ( prop == "damage" ){
        damage_frame->show();
    } else if ( prop == "contents-item" ){
        contentsItem_frame->show();
    } else if ( prop == "contents-group" ){
        contentsGroup_frame->show();
    } else if ( prop == "container-item" ){
        containerItem_frame->show();
    } else if ( prop == "ammo-item" ){
        ammoItem_frame->show();
    } else if ( prop == "entry-wrapper" ){
        entryWrapper_frame->show();
    } else if ( prop == "sealed" ){
        sealed_frame->show();
    } else if ( prop == "variant" ){
        variant_frame->show();
    }

    add_property->removeItem( add_property->currentIndex() );
    change_notify_top_parent();
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
    if( !prob_frame->isHidden() ) {
        prob_frame->get_json( jo );
    }
    if( !count_frame->isHidden() ) {
        count_frame->get_json( jo );
    }
    if( !charges_frame->isHidden() ) {
        charges_frame->get_json( jo );
    }
    if( !damage_frame->isHidden() ) {
        damage_frame->get_json( jo );
    }
    if( !contentsItem_frame->isHidden() ) {
        contentsItem_frame->get_json( jo );
    }
    if( !contentsGroup_frame->isHidden() ) {
        contentsGroup_frame->get_json( jo );
    }
    if( !containerItem_frame->isHidden() ) {
        containerItem_frame->get_json( jo );
    }
    if( !ammoItem_frame->isHidden() ) {
        ammoItem_frame->get_json( jo );
    }
    if( !entryWrapper_frame->isHidden() ) {
        entryWrapper_frame->get_json( jo );
    }
    if( !sealed_frame->isHidden() ) {
        sealed_frame->get_json( jo );
    }
    if( this->objectName() == "item" ) {
        if( !variant_frame->isHidden() ) {
            variant_frame->get_json( jo );
        }
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
