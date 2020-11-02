#ifndef CATA_OBJECT_CREATOR_DUAL_LIST_BOX_H
#define CATA_OBJECT_CREATOR_DUAL_LIST_BOX_H

#include "QtWidgets/qcombobox.h"
#include "QtWidgets/qgridlayout.h"
#include "QtWidgets/qlabel.h"
#include "QtWidgets/qpushbutton.h"

namespace creator
{
class dual_list_box : public QWidget
{
        Q_OBJECT
    public:
        dual_list_box() {}

        void initialize( const QStringList &items, const QSize &default_text_box_size );

        QStringList get_included() const;
    Q_SIGNALS:
        void click();
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
