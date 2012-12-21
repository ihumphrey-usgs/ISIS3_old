#include "IsisDebug.h"

#include "ControlCubeGraphNode.h"

#include <iostream>

#include <QHash>
#include <QString>

#include "ControlMeasure.h"
#include "ControlPoint.h"
#include "IException.h"
#include "IString.h"


namespace Isis {
  /**
  * Create an empty SerialNumber object.
  */
  ControlCubeGraphNode::ControlCubeGraphNode(QString sn) {
    nullify();

    serialNumber = new QString(sn);
    measures = new QHash<ControlPoint *, ControlMeasure *>;
    connections = new QHash < ControlCubeGraphNode *, QList< ControlPoint * > >;
  }


  ControlCubeGraphNode::ControlCubeGraphNode(const ControlCubeGraphNode &other) {
    nullify();

    serialNumber = new QString(*other.serialNumber);
    measures = new QHash<ControlPoint *, ControlMeasure *>;
    connections = new QHash < ControlCubeGraphNode *, QList< ControlPoint * > >;

    *measures = *other.measures;
  }


  void ControlCubeGraphNode::nullify() {
    serialNumber = NULL;
    measures = NULL;
    connections = NULL;
  }


  /**
   * Destroy a SerialNumber object.
  */
  ControlCubeGraphNode::~ControlCubeGraphNode() {
    if (serialNumber) {
      delete serialNumber;
      serialNumber = NULL;
    }

    if (measures) {
      delete measures;
      measures = NULL;
    }

    if (connections) {
      delete connections;
      connections = NULL;
    }
  }


  /**
   * @param point The ControlPoint to check for
   *
   * @returns true if the point is contained, false otherwise
   */
  bool ControlCubeGraphNode::contains(ControlPoint *point) const {
    return measures->contains(point);
  }


  /**
   * Adds a measure
   *
   * @param measure The ControlMeasure to add
   */
  void ControlCubeGraphNode::addMeasure(ControlMeasure *measure) {
    ASSERT(measure);

    if (measure->GetCubeSerialNumber() != *serialNumber) {
      QString msg = "Attempted to add Control Measure with Cube Serial Number ";
      msg += "[" + measure->GetCubeSerialNumber() + "] does not match Serial ";
      msg += "Number [" + *serialNumber + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    measure->associatedCSN = this;
    ASSERT(!measures->contains(measure->Parent()));
    (*measures)[measure->Parent()] = measure;
  }


  void ControlCubeGraphNode::removeMeasure(ControlMeasure *measure) {

    if (measures->remove(measure->Parent()) != 1) {
      ASSERT(0);
    }

    measure->associatedCSN = NULL;
  }


  void ControlCubeGraphNode::addConnection(ControlCubeGraphNode *node,
      ControlPoint *point) {
    ASSERT(node);
    ASSERT(point);

    if (connections->contains(node)) {
      if (!(*connections)[node].contains(point))
        (*connections)[node].append(point);
    }
    else {
      QList< ControlPoint * > newConnectionList;
      newConnectionList.append(point);
      (*connections)[node] = newConnectionList;
    }
  }


  void ControlCubeGraphNode::removeConnection(ControlCubeGraphNode *node,
      ControlPoint *point) {
    ASSERT(node);
    ASSERT(point);
    ASSERT(connections);

    if (connections->contains(node)) {
      if ((*connections)[node].contains(point)) {
        (*connections)[node].removeOne(point);
        if (!(*connections)[node].size())
          connections->remove(node);
      }
    }
  }


  int ControlCubeGraphNode::getMeasureCount() const {
    return measures->size();
  }


  QString ControlCubeGraphNode::getSerialNumber() const {
    return *serialNumber;
  }


  QList< ControlMeasure * > ControlCubeGraphNode::getMeasures() const {
    return measures->values();
  }


  QList< ControlMeasure * > ControlCubeGraphNode::getValidMeasures() const {
    QList< ControlMeasure * > validMeasures;

    QList< ControlMeasure * > measureList = measures->values();
    foreach(ControlMeasure * measure, measureList) {
      if (!measure->IsIgnored())
        validMeasures.append(measure);
    }

    return validMeasures;
  }


  QList< ControlCubeGraphNode * > ControlCubeGraphNode::getAdjacentNodes() const {
    return connections->keys();
  }


  bool ControlCubeGraphNode::isConnected(ControlCubeGraphNode *other) const {
    return connections->contains(other);
  }


  ControlMeasure *ControlCubeGraphNode::getMeasure(ControlPoint *point) {
    if (!measures->contains(point)) {
      QString msg = "point [";
      msg += (QString) point->GetId();
      msg += "] not found in the ControlCubeGraphNode";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return (*measures)[point];
  }


  const ControlMeasure *ControlCubeGraphNode::getMeasure(
    ControlPoint *point) const {
    if (!measures->contains(point)) {
      QString msg = "point [";
      msg += (QString) point->GetId();
      msg += "] not found in the ControlCubeGraphNode";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return measures->value(point);
  }


  ControlMeasure *ControlCubeGraphNode::operator[](ControlPoint *point) {
    return getMeasure(point);
  }


  const ControlMeasure *ControlCubeGraphNode::operator[](
    ControlPoint *point) const {
    return getMeasure(point);
  }


  const ControlCubeGraphNode &ControlCubeGraphNode::operator=(
    ControlCubeGraphNode other) {
    if (this == &other)
      return *this;

    if (serialNumber) {
      delete serialNumber;
      serialNumber = NULL;
    }

    if (measures) {
      delete measures;
      measures = NULL;
    }

    if (connections) {
      delete connections;
      connections = NULL;
    }

    serialNumber = new QString;
    measures = new QHash< ControlPoint *, ControlMeasure *>;
    connections = new QHash< ControlCubeGraphNode *, QList< ControlPoint * > >;

    *serialNumber = *other.serialNumber;
    *measures = *other.measures;
    *connections = *other.connections;

    return *this;
  }


  QString ControlCubeGraphNode::connectionsToString() const {
    QHashIterator< ControlCubeGraphNode *, QList< ControlPoint * > > i(
      *connections);

    QStringList serials;
    while (i.hasNext()) {
      i.next();
      QString line = "    " + (QString) i.key()->getSerialNumber();
      line += " :  ";
      for (int j = 0; j < i.value().size(); j++) {
        line += (QString) i.value()[j]->GetId();
        if (j != i.value().size() - 1)
          line += ", ";
      }
      serials << line;
    }
    qSort(serials);

    return serials.join("\n");
  }

}
