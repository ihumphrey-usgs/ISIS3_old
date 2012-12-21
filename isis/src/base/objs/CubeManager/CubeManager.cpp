#include "Cube.h"
#include "CubeAttribute.h"
#include "CubeManager.h"
#include "FileName.h"
#include "IString.h"

#include <iostream>

namespace Isis {
  CubeManager CubeManager::p_instance;

  /**
   * This initializes a CubeManager object
   *
   */
  CubeManager::CubeManager() {
    p_minimumCubes = 0;
  }

  /**
   * This is the CubeManager destructor. This method calls CleanCubes().
   *
   */
  CubeManager::~CubeManager() {
    CleanCubes();
  }

  /**
   * This method opens a cube. If the cube is already opened, this method will
   * return the cube from memory. The CubeManager class retains ownership of this
   * cube pointer, so do not close the cube, destroy the pointer, or otherwise
   * modify the cube object or pointer such that another object using them would
   * fail. This method does not guarantee you are the only one with this pointer,
   * nor is it recommended to keep this pointer out of a local (method) scope.
   *
   * @param cubeFileName The filename of the cube you wish to open
   *
   * @return Cube* A pointer to the cube object that CubeManager retains ownership
   *         to and may delete at any time
   */
  Cube *CubeManager::OpenCube(const QString &cubeFileName) {
    CubeAttributeInput attIn(cubeFileName);
    IString attri = attIn.toString();
    IString expName = FileName(cubeFileName).expanded();

    // If there are attributes, we need a plus sign on the name
    if(attri.size() > 0) {
      expName += "+";
    }

    IString fullName = expName + attri;
    QString fileName(fullName.ToQt());
    QMap<QString, Cube *>::iterator searchResult = p_cubes.find(fileName);

    if(searchResult == p_cubes.end()) {
      p_cubes.insert(fileName, new Cube());
      searchResult = p_cubes.find(fileName);
      // Bands are the only thing input attributes can affect
      (*searchResult)->setVirtualBands(attIn.bands());

      try {
        (*searchResult)->open(fileName);
      }
      catch(IException &e) {
        CleanCubes(fileName);
        throw;
      }
    }

    // Keep track of the newly opened cube in our queue
    p_opened.removeAll(fileName);
    p_opened.enqueue(fileName);

    // cleanup if necessary
    if(p_minimumCubes != 0) {
      while(p_opened.size() > (int)(p_minimumCubes)) {
        QString needsCleaned = p_opened.dequeue();
        CleanCubes(needsCleaned);
      }
    }

    return (*searchResult);
  }

  /**
   * This method removes a cube from memory, if it exists. If the cube is not
   * loaded into memory, nothing happens. This will cause any pointers to this
   * cube, obtained via OpenCube, to be invalid.
   *
   * @param cubeFileName The filename of the cube to remove from memory
   */
  void CubeManager::CleanCubes(const QString &cubeFileName) {
    QString fileName(FileName(cubeFileName).expanded());
    QMap<QString, Cube *>::iterator searchResult = p_cubes.find(fileName);

    if(searchResult == p_cubes.end()) {
      return;
    }

    (*searchResult)->close();
    delete *searchResult;
    p_cubes.erase(searchResult);
  }

  /**
   * This method removes all cubes from memory. All pointers returned via OpenCube
   * will be invalid.
   */
  void CubeManager::CleanCubes() {
    QMap<QString, Cube *>::iterator pos = p_cubes.begin();

    while(pos != p_cubes.end()) {
      (*pos)->close();
      delete *pos;
      pos ++;
    }

    p_cubes.clear();
  }
}
