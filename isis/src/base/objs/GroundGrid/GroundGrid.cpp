#include "GroundGrid.h"

#include <cmath>
#include <iomanip>

#include <QVector>

#include "Angle.h"
#include "UniversalGroundMap.h"
#include "Camera.h"
#include "Distance.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Progress.h"
#include "Projection.h"
#include "ProjectionFactory.h"

using namespace std;

namespace Isis {
  /**
   * This method initializes the class by allocating the grid,
   * calculating the lat/lon range, and getting a default grid
   * resolution.
   *
   * @param gmap A universal ground map to use for calculating the grid
   * @param splitLatLon Make two grids: one for latitude lines and
   *                    one for longitude lines
   * @param width The width of the grid; often cube samples
   * @param height The height of the grid; often cube samples
   */
  GroundGrid::GroundGrid(UniversalGroundMap *gmap, bool splitLatLon,
                         unsigned int width, unsigned int height) {
    p_width = width;
    p_height = height;

    // Initialize our grid pointer to null
    p_grid = 0;
    p_latLinesGrid = 0;
    p_lonLinesGrid = 0;

    // Now let's figure out how big the grid needs to be, then allocate and
    //   initialize
    p_gridSize = (unsigned long)(ceil(width * height / 8.0) + 0.5);

    if(!splitLatLon) {
      p_grid = new char[p_gridSize];
    }
    else {
      p_latLinesGrid = new char[p_gridSize];
      p_lonLinesGrid = new char[p_gridSize];
    }

    for(unsigned long i = 0; i < p_gridSize; i++) {
      if(p_grid) p_grid[i] = 0;
      if(p_latLinesGrid) p_latLinesGrid[i] = 0;
      if(p_lonLinesGrid) p_lonLinesGrid[i] = 0;
    }

    // The first call of CreateGrid doesn't have to reinitialize
    p_reinitialize = false;

    p_groundMap = gmap;

    // We need a lat/lon range for gridding, use the mapping group
    //  (in case of camera, use BasicMapping)
    p_minLat = NULL;
    p_minLon = NULL;
    p_maxLat = NULL;
    p_maxLon = NULL;

    p_mapping = new PvlGroup;

    if(p_groundMap->Camera()) {
      Pvl tmp;
      p_groundMap->Camera()->BasicMapping(tmp);
      *p_mapping = tmp.findGroup("Mapping");
    }
    else {
      *p_mapping = p_groundMap->Projection()->Mapping();
    }

    Distance radius1 = Distance((double)(*p_mapping)["EquatorialRadius"],
        Distance::Meters);
    Distance radius2 = Distance((double)(*p_mapping)["PolarRadius"],
        Distance::Meters);

    if(p_mapping->hasKeyword("MinimumLatitude")) {
      p_minLat = new Latitude(
          toDouble((*p_mapping)["MinimumLatitude"][0]), *p_mapping,
          Angle::Degrees);
    }
    else {
      p_minLat = new Latitude;
    }

    if(p_mapping->hasKeyword("MaximumLatitude")) {
      p_maxLat = new Latitude(
          toDouble((*p_mapping)["MaximumLatitude"][0]), *p_mapping,
          Angle::Degrees);
    }
    else {
      p_maxLat = new Latitude;
    }

    if(p_mapping->hasKeyword("MinimumLongitude")) {
      p_minLon = new Longitude(
          toDouble((*p_mapping)["MinimumLongitude"][0]), *p_mapping,
          Angle::Degrees);
    }
    else {
      p_minLon = new Longitude;
    }

    if(p_mapping->hasKeyword("MaximumLongitude")) {
      p_maxLon = new Longitude(
          toDouble((*p_mapping)["MaximumLongitude"][0]), *p_mapping,
          Angle::Degrees);
    }
    else {
      p_maxLon = new Longitude;
    }

    if(p_minLon->isValid() && p_maxLon->isValid()) {
      if(*p_minLon > *p_maxLon) {
        Longitude tmp(*p_minLon);
        *p_minLon = *p_maxLon;
        *p_maxLon = tmp;
      }
    }

    Distance largerRadius = max(radius1, radius2);

    // p_defaultResolution is in degrees/pixel

    if(p_groundMap->HasCamera()) {
      p_defaultResolution =
        (p_groundMap->Camera()->HighestImageResolution() /
         largerRadius.meters()) * 10;
    }
    else {
      p_defaultResolution = (p_groundMap->Resolution() /
        largerRadius.meters()) * 10;
    }

    if(p_defaultResolution < 0) {
      p_defaultResolution = 10.0 / largerRadius.meters();
    }
  }

  /**
   * Delete the object
   */
  GroundGrid::~GroundGrid() {
    if(p_minLat) {
      delete p_minLat;
      p_minLat = NULL;
    }

    if(p_minLon) {
      delete p_minLon;
      p_minLon = NULL;
    }

    if(p_maxLat) {
      delete p_maxLat;
      p_maxLat = NULL;
    }

    if(p_maxLon) {
      delete p_maxLon;
      p_maxLon = NULL;
    }

    if(p_mapping) {
      delete p_mapping;
      p_mapping = NULL;
    }

    if(p_grid) {
      delete [] p_grid;
      p_grid = NULL;
    }

    if(p_latLinesGrid) {
      delete [] p_latLinesGrid;
      p_latLinesGrid = NULL;
    }

    if(p_lonLinesGrid) {
      delete [] p_lonLinesGrid;
      p_lonLinesGrid = NULL;
    }
  }

  /**
   * This method draws the grid internally, using default resolutions.
   *
   * @param baseLat Latitude to hit in the grid
   * @param baseLon Longitude to hit in the grid
   * @param latInc  Distance between latitude lines
   * @param lonInc  Distance between longitude lines
   * @param progress If passed in, this progress will be used
   */
  void GroundGrid::CreateGrid(Latitude baseLat, Longitude baseLon,
                              Angle latInc,  Angle lonInc,
                              Progress *progress) {
    CreateGrid(baseLat, baseLon, latInc, lonInc, progress, Angle(), Angle());
  }


  /**
   * This method draws the grid internally. It is not valid to call PixelOnGrid
   * until this method has been called.
   *
   * @param baseLat Latitude to hit in the grid
   * @param baseLon Longitude to hit in the grid
   * @param latInc  Distance between latitude lines
   * @param lonInc  Distance between longitude lines
   * @param progress If passed in, this progress will be used
   * @param latRes  Resolution of latitude lines (in degrees/pixel)
   * @param lonRes  Resolution of longitude lines (in degrees/pixel)
   */
  void GroundGrid::CreateGrid(Latitude baseLat, Longitude baseLon,
                              Angle latInc,  Angle lonInc,
                              Progress *progress,
                              Angle latRes, Angle lonRes) {
    if(p_groundMap == NULL ||
        (p_grid == NULL && p_latLinesGrid == NULL && p_lonLinesGrid == NULL)) {
      IString msg = "GroundGrid::CreateGrid missing ground map or grid array";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if(p_reinitialize) {
      for(unsigned long i = 0; i < p_gridSize; i++) {
        if(p_grid) p_grid[i] = 0;
        if(p_latLinesGrid) p_latLinesGrid[i] = 0;
        if(p_lonLinesGrid) p_lonLinesGrid[i] = 0;
      }
    }

    // Verify lat/lon range is okay
    bool badLatLonRange = false;
    QVector<IString> badLatLonValues;
    if(!p_minLat || !p_minLat->isValid()) {
      badLatLonValues.append("MinimumLatitude");
      badLatLonRange = true;
    }

    if(!p_maxLat || !p_maxLat->isValid()) {
      badLatLonValues.append("MaximumLatitude");
      badLatLonRange = true;
    }

    if(!p_minLon || !p_minLon->isValid()) {
      badLatLonValues.append("MinimumLongitude");
      badLatLonRange = true;
    }

    if(!p_maxLon || !p_maxLon->isValid()) {
      badLatLonValues.append("MaximumLongitude");
      badLatLonRange = true;
    }


    if(badLatLonRange) {
      IString msg = "Could not determine values for [";
      for(int i = 0; i < badLatLonValues.size(); i++) {
        if(i != 0)
          msg += ",";

        msg += badLatLonValues[i];
      }

      msg += "], please specify them explicitly";

      // I chose parse because it's not really the user's fault or the
      //   programmer's. It's a stripped keyword in a Pvl.
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    // subsequent calls to this method must always reinitialize the grid
    p_reinitialize = true;

    // Find starting points for lat/lon
    Latitude startLat = Latitude(
        baseLat - Angle(floor((baseLat - *p_minLat) / latInc) * latInc),
        *GetMappingGroup());

    Longitude startLon = Longitude(
        baseLon - Angle(floor((baseLon - *p_minLon) / lonInc) * lonInc));

    if(!latRes.isValid() || latRes <= Angle(0, Angle::Degrees)) {
      latRes = Angle(p_defaultResolution, Angle::Degrees);
    }

    if(!lonRes.isValid() || latRes <= Angle(0, Angle::Degrees)) {
      lonRes = Angle(p_defaultResolution, Angle::Degrees);
    }

    Latitude endLat = Latitude(
        (long)((*p_maxLat - startLat) / latInc) * latInc + startLat,
        *GetMappingGroup());
    Longitude endLon =
        (long)((*p_maxLon - startLon) / lonInc) * lonInc + startLon;

    if(progress) {
      double numSteps = (double)((endLat - startLat) / latInc) + 1;
      numSteps += (double)((endLon - startLon) / lonInc) + 1;

      if(numSteps <= 0) {
        IString msg = "No gridlines would intersect the image";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      progress->SetMaximumSteps((long)(numSteps + 0.5));
      progress->CheckStatus();
    }

    for(Latitude lat = startLat; lat <= endLat + latInc / 2; lat += latInc) {
      unsigned int previousX = 0;
      unsigned int previousY = 0;
      bool havePrevious = false;

      for(Longitude lon = *p_minLon; lon <= *p_maxLon; lon += latRes) {
        unsigned int x = 0;
        unsigned int y = 0;
        bool valid = GetXY(lat, lon, x, y);

        if(valid && havePrevious) {
          if(previousX != x || previousY != y) {
            DrawLineOnGrid(previousX, previousY, x, y, true);
          }
        }

        havePrevious = valid;
        previousX = x;
        previousY = y;
      }

      if(progress) {
        progress->CheckStatus();
      }
    }

    for(Longitude lon = startLon; lon <= endLon + lonInc / 2; lon += lonInc) {
      unsigned int previousX = 0;
      unsigned int previousY = 0;
      bool havePrevious = false;

      for(Latitude lat = *p_minLat; lat <= *p_maxLat; lat += lonRes) {
        unsigned int x = 0;
        unsigned int y = 0;

        bool valid = GetXY(lat, lon, x, y);

        if(valid && havePrevious) {
          if(previousX == x && previousY == y) {
            continue;
          }

          DrawLineOnGrid(previousX, previousY, x, y, false);
        }

        havePrevious = valid;
        previousX = x;
        previousY = y;
      }

      if(progress) {
        progress->CheckStatus();
      }
    }
  }


  /**
   * This restricts (or grows) the ground range in which to draw grid lines.
   *
   * @param minLat The lowest latitude extreme to draw grid lines to
   * @param maxLat The highest latitude extreme to draw grid lines to
   * @param minLon The lowest longitude extreme to draw grid lines to
   * @param maxLon The highest longitude extreme to draw grid lines to
   */
  void GroundGrid::SetGroundLimits(Latitude minLat, Longitude minLon,
                                   Latitude maxLat, Longitude maxLon) {
    if(minLat.isValid()) *p_minLat = minLat;
    if(maxLat.isValid()) *p_maxLat = maxLat;
    if(minLon.isValid()) *p_minLon = minLon;
    if(maxLon.isValid()) *p_maxLon = maxLon;

    if(p_minLat->isValid() && p_maxLat->isValid() && *p_minLat > *p_maxLat) {
      Latitude tmp(*p_minLat);
      *p_minLat = *p_maxLat;
      *p_maxLat = tmp;
    }

    if(p_minLon->isValid() && p_maxLon->isValid() && *p_minLon > *p_maxLon) {
      Longitude tmp(*p_minLon);
      *p_minLon = *p_maxLon;
      *p_maxLon = tmp;
    }
  }

  /**
   * This draws grid lines along the extremes of the lat/lon box of the grid.
   */
  void GroundGrid::WalkBoundary() {
    Angle latRes = Angle(p_defaultResolution, Angle::Degrees);
    Angle lonRes = Angle(p_defaultResolution, Angle::Degrees);

    const Latitude  &minLat = *p_minLat;
    const Latitude  &maxLat = *p_maxLat;
    const Longitude &minLon = *p_minLon;
    const Longitude &maxLon = *p_maxLon;

    // Walk the minLat/maxLat lines
    for(Latitude lat = minLat; lat <= maxLat; lat += (maxLat - minLat)) {
      unsigned int previousX = 0;
      unsigned int previousY = 0;
      bool havePrevious = false;

      for(Longitude lon = minLon; lon <= maxLon; lon += latRes) {
        unsigned int x = 0;
        unsigned int y = 0;
        bool valid = GetXY(lat, lon, x, y);

        if(valid && havePrevious) {
          if(previousX != x || previousY != y) {
            DrawLineOnGrid(previousX, previousY, x, y, true);
          }
        }

        havePrevious = valid;
        previousX = x;
        previousY = y;
      }
    }

    // Walk the minLon/maxLon lines
    for(Longitude lon = minLon; lon <= maxLon; lon += (maxLon - minLon)) {
      unsigned int previousX = 0;
      unsigned int previousY = 0;
      bool havePrevious = false;

      for(Latitude lat = minLat; lat <= maxLat; lat += lonRes) {
        unsigned int x = 0;
        unsigned int y = 0;
        bool valid = GetXY(lat, lon, x, y);

        if(valid && havePrevious) {
          if(previousX != x || previousY != y) {
            DrawLineOnGrid(previousX, previousY, x, y, false);
          }
        }

        havePrevious = valid;
        previousX = x;
        previousY = y;
      }
    }
  }


  /**
   * Returns true if the grid is on this point. Using this method
   * is recommended if lat/lon grids are separate.
   *
   * @param x X-Coordinate of grid (0-based)
   * @param y Y-Coordinate of grid (0-based)
   * @param latGrid True for latitude lines, false for longitude
   *                lines
   *
   * @return bool Pixel lies on grid
   */
  bool GroundGrid::PixelOnGrid(int x, int y, bool latGrid) {
    if(x < 0) return false;
    if(y < 0) return false;

    if(x >= (int)p_width) return false;
    if(y >= (int)p_height) return false;

    return GetGridBit(x, y, latGrid);
  }


  /**
   * Returns true if the grid is on this point.
   *
   * @param x X-Coordinate of grid (0-based)
   * @param y Y-Coordinate of grid (0-based)
   *
   * @return bool Pixel lies on grid
   */
  bool GroundGrid::PixelOnGrid(int x, int y) {
    if(x < 0) return false;
    if(y < 0) return false;

    if(x >= (int)p_width) return false;
    if(y >= (int)p_height) return false;

    return GetGridBit(x, y, true); // third argument shouldnt matter
  }


  /**
   * This method converts a lat/lon to an X/Y. This implementation converts to
   * sample/line.
   *
   * @param lat Latitude of Lat/Lon pair to convert to S/L
   * @param lon Longitude of Lat/Lon pair to convert to S/L
   * @param x   Output sample (0-based)
   * @param y   Output line (0-based)
   *
   * @return bool Successful
   */
  bool GroundGrid::GetXY(Latitude lat, Longitude lon,
                         unsigned int &x, unsigned int &y) {
    if(!GroundMap()) return false;
    if(!GroundMap()->SetGround(lat, lon)) return false;
    if(p_groundMap->Sample() < 0.5 || p_groundMap->Line() < 0.5) return false;
    if(p_groundMap->Sample() < 0.5 || p_groundMap->Line() < 0.5) return false;

    x = (int)(p_groundMap->Sample() - 0.5);
    y = (int)(p_groundMap->Line() - 0.5);

    if(x >= p_width || y >= p_height) return false;

    return true;
  }


  /**
   * This flags a bit as on the grid lines.
   *
   * @param x X-Coordinate (0-based)
   * @param y Y-Coordinate (0-based)
   * @param latGrid True if this is derived from a latitude line
   */
  void GroundGrid::SetGridBit(unsigned int x, unsigned int y, bool latGrid) {
    unsigned long bitPosition = (y * p_width) + x;
    unsigned long byteContainer = bitPosition / 8;
    unsigned int bitOffset = bitPosition % 8;

    if(byteContainer < 0 || byteContainer > p_gridSize) return;

    if(p_grid) {
      char &importantByte = p_grid[byteContainer];
      importantByte |= (1 << bitOffset);
    }
    else if(latGrid && p_latLinesGrid) {
      char &importantByte = p_latLinesGrid[byteContainer];
      importantByte |= (1 << bitOffset);
    }
    else if(!latGrid && p_lonLinesGrid) {
      char &importantByte = p_lonLinesGrid[byteContainer];
      importantByte |= (1 << bitOffset);
    }
    else {
      IString msg = "GroundGrid::SetGridBit no grids available";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Returns true if the specified coordinate is on the grid lines.
   *
   * @param x X-Coordinate (0-based)
   * @param y Y-Coordinate (0-based)
   * @param latGrid True if you want to access the latitude grid where there are
   *   separate lat/lon grids. False for lon. Irrelevant if using a single grid.
   *
   * @return bool Value at grid coordinate
   */
  bool GroundGrid::GetGridBit(unsigned int x, unsigned int y, bool latGrid) {
    unsigned long bitPosition = (y * p_width) + x;
    unsigned long byteContainer = bitPosition / 8;
    unsigned int bitOffset = bitPosition % 8;

    if(byteContainer < 0 || byteContainer > p_gridSize) return false;

    bool result = false;

    if(p_grid) {
      char &importantByte = p_grid[byteContainer];
      result = (importantByte >> bitOffset) & 1;
    }
    else if(latGrid && p_latLinesGrid) {
      char &importantByte = p_latLinesGrid[byteContainer];
      result = (importantByte >> bitOffset) & 1;
    }
    else if(!latGrid && p_lonLinesGrid) {
      char &importantByte = p_lonLinesGrid[byteContainer];
      result = (importantByte >> bitOffset) & 1;
    }
    else {
      IString msg = "GroundGrid::GetGridBit no grids available";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return result;
  }


  /**
   * This sets the bits on the grid along the specified line.
   *
   * @param x1 Start X
   * @param y1 Start Y
   * @param x2 End X
   * @param y2 End Y
   * @param isLatLine
   */
  void GroundGrid::DrawLineOnGrid(unsigned int x1, unsigned int y1,
                                  unsigned int x2, unsigned int y2,
                                  bool isLatLine) {
    int dx = x2 - x1;
    int dy = y2 - y1;

    SetGridBit(x1, y1, isLatLine);

    if(dx != 0) {
      double m = (double)dy / (double)dx;
      double b = y1 - m * x1;

      dx = (x2 > x1) ? 1 : -1;
      while(x1 != x2) {
        x1 += dx;
        y1 = (int)(m * x1 + b + 0.5);
        SetGridBit(x1, y1, isLatLine);
      }
    }
  }
};
