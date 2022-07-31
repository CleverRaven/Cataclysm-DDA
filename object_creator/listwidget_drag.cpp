#include "listwidget_drag.h"

QMimeData* creator::ListWidget_Drag::mimeData( const QList<QListWidgetItem*> items ) const
{
    QMimeData* md = QListWidget::mimeData( items );
    QStringList texts;
    for( QListWidgetItem* item : selectedItems() ) {
        texts << item->text();
    }
    md->setText( texts.join( QStringLiteral("\n" ) ) );
    return md;
}
