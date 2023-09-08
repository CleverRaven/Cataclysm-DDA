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
    layout->addWidget(scrollArea);

    //set the backgroundcolor of the widget to yeklkow
    this->setStyleSheet("background-color: yellow");
    //set the backgroundcolor of the scrollarea to red
    scrollArea->setStyleSheet("background-color: magenta");


    //When the button is pressed, toggle the content
    connect(button, &QToolButton::toggled, [=](){
        button->setArrowType(checked ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
        scrollArea != nullptr && checked ? showContent(scrollArea) : hideContent(scrollArea);
    });
}

void creator::collapsing_widget::hideContent(QScrollArea* scrollArea) {
    //set the heigth of content to 0
    scrollArea->setMaximumHeight(0);
    //set the backgroundcolor of the widget to yeklkow
    scrollArea->setStyleSheet("background-color: mintcream");
    //set the height of this widget to the size of the label
    // this->setMaximumHeight(50);
    // this->setMinimumHeight(30);
    //Call the adjustsze method of this widget
    this->adjustSize();
    //set checked to true
    checked = true;
}

void creator::collapsing_widget::showContent(QScrollArea* scrollArea) {
   //set the height of the contentLayout to the size of the widgets in it plus 20
    scrollArea->setMaximumHeight(scrollArea->layout()->sizeHint().height());
    scrollArea->setStyleSheet("background-color: green");
     //set the height of this widget to the size of the label
    // this->setMaximumHeight(this->layout()->sizeHint().height());
    // this->setMinimumHeight(this->layout()->sizeHint().height());
    //Call the adjustsze method of this widget
    this->adjustSize();
    
    //set checked to false
    checked = false;
}
