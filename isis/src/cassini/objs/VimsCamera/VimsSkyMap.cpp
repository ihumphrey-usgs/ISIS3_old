/**
 * @file
 *
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
#include "VimsSkyMap.h"

#include <iostream>
#include <iomanip>

#include "Camera.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "LeastSquares.h"
#include "PolynomialBivariate.h"
#include "Preference.h"
#include "SpecialPixel.h"

using namespace std;

namespace Isis {
  /**
   * Constructs the VimsSkyMap object
   *
   * @param parent A pointer to the parent Camera object
   * @param lab Pvl labels for the image
   *
   * @internal
   */
  VimsSkyMap::VimsSkyMap(Camera *parent, Pvl &lab) :
    CameraSkyMap(parent) {
  }


  /**
   * Initialize vims sky model
   *
   * @param [in] lab   (Pvl &)      Cube Pvl label
   *
   * @throw iException::Io - "Cannot process NYQUIST(undersampled) mode "
   * @throw iException::Io - "Can't open unit vector file"
   * @internal
   *   @history  2007-04-16 Tracie Sucharski - Look for unit vectors in
   *                              the proper directory.
   *   @history  2007-04-18 Tracie Sucharski, The inaccuracy of the 15 Mhz clock
   *                    (exposure , interline_delay) is already taken care of
   *                     in the labels values, so remove the adjustment from the
   *                     code (exp * 1.01725).
   *                     *Reference:  email from John Ivens 11/27/2006.
   */
  void VimsSkyMap::Init(Pvl &lab) {

    PvlGroup inst = lab.findGroup("Instrument", Pvl::Traverse);

    //  Vis or IR
    p_channel = (QString) inst ["Channel"];
    // Get the start time in et
    QString stime = (QString) inst ["NativeStartTime"];
    QString intTime = stime.split(".").first();
    stime = stime.split(".").last();

    p_etStart = p_camera->getClockTime(intTime).Et();
    p_etStart += toDouble(stime) / 15959.0;

    //----------------------------------------------------------------------
    //  Because of inaccuracy with the 15 Mhz clock, the IR exposure and
    //  interline delay need to be adjusted.
    //----------------------------------------------------------------------
    p_irExp = (double) inst ["ExposureDuration"] / 1000.;
    p_visExp = toDouble(inst ["ExposureDuration"][1]) / 1000.;
    p_interlineDelay =
      (double) inst ["InterlineDelayDuration"] / 1000.;

    // Get summation mode
    QString sampMode = QString((QString)inst ["SamplingMode"]).toUpper();

    //  Get sample/line offsets
    int sampOffset = inst ["XOffset"];
    int lineOffset = inst ["ZOffset"];

    //  Get swath width/length which will be image size unless occultation image
    p_swathWidth = inst ["SwathWidth"];
    p_swathLength = inst ["SwathLength"];

    //-----------------------------------------------------------------------
    // Set up correct limits for unit vector file,  calculate offsets and
    //  Read unit vectors.
    //-----------------------------------------------------------------------
    //int code = naifIkCode();
    //string key = "INS" + IString(code) + "_UNIT_VECTORS";
    //FileName vectorFile(Spice::getString(key,0));
    //tack on _HR or _NY for other summing modes.
    // Get the directory for the unit vector files.
    PvlGroup &dataDir = Preference::Preferences().findGroup("DataDirectory");
    QString vecDir = (QString) dataDir["Cassini"] + "/unitVectors/";

    FileName vectorFile;
    if(p_channel == "VIS") {
      if(sampMode == "NORMAL") {
        vectorFile = vecDir + "VIS_NORMAL_uv.bin";
        p_nsUv = 64;
        p_nlUv = 64;
        p_camSampOffset = sampOffset - 1;
        p_camLineOffset = lineOffset - 1;
      }
      else {
        vectorFile = vecDir + "VIS_HI-RES_uv.bin";
        p_nsUv = 192;
        p_nlUv = 192;

        p_camSampOffset = 3 * (sampOffset - 1) + p_swathWidth;
        p_camLineOffset = 3 * (lineOffset - 1) + p_swathLength;
        /*p_camLineOffset = 3 * (lineOffset - 1) + swathLength - 6;*/
        /*p_camLineOffset = 93 - (3 * (lineOffset - 1)) - swathLength;*/
        /*  This is in range of 450 (top) to -537 (boresight at 0).  */
        /*  The unit Vector file is in the range 94 to -98 (boresight at 0).  */
        /*  Change to 0 to 191 range for indexing into uv file.   */
        /*p_camLineOffset = p_camLineOffset + 94;*/
      }
    }
    else if(p_channel == "IR") {
      if(sampMode == "NORMAL") {
        vectorFile = vecDir + "IR_NORMAL_uv.bin";
        p_nsUv = 64;
        p_nlUv = 64;
        p_camSampOffset = sampOffset - 1;
        p_camLineOffset = lineOffset - 1;
      }
      if(sampMode == "HI-RES") {
        vectorFile = vecDir + "IR_HI-RES_uv.bin";
        p_nsUv = 128;
        p_nlUv = 64;
        p_camSampOffset = 2 * ((sampOffset - 1) + ((p_swathWidth - 1) / 4));
        p_camLineOffset = lineOffset - 1;
        /*  camSampOffset = 2 * (sampOffset - 1);   SHIFT TOO FAR TO RIGHT    */
        /*sampOffset = sampOffset * 2;*/
      }
      if(sampMode == "NYQUIST") {
        string msg = "Cannot process NYQUIST(undersampled) mode ";
        throw IException(IException::Io, msg, _FILEINFO_);
      }
    }

    ifstream fin;
    QString vectorFileName(vectorFile.expanded());
    fin.open(vectorFileName.toAscii().data(), ios::in | ios::binary);
    if(!fin.is_open()) {
      string msg = "Can't open unit vector file";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
    //  Read correct band
    /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    /  DONT FORGET TO TAKE THIS OUT WHEN WE HAVE UNIT VECTORS FOR ALL BANDS.
    /  This needs to be smartened up for different summing modes.        */

    int band = 0;
    int startByte = (band * (p_nsUv * p_nlUv * 3)) * 8;
    fin.seekg(startByte, ios_base::beg);

    for(int line = 0; line < p_nlUv; line++) {
      for(int samp = 0; samp < p_nsUv; samp++) {
        fin.read((char *)&p_unitVector[line][samp][0], sizeof(double));
        fin.read((char *)&p_unitVector[line][samp][1], sizeof(double));
        fin.read((char *)&p_unitVector[line][samp][2], sizeof(double));
      }
    }

    //  Rotate unit vector -90 degrees around the x-axis to match the
    //  boresight defined in the frames kernel.
    SpiceDouble Z[3] = {1., 0., 0.};
    SpiceDouble newvec[3];

    for(int line = 0; line < p_nlUv; line++) {
      for(int samp = 0; samp < p_nsUv; samp++) {
        vrotv_c(&p_unitVector[line][samp][0], Z, -1.570796327, newvec);
        memcpy(&p_unitVector[line][samp][0], &newvec, 3 * sizeof(SpiceDouble));
      }
    }
    fin.close();

    //  Calculate lat/lon maps
    for(int line = 0; line < p_camera->ParentLines(); line++) {
      for(int samp = 0; samp < p_camera->ParentSamples(); samp++) {
        p_raMap[line][samp] = Isis::NULL8;
        p_decMap[line][samp] = Isis::NULL8;
      }
    }

    //---------------------------------------------------------------------
    //  Loop for each pixel in cube, get pointing information and calculate
    //  control point (line,sample,lat,lon) for later use in latlon_to_linesamp.
    //---------------------------------------------------------------------
    p_minRa = 99999.;
    p_minDec = 99999.;
    p_maxRa = -99999.;
    p_maxDec = -99999.;

    p_camera->IgnoreProjection(true);
    for(int line = 0; line < p_camera->ParentLines(); line++) {

      //  VIS exposure is for a single line.  According to SIS,
      //  NATIVE_START_TIME is for the first pixel of the IR exposure.
      //  "The offset from IR start to VIS start is calculated by
      //   IrExposMsec - VisExposMsec)/2".
      //  This needs to be moved to forward routine

      if(p_channel == "VIS") {
        double et = ((double)p_etStart + (((p_irExp * p_swathWidth) - p_visExp) / 2.)) +
                    ((line + 0.5) * p_visExp);
        p_camera->setTime(et);
      }

      for(int samp = 0; samp < p_camera->ParentSamples(); samp++) {
        if(p_channel == "IR") {
          double et = (double)p_etStart +
                      (line * p_camera->ParentSamples() * p_irExp) +
                      (line * p_interlineDelay) + ((samp + 0.5) * p_irExp);
          p_camera->setTime(et);
        }

        p_camera->SetImage((double) samp + 1, (double)line + 1);
        double ra = p_camera->RightAscension();
        double dec = p_camera->Declination();
        if(ra < p_minRa) p_minRa = ra;
        if(ra > p_maxRa) p_maxRa = ra;
        if(dec < p_minDec) p_minDec = dec;
        if(dec > p_maxDec) p_maxDec = dec;
        p_raMap[line][samp] = ra;
        p_decMap[line][samp] = dec;
      }
    }
    p_camera->IgnoreProjection(false);

  }


  /** Compute ground position from focal plane coordinate
   *
   * This method will compute the ground position given an
   * undistorted focal plane coordinate.  Note that the latitude/longitude
   * value can be obtained from the camera class passed into the constructor.
   *
   * @param ux distorted focal plane x in millimeters
   * @param uy distorted focal plane y in millimeters
   * @param uz distorted focal plane z in millimeters
   *
   * @return conversion was successful
   */
  bool VimsSkyMap::SetFocalPlane(const double ux, const double uy,
                                 const double uz) {

    int imgSamp = (int)(ux + .5);
    int imgLine = (int)(uy + .5);

    if((imgLine < 1) || (imgLine > p_camera->ParentLines()) ||
        (imgSamp < 1) || (imgSamp > p_camera->ParentSamples())) {
      return false;
    }
    imgLine--;
    imgSamp--;

    // does interline_delay & exposure-duration account for summing modes?
    // if not, won't use p_parentLine/p_parentSample
    double et = 0.;
    if(p_channel == "VIS") {
      et = (p_etStart + ((p_irExp * p_swathWidth) - p_visExp) / 2.) +
           ((imgLine + 0.5) * p_visExp);
    }
    else if(p_channel == "IR") {
      et = (double)p_etStart +
           (imgLine * p_camera->ParentSamples() * p_irExp) +
           (imgLine * p_interlineDelay) + ((imgSamp + 0.5) * p_irExp);
    }
    p_camera->setTime(et);

    // Make sure line/samp fall within unitVector ,if not return false???
    int uvLine = imgLine + p_camLineOffset;
    int uvSamp = imgSamp + p_camSampOffset;
    if(uvSamp < 0 || uvSamp > p_nsUv ||
        uvLine < 0 || uvLine > p_nlUv) {
      return false;
    }

    SpiceDouble lookC[3];
    lookC[0] = p_unitVector[uvLine][uvSamp][0];
    lookC[1] = p_unitVector[uvLine][uvSamp][1];
    lookC[2] = p_unitVector[uvLine][uvSamp][2];

    SpiceDouble unitLookC[3];
    vhat_c(lookC, unitLookC);
    return p_camera->SetLookDirection(unitLookC);
  }

  /**
   *  Sets the sky position to the given ra and dec
   *
   * @param ra the right ascension
   * @param dec the declination
   *
   * @return set sky was successful
   */
  bool VimsSkyMap::SetSky(const double ra, const double dec) {
    if(ra < p_minRa || ra > p_maxRa ||
        dec < p_minDec || dec > p_maxDec) {
      return false;
    }
    //  Find closest points  ??? what tolerance ???
    double minDist = 9999.;
    int minSamp = -1;
    int minLine = -1;

    for(int line = 0; line < p_camera->ParentLines(); line++) {

      for(int samp = 0; samp < p_camera->ParentSamples(); samp++) {
        double mapRa = p_raMap[line][samp];
        if(mapRa == Isis::NULL8) continue;
        double mapDec = p_decMap[line][samp];
        if(mapDec == Isis::NULL8) continue;
        //  If on boundary convert lons.  If trying to find 360, convert
        //  lons on other side of meridian to values greater than 360.  If
        //  trying to find 1.0, convert lons on other side to negative numbers.
        if(abs(mapRa - ra) > 180) {
          if((ra - mapRa) > 0) {
            mapRa = 360. + mapRa;
          }
          else if((ra - mapRa) < 0) {
            mapRa = mapRa - 360.;
          }
        }
        double dist = ((ra - mapRa) * (ra - mapRa)) +
                      ((dec - mapDec) * (dec - mapDec));
        if(dist < minDist) {
          minDist = dist;
          minSamp = samp;
          minLine = line;
        }
      }
    }

    //-----------------------------------------------------------------
    //  If dist is less than some ??? tolerance ??? this is the
    //  closest point.  Use this point and surrounding 8 pts as
    //  control pts.
    //----------------------------------------------------------------
    if(minDist >= 9999.) return false;

    //-------------------------------------------------------------
    //  Set-up for LU decomposition (least2 fit).
    //  Assume we will have 9 control points, this may not be true
    //  and will need to be adjusted before the final solution.
    //-------------------------------------------------------------
    PolynomialBivariate sampBasis(1);
    PolynomialBivariate lineBasis(1);
    LeastSquares sampLsq(sampBasis);
    LeastSquares lineLsq(lineBasis);
    vector<double> known(2);

    for(int line = minLine - 1; line < minLine + 2; line++) {
      if(line < 0 || line > p_camera->ParentLines() - 1) continue;
      for(int samp = minSamp - 1; samp < minSamp + 2; samp++) {
        //  Check for edges
        if(samp < 0 || samp > p_camera->ParentSamples() - 1) continue;

        double mapRa = p_raMap[line][samp];
        double mapDec = p_decMap[line][samp];
        if((mapRa == Isis::NULL8) || (mapDec == Isis::NULL8)) continue;

        //  If on boundary convert lons.  If trying to find 360, convert
        //  lons on other side of meridian to values greater than 360.  If
        //  trying to find 1.0, convert lons on other side to negative numbers.
        if(abs(mapRa - ra) > 180) {
          if((ra - mapRa) > 0) {
            mapRa = 360. + mapRa;
          }
          else if((ra - mapRa) < 0) {
            mapRa = mapRa - 360.;
          }
        }

        known[0] = mapDec;
        known[1] = mapRa;
        sampLsq.AddKnown(known, samp + 1);
        lineLsq.AddKnown(known, line + 1);
      }
    }
    if(sampLsq.Knowns() < 3) return false;

    sampLsq.Solve();
    lineLsq.Solve();

    //  Solve for new sample position
    known[0] = dec;
    known[1] = ra;
    double inSamp = sampLsq.Evaluate(known);
    double inLine = lineLsq.Evaluate(known);

    if(inSamp < 0 || inSamp > p_camera->ParentSamples() + 0.5 ||
        inLine < 0 || inLine > p_camera->ParentLines() + 0.5) {
      return false;
    }

    p_camera->IgnoreProjection(true);
    p_camera->SetImage(inSamp, inLine);
    p_camera->IgnoreProjection(false);
    p_focalPlaneX = inSamp;
    p_focalPlaneY = inLine;

    return true;
  }
}
