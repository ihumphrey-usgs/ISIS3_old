/**
 * @file
 * $Revision: 1.7 $
 * $Date: 2010/06/07 22:42:38 $
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
#include "CameraPointInfo.h"

#include "Brick.h"
#include "Camera.h"
#include "CubeManager.h"
#include "Distance.h"
#include "IException.h"
#include "iTime.h"
#include "Longitude.h"
#include "Projection.h"
#include "PvlGroup.h"

using namespace std;

namespace Isis {


  /**
   * Constructor, initializes CubeManager and other variables for
   * use.
   *
   */
  CameraPointInfo::CameraPointInfo() {
    usedCubes = NULL;
    usedCubes = new CubeManager();
    usedCubes->SetNumOpenCubes(50);
    currentCube = NULL;
    camera = NULL;
  }

  /**
   * Destructor, deletes CubeManager object used.
   *
   */
  CameraPointInfo::~CameraPointInfo() {
    if(usedCubes) {
      delete usedCubes;
      usedCubes = NULL;
    }
  }


  /**
   * SetCube opens the given cube in a CubeManager. The
   * CubeManager is for effeciency when working with control nets
   * where cubes are accesed multiple times.
   *
   * @param cubeFileName A cube filename
   */
  void CameraPointInfo::SetCube(const QString &cubeFileName) {
    currentCube = usedCubes->OpenCube(cubeFileName);
    camera = currentCube->camera();
  }


  /**
   * SetImage sets a sample, line image coordinate in the camera
   * so data can be accessed.
   *
   * @param sample A sample coordinate in or almost in the cube
   * @param line A line coordinate in or almost in the cubei
   *
   * @return PvlGroup* The pertinent data from the Camera class on
   *         the point. Ownership is passed to caller.
   */
  PvlGroup *CameraPointInfo::SetImage(const double sample, const double line,
                                      const bool outside, const bool errors) {
    if(CheckCube()) {
      bool passed = camera->SetImage(sample, line);
      return GetPointInfo(passed, outside, errors);
    }
    // Should never get here, error will be thrown in CheckCube()
    return NULL;
  }

  /**
   * SetCenter sets the image coordinates to the center of the image.
   *
   * @return PvlGroup* The pertinent data from the Camera class on
   *         the point. Ownership is passed to caller.
   */
  PvlGroup *CameraPointInfo::SetCenter(const bool outside, const bool errors) {
    if(CheckCube()) {
      bool passed = camera->SetImage(currentCube->sampleCount() / 2.0, currentCube->lineCount() / 2.0);
      return GetPointInfo(passed, outside, errors);
    }
    // Should never get here, error will be thrown in CheckCube()
    return NULL;
  }


  /**
   * SetSample sets the image coordinates to the center line and the
   * given sample.
   *
   * @return PvlGroup* The pertinent data from the Camera class on
   *         the point. Ownership is passed to caller.
   */
  PvlGroup *CameraPointInfo::SetSample(const double sample,
                                       const bool outside, const bool errors) {
    if(CheckCube()) {
      bool passed = camera->SetImage(sample, currentCube->lineCount() / 2.0);
      return GetPointInfo(passed, outside, errors);
    }
    // Should never get here, error will be thrown in CheckCube()
    return NULL;
  }


  /**
   * SetLine sets the image coordinates to the center sample and the
   * given line.
   *
   * @return PvlGroup* The pertinent data from the Camera class on
   *         the point. Ownership is passed to caller.
   */
  PvlGroup *CameraPointInfo::SetLine(const double line,
                                     const bool outside, const bool errors) {
    if(CheckCube()) {
      bool passed = camera->SetImage(currentCube->sampleCount() / 2.0, line);
      return GetPointInfo(passed, outside, errors);
    }
    // Should never get here, error will be thrown in CheckCube()
    return NULL;
  }


  /**
   * SetGround sets a latitude, longitude grrund coordinate in the
   * camera so data can be accessed.
   *
   * @param latitude A latitude coordinate in or almost in the
   *                 cube
   * @param longitude A longitude coordinate in or almost in the
   *                  cube
   *
   * @return PvlGroup* The pertinent data from the Camera class on
   *         the point. Ownership is passed to caller.
   */
  PvlGroup *CameraPointInfo::SetGround(const double latitude, const double longitude,
                                       const bool outside, const bool errors) {
    if(CheckCube()) {
      bool passed = camera->SetUniversalGround(latitude, longitude);
      return GetPointInfo(passed, outside, errors);
    }
    // Should never get here, error will be thrown in CheckCube()
    return NULL;
  }


  /**
   * CheckCube checks that a cube has been set before the data for
   * a point is accessed.
   *
   * @return bool Whether or not a cube has been set, true if it has been.
   */
  bool CameraPointInfo::CheckCube() {
    if(currentCube == NULL) {
      string msg = "Please set a cube before setting parameters";
      throw IException(IException::Programmer, msg, _FILEINFO_);
      return false;
    }
    return true;
  }

  /**
   * GetPointInfo builds the PvlGroup containing all the important
   * information derived from the Camera.
   *
   * @return PvlGroup* Data taken directly from the Camera and
   *         drived from Camera information. Ownership passed.
   */
  PvlGroup *CameraPointInfo::GetPointInfo(bool passed, bool allowOutside, bool allowErrors) {
    PvlGroup *gp = new PvlGroup("GroundPoint");
    {
      gp->addKeyword(PvlKeyword("Filename"));
      gp->addKeyword(PvlKeyword("Sample"));
      gp->addKeyword(PvlKeyword("Line"));
      gp->addKeyword(PvlKeyword("PixelValue"));
      gp->addKeyword(PvlKeyword("RightAscension"));
      gp->addKeyword(PvlKeyword("Declination"));
      gp->addKeyword(PvlKeyword("PlanetocentricLatitude"));
      gp->addKeyword(PvlKeyword("PlanetographicLatitude"));
      gp->addKeyword(PvlKeyword("PositiveEast360Longitude"));
      gp->addKeyword(PvlKeyword("PositiveEast180Longitude"));
      gp->addKeyword(PvlKeyword("PositiveWest360Longitude"));
      gp->addKeyword(PvlKeyword("PositiveWest180Longitude"));
      gp->addKeyword(PvlKeyword("BodyFixedCoordinate"));
      gp->addKeyword(PvlKeyword("LocalRadius"));
      gp->addKeyword(PvlKeyword("SampleResolution"));
      gp->addKeyword(PvlKeyword("LineResolution"));
      gp->addKeyword(PvlKeyword("SpacecraftPosition"));
      gp->addKeyword(PvlKeyword("SpacecraftAzimuth"));
      gp->addKeyword(PvlKeyword("SlantDistance"));
      gp->addKeyword(PvlKeyword("TargetCenterDistance"));
      gp->addKeyword(PvlKeyword("SubSpacecraftLatitude"));
      gp->addKeyword(PvlKeyword("SubSpacecraftLongitude"));
      gp->addKeyword(PvlKeyword("SpacecraftAltitude"));
      gp->addKeyword(PvlKeyword("OffNadirAngle"));
      gp->addKeyword(PvlKeyword("SubSpacecraftGroundAzimuth"));
      gp->addKeyword(PvlKeyword("SunPosition"));
      gp->addKeyword(PvlKeyword("SubSolarAzimuth"));
      gp->addKeyword(PvlKeyword("SolarDistance"));
      gp->addKeyword(PvlKeyword("SubSolarLatitude"));
      gp->addKeyword(PvlKeyword("SubSolarLongitude"));
      gp->addKeyword(PvlKeyword("SubSolarGroundAzimuth"));
      gp->addKeyword(PvlKeyword("Phase"));
      gp->addKeyword(PvlKeyword("Incidence"));
      gp->addKeyword(PvlKeyword("Emission"));
      gp->addKeyword(PvlKeyword("NorthAzimuth"));
      gp->addKeyword(PvlKeyword("EphemerisTime"));
      gp->addKeyword(PvlKeyword("UTC"));
      gp->addKeyword(PvlKeyword("LocalSolarTime"));
      gp->addKeyword(PvlKeyword("SolarLongitude"));
      if(allowErrors) gp->addKeyword(PvlKeyword("Error"));
    }

    bool noErrors = passed;
    QString error = "";
    if(!camera->HasSurfaceIntersection()) {
      error = "Requested position does not project in camera model; no surface intersection";
      noErrors = false;
      if(!allowErrors) throw IException(IException::Unknown, error, _FILEINFO_);
    }
    if(!camera->InCube() && !allowOutside) {
      error = "Requested position does not project in camera model; not inside cube";
      noErrors = false;
      if(!allowErrors) throw IException(IException::Unknown, error, _FILEINFO_);
    }

    if(!noErrors) {
      for(int i = 0; i < gp->keywords(); i++) {
        QString name = (*gp)[i].name();
        // These three keywords have 3 values, so they must have 3 N/As
        if(name == "BodyFixedCoordinate" || name == "SpacecraftPosition" ||
            name == "SunPosition") {
          (*gp)[i].addValue("N/A");
          (*gp)[i].addValue("N/A");
          (*gp)[i].addValue("N/A");
        }
        else {
          (*gp)[i].setValue("N/A");
        }
      }
      // Set all keywords that still have valid information
      gp->findKeyword("Error").setValue(error);
      gp->findKeyword("FileName").setValue(currentCube->fileName());
      gp->findKeyword("Sample").setValue(toString(camera->Sample()));
      gp->findKeyword("Line").setValue(toString(camera->Line()));
      gp->findKeyword("EphemerisTime").setValue(toString(camera->time().Et()), "seconds");
      gp->findKeyword("EphemerisTime").addComment("Time");
      QString utc = camera->time().UTC();
      gp->findKeyword("UTC").setValue(utc);
      gp->findKeyword("SpacecraftPosition").addComment("Spacecraft Information");
      gp->findKeyword("SunPosition").addComment("Sun Information");
      gp->findKeyword("Phase").addComment("Illumination and Other");
    }

    else {

      Brick b(3, 3, 1, currentCube->pixelType());

      int intSamp = (int)(camera->Sample() + 0.5);
      int intLine = (int)(camera->Line() + 0.5);
      b.SetBasePosition(intSamp, intLine, 1);
      currentCube->read(b);

      double pB[3], spB[3], sB[3];
      QString utc;
      double ssplat, ssplon, sslat, sslon, pwlon, oglat;

      {
        gp->findKeyword("FileName").setValue(currentCube->fileName());
        gp->findKeyword("Sample").setValue(toString(camera->Sample()));
        gp->findKeyword("Line").setValue(toString(camera->Line()));
        gp->findKeyword("PixelValue").setValue(PixelToString(b[0]));
        gp->findKeyword("RightAscension").setValue(toString(camera->RightAscension()));
        gp->findKeyword("Declination").setValue(toString(camera->Declination()));
        gp->findKeyword("PlanetocentricLatitude").setValue(toString(camera->UniversalLatitude()));

        // Convert lat to planetographic
        Distance radii[3];
        camera->radii(radii);
        oglat = Isis::Projection::ToPlanetographic(camera->UniversalLatitude(),
                radii[0].kilometers(), radii[2].kilometers());
        gp->findKeyword("PlanetographicLatitude").setValue(toString(oglat));

        gp->findKeyword("PositiveEast360Longitude").setValue(toString(
          camera->UniversalLongitude()));

        //Convert lon to -180 - 180 range
        gp->findKeyword("PositiveEast180Longitude").setValue(toString(
          Isis::Projection::To180Domain(
            camera->UniversalLongitude())));

        //Convert lon to positive west
        pwlon = Isis::Projection::ToPositiveWest(camera->UniversalLongitude(),
                360);
        gp->findKeyword("PositiveWest360Longitude").setValue(toString(pwlon));

        //Convert pwlon to -180 - 180 range
        gp->findKeyword("PositiveWest180Longitude").setValue(toString(
          Isis::Projection::To180Domain(pwlon)));

        camera->Coordinate(pB);
        gp->findKeyword("BodyFixedCoordinate").addValue(toString(pB[0]), "km");
        gp->findKeyword("BodyFixedCoordinate").addValue(toString(pB[1]), "km");
        gp->findKeyword("BodyFixedCoordinate").addValue(toString(pB[2]), "km");

        gp->findKeyword("LocalRadius").setValue(toString(camera->LocalRadius().meters()), "meters");
        gp->findKeyword("SampleResolution").setValue(toString(camera->SampleResolution()), "meters/pixel");
        gp->findKeyword("LineResolution").setValue(toString(camera->LineResolution()), "meters/pixel");

        //body fixed
        camera->instrumentPosition(spB);
        gp->findKeyword("SpacecraftPosition").addValue(toString(spB[0]), "km");
        gp->findKeyword("SpacecraftPosition").addValue(toString(spB[1]), "km");
        gp->findKeyword("SpacecraftPosition").addValue(toString(spB[2]), "km");
        gp->findKeyword("SpacecraftPosition").addComment("Spacecraft Information");

        gp->findKeyword("SpacecraftAzimuth").setValue(toString(camera->SpacecraftAzimuth()));
        gp->findKeyword("SlantDistance").setValue(toString(camera->SlantDistance()), "km");
        gp->findKeyword("TargetCenterDistance").setValue(toString(camera->targetCenterDistance()), "km");
        camera->subSpacecraftPoint(ssplat, ssplon);
        gp->findKeyword("SubSpacecraftLatitude").setValue(toString(ssplat));
        gp->findKeyword("SubSpacecraftLongitude").setValue(toString(ssplon));
        gp->findKeyword("SpacecraftAltitude").setValue(toString(camera->SpacecraftAltitude()), "km");
        gp->findKeyword("OffNadirAngle").setValue(toString(camera->OffNadirAngle()));
        double subspcgrdaz;
        subspcgrdaz = camera->GroundAzimuth(camera->UniversalLatitude(), camera->UniversalLongitude(),
                                            ssplat, ssplon);
        gp->findKeyword("SubSpacecraftGroundAzimuth").setValue(toString(subspcgrdaz));

        camera->sunPosition(sB);
        gp->findKeyword("SunPosition").addValue(toString(sB[0]), "km");
        gp->findKeyword("SunPosition").addValue(toString(sB[1]), "km");
        gp->findKeyword("SunPosition").addValue(toString(sB[2]), "km");
        gp->findKeyword("SunPosition").addComment("Sun Information");

        gp->findKeyword("SubSolarAzimuth").setValue(toString(camera->SunAzimuth()));
        gp->findKeyword("SolarDistance").setValue(toString(camera->SolarDistance()), "AU");
        camera->subSolarPoint(sslat, sslon);
        gp->findKeyword("SubSolarLatitude").setValue(toString(sslat));
        gp->findKeyword("SubSolarLongitude").setValue(toString(sslon));
        double subsolgrdaz;
        subsolgrdaz = camera->GroundAzimuth(camera->UniversalLatitude(), camera->UniversalLongitude(),
                                            sslat, sslon);
        gp->findKeyword("SubSolarGroundAzimuth").setValue(toString(subsolgrdaz));

        gp->findKeyword("Phase").setValue(toString(camera->PhaseAngle()));
        gp->findKeyword("Phase").addComment("Illumination and Other");
        gp->findKeyword("Incidence").setValue(toString(camera->IncidenceAngle()));
        gp->findKeyword("Emission").setValue(toString(camera->EmissionAngle()));
        gp->findKeyword("NorthAzimuth").setValue(toString(camera->NorthAzimuth()));

        gp->findKeyword("EphemerisTime").setValue(toString(camera->time().Et()), "seconds");
        gp->findKeyword("EphemerisTime").addComment("Time");
        utc = camera->time().UTC();
        gp->findKeyword("UTC").setValue(utc);
        gp->findKeyword("LocalSolarTime").setValue(toString(camera->LocalSolarTime()), "hour");
        gp->findKeyword("SolarLongitude").setValue(toString(camera->solarLongitude().degrees()));
        if(allowErrors) gp->findKeyword("Error").setValue("N/A");
      }
    }
    return gp;
  }
}
