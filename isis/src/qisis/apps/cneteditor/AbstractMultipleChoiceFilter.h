#ifndef AbstractMultipleChoiceFilter_H
#define AbstractMultipleChoiceFilter_H


// parent
#include "AbstractFilter.h"


class QComboBox;
class QString;


namespace Isis
{
  namespace CnetViz
  {
    class AbstractFilterSelector;

    /**
     * @brief Base class for combo box type filters
     *
     * This class is the base class that all filters that use combo boxes.
     *
     * @author ????-??-?? Eric Hyer
     *
     * @internal
     */
    class AbstractMultipleChoiceFilter : public AbstractFilter
    {
        Q_OBJECT

      public:
        AbstractMultipleChoiceFilter(AbstractFilter::FilterEffectivenessFlag,
            int minimumForSuccess = -1);
        AbstractMultipleChoiceFilter(const AbstractMultipleChoiceFilter & other);
        virtual ~AbstractMultipleChoiceFilter();

        
      protected:
        void createWidget(QStringList options);
        QString const & getCurrentChoice() const;
        
        
      private:
        void nullify();
        
        
      private slots:
        void updateCurChoice(QString);


      private:
        QComboBox * combo;
        QString * curChoice;
    };
  }
}

#endif
