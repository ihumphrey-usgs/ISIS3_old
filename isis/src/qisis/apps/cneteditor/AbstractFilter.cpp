#include "IsisDebug.h"

#include <iostream>
#include <limits>

#include "AbstractFilter.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QMargins>
#include <QRadioButton>
#include <QSpinBox>


using std::cerr;


namespace Isis
{
  AbstractFilter::AbstractFilter(int minimumForImageSuccess) :
      minForImageSuccess(minimumForImageSuccess)
  {
    cerr << this << " AbstractFilter::AbstractFilter: " << minForImageSuccess << "\n";
  }


  AbstractFilter::~AbstractFilter()
  {
  }


  void AbstractFilter::nullify()
  {
    mainLayout = NULL;
    inclusiveExclusiveGroup = NULL;
  }


  void AbstractFilter::createWidget()
  {
    QRadioButton * inclusiveButton = new QRadioButton("Inclusive");
    QRadioButton * exclusiveButton = new QRadioButton("Exclusive");

    inclusiveExclusiveGroup = new QButtonGroup;
    connect(inclusiveExclusiveGroup, SIGNAL(buttonClicked(int)),
        this, SIGNAL(filterChanged()));
    inclusiveExclusiveGroup->addButton(inclusiveButton, 0);
    inclusiveExclusiveGroup->addButton(exclusiveButton, 1);
    
    mainLayout = new QHBoxLayout;
    QMargins margins = mainLayout->contentsMargins();
    margins.setTop(0);
    margins.setBottom(0);
    mainLayout->setContentsMargins(margins);
    mainLayout->addWidget(inclusiveButton);
    mainLayout->addWidget(exclusiveButton);
    cerr << this << " minForImageSuccess: " << minForImageSuccess << "\n";
    if (minForImageSuccess != -1)
    {
      QLabel * label = new QLabel("Min Count: ");
      QSpinBox * spinBox = new QSpinBox;
      spinBox->setRange(0, std::numeric_limits< int >::max());
      spinBox->setValue(1);
      connect(spinBox, SIGNAL(valueChanged(int)),
          this, SLOT(updateMinForImageSuccess(int)));
      mainLayout->addWidget(label);
      mainLayout->addWidget(spinBox);
    }

    setLayout(mainLayout);

    // FIXME: QSettings should handle this
    inclusiveButton->click();
  }


  bool AbstractFilter::inclusive() const
  {
    return inclusiveExclusiveGroup->checkedId() == 0;
  }
  
  
  void AbstractFilter::updateMinForImageSuccess(int newMin)
  {
    minForImageSuccess = newMin;
  }
}
