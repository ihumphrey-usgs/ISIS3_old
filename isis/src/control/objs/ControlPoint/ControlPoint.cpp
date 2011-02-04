#include "ControlPoint.h"

#include <boost/numeric/ublas/symmetric.hpp>
#include <boost/numeric/ublas/io.hpp>

#include <QHash>
#include <QString>
#include <QStringList>

#include "Application.h"
#include "CameraDetectorMap.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "ControlNet.h"
#include "Cube.h"
#include "iString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "PBControlNetIO.pb.h"
#include "PBControlNetLogData.pb.h"
#include "PvlObject.h"
#include "SerialNumberList.h"
#include "SpecialPixel.h"

using boost::numeric::ublas::symmetric_matrix;
using boost::numeric::ublas::upper;

namespace Isis {
  /**
   * Construct a control point
   *
   * @author tsucharski (5/5/2010)
   *
   */
  ControlPoint::ControlPoint() : p_invalid(false) {
    p_measures = NULL;
    cubeSerials = NULL;

    p_measures = new QHash< QString, ControlMeasure * >;
    cubeSerials = new QStringList;

    p_type = Tie;
    p_dateTime = "";
    p_editLock = false;
    p_ignore = false;
    p_jigsawRejected = false;
    p_aprioriSurfacePointSource = SurfacePointSource::None;
    p_aprioriRadiusSource = RadiusSource::None;
    parentNetwork = NULL;
    referenceMeasure = NULL;
  }

  ControlPoint::ControlPoint(const ControlPoint &other) {
    p_measures = NULL;
    cubeSerials = NULL;
    referenceMeasure = NULL;

    p_measures = new QHash< QString, ControlMeasure * >;
    cubeSerials = new QStringList;

    QHashIterator< QString, ControlMeasure * > i(*other.p_measures);
    while (i.hasNext()) {
      i.next();
      ControlMeasure *newMeasure = new ControlMeasure(*i.value());
      if (other.referenceMeasure == i.value())
        referenceMeasure = newMeasure; 
      QString newSerial = newMeasure->GetCubeSerialNumber();
      newMeasure->parentPoint = this;
      p_measures->insert(newSerial, newMeasure);
      cubeSerials->append(newSerial);
    }

    if (referenceMeasure == NULL && cubeSerials->size() != 0)
      referenceMeasure = p_measures->value(cubeSerials->at(0));

    parentNetwork = other.parentNetwork;
    p_id = other.p_id;
    p_chooserName = other.p_chooserName;
    p_dateTime = other.p_dateTime;
    p_type = other.p_type;
    p_invalid = other.p_invalid;
    p_editLock = other.p_editLock;
    p_jigsawRejected = other.p_jigsawRejected;
    p_ignore = other.p_ignore;
    p_aprioriSurfacePointSource = other.p_aprioriSurfacePointSource;
    p_aprioriSurfacePointSourceFile = other.p_aprioriSurfacePointSourceFile;
    p_aprioriRadiusSource = other.p_aprioriRadiusSource;
    p_aprioriRadiusSourceFile = other.p_aprioriRadiusSourceFile;
    p_aprioriSurfacePoint = other.p_aprioriSurfacePoint;
    p_surfacePoint = other.p_surfacePoint;
    p_numberOfRejectedMeasures = other.p_numberOfRejectedMeasures;
  }

  ControlPoint::ControlPoint(const PBControlNet_PBControlPoint &protoBufPt) {
    p_measures = NULL;
    cubeSerials = NULL;
    referenceMeasure = NULL;

    p_measures = new QHash< QString, ControlMeasure * >;
    cubeSerials = new QStringList;
    Init(protoBufPt);

    for (int m = 0 ; m < protoBufPt.measures_size() ; m++) {
      // Create a PControlMeasure and fill in it's info.
      // with the values from the input file.
      ControlMeasure *measure = new ControlMeasure(protoBufPt.measures(m));
      AddMeasure(measure);
    }
  }


  ControlPoint::ControlPoint(const PBControlNet_PBControlPoint &protoBufPt,
                             const PBControlNetLogData_Point &logProtoBuf) {
    p_measures = NULL;
    cubeSerials = NULL;
    referenceMeasure = NULL;

    p_measures = new QHash< QString, ControlMeasure * >;
    cubeSerials = new QStringList;
    Init(protoBufPt);

    for (int m = 0 ; m < protoBufPt.measures_size() ; m++) {
      // Create a PControlMeasure and fill in it's info.
      // with the values from the input file.
      ControlMeasure *measure = new ControlMeasure(protoBufPt.measures(m), logProtoBuf.measures(m));
      AddMeasure(measure);
    }
  }


  /**
   * Construct a control point with given Id
   *
   * @param id Control Point Id
   */
  ControlPoint::ControlPoint(const iString &id) : p_invalid(false) {
    parentNetwork = NULL;
    p_measures = NULL;
    referenceMeasure = NULL;
    p_measures = new QHash< QString, ControlMeasure * >;
    cubeSerials = new QStringList;

    p_id = id;
    p_type = Tie;
    p_editLock = false;
    p_jigsawRejected = false;
    p_ignore = false;
    p_aprioriSurfacePointSource = SurfacePointSource::None;
    p_aprioriRadiusSource = RadiusSource::None;
  }


  /**
   * This destroys the current instance and cleans up any and all allocated
   *    memory.
   */
  ControlPoint::~ControlPoint() {
    if (p_measures != NULL) {
      QList< QString > keys = p_measures->keys();
      for (int i = 0; i < keys.size(); i++) {
        delete(*p_measures)[keys[i]];
        (*p_measures)[keys[i]] = NULL;
      }

      delete p_measures;
      p_measures = NULL;
    }

    if (cubeSerials) {
      delete cubeSerials;
      cubeSerials = NULL;
    }

    referenceMeasure = NULL;
  }

  /**
  * Loads the PvlObject into a ControlPoint
  *
  * @param p PvlObject containing ControlPoint information
  * @param forceBuild Allows invalid Control Measures to be added to this
  *                   Control Point
  *
  * @throws Isis::iException::User - Invalid Point Type
  * @throws Isis::iException::User - Unable to add ControlMeasure to Control
  *                                  Point
  *
  * @history 2008-06-18  Tracie Sucharski/Jeannie Walldren, Fixed bug with
  *                         checking for "True" vs "true", change to
  *                         lower case for comparison.
  * @history 2009-12-29  Tracie Sucharski - Added new ControlPoint information.
  * @history 2010-01-13  Tracie Sucharski - Changed from Set methods to simply
  *                         setting private variables to increase speed?
  * @history 2010-07-30  Tracie Sucharski, Updated for changes made after
  *                         additional working sessions for Control network
  *                         design.
  * @history 2010-09-01  Tracie Sucharski, Add checks for AprioriLatLonSource
  *                         AprioriLatLonSourceFile.  If there are
  *                         AprioriSigmas,but no AprioriXYZ, use the XYZ values.
  * @history 2010-09-15 Tracie Sucharski, It was decided after mtg with
  *                         Debbie, Stuart, Ken and Tracie that ControlPoint
  *                         will only function with x/y/z, not lat/lon/radius.
  *                         It will be the responsibility of the application
  *                         or class using ControlPoint to set up a
  *                         SurfacePoint object to do conversions between x/y/z
  *                         and lat/lon/radius.
  *                         So... remove all conversion methods from this
  *                         class.
  *                         It was also decided that when importing old
  *                         networks that contain Sigmas, the sigmas will not
  *                         be imported , due to conflicts with the units of
  *                         the sigmas,we cannot get accurate x,y,z sigams from
  *                         the lat,lon,radius sigmas without the covariance
  *                         matrix.
  * @history 2010-09-28 Tracie Sucharski, Added back the conversion methods
  *                         from lat,lon,radius to x,y,z only for the point,
  *                         since that is what most applications need.
  * @history 2010-12-02 Debbie A. Cook, Added units to
  *                         SurfacePoint.SetSpherical calls.
  */
  void ControlPoint::Load(PvlObject &p) {
    p_id = (std::string) p["PointId"];
    if ((std::string)p["PointType"] == "Ground") {
      p_type = Ground;
    }
    else if ((std::string)p["PointType"] == "Tie") {
      p_type = Tie;
    }
    else {
      std::string msg = "Invalid Point Type, [" + (std::string)p["PointType"] +
                        "]";
      throw iException::Message(iException::User, msg, _FILEINFO_);
    }
    if (p.HasKeyword("Ignore")) {
      iString ignore = (std::string)p["Ignore"];
      if (ignore.DownCase() == "true")
        p_ignore = true;
    }
    if (p.HasKeyword("AprioriXYZSource")) {
      if ((std::string)p["AprioriXYZSource"] == "None") {
        p_aprioriSurfacePointSource = SurfacePointSource::None;
      }
      else if ((std::string)p["AprioriXYZSource"] == "User") {
        p_aprioriSurfacePointSource = SurfacePointSource::User;
      }
      else if ((std::string)p["AprioriXYZSource"] == "AverageOfMeasures") {
        p_aprioriSurfacePointSource = SurfacePointSource::AverageOfMeasures;
      }
      else if ((std::string)p["AprioriXYZSource"] == "Reference") {
        p_aprioriSurfacePointSource = SurfacePointSource::Reference;
      }
      else if ((std::string)p["AprioriXYZSource"] == "Basemap") {
        p_aprioriSurfacePointSource = SurfacePointSource::Basemap;
      }
      else if ((std::string)p["AprioriXYZSource"] == "BundleSolution") {
        p_aprioriSurfacePointSource = SurfacePointSource::BundleSolution;
      }
      else {
        std::string msg = "Invalid AprioriXYZSource, [" +
                          (std::string)p["AprioriXYZSource"] + "]";
        throw iException::Message(iException::User, msg, _FILEINFO_);
      }
    }
    if (p.HasKeyword("AprioriXYZSourceFile")) {
      p_aprioriSurfacePointSourceFile = (std::string)p["AprioriXYZSourceFile"];
    }

    //  Look for AprioriLatLonSource.  These keywords may exist in old nets.
    if (p.HasKeyword("AprioriLatLonSource")) {
      if ((std::string)p["AprioriLatLonSource"] == "None") {
        p_aprioriSurfacePointSource = SurfacePointSource::None;
      }
      else if ((std::string)p["AprioriLatLonSource"] == "User") {
        p_aprioriSurfacePointSource = SurfacePointSource::User;
      }
      else if ((std::string)p["AprioriLatLonSource"] == "AverageOfMeasures") {
        p_aprioriSurfacePointSource = SurfacePointSource::AverageOfMeasures;
      }
      else if ((std::string)p["AprioriLatLonSource"] == "Reference") {
        p_aprioriSurfacePointSource = SurfacePointSource::Reference;
      }
      else if ((std::string)p["AprioriLatLonSource"] == "Basemap") {
        p_aprioriSurfacePointSource = SurfacePointSource::Basemap;
      }
      else if ((std::string)p["AprioriLatLonSource"] == "BundleSolution") {
        p_aprioriSurfacePointSource = SurfacePointSource::BundleSolution;
      }
      else {
        std::string msg = "Invalid AprioriXYZSource, [" +
                          (std::string)p["AprioriXYZSource"] + "]";
        throw iException::Message(iException::User, msg, _FILEINFO_);
      }
    }
    if (p.HasKeyword("AprioriLatLonSourceFile")) {
      p_aprioriSurfacePointSourceFile = p["AprioriLatLonSourceFile"][0];
    }

    if (p.HasKeyword("AprioriRadiusSource")) {
      if ((std::string)p["AprioriRadiusSource"] == "None") {
        p_aprioriRadiusSource = RadiusSource::None;
      }
      else if ((std::string)p["AprioriRadiusSource"] == "User") {
        p_aprioriRadiusSource = RadiusSource::User;
      }
      else if ((std::string)p["AprioriRadiusSource"] == "AverageOfMeasures") {
        p_aprioriRadiusSource = RadiusSource::AverageOfMeasures;
      }
      else if ((std::string)p["AprioriRadiusSource"] == "Ellipsoid") {
        p_aprioriRadiusSource = RadiusSource::Ellipsoid;
      }
      else if ((std::string)p["AprioriRadiusSource"] == "DEM") {
        p_aprioriRadiusSource = RadiusSource::DEM;
      }
      else if ((std::string)p["AprioriRadiusSource"] == "BundleSolution") {
        p_aprioriRadiusSource = RadiusSource::BundleSolution;
      }
      else {
        std::string msg = "Invalid AprioriRadiusSource, [" +
                          (std::string)p["AprioriRadiusSource"] + "]";
        throw iException::Message(iException::User, msg, _FILEINFO_);
      }
    }
    if (p.HasKeyword("AprioriRadiusSourceFile")) {
      p_aprioriRadiusSourceFile = (std::string)p["AprioriRadiusSourceFile"];
    }

    if (p.HasKeyword("AprioriX") &&
        p.HasKeyword("AprioriY") &&
        p.HasKeyword("AprioriZ")) {
      p_aprioriSurfacePoint.SetRectangular(Displacement(p["AprioriX"]),
                                           Displacement(p["AprioriY"]), Displacement(p["AprioriZ"]));
    }

    //  Look for AprioriLatitude/Longitude/Radius.  These keywords may
    //  exist in old nets.  Convert to x/y/z.
    else if (p.HasKeyword("AprioriLatitude") &&
             p.HasKeyword("AprioriLongitude") &&
             p.HasKeyword("AprioriRadius")) {
      p_aprioriSurfacePoint.SetSpherical(
        Latitude(p["AprioriLatitude"], Angle::Degrees),
        Longitude(p["AprioriLongitude"], Angle::Degrees),
        Distance(p["AprioriRadius"]));
    }

    if (p.HasKeyword("X") && p.HasKeyword("Y") && p.HasKeyword("Z")) {
      p_surfacePoint.SetRectangular(Displacement(p["X"]), Displacement(p["Y"]),
                                    Displacement(p["Z"]));
    }

    // Look for Latitude/Longitude/Radius.  These keywords may exist in old
    // nets.  Convert to x/y/z.
    else if (p.HasKeyword("Latitude") && p.HasKeyword("Longitude") &&
             p.HasKeyword("Radius")) {
      p_surfacePoint.SetSpherical(
        Latitude(p["Latitude"], Angle::Degrees),
        Longitude(p["Longitude"], Angle::Degrees),
        Distance(p["Radius"]));
    }

    if (p.HasKeyword("AprioriCovarianceMatrix")) {
      PvlKeyword &matrix = p["AprioriCovarianceMatrix"];
      symmetric_matrix<double, upper> aprioriCovariance;
      aprioriCovariance.resize(3);
      aprioriCovariance.clear();

      aprioriCovariance(0, 0) = matrix[0];
      aprioriCovariance(0, 1) = matrix[1];
      aprioriCovariance(0, 2) = matrix[2];
      aprioriCovariance(1, 1) = matrix[3];
      aprioriCovariance(1, 2) = matrix[4];
      aprioriCovariance(2, 2) = matrix[5];

      p_aprioriSurfacePoint.SetRectangularMatrix(aprioriCovariance);
    }

    if (p.HasKeyword("ApostCovarianceMatrix")) {
      PvlKeyword &matrix = p["ApostCovarianceMatrix"];

      symmetric_matrix<double, upper> apostCovariance;
      apostCovariance.resize(3);
      apostCovariance.clear();

      apostCovariance(0, 0) = matrix[0];
      apostCovariance(0, 1) = matrix[1];
      apostCovariance(0, 2) = matrix[2];
      apostCovariance(1, 1) = matrix[3];
      apostCovariance(1, 2) = matrix[4];
      apostCovariance(2, 2) = matrix[5];

      p_surfacePoint.SetRectangularMatrix(apostCovariance);
    }

    if (p.HasKeyword("ChooserName"))
      p_chooserName = p["ChooserName"][0];
    if (p.HasKeyword("DateTime"))
      p_dateTime = p["DateTime"][0];
    if (p.HasKeyword("EditLock")) {
      iString locked = (std::string)p["EditLock"];
      if (locked.DownCase() == "true")
        p_editLock = true;
    }
    if (p.HasKeyword("JigsawRejected")) {
      iString reject = p["JigsawRejected"][0];
      if (reject.DownCase() == "true")
        p_jigsawRejected = true;
    }

    //  Process Measures
    for (int g = 0; g < p.Groups(); g++) {
      try {
        if (p.Group(g).IsNamed("ControlMeasure")) {
          ControlMeasure *cm = new ControlMeasure;
          cm->Load(p.Group(g));
          AddMeasure(cm);
        }
      }
      catch (iException &e) {
        iString msg = "Unable to add Control Measure to ControlPoint [" +
                      GetId() + "]";
        throw iException::Message(iException::User, msg, _FILEINFO_);
      }
    }
  }


  /**
   * Add a measurement to the control point, taking ownership of the measure in
   * the process.
   *
   * @param measure The ControlMeasure to add
   */
  void ControlPoint::Add(ControlMeasure *measure) {
    PointModified();
    AddMeasure(measure);
  }

  void ControlPoint::AddMeasure(ControlMeasure * cmeasure) {
    // Make sure measure is unique
    foreach(ControlMeasure * m, p_measures->values()) {
      if (m->GetCubeSerialNumber() == cmeasure->GetCubeSerialNumber()) {
        iString msg = "The SerialNumber is not unique. A measure with "
                      "serial number [" + cmeasure->GetCubeSerialNumber() + "] already "
                      "exists for ControlPoint [" + GetId() + "]";
        throw iException::Message(iException::Programmer, msg, _FILEINFO_);
      }
    }

    // If its type is Reference, make sure we don't already have a reference
    // measure, if we do, throw an error, if not, make it the reference.
    if (cmeasure->GetType() == ControlMeasure::Reference) {
      if (referenceMeasure != NULL) {
        if (referenceMeasure->GetType() != ControlMeasure::Reference) { 
          referenceMeasure = cmeasure;
        }
        else {
          iString msg = "Cannot add second ControlMeasure with type Reference";
          throw iException::Message(iException::Programmer, msg, _FILEINFO_); 
        }
      } 
    }

    // If we still don't have a reference measure, if it's measured, make it
    // the reference measure.
    if (referenceMeasure == NULL && cmeasure->IsMeasured()) {
      referenceMeasure = cmeasure;
    }

    cmeasure->parentPoint = this;
    QString newSerial = cmeasure->GetCubeSerialNumber();
    p_measures->insert(newSerial, cmeasure);
    cubeSerials->append(newSerial);
  }


  void ControlPoint::validateMeasure(iString serialNumber, bool checkRef) const {
    if (!p_measures->contains(serialNumber)) {
      iString msg = "No measure with serial number [" + serialNumber +
                    "] is owned by this point";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    if (checkRef && referenceMeasure->GetCubeSerialNumber() == serialNumber) {
      iString msg = "Point Id [" + GetId() + "] can not do requested operation"
                    " on reference measure [" + serialNumber + "]";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Remove a measurement from the control point, deleting reference measure
   * is allowed.
   *
   * @param serialNumber The serial number of the measure to delete
   */
  void ControlPoint::Delete(iString serialNumber) {
    validateMeasure(serialNumber, true);

    PointModified();
    p_measures->remove(serialNumber);
    cubeSerials->removeAt(cubeSerials->indexOf(serialNumber));
  }


  /**
   * Remove a measurement from the control point, deleting reference measure
   * is allowed.
   *
   * @param index The index of the control measure to delete
   */
  void ControlPoint::Delete(int index) {
    if (index < 0 || index >= cubeSerials->size()) {
      iString msg = "index [" + iString(index) + "] out of bounds";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }
  
    Delete(cubeSerials->at(index));
  }


  /**
   * Reset all the Apriori info to defaults
   *
   * @author Sharmila Prasad (10/22/2010)
   */
  ControlPoint::Status ControlPoint::ResetApriori() {
    if (IsEditLocked())
      return PointLocked;

    p_aprioriSurfacePointSource = SurfacePointSource::None;
    p_aprioriSurfacePointSourceFile    = "";
    p_aprioriRadiusSource     = RadiusSource::None;
    p_aprioriRadiusSourceFile = "";

    p_aprioriSurfacePoint = SurfacePoint();

    return Success;
  }


  /**
   * Get a control measure based on its cube's serial number.
   *
   * @param serialNumber serial number of measure to get
   * @returns control measure with matching serial number
   */
  ControlMeasure *ControlPoint::GetMeasure(iString serialNumber) {
    validateMeasure(serialNumber);
    return (*p_measures)[serialNumber];
  }


  /**
   * Get a control measure based on its cube's serial number.
   *
   * @param serialNumber serial number of measure to get
   * @returns const control measure with matching serial number
   */
  const ControlMeasure *ControlPoint::GetMeasure(iString serialNumber) const {
    validateMeasure(serialNumber);
    return p_measures->value(serialNumber);
  }


  const ControlMeasure * ControlPoint::GetMeasure(int index) const
  {
    if (index < 0 || index >= cubeSerials->size())
    {
      iString msg = "Index [" + iString(index) + "] out of range";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    return GetMeasure(cubeSerials->at(index));
  }


  ControlMeasure * ControlPoint::GetMeasure(int index)
  {
    if (index < 0 || index >= cubeSerials->size())
    {
      iString msg = "Index [" + iString(index) + "] out of range";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    return GetMeasure(cubeSerials->at(index));
  }


  /**
   * Get the reference control measure.
   *
   * @returns const reference measure for this point
   */
  const ControlMeasure *ControlPoint::GetReferenceMeasure() const {
    if (referenceMeasure == NULL) {
      iString msg = "Control point [" + GetId() + "] has no reference measure!";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    const ControlMeasure *ref = referenceMeasure;
    return ref;
  }


  ControlMeasure *ControlPoint::GetReferenceMeasure() {
    if (referenceMeasure == NULL) {
      iString msg = "Control point [" + GetId() + "] has no reference measure!";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    return referenceMeasure;
  }


  /**
   * Set the point's chooser name. This will be lost if any attributes relating
   *   to this point is later changed and the current user will be set. This is
   *   one of the 'last modified attributes' referred to in other comments.
   *
   * @param name The username of the person who last modified this control point
   */
  ControlPoint::Status ControlPoint::SetChooserName(iString name) {
    if (p_editLock)
      return PointLocked;
    p_chooserName = name;
    return Success;
  }


  /**
   * Set the point's last modified time. This will be lost if any attributes
   *   relating to this point are later changed and the current time will be
   *   set. This is one of the 'last modified attributes' referred to in other
   *   comments.
   *
   * @param dateTime The date and time this control point was last modified
   */
  ControlPoint::Status ControlPoint::SetDateTime(iString dateTime) {
    if (p_editLock)
      return PointLocked;
    p_dateTime = dateTime;
    return Success;
  }


  /**
   * Set the EditLock state. If edit lock is on, then most attributes relating
   *   to this point are not modifiable. Edit lock is like "Don't modify my
   *   attributes, but you can still modify my measures' attributes". The
   *   reference measure is implicitely edit locked if the point is edit locked.
   *
   * @param lock True to enable edit lock, false to disable it and allow the
   *   point to be modified.
   */
  ControlPoint::Status ControlPoint::SetEditLock(bool lock) {
    p_editLock = lock;
    return Success;
  }


  /**
   * Set the jigsawRejected state. If IsRejected is true, then this point should be
   *   ignored until the next iteration in the bundle adjustement.  BundleAdjust
   *   decides when to reject or accept a point. The initial IsRejected state of
   *   a measure is false.
   *
   * @param reject True to reject a measure, false to include it in the adjustment
   */
  ControlPoint::Status ControlPoint::SetRejected(bool reject) {
    p_jigsawRejected = reject;
    return Success;
  }


  /**
   * Sets the Id of the control point
   *
   * @param id Control Point Id
   *
   * @return  (int) status Success or PointLocked
   */
  ControlPoint::Status ControlPoint::SetId(iString id) {
    if (p_editLock)
      return PointLocked;
    p_id = id;
    return Success;
  }


  /**
   * Set whether to ignore or use control point
   *
   * @param ignore True to ignore this Control Point, False to un-ignore
   */
  ControlPoint::Status ControlPoint::SetIgnored(bool ignore) {
    if (p_editLock)
      return PointLocked;
    PointModified();
    p_ignore = ignore;
    return Success;
  }


  /**
   * Set or update the surface point relating to this control point. This is the
   *   point on the surface of the planet that the measures are tied to. This
   *   updates the last modified attributes of this point.
   *
   * @param surfacePoint The point on the target's surface the measures are
   *   tied to
   */
  ControlPoint::Status ControlPoint::SetSurfacePoint(
    SurfacePoint surfacePoint) {
    if (p_editLock)
      return PointLocked;
    PointModified();
    p_surfacePoint = surfacePoint;
    return Success;
  }


  /**
   * Updates the control point's type. This updates the last modified attributes
   *   of this point.
   *
   * @see PointType
   *
   * @param type The new type this control point should be
   */
  ControlPoint::Status ControlPoint::SetType(PointType type) {
    if (type != Ground && type != Tie) {
      iString msg = "Invalid Point Enumeration, [" + iString(type) + "], for "
                    "Control Point [" + GetId() + "]";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    if (p_editLock)
      return PointLocked;
    PointModified();
    p_type = type;
    return Success;
  }


  /**
   * This updates the source of the radius of the apriori surface point.
   *
   * @see RadiusSource::Source
   *
   * @param source Where the radius came from
   */
  ControlPoint::Status ControlPoint::SetAprioriRadiusSource(
    RadiusSource::Source source) {
    if (p_editLock)
      return PointLocked;
    PointModified();
    p_aprioriRadiusSource = source;
    return Success;
  }


  /**
   * This updates the filename of the DEM that the apriori radius came from. It
   *   doesn't really make sense to call this unless the RadiusSource is DEM.
   *
   * @see RadiusSource::Source
   *
   * @param source Where the radius came from
   */
  ControlPoint::Status ControlPoint::SetAprioriRadiusSourceFile(
    iString sourceFile) {
    if (p_editLock)
      return PointLocked;
    PointModified();
    p_aprioriRadiusSourceFile = sourceFile;
    return Success;
  }


  /**
   * This updates the apriori surface point.
   *
   * @see SetAprioriRadiusSource
   * @see SetAprioriRadiusSourceFile
   * @see SetAprioriPointSource
   * @see SetAprioriPointSourceFile
   * @see p_aprioriSurfacePoint
   *
   * @param aprioriSurfacePoint The apriori surface point to remember
   */
  ControlPoint::Status ControlPoint::SetAprioriSurfacePoint(
    SurfacePoint aprioriSurfacePoint) {
    if (p_editLock)
      return PointLocked;
    PointModified();
    p_aprioriSurfacePoint = aprioriSurfacePoint;
    return Success;
  }


  /**
   * This updates the source of the surface point
   *
   * @see SurfacePointSource::Source
   *
   * @param source Where the surface point came from
   */
  ControlPoint::Status ControlPoint::SetAprioriSurfacePointSource(
    SurfacePointSource::Source source) {
    if (p_editLock)
      return PointLocked;
    PointModified();
    p_aprioriSurfacePointSource = source;
    return Success;
  }


  /**
   * This updates the filename of where the apriori surface point came from.
   *
   * @see RadiusSource::Source
   *
   * @param sourceFile Where the surface point came from
   */
  ControlPoint::Status ControlPoint::SetAprioriSurfacePointSourceFile(
    iString sourceFile) {
    if (p_editLock)
      return PointLocked;
    PointModified();
    p_aprioriSurfacePointSourceFile = sourceFile;
    return Success;
  }


  /**
   * This method computes the apriori lat/lon for a point.  It computes this
   * by determining the average lat/lon of all the measures.  Note that this
   * does not change held, ignored, or ground points.  Also, it does not
   * use unmeasured or ignored measures when computing the lat/lon.
   * @internal
   *   @history 2008-06-18  Tracie Sucharski/Jeannie Walldren,
   *                               Changed error messages for
   *                               Held/Ground points.
   *   @history 2009-10-13 Jeannie Walldren - Added detail to
   *                               error message.
   *   @history 2010-11-29 Tracie Sucharski - Remove call to ControlMeasure::
   *                               SetMeasuredEphemerisTime, the values were
   *                               never used. so these methods were removed
   *                               from ControlMeasure and the call was removed
   *                               here.
   *   @history 2010-12-02 Debbie A. Cook - Added units to SetRectangular
   *                               calls since default is meters and units
   *                               are km.
   *
   * @return Status Success or PointLocked
   */
  ControlPoint::Status ControlPoint::ComputeApriori() {

    if (p_editLock)
      return PointLocked;
    // Should we ignore the point altogether?
    if (IsIgnored())
      return Failure;

    PointModified();

    // Don't goof with ground points.  The lat/lon is what it is ... if
    // it exists!
    if (GetType() == Ground) {
      if (!p_surfacePoint.Valid()) {
        iString msg = "ControlPoint [" + GetId() + "] is a ground point ";
        msg += "and requires x/y/z";
        throw iException::Message(iException::User, msg, _FILEINFO_);
      }
      // Don't return until after the FocalPlaneMeasures have been set
      //      return;
    }

    double xB = 0.0;
    double yB = 0.0;
    double zB = 0.0;
    int goodMeasures = 0;

    // Loop for each measure and compute the sum of the lat/lon/radii
    for (int j = 0; j < cubeSerials->size(); j++) {
      ControlMeasure *m = GetMeasure(j);
      if (!m->IsMeasured()) {
        // TODO: How do we deal with unmeasured measures
      }
      else if (m->IsIgnored()) {
        // TODO: How do we deal with ignored measures
      }
      else {
        Camera *cam = m->Camera();
        if (cam == NULL) {
          iString msg = "The Camera must be set prior to calculating apriori";
          throw iException::Message(iException::Programmer, msg, _FILEINFO_);
        }
        if (cam->SetImage(m->GetSample(), m->GetLine())) {
          goodMeasures++;
          double pB[3];
          cam->Coordinate(pB);
          xB += pB[0];
          yB += pB[1];
          zB += pB[2];

          double x = cam->DistortionMap()->UndistortedFocalPlaneX();
          double y = cam->DistortionMap()->UndistortedFocalPlaneY();
          m->SetFocalPlaneMeasured(x, y);
        }
        else {
          // JAA: Don't stop if we know the lat/lon.  The SetImage may fail
          // but the FocalPlane measures have been set
          if (GetType() == Ground)
            continue;

          // TODO: What do we do
          iString msg = "Cannot compute lat/lon/radius (x/y/z) for "
                        "ControlPoint [" + GetId() + "], measure [" +
                        m->GetCubeSerialNumber() + "]";
          throw iException::Message(iException::User, msg, _FILEINFO_);

          // m->SetFocalPlaneMeasured(?,?);
        }
      }
    }

    // Don't update the x/y/z for ground points
    if (GetType() == Ground)
      return Success;

    // Did we have any measures?
    if (goodMeasures == 0) {
      iString msg = "ControlPoint [" + GetId() + "] has no measures which "
                    "project to lat/lon/radius (x/y/z)";
      throw iException::Message(iException::User, msg, _FILEINFO_);
    }

    // Compute the averages
    p_aprioriSurfacePoint.SetRectangular(
      Displacement((xB / goodMeasures), Displacement::Kilometers),
      Displacement((yB / goodMeasures), Displacement::Kilometers),
      Displacement((zB / goodMeasures), Displacement::Kilometers));

    SetAprioriSurfacePointSource(SurfacePointSource::AverageOfMeasures);
    SetAprioriRadiusSource(RadiusSource::AverageOfMeasures);

    return Success;
  }


  /**
   * This method computes the residuals for a point.
   *
   * @history 2008-07-17 Tracie Sucharski,  Added ptid and measure serial
   *                            number to the unable to map to surface error.
   * @history 2009-12-06 Tracie Sucharski, Renamed from ComputeErrors
   * @history 2010-08-05 Tracie Sucharski, Changed lat/lon/radius to x/y/z
   * @history 2010-12-10 Debbie A. Cook,  Revised error calculation for radar
   *                            because it was always reporting line errors=0.
   */
  ControlPoint::Status ControlPoint::ComputeResiduals() {
    if (p_editLock)
      return PointLocked;
    if (IsIgnored())
      return Failure;

    PointModified();

    // Loop for each measure to compute the error
    QList<QString> keys = p_measures->keys();
    for (int j = 0; j < keys.size(); j++) {
      ControlMeasure *m = (*p_measures)[keys[j]];
      if (m->IsIgnored())
        continue;
      if (!m->IsMeasured())
        continue;

      // TODO:  Should we use crater diameter?
      Camera *cam = m->Camera();
      cam->SetImage(m->GetSample(), m->GetLine());

      double cuSamp;
      double cuLine;
      CameraFocalPlaneMap *fpmap = m->Camera()->FocalPlaneMap();

      if (cam->GetCameraType()  !=  Isis::Camera::Radar) {

        // Map the lat/lon/radius of the control point through the Spice of the
        // measurement sample/line to get the computed sample/line.  This must be
        // done manually because the camera will compute a new time for line scanners,
        // instead of using the measured time.
        double cudx, cudy;
        cam->GroundMap()->GetXY(GetSurfacePoint(), &cudx, &cudy);
        m->SetFocalPlaneComputed(cudx, cudy);

        // Now things get tricky.  We want to produce errors in pixels not mm
        // but some of the camera maps could fail.  One that won't is the
        // FocalPlaneMap which takes x/y to detector s/l.  We will bypass the
        // distortion map and have residuals in undistorted pixels.
        if (!fpmap->SetFocalPlane(m->GetFocalPlaneComputedX(), m->GetFocalPlaneComputedY())) {
          iString msg = "Sanity check #1 for ControlPoint [" + GetId() +
                        "], ControlMeasure [" + m->GetCubeSerialNumber() + "]";
          throw iException::Message(iException::Programmer, msg, _FILEINFO_);
          // This error shouldn't happen but check anyways
        }

        cuSamp = fpmap->DetectorSample();
        cuLine = fpmap->DetectorLine();
      }

      else {
        // For radar we can't map through the current Spice, because y in the
        // focal plane is doppler shift.  Line is calculated from time.  If
        // we hold time and the Spice, we'll get the same sample/line as
        // measured
        double lat = GetSurfacePoint().GetLatitude().GetDegrees();
        double lon = GetSurfacePoint().GetLatitude().GetDegrees();
        double rad = GetSurfacePoint().GetLocalRadius().GetMeters();
        if (!cam->SetUniversalGround(lat, lon, rad)) {
          std::string msg = "ControlPoint [" +
                            GetId() + "], ControlMeasure [" +
                            m->GetCubeSerialNumber() + "]" +
                            " does not map into image";
          throw iException::Message(iException::User, msg, _FILEINFO_);
        }

        cuSamp = cam->Sample();
        cuLine = cam->Line();
      }

      double muSamp;
      double muLine;

      if (cam->GetCameraType()  !=  Isis::Camera::Radar) {
        // Again we will bypass the distortion map and have residuals in undistorted pixels.
        if (!fpmap->SetFocalPlane(m->GetFocalPlaneMeasuredX(), m->GetFocalPlaneMeasuredY())) {
          iString msg = "Sanity check #2 for ControlPoint [" + GetId() +
                        "], ControlMeasure [" + m->GetCubeSerialNumber() + "]";
          throw iException::Message(iException::Programmer, msg, _FILEINFO_);
          // This error shouldn't happen but check anyways
        }
        muSamp = fpmap->DetectorSample();
        muLine = fpmap->DetectorLine();
      }
      else {
        muSamp = m->GetSample();
        muLine = m->GetLine();
      }

      // The units are in detector sample/lines.  We will apply the instrument
      // summing mode to get close to real pixels.  Note however we are in
      // undistorted pixels
      double sampResidual = muSamp - cuSamp;
      double lineResidual = muLine - cuLine;
      m->SetResidual(sampResidual, lineResidual);
    }

    return Success;
  }


  iString ControlPoint::GetChooserName() const {
    if (p_chooserName != "") {
      return p_chooserName;
    }
    else {
      return Filename(Application::Name()).Name();
    }
  }


  iString ControlPoint::GetDateTime() const {
    if (p_dateTime != "") {
      return p_dateTime;
    }
    else {
      return Application::DateTime();
    }
  }


  bool ControlPoint::IsEditLocked() const {
    return p_editLock;
  }


  bool ControlPoint::IsRejected() const {
    return p_jigsawRejected;
  }


  /**
   * Return the Id of the control point
   *
   * @return Control Point Id
   */
  iString ControlPoint::GetId() const {
    return p_id;
  }


  bool ControlPoint::IsIgnored() const {
    return p_ignore;
  }


  bool ControlPoint::IsValid() const {
    return !p_invalid;
  }


  bool ControlPoint::IsInvalid() const {
    return p_invalid;
  }


  /**
   *  Obtain a string representation of a given PointType
   *
   *  @param type PointType to convert to a string
   *
   *  @returns A string representation of type
   *
   *  @throws iException::Programmer When unable to translate type
   * @internal
   *   @history 2009-10-13 Jeannie Walldren - Added detail to
   *            error message.
   *   @history 2010-06-04 Eric Hyer - removed parameter
   *   @history 2010-10-27 Mackenzie Boyd - changed to static
   */
  iString ControlPoint::PointTypeToString(PointType type) {
    iString str;

    switch (type) {
      case Ground:
        str = "Ground";
        break;
      case Tie:
        str = "Tie";
        break;
    }

    if (str == "") {
      iString msg = "Point type [" + iString(type) + "] cannot be converted "
                    "to a string";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    return str;
  }

  /**
   * Obtain a string representation of the PointType
   *
   * @return A string representation of the PointType
   */
  iString ControlPoint::GetPointTypeString() const {
    return PointTypeToString(p_type);
  }

  /**
   *  Obtain a string representation of a given RadiusSource
   *
   *  @param source RadiusSource to convert to string
   *
   *  @returns A string representation of RadiusSource
   *
   *  @throws iException::Programmer When unable to translate source
   */
  iString ControlPoint::RadiusSourceToString(RadiusSource::Source source) {
    iString str;

    switch (source) {
      case RadiusSource::None:
        str = "None";
        break;
      case RadiusSource::User:
        str = "User";
        break;
      case RadiusSource::AverageOfMeasures:
        str = "AverageOfMeasures";
        break;
      case RadiusSource::Ellipsoid:
        str = "Ellipsoid";
        break;
      case RadiusSource::DEM:
        str = "DEM";
        break;
      case RadiusSource::BundleSolution:
        str = "BundleSolution";
        break;
    }

    if (str == "") {
      iString msg = "Radius source [" + iString(source) + "] cannot be converted "
                    "to a string";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    return str;
  }

  /**
   * Obtain a string representation of the RadiusSource
   *
   * @return A string representation of the RadiusSource
   */
  iString ControlPoint::GetRadiusSourceString() const {
    return RadiusSourceToString(p_aprioriRadiusSource);
  }

  /**
   *  Obtain a string representation of a given SurfacePointSource
   *
   *  @param souce SurfacePointSource to get a string representation of
   *
   *  @returns A string representation of SurfacePointSource
   *
   *  @throws iException::Programmer When unable to translate source
   */
  iString ControlPoint::SurfacePointSourceToString(SurfacePointSource::Source source) {
    iString str;

    switch (source) {
      case SurfacePointSource::None:
        str = "None";
        break;
      case SurfacePointSource::User:
        str = "User";
        break;
      case SurfacePointSource::AverageOfMeasures:
        str = "AverageOfMeasures";
        break;
      case SurfacePointSource::Reference:
        str = "Reference";
        break;
      case SurfacePointSource::Basemap:
        str = "Basemap";
        break;
      case SurfacePointSource::BundleSolution:
        str = "BundleSolution";
        break;
    }

    if (str == "") {
      iString msg = "Surface point source [" + iString(source) + "] cannot be converted "
                    "to a string";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    return str;
  }

  /**
   * Obtain a string representation of the SurfacePointSource
   *
   * @return A string representation of the SurfacePointSource
   */
  iString ControlPoint::GetSurfacePointSourceString() const {
    return SurfacePointSourceToString(p_aprioriSurfacePointSource);
  }

  SurfacePoint ControlPoint::GetSurfacePoint() const {
    return p_surfacePoint;
  }


  ControlPoint::PointType ControlPoint::GetType() const {
    return p_type;
  }



  bool ControlPoint::IsGround() const {
    return (p_type == Ground);
  }


  SurfacePoint ControlPoint::GetAprioriSurfacePoint() const {
    return p_aprioriSurfacePoint;
  }


  ControlPoint::RadiusSource::Source ControlPoint::GetAprioriRadiusSource()
  const {
    return p_aprioriRadiusSource;
  }


  iString ControlPoint::GetAprioriRadiusSourceFile() const {
    return p_aprioriRadiusSourceFile;
  }

  ControlPoint::SurfacePointSource::Source
  ControlPoint::GetAprioriSurfacePointSource() const {
    return p_aprioriSurfacePointSource;
  }


  iString ControlPoint::GetAprioriSurfacePointSourceFile() const {
    return p_aprioriSurfacePointSourceFile;
  }


  int ControlPoint::GetNumMeasures() const {
    return p_measures->size();
  }


  /**
   *
   * @return Number of valid control measures
   */
  int ControlPoint::GetNumValidMeasures() const {
    int size = 0;
    QList<QString> keys = p_measures->keys();
    for (int cm = 0; cm < keys.size(); cm++) {
      if (!(*p_measures)[keys[cm]]->IsIgnored())
        size++;
    }
    return size;
  }


  /**
   * Returns the number of locked control measures
   *
   * @return Number of locked control measures
   */
  int ControlPoint::GetNumLockedMeasures() const {
    int size = 0;
    QList<QString> keys = p_measures->keys();
    for (int cm = 0; cm < keys.size(); cm++) {
      if ((*p_measures)[keys[cm]]->IsEditLocked())
        size++;
    }
    return size;
  }


  /**
   *  Return true if given serial number exists in point
   *
   *  @param serialNumber  The serial number
   *  @return True if point contains serial number, false if not
   */
  bool ControlPoint::HasSerialNumber(iString serialNumber) const {
    QList<QString> keys = p_measures->keys();
    for (int m = 0; m < keys.size(); m++) {
      if ((*p_measures)[keys[m]]->GetCubeSerialNumber() == serialNumber) {
        return true;
      }
    }

    return false;
  }


  /**
   * Return true if there is a Reference measure, otherwise return false
   *
   * @todo  ??? Check for more than one reference measure ???
   *          Should print error, this check should also go in
   *          ReferenceIndex.
   */
  bool ControlPoint::HasReference() const {
    // If it is set, then there is a reference measure
    return referenceMeasure != NULL;
  }


  /**
   * Returns a Reference Index of the Control Point. If none then returns the
   * first measure as Reference. If there are no measures then returns -1;
   *
   * @author Sharmila Prasad (5/11/2010)
   *
   * @history 2010-07-21 Tracie Sucharski - Replaced IsReferece call with
   *                        comparison of MeasureType.
   * @return int
   */
  QString ControlPoint::GetReferenceKeyNoException() const {
    if (p_measures->size() == 0) {
      return NULL;
    }

    return referenceMeasure->GetCubeSerialNumber();
  }


  /**
   * Return the index of the reference measurement
   * if none is specified, return the first measured CM
   *
   * @return The PvlObject created
   */
  QString ControlPoint::GetReferenceKey() const {
    if (p_measures->size() == 0) {
      iString msg = "There are no ControlMeasures in the ControlPoint [" +
                    GetId() + "]";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }
    if (referenceMeasure == NULL) {
      iString msg = "There is no reference measure set in the ControlPoint [" +
                    GetId() + "]";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    return referenceMeasure->GetCubeSerialNumber();
  }


  /**
     * Returns a Reference Index of the Control Point. If none then returns the
     * first measure as Reference. If there are no measures then returns -1;
     *
     * @author Sharmila Prasad (5/11/2010)
     *
     * @history 2010-07-21 Tracie Sucharski - Replaced IsReferece call with
     *                        comparison of MeasureType.
     * @return int
     */
  int ControlPoint::GetReferenceIndexNoException() const {
    if (p_measures->size() == 0) {
      return -1;
    }

    if (referenceMeasure != NULL) {
      int index = cubeSerials->indexOf(referenceMeasure->GetCubeSerialNumber());
      // If the serial number isn't contained, there is something very odd going on
      if (index == -1) return 0;
      else return index; 
    }

    return 0;
  }


  /**
   * Return the index of the reference measurement
   * if none is specified, return the first measured CM
   *
   * @return The PvlObject created
   */
  int ControlPoint::GetReferenceIndex() const {
    if (p_measures->size() == 0) {
      iString msg = "There are no ControlMeasures in the ControlPoint [" +
                    GetId() + "]";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    if (referenceMeasure != NULL) {
      int index = cubeSerials->indexOf(referenceMeasure->GetCubeSerialNumber());
      // If the serial number isn't contained, there is something very odd going on
      if (index != -1) {
        return index; 
      }
      else { 
        iString msg = "Reference measure serial number [" + 
                      referenceMeasure->GetCubeSerialNumber()
                      + "] is not contained in the point";
        throw iException::Message(iException::Programmer, msg, _FILEINFO_);
      }
    }

    iString msg = "There are no Measured ControlMeasures in the ControlPoint ["
                  + GetId() + "]";
    throw iException::Message(iException::Programmer, msg, _FILEINFO_);
  }



  /**
   * Return the status of Reference Measure's Edit Lock
   *
   * @author Sharmila Prasad (10/6/2010)
   *
   * @return bool - True/False for EditLock
   */
  bool ControlPoint::IsReferenceLocked() const {
    if (p_measures->size() == 0)
      return false;
    if (referenceMeasure == NULL) {
      iString msg = "There is no reference measure set in the ControlPoint ["
                    + GetId() + "]";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    return referenceMeasure->IsEditLocked();
  }


  /**
   *  Return the average residual of all measurements
   *
   *  @history 2010-12-06  Tracie Sucharski, Renamed from AverageError
   */
  double ControlPoint::GetAverageResidual() const {
    double errorSum = 0.0;
    int errorCount = 0;
    QList<QString> keys = p_measures->keys();
    for (int i = 0; i < keys.size(); i++) {
      const ControlMeasure *measure = (*p_measures)[keys[i]];
      if (measure->IsIgnored())
        continue;
      if (!measure->IsMeasured())
        continue;

      errorSum += measure->GetResidualMagnitude();
      errorCount ++;
    }

    if (errorCount == 0)
      return 0.0;
    return errorSum / (double) errorCount;
  }


  /**
   * Return the minimum residual magnitude of the measures in the point.
   * Ignored and Unmeasured measures will not be included
   *
   * @author Sharmila Prasad (8/26/2010)
   *
   * @return double
   */
  double ControlPoint::GetMinimumResidual() const {
    double dMinError = VALID_MAX4;
    if (IsIgnored())
      return dMinError;

    QList<QString> keys = p_measures->keys();
    for (int j = 0; j < keys.size(); j++) {
      const ControlMeasure *measure = (*p_measures)[keys[j]];
      if (measure->IsIgnored())
        continue;
      if (measure->GetType() == ControlMeasure::Candidate)
        continue;

      double dErr = measure->GetResidualMagnitude();
      if (dErr < dMinError) {
        dMinError = dErr;
      }
    }

    return dMinError;
  }


  /**
   * Get the minimum sample residual for the control point
   *
   * @author Sharmila Prasad (8/26/2010)
   *
   * @return double
   */
  double ControlPoint::GetMinimumSampleResidual() const {
    double dMinError = VALID_MAX4;
    if (IsIgnored())
      return dMinError;

    QList<QString> keys = p_measures->keys();
    for (int j = 0; j < keys.size(); j++) {
      const ControlMeasure *measure = (*p_measures)[keys[j]];
      if (measure->IsIgnored())
        continue;
      if (measure->GetType() == ControlMeasure::Candidate)
        continue;

      double dErr = measure->GetSampleResidual();
      if (dErr < dMinError) {
        dMinError = dErr;
      }
    }

    return dMinError;
  }


  /**
   * Get the minimum line residual for the control point
   *
   * @author Sharmila Prasad (8/26/2010)
   *
   * @return double
   */
  double ControlPoint::GetMinimumLineResidual() const {
    double dMinError = VALID_MAX4;
    if (IsIgnored())
      return dMinError;

    QList<QString> keys = p_measures->keys();
    for (int j = 0; j < keys.size(); j++) {
      const ControlMeasure *measure = (*p_measures)[keys[j]];
      if (measure->IsIgnored())
        continue;
      if (measure->GetType() == ControlMeasure::Candidate)
        continue;

      double dErr = measure->GetLineResidual();
      if (dErr < dMinError) {
        dMinError = dErr;
      }
    }

    return dMinError;
  }


  /**
   * Return the maximum residual magnitude of the measures in the point.
   * Ignored and estimateded measures will not be included.
   *
   * @history 2010-12-06  Tracie Sucharski, Renamed from MaximumError
   */
  double ControlPoint::GetMaximumResidual() const {
    double maxResidual = 0.0;
    if (IsIgnored())
      return maxResidual;

    QList<QString> keys = p_measures->keys();
    for (int j = 0; j < keys.size(); j++) {
      const ControlMeasure *measure = (*p_measures)[keys[j]];
      if (measure->IsIgnored())
        continue;
      if (!measure->IsMeasured())
        continue;
      if (measure->GetResidualMagnitude() > maxResidual) {
        maxResidual = measure->GetResidualMagnitude();
      }
    }
    return maxResidual;
  }


  /**
   * Get the maximum sample residual for the control point
   *
   * @author Sharmila Prasad (8/26/2010)
   *
   * @return double
   */
  double ControlPoint::GetMaximumSampleResidual() const {
    double dMaxError = 0.0;
    if (IsIgnored())
      return dMaxError;

    QList<QString> keys = p_measures->keys();
    for (int j = 0; j < keys.size(); j++) {
      const ControlMeasure *measure = (*p_measures)[keys[j]];
      if (measure->IsIgnored())
        continue;
      if (measure->GetType() == ControlMeasure::Candidate)
        continue;

      double dErr = measure->GetSampleResidual();
      if (dErr > dMaxError) {
        dMaxError = dErr;
      }
    }

    return dMaxError;
  }


  /**
   * Get the maximum line residual for the control point
   *
   * @author Sharmila Prasad (8/26/2010)
   *
   * @return double
   */
  double ControlPoint::GetMaximumLineResidual() const {
    double dMaxError = 0.0;
    if (IsIgnored())
      return dMaxError;

    QList<QString> keys = p_measures->keys();
    for (int j = 0; j < keys.size(); j++) {
      const ControlMeasure *measure = (*p_measures)[keys[j]];
      if (measure->IsIgnored())
        continue;
      if (measure->GetType() == ControlMeasure::Candidate)
        continue;

      double dErr = measure->GetLineResidual();
      if (dErr > dMaxError) {
        dMaxError = dErr;
      }
    }

    return dMaxError;
  }


  QList< QString > ControlPoint::GetCubeSerialNumbers() const {
    return *cubeSerials;
  }


  /**
   * Creates a PvlObject from the ControlPoint
   *
   * @return The PvlObject created
   *
   */
  PvlObject ControlPoint::ToPvlObject() const {
    PvlObject p("ControlPoint");

    p += PvlKeyword("PointType", GetPointTypeString());

    p += PvlKeyword("PointId", p_id);
    p += PvlKeyword("ChooserName", GetChooserName());
    p += PvlKeyword("DateTime", GetDateTime());

    if (p_editLock == true) {
      p += PvlKeyword("EditLock", "True");
    }

    if (p_ignore == true) {
      p += PvlKeyword("Ignore", "True");
    }

    switch (p_aprioriSurfacePointSource) {
      case SurfacePointSource::None:
        break;
      case SurfacePointSource::User:
        p += PvlKeyword("AprioriXYZSource", "User");
        break;
      case SurfacePointSource::AverageOfMeasures:
        p += PvlKeyword("AprioriXYZSource", "AverageOfMeasures");
        break;
      case SurfacePointSource::Reference:
        p += PvlKeyword("AprioriXYZSource", "Reference");
        break;
      case SurfacePointSource::Basemap:
        p += PvlKeyword("AprioriXYZSource", "Basemap");
        break;
      case SurfacePointSource::BundleSolution:
        p += PvlKeyword("AprioriXYZSource", "BundleSolution");
        break;
      default:
        break;
    }

    if (!p_aprioriSurfacePointSourceFile.empty()) {
      p += PvlKeyword("AprioriXYZSourceFile", p_aprioriSurfacePointSourceFile);
    }

    if (p_aprioriRadiusSource != RadiusSource::None) {
      p += PvlKeyword("AprioriRadiusSource", GetRadiusSourceString());
    }

    if (!p_aprioriRadiusSourceFile.empty()) {
      p += PvlKeyword("AprioriRadiusSourceFile", p_aprioriRadiusSourceFile);
    }

    if (p_aprioriSurfacePoint.Valid()) {
      const SurfacePoint &apriori = p_aprioriSurfacePoint;

      p += PvlKeyword("AprioriX", apriori.GetX().GetMeters(), "meters");
      p += PvlKeyword("AprioriY", apriori.GetY().GetMeters(), "meters");
      p += PvlKeyword("AprioriZ", apriori.GetZ().GetMeters(), "meters");

      symmetric_matrix<double, upper> covar = apriori.GetRectangularMatrix();
      if (covar(0, 0) != 0. || covar(1, 1) != 0. || covar(2, 2) != 0.) {
        PvlKeyword matrix("AprioriCovarianceMatrix");
        matrix += covar(0, 0);
        matrix += covar(0, 1);
        matrix += covar(0, 2);
        matrix += covar(1, 1);
        matrix += covar(1, 2);
        matrix += covar(2, 2);
        p += matrix;
      }
    }

    if (p_surfacePoint.Valid()) {
      const SurfacePoint &point = p_surfacePoint;

      p += PvlKeyword("X", point.GetX().GetMeters(), "meters");
      p += PvlKeyword("Y", point.GetY().GetMeters(), "meters");
      p += PvlKeyword("Z", point.GetZ().GetMeters(), "meters");

      symmetric_matrix<double, upper> covar = point.GetRectangularMatrix();
      if (covar(0, 0) != 0. || covar(1, 1) != 0. ||
          covar(2, 2) != 0.) {
        PvlKeyword matrix("ApostCovarianceMatrix");
        matrix += covar(0, 0);
        matrix += covar(0, 1);
        matrix += covar(0, 2);
        matrix += covar(1, 1);
        matrix += covar(1, 2);
        matrix += covar(2, 2);
        p += matrix;
      }
    }

    for (int i = 0; i < cubeSerials->size(); i++)
      p.AddGroup((*p_measures)[cubeSerials->at(i)]->CreatePvlGroup());

    return p;
  }

  /**
   *  Same as GetMeasure (provided for convenience)
   *
   *  @param serialNumber Cube serial number of desired control measure
   *
   *  @returns const version of the measure which has the provided serial number
   */
  const ControlMeasure *ControlPoint::operator[](iString serialNumber) const {
    return GetMeasure(serialNumber);
  }


  /**
   *  Same as GetMeasure (provided for convenience)
   *
   *  @param serialNumber Cube serial number of desired control measure
   *
   *  @returns The measure which has the provided serial number
   */
  ControlMeasure *ControlPoint::operator[](iString serialNumber) {
    return GetMeasure(serialNumber);
  }


  /**
   *  Same as GetMeasure (provided for convenience)
   *
   *  @param index If there are n measures, the measure returned will be the
   *               ith measure added to the point
   *
   *  @returns const version of the measure which has the provided serial number
   */
  const ControlMeasure *ControlPoint::operator[](int index) const {
    return GetMeasure(index);
  }


  /**
   *  Same as GetMeasure (provided for convenience)
   *
   *  @param index If there are n measures, the measure returned will be the
   *               ith measure added to the point
   *
   *  @returns The measure which has the provided serial number
   */
  ControlMeasure *ControlPoint::operator[](int index) {
    return GetMeasure(index);
  }


  /**
   * Compare two Control Points for inequality
   *
   * @author Sharmila Prasad (4/20/2010)
   *
   * @history 2010-06-23 Tracie Sucharski, Added new keywords
   *
   * @param pPoint
   *
   * @return bool
   */
  bool ControlPoint::operator!=(const ControlPoint &pPoint) const {
    return !(*this == pPoint);
  }


  /**
   * Compare two Control Points for equality
   *
   * @author Sharmila Prasad (4/20/2010)
   *
   * @history 2010-06-23 Tracie Sucharski, Added new keywords
   * @history 2010-08-06 Tracie Sucharski, Re-wrote again for new-new keywords.
   *
   * @param pPoint to be compared against
   *
   * @return bool
   */
  bool ControlPoint::operator==(const ControlPoint &pPoint) const {
    return pPoint.GetNumMeasures() == GetNumMeasures() &&
           pPoint.p_id == p_id &&
           pPoint.p_type == p_type &&
           pPoint.p_chooserName == p_chooserName &&
           pPoint.p_editLock == p_editLock &&
           pPoint.p_ignore == p_ignore &&
           pPoint.p_aprioriSurfacePointSource  == p_aprioriSurfacePointSource &&
           pPoint.p_aprioriSurfacePointSourceFile
           == p_aprioriSurfacePointSourceFile &&
           pPoint.p_aprioriRadiusSource  == p_aprioriRadiusSource &&
           pPoint.p_aprioriRadiusSourceFile  == p_aprioriRadiusSourceFile &&
           pPoint.p_aprioriSurfacePoint == p_aprioriSurfacePoint &&
           pPoint.p_surfacePoint == p_surfacePoint &&
           pPoint.p_invalid == p_invalid &&
           pPoint.p_measures == p_measures;
  }


  /**
   *
   * @param pPoint
   *
   * @return ControlPoint&
   */
  const ControlPoint &ControlPoint::operator=(ControlPoint other) {
    if (this == &other)
      return *this;

    if (p_measures) {
      QList< QString > keys = p_measures->keys();
      for (int i = 0; i < keys.size(); i++) {
        delete(*p_measures)[keys[i]];
        (*p_measures)[keys[i]] = NULL;
      }

      delete p_measures;
      p_measures = NULL;
    }

    if (cubeSerials) {
      delete cubeSerials;
      cubeSerials = NULL;
    }

    p_measures = new QHash< QString, ControlMeasure * >;
    cubeSerials = new QStringList;

    QHashIterator< QString, ControlMeasure * > i(*other.p_measures);
    while (i.hasNext()) {
      i.next();
      ControlMeasure *newMeasure = new ControlMeasure(*i.value());
      newMeasure->parentPoint = this;
      p_measures->insert(i.key(), newMeasure);
      cubeSerials->append(i.key());
    }

    p_id             = other.p_id;
    p_chooserName    = other.p_chooserName;
    p_dateTime       = other.p_dateTime;
    p_type           = other.p_type;
    p_invalid        = other.p_invalid;
    p_editLock       = other.p_editLock;
    p_jigsawRejected = other.p_jigsawRejected;
    p_ignore         = other.p_ignore;
    p_aprioriSurfacePointSource      = other.p_aprioriSurfacePointSource;
    p_aprioriSurfacePointSourceFile  = other.p_aprioriSurfacePointSourceFile;
    p_aprioriRadiusSource            = other.p_aprioriRadiusSource;
    p_aprioriRadiusSourceFile        = other.p_aprioriRadiusSourceFile;
    p_aprioriSurfacePoint            = other.p_aprioriSurfacePoint;
    p_surfacePoint = other.p_surfacePoint;
    p_numberOfRejectedMeasures = other.p_numberOfRejectedMeasures;

    return *this;
  }


  void ControlPoint::Init(const PBControlNet_PBControlPoint &protoBufPt) {
    p_id = protoBufPt.id();
    p_dateTime = "";
    p_aprioriSurfacePointSource = SurfacePointSource::None;
    p_aprioriRadiusSource = RadiusSource::None;

    p_chooserName = protoBufPt.choosername();
    p_dateTime = protoBufPt.datetime();
    p_editLock = protoBufPt.editlock();

    parentNetwork = NULL;

    switch (protoBufPt.type()) {
      case PBControlNet_PBControlPoint_PointType_Tie:
        p_type = Tie;
        break;
      case PBControlNet_PBControlPoint_PointType_Ground:
        p_type = Ground;
        break;
    }

    p_ignore = protoBufPt.ignore();
    p_jigsawRejected = protoBufPt.jigsawrejected();

    // Read apriori keywords
    if (protoBufPt.has_apriorixyzsource()) {
      switch (protoBufPt.apriorixyzsource()) {
        case PBControlNet_PBControlPoint_AprioriSource_None:
          p_aprioriSurfacePointSource = SurfacePointSource::None;
          break;

        case PBControlNet_PBControlPoint_AprioriSource_User:
          p_aprioriSurfacePointSource = SurfacePointSource::User;
          break;

        case PBControlNet_PBControlPoint_AprioriSource_AverageOfMeasures:
          p_aprioriSurfacePointSource = SurfacePointSource::AverageOfMeasures;
          break;

        case PBControlNet_PBControlPoint_AprioriSource_Reference:
          p_aprioriSurfacePointSource = SurfacePointSource::Reference;
          break;

        case PBControlNet_PBControlPoint_AprioriSource_Basemap:
          p_aprioriSurfacePointSource = SurfacePointSource::Basemap;
          break;

        case PBControlNet_PBControlPoint_AprioriSource_BundleSolution:
          p_aprioriSurfacePointSource = SurfacePointSource::BundleSolution;
          break;

        case PBControlNet_PBControlPoint_AprioriSource_Ellipsoid:
        case PBControlNet_PBControlPoint_AprioriSource_DEM:
          break;
      }
    }

    if (protoBufPt.has_apriorixyzsourcefile()) {
      p_aprioriSurfacePointSourceFile = protoBufPt.apriorixyzsourcefile();
    }

    if (protoBufPt.has_aprioriradiussource()) {
      switch (protoBufPt.aprioriradiussource()) {
        case PBControlNet_PBControlPoint_AprioriSource_None:
          p_aprioriRadiusSource = RadiusSource::None;
          break;
        case PBControlNet_PBControlPoint_AprioriSource_User:
          p_aprioriRadiusSource = RadiusSource::User;
          break;
        case PBControlNet_PBControlPoint_AprioriSource_AverageOfMeasures:
          p_aprioriRadiusSource = RadiusSource::AverageOfMeasures;
          break;
        case PBControlNet_PBControlPoint_AprioriSource_Ellipsoid:
          p_aprioriRadiusSource = RadiusSource::Ellipsoid;
          break;
        case PBControlNet_PBControlPoint_AprioriSource_DEM:
          p_aprioriRadiusSource = RadiusSource::DEM;
          break;
        case PBControlNet_PBControlPoint_AprioriSource_BundleSolution:
          p_aprioriRadiusSource = RadiusSource::BundleSolution;
          break;

        case PBControlNet_PBControlPoint_AprioriSource_Reference:
        case PBControlNet_PBControlPoint_AprioriSource_Basemap:
          break;
      }
    }

    if (protoBufPt.has_aprioriradiussourcefile()) {
      p_aprioriRadiusSourceFile = protoBufPt.aprioriradiussourcefile();
    }

    if (protoBufPt.has_apriorix() && protoBufPt.has_aprioriy() &&
        protoBufPt.has_aprioriz()) {
      SurfacePoint apriori(Displacement(protoBufPt.apriorix()),
                           Displacement(protoBufPt.aprioriy()),
                           Displacement(protoBufPt.aprioriz()));

      if (protoBufPt.aprioricovar_size() > 0) {
        symmetric_matrix<double, upper> covar;
        covar.resize(3);
        covar.clear();
        covar(0, 0) = protoBufPt.aprioricovar(0);
        covar(0, 1) = protoBufPt.aprioricovar(1);
        covar(0, 2) = protoBufPt.aprioricovar(2);
        covar(1, 1) = protoBufPt.aprioricovar(3);
        covar(1, 2) = protoBufPt.aprioricovar(4);
        covar(2, 2) = protoBufPt.aprioricovar(5);
        apriori.SetRectangularMatrix(covar);
      }

      p_aprioriSurfacePoint = apriori;
    }

    if (protoBufPt.has_x() && protoBufPt.has_y() && protoBufPt.has_z()) {
      SurfacePoint apost(Displacement(protoBufPt.x()),
                         Displacement(protoBufPt.y()),
                         Displacement(protoBufPt.z()));

      if (protoBufPt.apostcovar_size() > 0) {
        symmetric_matrix<double, upper> covar;
        covar.resize(3);
        covar.clear();
        covar(0, 0) = protoBufPt.aprioricovar(0);
        covar(0, 1) = protoBufPt.aprioricovar(1);
        covar(0, 2) = protoBufPt.aprioricovar(2);
        covar(1, 1) = protoBufPt.aprioricovar(3);
        covar(1, 2) = protoBufPt.aprioricovar(4);
        covar(2, 2) = protoBufPt.aprioricovar(5);
        apost.SetRectangularMatrix(covar);
      }

      p_surfacePoint = apost;
    }
  }


  void ControlPoint::PointModified() {
    p_dateTime = "";
  }


  //! Initialize the number of rejected measures to 0
  void ControlPoint::ZeroNumberOfRejectedMeasures() {
    p_numberOfRejectedMeasures = 0;
  }


  /**
   * Set (update) the number of rejected measures for the control point
   *
   * @param numRejected    The number of rejected measures
   *
   */
  void ControlPoint::SetNumberOfRejectedMeasures(int numRejected) {
    p_numberOfRejectedMeasures = numRejected;
  }


  /**
   * Get the number of rejected measures on the control point
   *
   * @return The number of rejected measures on this control point
   *
   */
  int ControlPoint::GetNumberOfRejectedMeasures() const {
    return p_numberOfRejectedMeasures;
  }


  PBControlNet_PBControlPoint ControlPoint::ToProtocolBuffer() const {
    PBControlNet_PBControlPoint pbPoint;

    pbPoint.set_id(GetId());
    switch (GetType()) {
      case ControlPoint::Tie:
        pbPoint.set_type(PBControlNet_PBControlPoint::Tie);
        break;
      case ControlPoint::Ground:
        pbPoint.set_type(PBControlNet_PBControlPoint::Ground);
        break;
    }

    if (!GetChooserName().empty()) {
      pbPoint.set_choosername(GetChooserName());
    }
    if (!GetDateTime().empty()) {
      pbPoint.set_datetime(GetDateTime());
    }
    if (IsEditLocked())
      pbPoint.set_editlock(true);
    if (IsIgnored())
      pbPoint.set_ignore(true);
    if (IsRejected())
      pbPoint.set_jigsawrejected(true);

    switch (GetAprioriSurfacePointSource()) {
      case ControlPoint::SurfacePointSource::None:
        break;
      case ControlPoint::SurfacePointSource::User:
        pbPoint.set_apriorixyzsource(PBControlNet_PBControlPoint_AprioriSource_User);
        break;
      case ControlPoint::SurfacePointSource::AverageOfMeasures:
        pbPoint.set_apriorixyzsource(PBControlNet_PBControlPoint_AprioriSource_AverageOfMeasures);
        break;
      case ControlPoint::SurfacePointSource::Reference:
        pbPoint.set_apriorixyzsource(PBControlNet_PBControlPoint_AprioriSource_Reference);
        break;
      case ControlPoint::SurfacePointSource::Basemap:
        pbPoint.set_apriorixyzsource(PBControlNet_PBControlPoint_AprioriSource_Basemap);
        break;
      case ControlPoint::SurfacePointSource::BundleSolution:
        pbPoint.set_apriorixyzsource(PBControlNet_PBControlPoint_AprioriSource_BundleSolution);
        break;
      default:
        break;
    }
    if (!GetAprioriSurfacePointSourceFile().empty()) {
      pbPoint.set_apriorixyzsourcefile(GetAprioriSurfacePointSourceFile());
    }
    switch (GetAprioriRadiusSource()) {
      case ControlPoint::RadiusSource::None:
        break;
      case ControlPoint::RadiusSource::User:
        pbPoint.set_aprioriradiussource(PBControlNet_PBControlPoint_AprioriSource_User);
        break;
      case ControlPoint::RadiusSource::AverageOfMeasures:
        pbPoint.set_aprioriradiussource(PBControlNet_PBControlPoint_AprioriSource_AverageOfMeasures);
        break;
      case ControlPoint::RadiusSource::Ellipsoid:
        pbPoint.set_aprioriradiussource(PBControlNet_PBControlPoint_AprioriSource_Ellipsoid);
        break;
      case ControlPoint::RadiusSource::DEM:
        pbPoint.set_aprioriradiussource(PBControlNet_PBControlPoint_AprioriSource_DEM);
        break;
      case ControlPoint::RadiusSource::BundleSolution:
        pbPoint.set_aprioriradiussource(PBControlNet_PBControlPoint_AprioriSource_BundleSolution);
        break;
      default:
        break;
    }
    if (!GetAprioriRadiusSourceFile().empty()) {
      pbPoint.set_aprioriradiussourcefile(GetAprioriRadiusSourceFile());
    }

    if (GetAprioriSurfacePoint().Valid()) {
      SurfacePoint apriori = GetAprioriSurfacePoint();
      pbPoint.set_apriorix(apriori.GetX());
      pbPoint.set_aprioriy(apriori.GetY());
      pbPoint.set_aprioriz(apriori.GetZ());

      symmetric_matrix< double, upper > covar = apriori.GetRectangularMatrix();
      if (covar(0, 0) != 0. || covar(0, 1) != 0. ||
          covar(0, 2) != 0. || covar(1, 1) != 0. ||
          covar(1, 2) != 0. || covar(2, 2) != 0.) {
        pbPoint.add_aprioricovar(covar(0, 0));
        pbPoint.add_aprioricovar(covar(0, 1));
        pbPoint.add_aprioricovar(covar(0, 2));
        pbPoint.add_aprioricovar(covar(1, 1));
        pbPoint.add_aprioricovar(covar(1, 2));
        pbPoint.add_aprioricovar(covar(2, 2));
      }
    }


    if (GetSurfacePoint().Valid()) {
      SurfacePoint apost = GetSurfacePoint();
      pbPoint.set_x(apost.GetX());
      pbPoint.set_y(apost.GetY());
      pbPoint.set_z(apost.GetZ());

      symmetric_matrix< double, upper > covar = apost.GetRectangularMatrix();
      if (covar(0, 0) != 0. || covar(0, 1) != 0. ||
          covar(0, 2) != 0. || covar(1, 1) != 0. ||
          covar(1, 2) != 0. || covar(2, 2) != 0.) {
        pbPoint.add_apostcovar(covar(0, 0));
        pbPoint.add_apostcovar(covar(0, 1));
        pbPoint.add_apostcovar(covar(0, 2));
        pbPoint.add_apostcovar(covar(1, 1));
        pbPoint.add_apostcovar(covar(1, 2));
        pbPoint.add_apostcovar(covar(2, 2));
      }
    }

    //  Process all measures in the point
    for (int i = 0; i < cubeSerials->size(); i++)
      *pbPoint.add_measures() = (*p_measures)[cubeSerials->at(i)]->ToProtocolBuffer();

    return pbPoint;
  }


  PBControlNetLogData_Point ControlPoint::GetLogProtocolBuffer() const {
    PBControlNetLogData_Point protoBufLog;

    ControlMeasure *measure;
    foreach(measure, *p_measures) {
      *protoBufLog.add_measures() = measure->GetLogProtocolBuffer();
    }

    return protoBufLog;
  }
}
