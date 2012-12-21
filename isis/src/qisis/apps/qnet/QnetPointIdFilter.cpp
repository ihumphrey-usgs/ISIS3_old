#include <QGridLayout>
#include <QMessageBox>
#include <QRegExp>
#include "QnetPointIdFilter.h"
#include "QnetNavTool.h"
#include "ControlNet.h"
#include "SerialNumberList.h"

#include "qnet.h"

using namespace Isis::Qnet;
using namespace std;

namespace Isis {
  /**
   * Contructor for the Cube Image filter.  It creates the Cube Name filter window
   * found in the navtool
   *
   * @param parent The parent widget for the
   *               cube points filter
   * @internal
   *   @history 2010-06-03 Jeannie Walldren - Initialized pointer
   *                          to null.
   *
   */
  QnetPointIdFilter::QnetPointIdFilter(QWidget *parent) : QnetFilter(parent) {
    p_pointIdEdit = NULL;

    // Create the components for the filter window
    QLabel *label = new QLabel("Filter by Point ID (Regular Expressions)");
    p_pointIdEdit = new QLineEdit();

    // Create the layout and add the components to it
    QVBoxLayout *vertLayout = new QVBoxLayout();
    vertLayout->addWidget(label);
    vertLayout->addWidget(p_pointIdEdit);
    vertLayout->addStretch();
    this->setLayout(vertLayout);
  }

  /**
   * Filters a list of images looking for cube names using the regular expression
   * entered.  The filtered list will appear in the navtools cube list display.
   *
   * @internal
   *   @history 2009-01-08 Jeannie Walldren - Modified to remove
   *                          new filter points from the existing
   *                          filtered list.
   *   @history 2011-10-20 Tracie Sucharski - trim whitespace from input 
   */
  void QnetPointIdFilter::filter() {

    // Make sure there is a control net loaded
    if (g_controlNetwork == NULL) {
      QMessageBox::information((QWidget *)parent(),
          "Error", "No points to filter");
      return;
    }

    // Make sure the user has entered a regular expression for filtering
    QRegExp rx(p_pointIdEdit->text().trimmed());
    rx.setPatternSyntax(QRegExp::Wildcard);
    if (rx.isEmpty()) {
      QMessageBox::information((QWidget *)parent(),
          "Error", "Enter search string");
      return;
    }


    // Loop through each value of the filtered points list checking
    // the types of each control measure for each of the control points
    // Loop in reverse order since removal list of elements affects index number
    for (int i = g_filteredPoints.size() - 1; i >= 0; i--) {

      QString cNetId = (*g_controlNetwork)[g_filteredPoints[i]]->GetId();
      if (rx.indexIn(QString(cNetId)) != -1) {
        continue;
      }
      else
        g_filteredPoints.removeAt(i);
    }

    // Tell the navtool a list has been filtered and it needs to update
    emit filteredListModified();
    return;
  }
}
