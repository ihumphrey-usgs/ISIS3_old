/**
 * @file
 * $Revision: 1.19 $
 * $Date: 2010/03/22 19:44:53 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */
#include "SaveProjectWorkOrder.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrentMap>

#include "Cube.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "Project.h"

namespace Isis {

  SaveProjectWorkOrder::SaveProjectWorkOrder(Project *project) :
      WorkOrder(project) {
    QAction::setText(tr("&Save Project"));
    QUndoCommand::setText(tr("Save Project"));

    setCreatesCleanState(true);
  }


  SaveProjectWorkOrder::SaveProjectWorkOrder(const SaveProjectWorkOrder &other) :
      WorkOrder(other) {
  }


  SaveProjectWorkOrder::~SaveProjectWorkOrder() {

  }


  SaveProjectWorkOrder *SaveProjectWorkOrder::clone() const {
    return new SaveProjectWorkOrder(*this);
  }


  bool SaveProjectWorkOrder::setupExecution() {
    bool success = WorkOrder::setupExecution();

    if (success) {
      project()->save();
      project()->setClean(true);
    }

    return success;
  }
}
