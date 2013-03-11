/**
 * @file
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software
 *   and related material nor shall the fact of distribution constitute any such
 *   warranty, and no responsibility is assumed by the USGS in connection
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

#include "Latitude.h"

#include <cmath>

#include "Constants.h"
#include "Distance.h"
#include "IException.h"
#include "IString.h"
#include "Projection.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"

namespace Isis {
  /**
   * Create a blank Latitude object without Planetographic support.
   */
  Latitude::Latitude() : Angle() {
    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    m_errors = ThrowAllErrors;
  }


  /**
   * Create and initialize a Latitude value without planetographic support.
   *
   * @see ErrorChecking
   * @see CoordinateType
   * @param latitude The latitude value this instance will represent,
   *     in the planetocentric coordinate system
   * @param latitudeUnits The angular units of the latitude value (degs, rads)
   * @param errors Error checking conditions
   */
  Latitude::Latitude(double latitude, Angle::Units latitudeUnits,
                     ErrorChecking errors) : Angle() {
    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    m_errors = errors;

    setPlanetocentric(latitude, latitudeUnits);
  }


  /**
   * Create and initialize a Latitude value in the planetocentric domain within
   * the given angle.
   *
   * @param latitude The latitude value this instance will represent
   * @param errors Error checking conditions
   */
  Latitude::Latitude(Angle latitude, ErrorChecking errors) : Angle() {
    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    m_errors = errors;

    setPlanetocentric(latitude.radians(), Radians);
  }


  /**
   * Create and initialize a Latitude value using the mapping group's latitude
   * units and radii.
   *
   * @see ErrorChecking
   * @see CoordinateType
   * @param latitude The latitude value this instance will represent,
   *     in the mapping group's units
   * @param mapping A mapping group
   * @param latitudeUnits The angular units of the latitude value (degs, rads)
   * @param errors Error checking conditions
   */
  Latitude::Latitude(Angle latitude, PvlGroup mapping,
            ErrorChecking errors) : Angle(latitude) {
    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    if (mapping.hasKeyword("EquatorialRadius") && mapping.hasKeyword("PolarRadius")) {
      m_equatorialRadius = new Distance(toDouble(mapping["EquatorialRadius"][0]),
          Distance::Meters);
      m_polarRadius = new Distance(toDouble(mapping["PolarRadius"][0]),
          Distance::Meters);
    }
    else {
      PvlGroup radiiGrp = Projection::TargetRadii(mapping["TargetName"]);

      m_equatorialRadius = new Distance(toDouble(radiiGrp["EquatorialRadius"][0]),
          Distance::Meters);
      m_polarRadius = new Distance(toDouble(radiiGrp["PolarRadius"][0]),
          Distance::Meters);
    }

    m_errors = errors;

    if(mapping["LatitudeType"][0] == "Planetographic") {
      setPlanetographic(latitude.radians(), Radians);
    }
    else if(mapping["LatitudeType"][0] == "Planetocentric") {
      setPlanetocentric(latitude.radians(), Radians);
    }
    else {
      QString msg = "Latitude type [" + mapping["LatitudeType"][0] +
        "] is not recognized";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Create and initialize a Latitude value using the mapping group's latitude
   * units and radii.
   *
   * @see ErrorChecking
   * @see CoordinateType
   * @param latitude The latitude value this instance will represent,
   *     in the mapping group's units
   * @param mapping A mapping group
   * @param latitudeUnits The angular units of the latitude value (degs, rads)
   * @param errors Error checking conditions
   */
  Latitude::Latitude(double latitude,
            PvlGroup mapping,
            Angle::Units latitudeUnits,
            ErrorChecking errors) : Angle(latitude, latitudeUnits) {
    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    if (mapping.hasKeyword("EquatorialRadius") && mapping.hasKeyword("PolarRadius")) {
      m_equatorialRadius = new Distance(toDouble(mapping["EquatorialRadius"][0]),
          Distance::Meters);
      m_polarRadius = new Distance(toDouble(mapping["PolarRadius"][0]),
          Distance::Meters);
    }
    else {
      PvlGroup radiiGrp = Projection::TargetRadii(mapping["TargetName"]);

      m_equatorialRadius = new Distance(toDouble(radiiGrp["EquatorialRadius"][0]),
          Distance::Meters);
      m_polarRadius = new Distance(toDouble(radiiGrp["PolarRadius"][0]),
          Distance::Meters);
    }

    m_errors = errors;

    if(mapping["LatitudeType"][0] == "Planetographic") {
      setPlanetographic(latitude, latitudeUnits);
    }
    else if(mapping["LatitudeType"][0] == "Planetocentric") {
      setPlanetocentric(latitude, latitudeUnits);
    }
    else {
      IString msg = "Latitude type [" + IString(mapping["LatitudeType"][0]) +
        "] is not recognized";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Create and initialize a Latitude value with planetographic support.
   *
   * @see ErrorChecking
   * @see CoordinateType
   * @param latitude The latitude value this instance will represent,
   *     in planetocentric
   * @param equatorialRadius Radius of the target (planet) at the equator
   * @param polarRadius Radius of the target (planet) at the poles
   * @param latType The coordinate system of the latitude parameter
   * @param latitudeUnits The angular units of the latitude value (degs, rads)
   * @param errors Error checking conditions
   */
  Latitude::Latitude(double latitude,
                     Distance equatorialRadius, Distance polarRadius,
                     CoordinateType latType,
                     Angle::Units latitudeUnits,
                     ErrorChecking errors) : Angle(latitude, latitudeUnits) {
    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    m_equatorialRadius = new Distance(equatorialRadius);
    m_polarRadius = new Distance(polarRadius);

    m_errors = errors;

    if (latType == Planetocentric) {
      setPlanetocentric(latitude, latitudeUnits);
    }
    else if (latType == Planetographic) {
      setPlanetographic(latitude, latitudeUnits);
    }
    else {
      IString msg = "Enumeration value [" + IString(latType) + "] is not a "
        "valid CoordinateType";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * This copies the given latitude exactly.
   *
   * @param latitudeToCopy The latitude we're duplicating
   */
  Latitude::Latitude(const Latitude &latitudeToCopy) : Angle(latitudeToCopy) {
    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    m_errors = latitudeToCopy.m_errors;

    if(latitudeToCopy.m_equatorialRadius) {
      m_equatorialRadius = new Distance(*latitudeToCopy.m_equatorialRadius);
    }

    if(latitudeToCopy.m_polarRadius) {
      m_polarRadius = new Distance(*latitudeToCopy.m_polarRadius);
    }
  }


  /**
   * This cleans up the Latitude class.
   */
  Latitude::~Latitude() {
    if (m_equatorialRadius) {
      delete m_equatorialRadius;
      m_equatorialRadius = NULL;
    }

    if (m_polarRadius) {
      delete m_polarRadius;
      m_polarRadius = NULL;
    }
  }


  /**
   * Get the latitude in the planetocentric (universal) coordinate system.
   *
   * @see CoordinateType
   * @param units The angular units to get the latitude in
   * @return The Planetocentric latitude value
   */
  double Latitude::planetocentric(Angle::Units units) const {
    return angle(units);
  }


  /**
   * Set the latitude given a value in the Planetocentric coordinate system
   *
   * @param latitude The planetographic latitude to set ourselves to
   * @param units The angular units latitude is in
   */
  void Latitude::setPlanetocentric(double latitude, Angle::Units units) {
    setAngle(latitude, units);
  }


  /**
   * Get the latitude in the planetographic coordinate system. If this instance
   *   was not constructed with the planetary radii, then an exception will be
   *   thrown.
   *
   * @see CoordinateType
   * @param units The angular units to get the latitude in
   * @return The Planetographic latitude value
   */
  double Latitude::planetographic(Angle::Units units) const {
    if (m_equatorialRadius == NULL || m_polarRadius == NULL) {
      IString msg = "Latitude [" + IString(degrees()) + " degrees] cannot "
          "be converted to Planetographic without the planetary radii, please "
          "use the other Latitude constructor";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (*this > Angle(90.0, Angle::Degrees) ||
        *this < Angle(-90.0, Angle::Degrees)) {
      IString msg = "Latitudes outside of the -90/90 range cannot be converted "
          "between Planetographic and Planetocentric";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if(!isValid()) {
      IString msg = "Invalid planetographic latitudes are not currently "
          "supported";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    double ographicLatitude = atan(tan(radians()) *
        (*m_equatorialRadius / *m_polarRadius) *
        (*m_equatorialRadius / *m_polarRadius));

    // This theoretically should just be an angle, but make it a Latitude so
    //   we can access angle
    return Latitude(ographicLatitude, Angle::Radians).angle(units);
  }


  /**
   * Set the latitude given a value in the Planetographic coordinate system
   *
   * @param latitude The planetographic latitude to set ourselves to
   * @param units The angular units latitude is in
   */
  void Latitude::setPlanetographic(double latitude, Angle::Units units) {
    if (m_equatorialRadius == NULL || m_polarRadius == NULL) {
      IString msg = "Latitude [" + IString(latitude) + "] cannot be "
          "converted to Planetocentic without the planetary radii, please use "
          "the other Latitude constructor";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    Angle inputAngle(latitude, units);

    if (inputAngle > Angle(90.0, Angle::Degrees) ||
        inputAngle < Angle(-90.0, Angle::Degrees)) {
      IString msg = "Latitudes outside of the -90/90 range cannot be converted "
          "between Planetographic and Planetocentric";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if(IsSpecial(latitude)) {
      IString msg = "Invalid planetographic latitudes are not currently "
          "supported";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    double ocentricLatitude = atan(tan(inputAngle.radians()) *
        (*m_polarRadius / *m_equatorialRadius) *
        (*m_polarRadius / *m_equatorialRadius));

    // Sometimes the trig functions return the negative of the expected value at the pole.
    if ((ocentricLatitude > 0) != (inputAngle.radians() > 0)) {
      ocentricLatitude *= -1;
    }

    setAngle(ocentricLatitude, Angle::Radians);
  }


  /**
   * Checks if this latitude value is within the given range.  Defines the
   * range as the change from the minimum latitude to the maximum latitude (an
   * angle), and returns whether the change from the minimum latitude to this
   * latitude is less than or equal to the maximum change allowed (the range).
   *
   * @param min The beginning of the valid latitude range
   * @param max The end of the valid latitude range
   *
   * @return Whether the latitude is in the given range
   */
  bool Latitude::inRange(Latitude min, Latitude max) const {
    // Validity check on the range
    if (min > max) {
      IString msg = "Minimum latitude [" + IString(min.degrees()) +
        "] degrees is greater than maximum latitude [" +
        IString(max.degrees()) + "] degrees";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Provide a little wriggle room for precision problems
    Angle epsilon(DBL_EPSILON, Angle::Degrees);
    Latitude adjustedMin = min - epsilon;
    Latitude adjustedMax = max + epsilon;

    // Is this latitude between the min and the max
    return *this >= adjustedMin && *this <= adjustedMax;
  }


  /**
    * This assigns another latitude to this one - making this latitude an
    *   exact duplicate of the other.
    *
    * @param latitudeToCopy The latitude we are assigning from
    * @return The result, a reference to this
    */
  Latitude& Latitude::operator=(const Latitude & latitudeToCopy) {
    if(this == &latitudeToCopy) return *this;

    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    m_errors = latitudeToCopy.m_errors;

    if(latitudeToCopy.m_equatorialRadius) {
      m_equatorialRadius = new Distance(*latitudeToCopy.m_equatorialRadius);
    }

    if(latitudeToCopy.m_polarRadius) {
      m_polarRadius = new Distance(*latitudeToCopy.m_polarRadius);
    }

    setPlanetocentric(latitudeToCopy.planetocentric());

    return *this;
  }
  

  /**
    * Adds an angle to this latitude. The adding method is determined by the
    *   latitude type.
    *
    * @param angleToAdd the latitude being added to this one
    * @param mapping the mapping group from a projection
    * @return The result
    */
  Latitude Latitude::add(Angle angleToAdd, PvlGroup mapping) {

    CoordinateType latType;

    Distance equatorialRadius;
    Distance polarRadius;
    if (mapping.hasKeyword("EquatorialRadius") && mapping.hasKeyword("PolarRadius")) {
      equatorialRadius = Distance(toDouble(mapping["EquatorialRadius"][0]),
          Distance::Meters);
      polarRadius = Distance(toDouble(mapping["PolarRadius"][0]),
          Distance::Meters);
    }
    else {
      PvlGroup radiiGrp = Projection::TargetRadii(mapping["TargetName"]);

      equatorialRadius = Distance(toDouble(radiiGrp["EquatorialRadius"][0]),
          Distance::Meters);
      polarRadius = Distance(toDouble(radiiGrp["PolarRadius"][0]),
          Distance::Meters);
    }

    if(mapping["LatitudeType"][0] == "Planetocentric")
      latType = Planetocentric;
    else if (mapping["LatitudeType"][0] == "Planetographic")
      latType = Planetographic;
    else {
      IString msg = "Latitude type [" + IString(mapping["LatitudeType"][0]) +
        "] is not recognized";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return add(angleToAdd, equatorialRadius, polarRadius, latType);
  }

  
  /**
    * Adds another latitude to this one. Handles planetographic latitudes.
    *
    * @param angleToAdd the latitude being added to this one
    * @param equatorialRadius
    * @param polarRadius
    * @param latType
    * @return The result
    */
  Latitude Latitude::add(Angle angleToAdd, Distance equatorialRadius, Distance polarRadius,
                         CoordinateType latType) {
    Latitude result;
    
    switch (latType) {
      case Planetocentric:
        result = Latitude(planetocentric() + angleToAdd.radians(), equatorialRadius, polarRadius,
                          latType, Radians, m_errors);
        break;

      case Planetographic:
        result = Latitude(planetographic() + angleToAdd.radians(), equatorialRadius, polarRadius,
                          latType, Radians, m_errors);
        break;
    }
    
    return result;
  }

 
  /**
   * We're overriding this method in order to do -90/90 degree checking
   *
   * @param angle The numeric value of the angle
   * @param units The units angle is in (radians or degrees typically)
   */
  void Latitude::setAngle(double angle, const Angle::Units &units) {
    // Check for passing 90 degrees if that error checking is on
    if (!IsSpecial(angle) && (m_errors | AllowPastPole) != AllowPastPole) {
      Angle tmpAngle(angle, units);
      if(tmpAngle > Angle(90, Angle::Degrees) ||
         tmpAngle < Angle(-90, Angle::Degrees)) {
        IString msg = "Latitudes past 90 degrees are not valid. The latitude "
            "[" + IString(tmpAngle.degrees()) + " degrees] is not allowed";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }

    Angle::setAngle(angle, units);
  }
}
