#ifndef PointFilterSelector_H
#define PointFilterSelector_H


#include "AbstractFilterSelector.h"


namespace Isis
{
  class AbstractFilter;

  class PointFilterSelector : public AbstractFilterSelector
  {
      Q_OBJECT

    public:
      PointFilterSelector();
      virtual ~PointFilterSelector();


    protected:
      void createSelector();


    protected: // slots (already marked as slots inside parent)
      void changeFilter(int);
  };
}

#endif
