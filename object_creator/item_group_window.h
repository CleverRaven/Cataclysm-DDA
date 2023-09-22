#ifndef CATA_OBJECT_CREATOR_ITEM_GROUP_WINDOW_H
#define CATA_OBJECT_CREATOR_ITEM_GROUP_WINDOW_H

#include "listwidget_drag.h"
#include "simple_property_widget.h"
#include "flowlayout.h"
#include "item_group.h"

#include "QtWidgets/qlistwidget.h"
#include <QtWidgets/qlineedit.h>
#include "QtWidgets/qgridlayout.h"
#include <QtWidgets/qmainwindow.h>
#include "QtWidgets/qlabel.h"
#include <QtWidgets/qplaintextedit.h>
#include <QtWidgets/qtablewidget.h>
#include "QtWidgets/qpushbutton.h"
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>

namespace creator
{
    class nested_group_container;
    class item_group_window : public QWidget
    {

    public:
        item_group_window( QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags() );

        void show() {
            item_group_json.show();
            QWidget::show();
        }

        void hide() {
            item_group_json.hide();
            QWidget::hide();
        }

    private:
        QTextEdit item_group_json;

        void write_json();
        void items_search_return_pressed();
        void group_search_return_pressed();
        void group_list_populate_filtered( std::string searchQuery = "" );
        void item_list_populate_filtered( std::string searchQuery = "" );
        void set_item_tooltip( QListWidgetItem* new_item, const itype* tmpItype );
        void set_group_tooltip( QListWidgetItem* new_item, const item_group_id tmpItype );

        QLabel* id_label;
        QLineEdit* id_box;

        QLabel* comment_label;
        QLineEdit* comment_box;

        nested_group_container* group_container;
        simple_property_widget* ammo_frame;
        simple_property_widget* magazine_frame;

        QComboBox* subtype;
        QLineEdit* containerItem;
        QComboBox* overflow;

        QLabel* item_search_label;
        QLineEdit* item_search_box;
        ListWidget_Drag* item_list_total_box;

        QLabel* group_search_label;
        QLineEdit* group_search_box;
        ListWidget_Drag* group_list_total_box;

        //Top-level frame to hold the distributionCollections and/or entrieslist
        QScrollArea* scrollArea;



    protected:
        bool event( QEvent* event ) override;
    };

    //Holds the most detailed data, an item or group and it's property
    class itemGroupEntry : public QFrame
    {
    public:
        explicit itemGroupEntry( QWidget* parent, QString entryText, bool group, 
                                item_group_window* top_parent );
        void get_json( JsonOut &jo );
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    private:
        void delete_self();
        void change_notify_top_parent();
        void add_property_changed();
        QLabel* title_label;
        simple_property_widget* variant_frame;
        simple_property_widget* contentsItem_frame;
        simple_property_widget* contentsGroup_frame;
        simple_property_widget* containerItem_frame;
        simple_property_widget* ammoItem_frame;
        simple_property_widget* entryWrapper_frame;
        simple_property_widget* sealed_frame;
        simple_property_widget* damage_frame;
        simple_property_widget* charges_frame;
        simple_property_widget* count_frame;
        simple_property_widget* prob_frame;
        FlowLayout* flowLayout;
        QComboBox* add_property;
        item_group_window* top_parent_widget;
    protected:
        bool event( QEvent* event ) override;
    };

    //Holds the properties for either a collection or a distribution
    //May contain entriesLists
    class distributionCollection : public QFrame
    {
    public:
        explicit distributionCollection( bool isCollection, 
                    item_group_window* top_parent = nullptr );
        void get_json( JsonOut &jo );
        void set_bg_color();
        void set_depth( int d );    
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;
    private:
        void add_collection();
        void add_distribution();
        void add_entry( QString entryText, bool group );
        void change_notify_top_parent( );
        void delete_self();
        item_group_window* top_parent_widget;
        int depth;
        QComboBox* entryType;
        QVBoxLayout* verticalBox;
        QSpinBox* prob;
        
    protected:
        void dragEnterEvent( QDragEnterEvent* event ) override;
        void dragMoveEvent( QDragMoveEvent* event ) override;
        void dropEvent( QDropEvent* event ) override;
    };

    //Top-level widget for items and distributions and collections
    class nested_group_container : public QFrame
    {
    public:
        explicit nested_group_container( QWidget* parent, item_group_window* top_parent );
        void get_json( JsonOut &jo );
    private:
        void add_distribution();
        void add_collection();
        void change_notify_top_parent();
        QFrame* items_container;
        item_group_window* top_parent_widget;
        QVBoxLayout* verticalBox;
    private:
        void add_entry( QString entryText, bool group );
    protected:
        void dragEnterEvent( QDragEnterEvent* event ) override;
        void dragMoveEvent( QDragMoveEvent* event ) override;
        void dropEvent( QDropEvent* event ) override;
    };

    class item_group_changed : public QEvent
    {
    public:
        item_group_changed() : QEvent( registeredType() ) { }
        static QEvent::Type eventType;

    private:
        static QEvent::Type registeredType();
    };
}

#endif
