#include "collapsing_widget.h"
//include qlabel and qtoolbutton
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qtoolbutton.h>

creator::collapsing_widget::collapsing_widget(QWidget *parent, const QString &text, QLayout& contentLayout) 
    : QWidget(parent)
{
    //Create a vertcal layout
    QVBoxLayout *layout = new QVBoxLayout(this);

    //Add a button and a label to a horizontal layout and add it to the vertical layout
    QHBoxLayout *hlayout = new QHBoxLayout();
    layout->addLayout(hlayout);
    QToolButton *button = new QToolButton();
    button->setCheckable(true);
    //make sure the button starts checked
    button->setArrowType(Qt::ArrowType::DownArrow);
    button->setChecked(true);
    hlayout->addWidget(button);
    QLabel *label = new QLabel();
    label->setText(text);
    hlayout->addWidget(label);

    //Add the content layout to a new scrollarea and add it to the vertical layout
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    scrollArea->setWidgetResizable(true);
    scrollArea->setLayout(&contentLayout);
    //Set the vertical spacing and the content margins to 0 for the content layout
    contentLayout.setContentsMargins(0, 0, 0, 0);
    contentLayout.setSpacing(0);
    scrollArea->setMaximumHeight(scrollArea->layout()->sizeHint().height());
    scrollArea->setMinimumHeight(scrollArea->layout()->sizeHint().height());
    layout->addWidget(scrollArea);

    //When the button is pressed, toggle the content
    connect(button, &QToolButton::toggled, [=](){
        button->setArrowType(checked ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
        scrollArea != nullptr && checked ? showContent(scrollArea) : hideContent(scrollArea);
    });
}

void creator::collapsing_widget::hideContent(QScrollArea* scrollArea) {
    scrollArea->setMaximumHeight(0);
    scrollArea->setMinimumHeight(0);
    this->adjustSize();
    this->parentWidget()->adjustSize();
    checked = true;
}

void creator::collapsing_widget::showContent(QScrollArea* scrollArea) {
    scrollArea->setMaximumHeight(scrollArea->layout()->sizeHint().height());
    scrollArea->setMinimumHeight(scrollArea->layout()->sizeHint().height());
    this->adjustSize();
    this->parentWidget()->adjustSize();
    checked = false;
}
