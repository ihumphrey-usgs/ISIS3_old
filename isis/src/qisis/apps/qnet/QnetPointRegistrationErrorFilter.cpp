#include <QGridLayout>
#include <QMessageBox>
#include "QnetPointRegistrationErrorFilter.h"
#include "QnetNavTool.h"
#include "ControlNet.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlPoint.h"
#include "Statistics.h"

#include "qnet.h"

using namespace Isis::Qnet;

namespace Isis {
  /**
  * Contructor for the Point Registration Error filter.  It
  * creates the Registration Error filter window found in the
  * navtool
  *
  * @param parent The parent widget for the point
  *               error filter
  *
  * @internal
  *  @history  2008-08-06 Tracie Sucharski - Added functionality
  *                          of filtering range of errors.
  *  @history  2010-06-02 Jeannie Walldren - Modify default
  *                          settings of checkboxes and line edits
  *  @history 2010-06-03 Jeannie Walldren - Initialized pointers
  *                          to null
  */
  QnetPointRegistrationErrorFilter::QnetPointRegistrationErrorFilter(QWidget *parent) : QnetFilter(parent) {
    p_lessThanCB = NULL;
    p_greaterThanCB = NULL;
    p_lessErrorEdit = NULL;
    p_greaterErrorEdit = NULL;

    // Create the components for the filter window
    QLabel *label = new QLabel("Registration Pixel Errors");
    p_lessThanCB = new QCheckBox("Less than");
    p_lessErrorEdit = new QLineEdit();
    p_greaterThanCB = new QCheckBox("Greater than");
    p_greaterErrorEdit = new QLineEdit();
    QLabel *pixels = new QLabel("pixels");
    QLabel *pad = new QLabel();

    p_lessThanCB->setChecked(false);
    p_lessErrorEdit->setEnabled(false);
    p_greaterThanCB->setChecked(true);
    p_greaterErrorEdit->setEnabled(true);

    connect(p_lessThanCB, SIGNAL(clicked()), this, SLOT(clearEdit()));
    connect(p_greaterThanCB, SIGNAL(clicked()), this, SLOT(clearEdit()));

    // Create the layout and add the components to it
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(label, 0, 0, 1, 2);
    gridLayout->addWidget(p_lessThanCB, 1, 0, 1, 2);
    gridLayout->addWidget(p_lessErrorEdit, 2, 0);
    gridLayout->addWidget(pixels, 2, 1);
    gridLayout->addWidget(p_greaterThanCB, 3, 0, 1, 2);
    gridLayout->addWidget(p_greaterErrorEdit, 4, 0);
    gridLayout->addWidget(pixels, 4, 1);
    gridLayout->addWidget(pad, 5, 0);
    gridLayout->setRowStretch(5, 50);
    this->setLayout(gridLayout);
  }

  /**
   * Filters a list of points for points that have less than or greater
   * than the entered registration error values.  The filtered list will appear 
   * in the navtools point list display. 
   *
   * @internal
   *   @history  2007-06-05 Tracie Sucharski - Look at ControlPoint::MaximumError
   *                           instead of ControlPoint::AverageError
   *   @history  2008-08-06 Tracie Sucharski - Added functionality of filtering
   *                           range of errors.
   *   @history  2009-01-08 Jeannie Walldren - Modified to remove
   *                           new filter points from the existing
   *                           filtered list. Previously, a new
   *                           filtered list was created from the
   *                           entire control net each time.
   *   @history  2010-07-14 Tracie Sucharski - ControlPoint::MaximumError
   *                           renamed to MaximumResidual.
   *   @history  2011-04-28 Tracie Sucharski - Sort points in decsending order
   *                           of max residual.
   */
  void QnetPointRegistrationErrorFilter::filter() {
    // Make sure we have a list of control points to filter
    if (g_controlNetwork == NULL) {
      QMessageBox::information((QWidget *)parent(),
          "Error", "No points to filter");
      return;
    }

    // Make sure the user entered a value to use in the filtering
    double lessNum = -1.;
    if (p_lessThanCB->isChecked() && p_lessErrorEdit->text() == "") {
      QMessageBox::information((QWidget *)parent(),
          "Error", "Error value must be entered");
      return;
    }
    double greaterNum = -1.;
    if (p_greaterThanCB->isChecked() && p_greaterErrorEdit->text() == "") {
      QMessageBox::information((QWidget *)parent(),
          "Error", "Error value must be entered");
      return;
    }

    // Get the user entered filtering value
    lessNum = p_lessErrorEdit->text().toDouble();
    greaterNum = p_greaterErrorEdit->text().toDouble();

    QMultiMap <double, int> pointMap;
    // Loop through each value of the filtered points list comparing the error
    // of its corresponding point with error the user entered value and remove
    // it from the filtered list if it is outside the filtering range Loop in
    // reverse order since removal list of elements affects index number
    for (int i = g_filteredPoints.size() - 1; i >= 0; i--) {
      ControlPoint &cp = *(*g_controlNetwork)[g_filteredPoints[i]];
//      double maxPixelError = calculateMaxError(cp);
      double maxPixelError =
                  cp.GetStatistic(&ControlMeasure::GetPixelShift).Maximum();
      if (p_lessThanCB->isChecked() && p_greaterThanCB->isChecked()) {
        if (maxPixelError < lessNum && maxPixelError > greaterNum) {
          pointMap.insert(maxPixelError, g_filteredPoints[i]);
          continue;
        }
        else
          g_filteredPoints.removeAt(i);
      }
      else if (p_lessThanCB->isChecked()) {
        if (maxPixelError < lessNum) {
          pointMap.insert(maxPixelError, g_filteredPoints[i]);
          continue;
        }
        else
          g_filteredPoints.removeAt(i);
      }
      else if (p_greaterThanCB->isChecked()) {
        if (maxPixelError > greaterNum) {
          pointMap.insert(maxPixelError, g_filteredPoints[i]);
          continue;
        }
        else
          g_filteredPoints.removeAt(i);
      }
    }

    int filteredIndex = 0;
    QMultiMap<double, int>::const_iterator i = pointMap.constEnd();
    while (i != pointMap.constBegin()) {
      --i;
      g_filteredPoints[filteredIndex] = i.value();
      filteredIndex++;
    }
    // Tell the navtool that a list has been filtered and it needs to update
    emit filteredListModified();
    return;
  }


  /**
   * Clears and disables the corresponding line edit if the "less
   * than" or "greater than" checkBox is "unchecked".
   *
   * @internal
   *   @history 2008-08-06 Tracie Sucharski - New method for
   *                         added functionality filtering range
   *                         of errors.
   *   @history 2010-06-02 Jeannie Walldren - Disable the line
   *            edit so the user can not enter a value unless the
   *            corresponding box is checked.
   */
  void QnetPointRegistrationErrorFilter::clearEdit() {

    if (p_lessThanCB->isChecked()) {
      p_lessErrorEdit->setEnabled(true);
    }
    else {
      p_lessErrorEdit->clear();
      p_lessErrorEdit->setEnabled(false);
    }
    if (p_greaterThanCB->isChecked()) {
      p_greaterErrorEdit->setEnabled(true);
    }
    else {
      p_greaterErrorEdit->clear();
      p_greaterErrorEdit->setEnabled(false);
    }
  }
}
