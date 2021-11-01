#ifndef CATA_OBJECT_CREATOR_FAKE_SPELL_LISTBOX_H
#define CATA_OBJECT_CREATOR_FAKE_SPELL_LISTBOX_H

#include "fake_spell_window.h"
#include "magic.h"

#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlistwidget.h>
#include <QtWidgets/qpushbutton.h>

namespace creator
{
class fake_spell_listbox : public QListWidget
{
        Q_OBJECT
    public:
        fake_spell_listbox( QWidget *parent = nullptr );

        void show() {
            QWidget::show();
        }

        void hide() {
            QWidget::hide();
        }

        void set_spells( const std::vector<fake_spell> &spells );
        std::vector<fake_spell> get_spells() const;

        void setText( const QString &str ) {
            id_label.setText( str );
        }
    Q_SIGNALS:
        // emits a signal if any attached windows are modified
        void modified();
    private:
        std::vector<fake_spell_window *> windows;

        QLabel id_label;

        QComboBox fake_spell_list_box;

        QPushButton add_spell_button;
        QPushButton del_spell_button;
};
}

#endif
