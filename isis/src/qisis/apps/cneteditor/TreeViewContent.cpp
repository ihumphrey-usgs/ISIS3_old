#include "IsisDebug.h"

#include "TreeViewContent.h"

#include <cmath>
#include <iostream>

#include <QAction>
#include <QLabel>
#include <QMutex>
#include <QPainter>
#include <QPaintEvent>
#include <QScrollBar>
#include <QSize>
#include <QtCore/qtextstream.h>
#include <QVariant>
#include <QVBoxLayout>

#include "IException.h"
#include "IString.h"

#include "AbstractTreeItem.h"
#include "TableColumn.h"
#include "AbstractTreeModel.h"


namespace Isis
{
  namespace CnetViz
  {
    TreeViewContent::TreeViewContent(QWidget * parent) :
        QAbstractScrollArea(parent)
    {
      nullify();

      parentView = (TreeView *) parent;

      items = new QList< AbstractTreeItem * >;
      mousePressPos = new QPoint;
      pressedItem = new QPair< AbstractTreeItem *, bool >(NULL, false);
      hoveredItem = new QPair< AbstractTreeItem *, bool >(NULL, false);
      lastShiftSelection = new QList<AbstractTreeItem *>;

      verticalScrollBar()->setSingleStep(1);
      horizontalScrollBar()->setSingleStep(10);
      rowHeight = QFontMetrics(font()).height() + ITEM_PADDING;
      contentWidth = 0;
      ASSERT(rowHeight > 0);

      setMouseTracking(true);
      setContextMenuPolicy(Qt::ActionsContextMenu);
      QAction * alternateRowsAct = new QAction("&Alternate row colors", this);
      alternateRowsAct->setCheckable(true);
      connect(alternateRowsAct, SIGNAL(toggled(bool)),
          this, SLOT(setAlternatingRowColors(bool)));
      addAction(alternateRowsAct);
      alternateRowsAct->setChecked(true);
    }


    TreeViewContent::~TreeViewContent()
    {
      delete items;
      items = NULL;

      delete mousePressPos;
      mousePressPos = NULL;

      delete pressedItem;
      pressedItem = NULL;

      delete hoveredItem;
      hoveredItem = NULL;

      delete lastShiftSelection;
      lastShiftSelection = NULL;
    }


    QSize TreeViewContent::minimumSizeHint() const
    {
      return QWidget::minimumSizeHint();
    }


    QSize TreeViewContent::sizeHint()
    {
      return minimumSizeHint();
    }


    AbstractTreeModel * TreeViewContent::getModel()
    {
      return model;
    }


    void TreeViewContent::setModel(AbstractTreeModel * someModel)
    {
      if (!someModel)
      {
        IString msg = "Attempted to set a NULL model!";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      if (model)
      {
        disconnect(model, SIGNAL(modelModified()), this, SLOT(refresh()));
        disconnect(model, SIGNAL(filterProgressChanged(int)),
                  this, SLOT(updateItemList()));
        disconnect(this,
                  SIGNAL(treeSelectionChanged(QList< AbstractTreeItem * >)),
                  model,
                  SIGNAL(treeSelectionChanged(QList< AbstractTreeItem * >)));
        disconnect(model, SIGNAL(tableSelectionChanged(QList<AbstractTreeItem*>)),
                  this, SLOT(scrollTo(QList<AbstractTreeItem*>)));
      }

      model = someModel;
      connect(model, SIGNAL(modelModified()), this, SLOT(refresh()));
      connect(model, SIGNAL(filterProgressChanged(int)),
              this, SLOT(updateItemList()));
      connect(this, SIGNAL(treeSelectionChanged(QList< AbstractTreeItem * >)),
              model, SIGNAL(treeSelectionChanged(QList< AbstractTreeItem * >)));
      connect(model, SIGNAL(tableSelectionChanged(QList<AbstractTreeItem*>)),
              this, SLOT(scrollTo(QList<AbstractTreeItem*>)));

      refresh();
    }


    void TreeViewContent::refresh()
    {
      ASSERT(model);
      if (model)
      {
        if (!model->isFiltering())
        {
          QSize modelVisibleSize =
            model->getVisibleSize(ITEM_INDENTATION);
          int rowCount = modelVisibleSize.height();
          contentWidth = modelVisibleSize.width() + ITEM_INDENTATION;
          verticalScrollBar()->setRange(0, qMax(rowCount - 1, 0));
          horizontalScrollBar()->setRange(0, contentWidth - viewport()->width()
              + horizontalScrollBar()->singleStep());
        }

        updateItemList();
        viewport()->update();
      }
    }


    bool TreeViewContent::eventFilter(QObject * target, QEvent * event)
    {
      return QObject::eventFilter(target, event);
    }


    void TreeViewContent::mouseDoubleClickEvent(QMouseEvent * event)
    {
      QPoint pressPos = event->pos();
      int index = pressPos.y() / rowHeight;

      if (index < items->size())
      {
        AbstractTreeItem * item = (*items)[index];
        item->setExpanded(!item->isExpanded());
        refresh();
      }
    }

    void TreeViewContent::mousePressEvent(QMouseEvent * event)
    {
      QPoint pressPos = event->pos();
      int index = pressPos.y() / rowHeight;

      pressedItem->first = NULL;
      pressedItem->second = false;

      if (index < items->size())
      {
        AbstractTreeItem * item = (*items)[index];
        if (item->isSelectable() ||
            (item->getFirstVisibleChild() &&
            getArrowRect(item).contains(pressPos)))
        {
          pressedItem->first = item;

          if (item->getFirstVisibleChild())
          {
            QRect arrowRect(getArrowRect(item));
            pressedItem->second = arrowRect.contains(pressPos);
          }

          QList< AbstractTreeItem * > newlySelectedItems;
          if (!pressedItem->second)
          {
            if (event->modifiers() & Qt::ControlModifier)
            {
              foreach (AbstractTreeItem * child, item->getChildren())
              {
                child->setSelected(!item->isSelected());
                if (child->isSelected())
                  newlySelectedItems.append(child);
              }

              item->setSelected(!item->isSelected());
              if (item->isSelected())
                newlySelectedItems.append(item);

              lastDirectlySelectedItem = item;
              lastShiftSelection->clear();
            }
            else
            {
              if (event->modifiers() & Qt::ShiftModifier)
              {
                foreach (AbstractTreeItem * i, *lastShiftSelection)
                  i->setSelected(false);

                if (lastDirectlySelectedItem)
                {
                  // gets the new shift selection without selecting children
                  QList< AbstractTreeItem * > tmp =
                      model->getItems(lastDirectlySelectedItem, item);

                  // use tmp to create a new lastShiftSelection with children
                  // selected as well
                  foreach (AbstractTreeItem * i, tmp)
                  {
                    lastShiftSelection->append(i);

                    // if this item is a point item then select its children
                    if (i->getPointerType() == AbstractTreeItem::Point)
                    {
                      foreach (AbstractTreeItem * child, i->getChildren())
                      {
                        child->setSelected(true);
                        lastShiftSelection->append(child);
                      }
                    }
                  }
                }
                else
                {
                  lastShiftSelection->clear();
                }

                foreach (AbstractTreeItem * i, *lastShiftSelection)
                {
                  i->setSelected(true);
                  newlySelectedItems.append(i);
                }
              }
              else
              {
                model->setGlobalSelection(false);
                item->setSelected(true);
                newlySelectedItems.append(item);
                lastDirectlySelectedItem = item;

                if (item->getPointerType() == AbstractTreeItem::Point)
                {
                  foreach (AbstractTreeItem * child, item->getChildren())
                  {
                    child->setSelected(true);
                    newlySelectedItems.append(child);
                  }
                }

                lastShiftSelection->clear();
              }
            }

            emit treeSelectionChanged(newlySelectedItems);
          }
        }
      }
      else
      {
        model->setGlobalSelection(false);
      }

      viewport()->update();
    }


    void TreeViewContent::mouseReleaseEvent(QMouseEvent * event)
    {
      AbstractTreeItem * item = pressedItem->first;
      if (item && getArrowRect(item).contains(event->pos()))
      {
        item->setExpanded(!item->isExpanded());
        refresh();
      }

      pressedItem->first = NULL;
      pressedItem->second = false;
      viewport()->update();

      QWidget::mousePressEvent(event);
    }


    void TreeViewContent::mouseMoveEvent(QMouseEvent * event)
    {
      QPoint cursorPos = event->pos();
      int index = cursorPos.y() / rowHeight;

      hoveredItem->first = NULL;
      hoveredItem->second = false;

      if (index < items->size() && index >= 0)
      {
        AbstractTreeItem * item = (*items)[index];
        if (item->isSelectable() ||
            (item->getFirstVisibleChild() &&
                getArrowRect(item).contains(cursorPos)))
        {
          hoveredItem->first = item;

          if (item->getFirstVisibleChild())
          {
            QRect arrowRect = getArrowRect(item);
            hoveredItem->second = arrowRect.contains(cursorPos);
          }
        }
      }

      viewport()->update();
    }


    void TreeViewContent::leaveEvent(QEvent * event)
    {
      hoveredItem->first = NULL;
      hoveredItem->second = false;
      viewport()->update();
    }


    void TreeViewContent::keyPressEvent(QKeyEvent * event)
    {
      if (event->key() == Qt::Key_A &&
          event->modifiers() == Qt::ControlModifier)
      {
        model->setGlobalSelection(true);
        viewport()->update();
        emit treeSelectionChanged();
      }
      else
      {
        QWidget::keyPressEvent(event);
      }
    }


    void TreeViewContent::paintEvent(QPaintEvent * event)
    {
      if (model)
      {
        int startRow = verticalScrollBar()->value();
        int rowCount = (int) ceil(viewport()->height() / (double) rowHeight);

        QPainter painter(viewport());
        painter.setRenderHints(QPainter::Antialiasing |
            QPainter::TextAntialiasing);

        for (int i = 0; i < rowCount; i++)
        {
          // Assume the background color should be the base.  Then set odd rows
          // to be the alternate row color if alternatingRowColors is set to
          // true.
          QColor backgroundColor = palette().base().color();

          if (i < items->size())
          {
            if (alternatingRowColors && (startRow + i) % 2 == 1)
              backgroundColor = palette().alternateBase().color();

            ASSERT(items->at(i));
            if (items->at(i)->isSelected())
              backgroundColor = palette().highlight().color();
          }

          // define the top left corner of the row and also how big the row is
          QPoint relativeTopLeft(0, i * rowHeight);
          QPoint scrollBarPos(horizontalScrollBar()->value(),
              verticalScrollBar()->value());
          QPoint absoluteTopLeft(relativeTopLeft + scrollBarPos);
          QSize rowSize(viewport()->width(), (int) rowHeight);

          // Fill in the background with the background color
          painter.fillRect(QRect(relativeTopLeft, rowSize), backgroundColor);

          // if the mouse is hovering over this item, then also draw a rect
          // around this item.
          if (i < items->size() && hoveredItem->first == (*items)[i] &&
              hoveredItem->first->isSelectable())
          {
            QPen prevPen(painter.pen());
            QPen borderPen(prevPen);
            borderPen.setWidth(1);
            borderPen.setColor(palette().highlight().color());
            painter.setPen(borderPen);
            QPoint borderTopLeft(relativeTopLeft.x() - absoluteTopLeft.x(),
                relativeTopLeft.y() + 1);

            int rectWidth = qMax(contentWidth +
                horizontalScrollBar()->singleStep(), viewport()->width());
            QSize borderSize(rectWidth, rowSize.height() - 2);
            painter.drawRect(QRect(borderTopLeft, borderSize));
            painter.setPen(prevPen);
          }

          // if this row has text then draw it
          if (i < items->size())
            paintItemText(&painter, i, absoluteTopLeft, relativeTopLeft);
        }
      }
      else
      {
        QWidget::paintEvent(event);
      }
    }


    void TreeViewContent::resizeEvent(QResizeEvent * event)
    {
      QAbstractScrollArea::resizeEvent(event);
      horizontalScrollBar()->setRange(0, contentWidth - viewport()->width()
          + horizontalScrollBar()->singleStep());
      updateItemList();
    }


    void TreeViewContent::scrollContentsBy(int dx, int dy)
    {
      QAbstractScrollArea::scrollContentsBy(dx, dy);
      updateItemList();
    }


    void TreeViewContent::nullify()
    {
      parentView = NULL;
      model = NULL;
      items = NULL;
      pressedItem = NULL;
      hoveredItem = NULL;
      lastDirectlySelectedItem = NULL;
      lastShiftSelection = NULL;
      mousePressPos = NULL;
    }


    void TreeViewContent::paintItemText(QPainter * painter,
        int index, QPoint absolutePosition, QPoint relativePosition)
    {
      ASSERT(items);
      ASSERT(index >= 0 && index < items->size());

      QPoint point(-absolutePosition.x(), relativePosition.y());

      AbstractTreeItem * item = (*items)[index];

      // should always be true, but prevents segfault in case of bug
      if (item)
      {
        // the parameter called point is given to us as the top left corner of
        // the row where the text should go.  We adjust this point until it can
        // be used to draw the text in the middle of the row.  First the x
        // component is adjusted.  How far the x component needs to be adjusted
        // is directly related to how many parents this item has, hence the
        // following while loop.  Note that even top level items have a parent
        // (the invisible root item).  Also note that top level items do not get
        // any adjustment from this while.  This is because all items need
        // exactly one adjustment in the x direction after the arrow is
        // potentially drawn.
        AbstractTreeItem * iteratorItem = item;
        while (iteratorItem->parent() && iteratorItem->parent()->parent())
        {
          point.setX(point.x() + ITEM_INDENTATION);
          iteratorItem = iteratorItem->parent();
        }

        QPen originalPen = painter->pen();
        if (item->isSelected())
        {
          painter->setPen(QPen(palette().highlightedText().color()));
        }

        // now that the x component has all but its last adjustment taken care
        // of, we then consider items with children.  These items need to have
        // an arrow drawn next to them, before the text is drawn
        if (item->getFirstVisibleChild())
        {
          // if the user is hovering over the arrow with the mouse, then draw
          // a box around where the arrow will be drawn
          QRect itemArrowRect(getArrowRect(item));
          if (item == hoveredItem->first && item == pressedItem->first)
          {
            if (pressedItem->second && hoveredItem->second)
            {
              QPainter::CompositionMode prevMode = painter->compositionMode();
              painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
              QColor color = palette().button().color().darker(160);
              color.setAlpha(100);
              painter->fillRect(itemArrowRect, color);
              painter->setCompositionMode(prevMode);
            }
          }

          // if the user has pressed the mouse over the arrow but has not yet
          // released it, then darken the background behind it
          if ((item == hoveredItem->first && hoveredItem->second) ||
              (item == pressedItem->first && pressedItem->second))
          {
            if (!pressedItem->first ||
                (item == pressedItem->first && pressedItem->second))
            {
              painter->drawRect(itemArrowRect);
            }
          }

          // draw the appropriate arrow based on the items expandedness
          if (item->isExpanded())
            drawExpandedArrow(painter, itemArrowRect);
          else
            drawCollapsedArrow(painter, itemArrowRect);
        }

        // the final x component adjustment is the same whether an arrow was
        // drawn or not
        point.setX(point.x() + ITEM_INDENTATION);

        // adjust the y component to center the text vertically in the row
        point.setY(point.y() + ITEM_PADDING / 2);

        // finally draw the text
        int textHeight = rowHeight - ITEM_PADDING;
        QRect rect(point, QSize(viewport()->width() - point.x(), textHeight));
        painter->drawText(rect, Qt::TextDontClip, item->getData().toString());
        painter->setPen(originalPen);
      }
    }


    void TreeViewContent::drawCollapsedArrow(QPainter * painter, QRect rect)
    {
      rect.setTopLeft(rect.topLeft() + QPoint(4, 3));
      rect.setBottomRight(rect.bottomRight() - QPoint(4, 2));

      QPoint top(rect.topLeft());
      QPoint bottom(rect.bottomLeft());
      QPoint right(rect.right(), rect.center().y());

      QPen prevPen = painter->pen();
      QPen arrowPen(prevPen);
      arrowPen.setCapStyle(Qt::RoundCap);
      arrowPen.setJoinStyle(Qt::RoundJoin);
      arrowPen.setWidth(2);
      painter->setPen(arrowPen);
      painter->drawLine(top, right);
      painter->drawLine(bottom, right);
      painter->setPen(prevPen);
    }


    void TreeViewContent::drawExpandedArrow(QPainter * painter, QRect rect)
    {
      rect.setTopLeft(rect.topLeft() + QPoint(3, 4));
      rect.setBottomRight(rect.bottomRight() - QPoint(2, 4));

      QPoint left(rect.topLeft());
      QPoint right(rect.topRight());
      QPoint bottom(rect.center().x(), rect.bottom());

      QPen prevPen = painter->pen();
      QPen arrowPen(prevPen);
      arrowPen.setCapStyle(Qt::RoundCap);
      arrowPen.setJoinStyle(Qt::RoundJoin);
      arrowPen.setWidth(2);
      painter->setPen(arrowPen);
      painter->drawLine(left, bottom);
      painter->drawLine(right, bottom);
      painter->setPen(prevPen);
    }


    void TreeViewContent::setAlternatingRowColors(bool newStatus)
    {
      alternatingRowColors = newStatus;
      viewport()->update();
    }


    void TreeViewContent::updateItemList()
    {
      int startRow = verticalScrollBar()->value();
      int rowCount = (int) ceil(viewport()->height() / (double) rowHeight);
      *items = model->getItems(startRow, startRow + rowCount,
                              AbstractTreeModel::AllItems, false);

      viewport()->update();
    }


    QRect TreeViewContent::getArrowRect(AbstractTreeItem * item) const
    {
      QRect arrowRect;
      if (item)
      {
        int index = items->indexOf(item);
        QPoint centerOfArrow(12 - horizontalScrollBar()->value(),
            (index * rowHeight) + (rowHeight / 2));
        int depth = item->getDepth() - 1;
        centerOfArrow.setX(centerOfArrow.x() + (depth * ITEM_INDENTATION));

        arrowRect = QRect(centerOfArrow.x() - 6, centerOfArrow.y() - 6, 12, 12);
      }

      return arrowRect;
    }

    void TreeViewContent::scrollTo(
        QList< AbstractTreeItem * > newlySelectedItems)
    {
      if (newlySelectedItems.size())
        scrollTo(newlySelectedItems.last());
    }


    void TreeViewContent::scrollTo(AbstractTreeItem * newlySelectedItem)
    {
      if (newlySelectedItem->getPointerType() == AbstractTreeItem::Measure)
        newlySelectedItem->parent()->setExpanded(true);

      int row = getModel()->indexOfVisibleItem(newlySelectedItem);

      if (row >= 0)
      {
        int topRow = verticalScrollBar()->value();

        if (row < topRow)
        {
          verticalScrollBar()->setValue(row);
        }
        else
        {
          int wholeVisibleRowCount = viewport()->height() / rowHeight;
          int bottomRow = topRow + wholeVisibleRowCount;
          if (row > bottomRow)
            verticalScrollBar()->setValue(row - wholeVisibleRowCount + 1);
        }
      }

      viewport()->update();
    }
  }
}
