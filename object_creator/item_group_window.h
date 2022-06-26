#ifndef CATA_OBJECT_CREATOR_ITEM_GROUP_WINDOW_H
#define CATA_OBJECT_CREATOR_ITEM_GROUP_WINDOW_H

#include "dual_list_box.h"
#include "item_group.h"

#include <QtWidgets/qcheckbox.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qgridlayout.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qlistwidget.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qplaintextedit.h>
#include <QtWidgets/qspinbox.h>
#include <QtWidgets/qtablewidget.h>

namespace creator
{
class item_group_window : public QMainWindow
{

    public:
        item_group_window( QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags() );

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
        void deleteEntriesLine();
        void items_search_return_pressed();
        void group_search_return_pressed();
        void group_list_populate_filtered( std::string searchQuery = "" );
        void selected_item_doubleclicked();
        void selected_group_doubleclicked();

        QLabel id_label;
        QLineEdit id_box;

        QLabel item_search_label;
        QLineEdit item_search_box;
        QListWidget item_list_total_box;

        QLabel group_search_label;
        QLineEdit group_search_box;
        QListWidget group_list_total_box;

        QTableWidget entries_box;

};
}

#endif
