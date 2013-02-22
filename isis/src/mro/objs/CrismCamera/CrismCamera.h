#ifndef CrismCamera_h
#define CrismCamera_h
/**
 * @file
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

#include <QString>

#include "LineScanCamera.h"
#include "VariableLineScanCameraDetectorMap.h"

namespace Isis {
  /**
   * @brief MRO CRISM camera model
   *
   *   This is the camera model for the MRO CRISM instrument.  It provides
   *   support for band independent geometry for the instrument.
   *
   * @ingroup SpiceInstrumentsAndCameras 
   * @ingroup MarsReconnaissanceOrbiter
   *
   * @see Camera
   *
   * @author 2013-02-21 Kris Becker
   *
   * @internal
   *
   *   @history 2013-02-21 Kris Becker Original version
   *
   */
  class CrismCamera : public LineScanCamera {
    public:
      // constructors
      CrismCamera(Pvl &lab);

      //! Destroys the CrismCamera object.
      virtual ~CrismCamera() {  }

      void SetBand (const int physicalBand);

      /**
       * @brief This is a band-dependant instrument
       * 
       * @author kbecker (4/12/2012)
       * 
       * @return bool 
       */
      bool IsBandIndependent ();

      /**
       * CK frame ID -  - Instrument Code from spacit run on CK
       *  
       * @return @b int The appropriate instrument code for the "Camera-matrix" 
       *         Kernel Frame ID
       */
      virtual int CkFrameId() const { return (-74000); }

      /** 
       * CK Reference ID - J2000
       * 
       * @return @b int The appropriate instrument code for the "Camera-matrix"
       *         Kernel Reference ID
       */
      virtual int CkReferenceId() const { return (-74900); }

      /** 
       *  SPK Reference ID - J2000
       *  
       * @return @b int The appropriate instrument code for the Spacecraft 
       *         Kernel Reference ID
       */
      virtual int SpkReferenceId() const { return (1); }

  private:
    std::vector<LineRateChange> m_lineRates;
    bool m_isBandDependent;


     double getEtTime(const QString &sclk);

     

  };
};
#endif
