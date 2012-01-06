#ifndef GoodnessOfFitFilter_H
#define GoodnessOfFitFilter_H

#include "AbstractNumberFilter.h"


class QString;


namespace Isis
{
  class ControlCubeGraphNode;
  class ControlMeasure;
  class ControlPoint;
  
  namespace CnetViz
  {
    class AbstractFilterSelector;
    
    /**
     * @brief Allows filtering by goodness of fit
     *
     * This class allows the user to filter control points and control measures
     * by goodness of fit. This allows the user to make a list of control
     * points that are potentially mis-registered.
     *
     * @author ????-??-?? Eric Hyer
     *
     * @internal
     */
    class GoodnessOfFitFilter : public AbstractNumberFilter
    {
        Q_OBJECT

      public:
        GoodnessOfFitFilter(AbstractFilter::FilterEffectivenessFlag flag,
            int minimumForSuccess = -1);
        GoodnessOfFitFilter(const GoodnessOfFitFilter & other);
        virtual ~GoodnessOfFitFilter();

        bool evaluate(const ControlCubeGraphNode *) const;
        bool evaluate(const ControlPoint *) const;
        bool evaluate(const ControlMeasure *) const;

        AbstractFilter * clone() const;

        QString getImageDescription() const;
        QString getPointDescription() const;
        QString getMeasureDescription() const;
    };
  }
}

#endif
