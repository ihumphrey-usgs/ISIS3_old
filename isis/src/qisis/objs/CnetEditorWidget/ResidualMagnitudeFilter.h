#ifndef ResidualMagnitudeFilter_H
#define ResidualMagnitudeFilter_H

#include "AbstractNumberFilter.h"


class QString;


namespace Isis {
  class AbstractFilterSelector;
  class ControlCubeGraphNode;
  class ControlMeasure;
  class ControlPoint;

  /**
   * @brief Filters by residual magnitude
   *
   * This class handles filtering by residual magnitudes.
   *
   * @author ????-??-?? Eric Hyer
   *
   * @internal 
   *   @history 2017-07-25 Summer Stapleton - Removed the CnetViz namespace. Fixes #5054. 
   */
  class ResidualMagnitudeFilter : public AbstractNumberFilter {
      Q_OBJECT

    public:
      ResidualMagnitudeFilter(AbstractFilter::FilterEffectivenessFlag flag,
          int minimumForSuccess = -1);
      ResidualMagnitudeFilter(const ResidualMagnitudeFilter &other);
      virtual ~ResidualMagnitudeFilter();

      bool evaluate(const ControlCubeGraphNode *) const;
      bool evaluate(const ControlPoint *) const;
      bool evaluate(const ControlMeasure *) const;

      AbstractFilter *clone() const;

      QString getImageDescription() const;
      QString getPointDescription() const;
      QString getMeasureDescription() const;
  };
}

#endif
