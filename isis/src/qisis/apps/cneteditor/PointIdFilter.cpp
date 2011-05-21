#include "IsisDebug.h"

#include "PointIdFilter.h"

#include <iostream>

#include <QHBoxLayout>
#include <QLineEdit>

#include "ControlPoint.h"


using std::cerr;


namespace Isis
{
  PointIdFilter::PointIdFilter(int minimumForImageSuccess) :
      AbstractFilter(minimumForImageSuccess)
  {
    nullify();
    createWidget();
  }


  PointIdFilter::~PointIdFilter()
  {
  }


  void PointIdFilter::nullify()
  {
    AbstractFilter::nullify();

    lineEdit = NULL;
  }


  void PointIdFilter::createWidget()
  {
    AbstractFilter::createWidget();

    lineEdit = new QLineEdit;
    lineEdit->setMinimumWidth(200);
    connect(lineEdit, SIGNAL(textChanged(QString)),
        this, SIGNAL(filterChanged()));
    mainLayout->addWidget(lineEdit);
  }


  /**
   * Given a point to evaluate, return true if it makes it through the filter,
   * and false otherwise.  Criteria defining the filter is defined in this
   * method.  Note that whether the filter is inclusive or exclusive is handled
   * in this method.
   *
   * @param point The point to evaluate
   *
   * @returns True if the point makes it through the filter, false otherwise
   */
  bool PointIdFilter::evaluate(const ControlPoint * point) const
  {
    bool evaluation = true;
    
    QString lineEditText = lineEdit->text();
    if (lineEditText.size() >= 1)
    {
      bool match = ((QString) point->GetId()).contains(
          lineEditText, Qt::CaseInsensitive);
      evaluation = !(inclusive() ^ match);
      
      //  inclusive() | match | evaluation
      //  ------------|-------|-----------
      //       T      |   T   |   T
      //  ------------|-------|-----------
      //       T      |   F   |   F
      //  ------------|-------|-----------
      //       F      |   T   |   F
      //  ------------|-------|-----------
      //       F      |   F   |   T
      //  ------------|-------|-----------
    }
    
    return evaluation;
  }


  bool PointIdFilter::evaluate(const ControlMeasure * measure) const
  {
    return true;
  }


  bool PointIdFilter::evaluate(const ControlCubeGraphNode * node) const
  {
    return true;
  }
  
  
  QString PointIdFilter::getDescription() const
  {
    cerr << "PointIdFilter::getDescription(): " << minForImageSuccess << "\n";
    QString description;
    
    ASSERT(lineEdit);
    if (lineEdit)
    {
      description = "have point id's ";
      if (inclusive())
        description += "containing ";
      else
        description += "that don't contain ";
      description += "\"" + lineEdit->text() + "\"";
    }
    
    return description;
  }
}
