#ifndef CATA_OBJECT_CREATOR_DUAL_LIST_BOX_H
#define CATA_OBJECT_CREATOR_DUAL_LIST_BOX_H

#include "QtWidgets/qcombobox.h"
#include "QtWidgets/qgridlayout.h"
#include "QtWidgets/qlabel.h"
#include "QtWidgets/qpushbutton.h"

namespace creator
{
    class dual_list_box : public QGridLayout
    {
    public:
        dual_list_box() {}
        dual_list_box( const QStringList &items );

        QStringList get_included() const;
    private:
        // the entire list of acceptable strings
        QStringList items;

        QComboBox included_box;
        QComboBox excluded_box;

        QPushButton include_all_button;
        QPushButton include_sel_button;
        QPushButton exclude_all_button;
        QPushButton exclude_sel_button;

        QLabel title;

        void include_all();
        void exclude_all();
        void include_selected();
        void exclude_selected();
    };
}
#endif
