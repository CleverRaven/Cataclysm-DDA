#include "fake_spell_listbox.h"

#include "bodypart.h"
#include "format.h"
#include "json.h"

#include <QtWidgets/qboxlayout.h>


creator::fake_spell_listbox::fake_spell_listbox( QWidget *parent )
    : QListWidget( parent )
{


    //Make a horizontal layout and add two vertical layouts to it
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    QVBoxLayout *vlayout1 = new QVBoxLayout();
    vlayout1->setContentsMargins(0, 0, 0, 0);
    vlayout1->setSpacing(0);
    QVBoxLayout *vlayout2 = new QVBoxLayout();
    vlayout2->setContentsMargins(0, 0, 0, 0);
    vlayout2->setSpacing(0);
    layout->addLayout(vlayout1);
    layout->addLayout(vlayout2);


    vlayout1->addWidget(&id_label);
    add_spell_button.setText( QString( "Add fake_spell" ) );
    QObject::connect( &add_spell_button, &QPushButton::clicked,
    [&]() {
        const int index = windows.size();
        windows.push_back( new fake_spell_window() );
        windows.back()->hide();
        windows.back()->index = index;
        QObject::connect( windows.back(), &fake_spell_window::modified, this,
                          &fake_spell_listbox::modified );
        fake_spell_list_box.addItem( QString( "NEW (edit)" ) );
        QObject::connect( windows.back(), &fake_spell_window::modified,
        [&]() {
            const fake_spell_window *win = windows.back();
            const fake_spell &sp = win->get_fake_spell();
            if( !sp.id.is_empty() ) {
                fake_spell_list_box.setItemText( win->index, sp.id.c_str() );
            }
        } );
    } );
    QObject::connect( &add_spell_button, &QPushButton::clicked, this, &fake_spell_listbox::modified );
    vlayout1->addWidget(&add_spell_button);

    del_spell_button.setText( QString( "Remove fake_spell" ) );
    QObject::connect( &del_spell_button, &QPushButton::clicked,
    [&]() {
        const int index = fake_spell_list_box.currentIndex();
        auto iter = windows.begin();
        std::advance( iter, index );
        ( *iter )->deleteLater();
        windows.erase( iter );
        fake_spell_list_box.removeItem( index );
    } );
    QObject::connect( &del_spell_button, &QPushButton::clicked, this, &fake_spell_listbox::modified );
    vlayout1->addWidget(&del_spell_button);


    // =========================================================================================
    // second column
    QObject::connect( &fake_spell_list_box, qOverload<int>( &QComboBox::currentIndexChanged ),
    [&]() {
        for( fake_spell_window *win : windows ) {
            win->hide();
        }
        if( windows.empty() ) {
            return;
        }
        const int cur_index = fake_spell_list_box.currentIndex();
        windows.at( cur_index )->show();
        fake_spell_list_box.setCurrentText( windows.at( cur_index )->get_fake_spell().id.c_str() );
    } );
    QObject::connect( &fake_spell_list_box, &QComboBox::currentTextChanged,
    [&]() {
        if( windows.empty() ) {
            return;
        }
        fake_spell_list_box.setCurrentText( windows.at( fake_spell_list_box.currentIndex() )
                                            ->get_fake_spell().id.c_str() );
    } );

    vlayout2->addWidget(&fake_spell_list_box);
    this->adjustSize();
}

void creator::fake_spell_listbox::set_spells( const std::vector<fake_spell> &spells )
{
    windows.clear();
    for( const fake_spell &sp : spells ) {
        windows.push_back( new fake_spell_window() );
        windows.back()->set_fake_spell( sp );
        fake_spell_list_box.addItem( QString( sp.id.c_str() ) );
    }
}

std::vector<fake_spell> creator::fake_spell_listbox::get_spells() const
{
    std::vector<fake_spell> spells;
    for( const fake_spell_window *win : windows ) {
        spells.push_back( win->get_fake_spell() );
    }
    return spells;
}
