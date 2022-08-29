#include "flowlayout.h"
#include <QtWidgets>

creator::FlowLayout::FlowLayout( QWidget *parent, int margin, int hSpacing, int vSpacing )
    : QLayout( parent ), m_hSpace( hSpacing ), m_vSpace( vSpacing ) {
    setContentsMargins( margin, margin, margin, margin );
}

creator::FlowLayout::FlowLayout( int margin, int hSpacing, int vSpacing )
    : m_hSpace( hSpacing ), m_vSpace( vSpacing ) {
    setContentsMargins( margin, margin, margin, margin );
}

creator::FlowLayout::~FlowLayout() {
    QLayoutItem *item;
    while( ( item = takeAt( 0 ) ) ) {
        delete item;
    }
}

void creator::FlowLayout::addItem( QLayoutItem *item ) {
    itemList.append( item );
}

int creator::FlowLayout::horizontalSpacing() const {
    if( m_hSpace >= 0 ) {
        return m_hSpace;
    } else {
        return smartSpacing( QStyle::PM_LayoutHorizontalSpacing );
    }
}

int creator::FlowLayout::verticalSpacing() const {
    if( m_vSpace >= 0 ) {
        return m_vSpace;
    } else{
        return smartSpacing( QStyle::PM_LayoutVerticalSpacing );
    }
}

int creator::FlowLayout::count() const {
    return itemList.size();
}

QLayoutItem *creator::FlowLayout::itemAt( int index ) const {
    return itemList.value( index );
}

QLayoutItem *creator::FlowLayout::takeAt( int index ) {
    if( index >= 0 && index < itemList.size() ) {
        return itemList.takeAt( index );
    }
    return nullptr;
}

Qt::Orientations creator::FlowLayout::expandingDirections() const {
    return { };
}

bool creator::FlowLayout::hasHeightForWidth() const {
    return true;
}

int creator::FlowLayout::heightForWidth( int width ) const {
    int height = doLayout( QRect( 0, 0, width, 0 ), true );
    return height;
}

void creator::FlowLayout::setGeometry( const QRect &rect ) {
    QLayout::setGeometry( rect );
    doLayout(rect, false);
}

QSize creator::FlowLayout::sizeHint() const {
    return minimumSize();
}

QSize creator::FlowLayout::minimumSize() const {
    QSize size;
    for( const QLayoutItem *item : qAsConst( itemList ) ) {
        size = size.expandedTo( item->minimumSize() );
    }
    const QMargins margins = contentsMargins();
    size += QSize( margins.left() + margins.right(), margins.top() + margins.bottom() );
    return size;
}

int creator::FlowLayout::doLayout( const QRect &rect, bool testOnly ) const {
    int left, top, right, bottom;
    getContentsMargins( &left, &top, &right, &bottom );
    QRect effectiveRect = rect.adjusted( +left, +top, -right, -bottom );
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;

    for( QLayoutItem *item : qAsConst( itemList ) ) {
        const QWidget *wid = item->widget();
        int spaceX = horizontalSpacing();
        if( spaceX == -1 ) {
            spaceX = wid->style()->layoutSpacing(
                QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal );
        }

        int spaceY = verticalSpacing();
        if( spaceY == -1 ) {
            spaceY = wid->style()->layoutSpacing( QSizePolicy::PushButton, 
                                    QSizePolicy::PushButton, Qt::Vertical );
            int nextX = x + item->sizeHint().width() + spaceX;
            if ( nextX - spaceX > effectiveRect.right() && lineHeight > 0 ) {
                x = effectiveRect.x();
                y = y + lineHeight + spaceY;
                nextX = x + item->sizeHint().width() + spaceX;
                lineHeight = 0;
            }

            if( !testOnly ) {
                item->setGeometry( QRect( QPoint( x, y ), item->sizeHint() ) );
            }

            x = nextX;
            lineHeight = qMax( lineHeight, item->sizeHint().height() );
        }
    }
    return y + lineHeight - rect.y() + bottom;
}

int creator::FlowLayout::smartSpacing( QStyle::PixelMetric pm ) const {
    QObject *parent = this->parent();
    if( !parent ) {
        return -1;
    } else if( parent->isWidgetType() ) {
        QWidget *pw = static_cast<QWidget *>( parent );
        return pw->style()->pixelMetric( pm, nullptr, pw );
    } else{
        return static_cast<QLayout *>( parent )->spacing();
    }
}