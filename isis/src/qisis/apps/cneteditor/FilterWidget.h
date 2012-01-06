#ifndef FilterWidget_H
#define FilterWidget_H


// parent
#include <QWidget>

// these are included because it is needed inside a templated method
#include "FilterGroup.h"


class QLabel;
template< class T > class QList;
class QPushButton;
class QString;
class QTextEdit;
class QVBoxLayout;


namespace Isis
{
  class ControlCubeGraphNode;
  class ControlMeasure;
  class ControlPoint;
  
  namespace CnetViz
  {
    class FilterGroup;
   
    /**
     * This widget contains filtering capabilities for a single filter type.
     *
     * The filter types are (strings containing exactly):
     *   Connections
     *   Images and Points
     *   Points and Measures
     *
     * @todo Add type safety to filter types.
     *
     * This class does the visualization and editing of the filter groups. An
     * interface is provided for evaluating all of the contained filters.
     *
     * @author ????-??-?? Eric Hyer
     *
     * @internal
     */ 
    class FilterWidget : public QWidget
    {
        Q_OBJECT

      public:
        explicit FilterWidget(QString);
        FilterWidget(const FilterWidget &);
        virtual ~FilterWidget();

        template< typename T >
        bool evaluate(const T * t, bool (AbstractFilter::*meth)() const) const
        {
          // if andFiltersTogether is true then we break out of the loop as
          // soon as any selectors evaluate to false.  If andFiltersTogether
          // is false then we are ORing them so we break out as soon as any
          // selector evaluates to true.  Whether we are looking for
          // successes or failures depends on whether we are ANDing or ORing
          // the filters (selectors) together!!!
          bool looking = true;
          for (int i = 0; looking && i < filterGroups->size(); i++)
          {
            if (filterGroups->at(i)->hasFilter(meth))
              looking = !(filterGroups->at(i)->evaluate(t, meth) ^
                  andGroupsTogether);
          }

          // It is good that we are still looking for failures if we were
          // ANDing filters together, but it is bad if we were ORing them
          // since in this case we were looking for success (unless of
          // course there were no filters to look through).
          return !(looking ^ andGroupsTogether) || !hasFilter(meth);
        }
        
        bool evaluate(const ControlCubeGraphNode * node) const;
        bool evaluate(const ControlPoint * point) const;
        bool evaluate(const ControlMeasure * measure) const;

        bool hasFilter(bool (AbstractFilter:: *)() const) const;

        FilterWidget & operator=(FilterWidget other);


      signals:
        void filterChanged();
        void scrollToBottom();


      private:
        void nullify();
        void init();
        QList< FilterGroup * > groupsWithCondition(
            bool (FilterGroup:: *)() const) const;
            
        void updateDescription(QLabel * label,
                               bool (AbstractFilter:: *)() const,
                               QString (AbstractFilter:: *)() const,
                               QString);
        
        void addGroup(FilterGroup * newGroup);


      private slots:
        void addGroup();
        void deleteGroup(FilterGroup *);
        void changeGroupCombinationLogic(int);
        void updateDescription();
        void maybeScroll(FilterGroup *);


      private:
        QPushButton * addGroupButton;
        QButtonGroup * buttonGroup;
        QLabel * imageDescription;
        QLabel * imageDummy;
        QLabel * pointDescription;
        QLabel * pointDummy;
        QLabel * measureDescription;
        QLabel * measureDummy;
        QVBoxLayout * mainLayout;
        QWidget * logicWidget;

        bool andGroupsTogether;

        QList< FilterGroup * > * filterGroups;
        QString * filterType;
    };
  }
}

#endif
