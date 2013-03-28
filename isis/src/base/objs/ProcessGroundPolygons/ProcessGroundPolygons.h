#ifndef ProcessGroundPolygons_h
#define ProcessGroundPolygons_h

#include "ProjectionFactory.h"
#include "Process.h"
#include "Brick.h"
#include "FileName.h"
#include "ProcessPolygons.h"
#include "UniversalGroundMap.h"

namespace Isis {
  /**
   * @brief Process cube polygons to map or camera projections
   *
   * This class allows a programmer to develop a program which
   * @ingroup HighLevelCubeIO
   *
   * @author  2008-12-14 Stacy Alley
   *
   * @internal
   *   @history 2008-05-12 Steven Lambright - Removed references to CubeInfo
   *   @history 2008-08-18 Steven Lambright - Updated to work with geos3.0.0
   *                           instead of geos2. Mostly namespace changes.
   *   @history 2012-02-24 Steven Lambright - Added Finalize()
   *   @history 2013-03-27 Jeannie Backer - Added programmer comments.
   *                           References #1248.
   */
  class ProcessGroundPolygons : public ProcessPolygons {
    public:
      ProcessGroundPolygons();

      // SetOutputCube() is not virtual in the Process class nor in the
      // ProcessPolygons class, so the following definitions for this method
      // are the only ones that are allowed for ProcessGroundPolygon objects
      // and child objects

      //Cube is an existing camera cube or projection cube
      void SetOutputCube(const QString &parameter, QString &cube);

      //Determine cube size from the projection map
      void SetOutputCube(const QString &parameter, Isis::Pvl &map, int bands);

      void SetOutputCube(const QString &avgFileName, const QString
                         &countFileName, Isis::CubeAttributeOutput &atts,
                         QString &cube);

      void SetOutputCube(const QString &avgFileName, const QString
                         &countFileName, Isis::CubeAttributeOutput &atts,
                         Isis::Pvl &map, int bands);

      void AppendOutputCube(QString &cube, const QString &avgFileName,
                            const QString &countFileName = "");

      void Rasterize(std::vector<double> &lat,
                     std::vector<double> &lon,
                     std::vector<double> &values);

      void Rasterize(std::vector<double> &lat,
                     std::vector<double> &lon,
                     int &band, double &value);

      void Rasterize(double &lat, double &lon, int &band, double &value);

      void EndProcess();
      void Finalize();
      UniversalGroundMap *GetUniversalGroundMap() {
        return p_groundMap;
      };

    private:
      void Convert(std::vector<double> &lat, std::vector<double> &lon);
      void Convert(double &lat, double &lon);
      UniversalGroundMap *p_groundMap;
      std::vector<double> p_samples, p_lines;

  };

};

#endif
