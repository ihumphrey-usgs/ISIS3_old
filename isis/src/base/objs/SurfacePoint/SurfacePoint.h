#ifndef SurfacePoint_h
#define SurfacePoint_h
/**
 * @file
 * $Revision: 1.6 $
 * $Date: 2010/04/29 00:54:15 $
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

#include <vector>
#include <cmath>

#include "boost/numeric/ublas/symmetric.hpp"
#include "boost/numeric/ublas/io.hpp"

#include "Displacement.h"
#include "Distance.h"
#include "Angle.h"

namespace Isis {
  class Latitude;
  class Longitude;

  /**
   * @brief This class defines a body-fixed surface point 
   *  
   * This class is a container for body-fixed surface points.  It provides 
   * methods to set and present the coordinates of surface points in various 
   * usable units to support projection of image points to the ground and 
   * bundle adjustment. 
   *
   * @ingroup Geometry
   *
   * @authors  2010-07-30 Tracie Sucharski, Ken L. Edmunson, and Debbie A. Cook
   * @history  2010-08-25 Debbie A. Cook  Added more error checking and testing
   * @history  2010-09-10 Debbie A. Cook  Made ocentric methods specific to
   *           units of sigmas (degrees or meters)
   * @history  2010-10-04 Debbie A. Cook  Remove using boost to avoid compile
   *           errors throughout Isis3 classes and added boost namespace name to
   *           all uses of matrix
   * @history  2010-10-20 Debbie A. Cook and Steven Lambright  Simplified the 
   *           class by using new Displacement, Distance, Latitude, Longitude,
   *           and Angle objects. 
   * @history  2010-12-28 Steven Lambright and Sharmila Prasad 
   *           Fixed a problem with retreiving longitudes outside of 0-360.
   * @history 2011-02-11 Steven Lambright Added SphericalDistanceToPoint and
   *           optimized for speed inside the cameras and typical use cases
   *           where we only have an x,y,z but no other data. Fixed a problem
   *           where points were not properly considered valid at some
   *           boundary conditions
   * @history 2011-03-05 Ken Edmundson Added GetLatWeight, GetLonWeight, 
   *           GetLocalRadiusWeight, and SetSphericalCoordinates methods for use
   *           in BundleAdjust.
   */

  class SurfacePoint {
    public:
      // Constructors
//      SurfacePoint(const std::vector <double> radii);
      SurfacePoint();
      SurfacePoint(const SurfacePoint &other);
      SurfacePoint(const Latitude &lat, const Longitude &lon,
                   const Distance &radius);
      SurfacePoint(const Latitude &lat, const Longitude &lon,
          const Distance &radius, const Angle &latSigma, const Angle &lonSigma,
          const Distance &radiusSigma);
      SurfacePoint(const Latitude &lat, const Longitude &lon,
                   const Distance &radius,
                   const boost::numeric::ublas::symmetric_matrix
                     <double,boost::numeric::ublas::upper>& covar);
      SurfacePoint(const Displacement &x, const Displacement &y,
                   const Displacement &z);
      SurfacePoint(const Displacement &x, const Displacement &y,
          const Displacement &z, const Distance &xSigma, const Distance &ySigma,
          const Distance &zSigma);
      SurfacePoint(const Displacement &x, const Displacement &y,
                   const Displacement &z,
                   const boost::numeric::ublas::symmetric_matrix
                     <double,boost::numeric::ublas::upper>& covar);
      ~SurfacePoint();

// Rectangular loading utilities
      void SetRectangular(const Displacement &x, const Displacement &y,
          const Displacement &z, const Distance &xSigma=Distance(),
          const Distance &ySigma=Distance(), const Distance &zSigma=Distance());

      void SetRectangular(const Displacement x, const Displacement y, const Displacement z,
        const boost::numeric::ublas::symmetric_matrix<double,boost::numeric::ublas::upper>& covar);

      //! Set surface point and sigmas in rectangular coordinates and convert to planetocentric
      void SetRectangularSigmas(const Distance &xSigma, const Distance &ySigma,
                                const Distance &zSigma);


      void SetRectangularMatrix(
        const boost::numeric::ublas::symmetric_matrix<double,boost::numeric::ublas::upper>& covar);

// Spherical loading utilities

      //! Set surface point and covariance matrix in planetocentric coordinates and convert to rectangular
      //! (Latitude, Longitude in degrees, Radius in meters; matrix in radians and radians**2)
      void SetSpherical (const Latitude &lat, const Longitude &lon,
          const Distance &radius, const Angle &latSigma=Angle(),
          const Angle &lonSigma=Angle(),
          const Distance &radiusSigma=Distance());

      void SetSpherical (const Latitude &lat, const Longitude &lon,
          const Distance &radius,
          const boost::numeric::ublas::symmetric_matrix
            <double,boost::numeric::ublas::upper>& covar);

      void SetSphericalCoordinates(const Latitude &lat, const Longitude &lon,
                                   const Distance &radius);

      void SetSphericalMatrix(
        const boost::numeric::ublas::symmetric_matrix<double,boost::numeric::ublas::upper>& covar);

      void SetSphericalSigmas(const Angle &latSigma, const Angle &lonSigma,
                              const Distance &radiusSigma);

      void SetSphericalSigmasDistance(const Distance &latSigma,
                                      const Distance &lonSigma,
                                      const Distance &radiusSigma);

      void SetRadii(const Distance &majorRadius, const Distance &minorRadius,
                    const Distance &polarRadius);

      void ResetLocalRadius(const Distance &radius);
      bool Valid() const;

// Output methods
      Displacement GetX() const;
      Displacement GetY() const;
      Displacement GetZ() const;
      Distance GetXSigma() const;
      Distance GetYSigma() const;
      Distance GetZSigma() const;
      boost::numeric::ublas::symmetric_matrix<double,boost::numeric::ublas::upper> 
        GetRectangularMatrix() const;
      Latitude GetLatitude() const; 
      Longitude GetLongitude() const;
      Distance GetLocalRadius() const;
      Angle GetLatSigma() const;
      Distance GetLatSigmaDistance() const;
      double GetLatWeight() const;
      Angle GetLonSigma() const;
      Distance GetLonSigmaDistance() const;
      double GetLonWeight() const;
      Distance GetLocalRadiusSigma() const;
      double GetLocalRadiusWeight() const;
      boost::numeric::ublas::symmetric_matrix
          <double,boost::numeric::ublas::upper> GetSphericalMatrix() const;

// Computational methods
      Distance GetDistanceToPoint(const SurfacePoint &other) const;
      Distance GetDistanceToPoint(const SurfacePoint &other,
          const Distance &sphereRadius) const;

// Misc methods
      void ToNaifArray(double naifOutput[3]) const;
      void FromNaifArray(const double naifValues[3]);

// Operators
      bool operator==(const SurfacePoint &other) const;
      SurfacePoint &operator=(const SurfacePoint &other);

    private:
      void InitCovariance();
      void InitPoint();
      void InitRadii();
      void SetRectangularPoint(const Displacement &x, const Displacement &y, const Displacement &z);
      void SetSphericalPoint(const Latitude &lat, const Longitude &lon, const Distance &radius);
      void FreeAllocatedMemory();

      Distance *p_majorAxis;
      Distance *p_minorAxis;
      Distance *p_polarAxis;
      Displacement *p_x;
      Displacement *p_y;
      Displacement *p_z;
      //! 3x3 upper triangular covariance matrix rectangular coordinates
      boost::numeric::ublas::symmetric_matrix
          <double,boost::numeric::ublas::upper> *p_rectCovar;
      //! 3x3 upper triangular covariance matrix ocentric coordinates
      boost::numeric::ublas::symmetric_matrix
          <double,boost::numeric::ublas::upper> *p_sphereCovar;
  };
};

#endif

