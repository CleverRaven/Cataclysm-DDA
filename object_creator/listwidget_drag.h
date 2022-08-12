#ifndef CATA_OBJECT_CREATOR_LISTWIDGET_DRAG_H
#define CATA_OBJECT_CREATOR_LISTWIDGET_DRAG_H

#include "QtWidgets/qlistwidget.h"
#include <QtCore/QMimeData>

namespace creator
{
//The same as a QListWidget but if you enable drag it stores the item text in the mimedata
class ListWidget_Drag : public QListWidget {
    public:
        using QListWidget::QListWidget;
    protected:
        QMimeData* mimeData( const QList<QListWidgetItem*> items ) const;
};
}
#endif
