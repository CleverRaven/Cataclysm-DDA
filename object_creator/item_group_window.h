#ifndef CATA_OBJECT_CREATOR_ITEM_GROUP_WINDOW_H
#define CATA_OBJECT_CREATOR_ITEM_GROUP_WINDOW_H

#include "listwidget_drag.h"
#include "item_group.h"

#include "QtWidgets/qlistwidget.h"
#include <QtWidgets/qlineedit.h>
#include "QtWidgets/qgridlayout.h"
#include <QtWidgets/qmainwindow.h>
#include "QtWidgets/qlabel.h"
#include <QtWidgets/qplaintextedit.h>
#include <QtWidgets/qtablewidget.h>
#include "QtWidgets/qpushbutton.h"

namespace creator
{
    class item_group_window : public QMainWindow
    {

    public:
        item_group_window( QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags() );
        void entries_box_content_changed();

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
        void set_item_tooltip( QListWidgetItem* new_item, item tmpItem );

        QLabel id_label;
        QLineEdit id_box;

        QLabel item_search_label;
        QLineEdit item_search_box;
        ListWidget_Drag item_list_total_box;

        QLabel group_search_label;
        QLineEdit group_search_box;
        ListWidget_Drag group_list_total_box;

        //Top-level frame to hold the distributionCollections and/or entrieslist
        QFrame* entries_box;


    protected:
        bool event( QEvent* event ) override;
    };

    //Holds the most detailed data, a list of items and spawn properties
    class entriesList : public QTableWidget
    {
    public:
        explicit entriesList( QWidget* parent = nullptr );
        void add_item( QString itemText, bool group );
        void deleteEntriesLine();

    protected:
        void dragEnterEvent( QDragEnterEvent* event ) override;
        void dragMoveEvent( QDragMoveEvent* event ) override;
        void dropEvent( QDropEvent* event ) override;
    };

    //Holds the properties for either a collection or a distribution
    //May contain entriesLists
    class distributionCollection : public QFrame
    {
    public:
        explicit distributionCollection( QWidget* parent = nullptr );
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
