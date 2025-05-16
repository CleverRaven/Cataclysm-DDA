#ifndef CATA_OBJECT_CREATOR_COLLAPSING_WIDGET_H
#define CATA_OBJECT_CREATOR_COLLAPSING_WIDGET_H

#include <QtWidgets/qscrollarea.h>
#include <QtCore/qstring.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qwidget.h>


namespace creator
{
class collapsing_widget : public QWidget {
    public:
        collapsing_widget(QWidget *parent, const QString &text, QLayout& contentLayout);
        void hideContent(QScrollArea* scrollArea);
        void showContent(QScrollArea* scrollArea);

    private:
        bool checked = false;
};
}

#endif
