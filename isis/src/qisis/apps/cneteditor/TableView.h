#ifndef TableView_H
#define TableView_H

#include <QWidget>

#include "AbstractTableModel.h"

class QLabel;
template<typename T> class QList;
class QString;

namespace Isis
{
  namespace CnetViz
  {
    class AbstractTableModel;
    class AbstractTreeItem;
    class TableViewContent;
    class TableViewHeader;
    class TableColumnList;

    /**
     * @author ????-??-?? Unknown
     *
     * @internal
     */
    class TableView : public QWidget
    {
        Q_OBJECT

      public:
        TableView(AbstractTableModel * someModel,
                      QString pathForSettigs, QString objName);
        virtual ~TableView();
        QSize sizeHint();
        QFont getContentFont() const;
        TableViewHeader * getHorizontalHeader();

        QStringList getTitles() const;
        void setTitles(QStringList someTitle);

        void setColumnVisible(QString, bool);

        AbstractTableModel * getModel();
  //       void setModel(AbstractTableModel * newModel);
        void readSettings();
        void writeSettings();


      public slots:
        void displayWarning(AbstractTableModel::Warning);
        void handleModelSelectionChanged();
        void handleModelSelectionChanged(QList< AbstractTreeItem * >);


      signals:
        void activated();
        void rebuildModels(QList< CnetViz::AbstractTreeItem * >);
        void selectionChanged();
        void modelDataChanged();
        void tableSelectionChanged(QList< AbstractTreeItem * >);
        void filterCountsChanged(int visibleRows, int totalRows);
        

      private: // disable copying and assigning of this class
        TableView(const TableView &);
        TableView & operator=(const TableView & other);


      private: // methods
        void nullify();


      private: // data
        TableViewHeader * header;
        TableViewContent * content;
        TableColumnList * columns;
        AbstractTableModel * model;
        QString * settingsPath;
        QLabel * warningLabel;
    };
  }
}

#endif
