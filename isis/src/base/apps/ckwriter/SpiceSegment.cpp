/**
 * @file
 * $Revision$
 * $Date$
 * $Id$
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
#include <vector>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <functional>


#include "SpiceSegment.h"
#include "IString.h"
#include "FileName.h"
#include "Cube.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "Table.h"
#include "NaifStatus.h"
#include "IException.h"

#include "naif/SpiceUsr.h"

using namespace std;

namespace Isis {


  /** Default constructor */
SpiceSegment::SpiceSegment() {
  init();
}

/** Construct with an ISIS cube file */
SpiceSegment::SpiceSegment(const QString &fname) {
  init();
  Cube cube;
  cube.open(fname);
  import(cube);
}

/** Construct with a Cube and optional naming of table */
SpiceSegment::SpiceSegment(Cube &cube, const QString &tblname) {
  init();
  import(cube, tblname);
}

/** Set the name of the CK SPICE segment */
void SpiceSegment::setId(const QString &name) {
  _name = name;
  return;
}

/**
 * @brief Provide specified kernel types
 *
 * This method is provided to load (furnsh) NAIF kernels.  This is typically
 * required at the time the output CK file is created.
 *
 * @param ktypes Kernel types to load.  If empty, all kernels in the cube file
 *               will be loaded.  Example would be "FK,SCLK,LSK".
 *
 * @return int Returns number of kernels loaded
 */
int SpiceSegment::FurnshKernelType(const QString &ktypes) const {
  return (_kernels.Load(ktypes));
}

/**
 * @brief Unload NAIF kernels of specified type
 *
 * This method unloads NAIF SPICE kernels that are associate with the specified
 * types in the ktypes parameter.  It can be used in succession with the
 * FurnshKernelType() method above.
 *
 * @param ktypes  Kernel types to unload.  If empty, all kernels in the cube
 *               file will be unloaded.  Example would be "FK,SCLK,LSK".
 *
 * @return int  Returns number of kernels unloaded
 */
int SpiceSegment::UnloadKernelType(const QString &ktypes) const {
  return (_kernels.UnLoad(ktypes));
}

/**
 * @brief Returns the start time of intervals in segments
 *
 * This routine is needed for writing CK kernels.  It is assumed that the
 * complete segment is one interval, thus this implementation will return a
 * 1-element vector with the start time of the first quaternion.
 *
 *
 * @return SpiceSegment::SVector& Vector of start times for each interval
 */
SpiceSegment::SVector SpiceSegment::SCLKStartIntervals() const {
  return (SVector(1, _times[0]));
}

/**
 * @brief Returns the stop time of intervals in segments
 *
 * This routine is needed for writing CK kernels.  It is assumed that the
 * complete segment is one interval, thus this implementation will return a
 * 1-element vector with the stop time of the last quaternion.
 *
 *
 * @return SpiceSegment::SVector& Vector of stop times for each interval
 */
SpiceSegment::SVector SpiceSegment::SCLKStopIntervals() const {
  return (SVector(1, _times[size(_times)-1]));
}

/**
 * @brief Returns the SCLK tick rates of intervals in segments
 *
 * This routine is needed for writing CK kernels.  It is assumed that the
 * complete segment is one interval, thus this implementation will return a
 * 1-element vector with a single tick rate.
 *
 * The tick rate is determined by taking the SCLK of the first quaternion,
 * adding one tick to it and converting the two times to ET.  The difference of
 * the two ET times is the tick rate.  This determination is done at import
 * time.
 *
 *
 * @return SpiceSegment::SVector& Vector of tick rates for each interval
 */
SpiceSegment::SVector SpiceSegment::TickRate() const {
  return (SVector(1, _tickRate));
}

QString SpiceSegment::getKeyValue(PvlObject &label,
                                      const QString &keyword) {
  QString value("");
  if ( label.hasKeyword(keyword,Pvl::Traverse) ) {
    value = label.findKeyword(keyword,Pvl::Traverse)[0];
  }
  return (value);
}

void SpiceSegment::import(Cube &cube, const QString &tblname) {

  _fname = cube.fileName();

  //  Extract ISIS CK blob and transform to CK 3 content
  NaifStatus::CheckErrors();
  try {

    // Order is somewhat important here.  The call to initialize Kernels
    // object checks the NAIF pool for existance.  It logs their NAIF
    // status as loaded which may cause trouble from here on...
    Pvl *label = cube.label();
    _kernels.Init(*label);

    Camera *camera;
    // Remove the ideal camera instrument group and rename the
    // OriginalInstrument group to Instrument for the scope of this
    // application.  Only label manipulation occurs and no change to the
    // pixels.
    if (label->findObject("IsisCube").hasGroup("OriginalInstrument")) {
      label->findObject("IsisCube").deleteGroup("Instrument");
      Isis::PvlGroup inst =
          label->findObject("IsisCube").findGroup("OriginalInstrument",
          Isis::Pvl::Traverse);
      inst.setName("Instrument");
      label->findObject("IsisCube").addGroup(inst);
      camera = CameraFactory::Create(*label);
    }
    else {
      camera = cube.camera();
    }

    //  Determine segment ID from product ID if it exists, otherwise basename
    if ( _name.isEmpty() ) {
      _name = getKeyValue(*label, "ProductId");
      if (_name.isEmpty() ) {
        _name = FileName(_fname).baseName();
      }
    }

    QString value("");
    value = getKeyValue(*label, "InstrumentId");
    if (!value.isEmpty()) { _instId = value; }
    value = getKeyValue(*label, "TargetName");
    if (!value.isEmpty()) { _target = value; }
     _camVersion = _kernels.CameraVersion();

    //  Get the SPICE data
    Table ckCache = camera->instrumentRotation()->LineCache(tblname);
    SMatrix spice = load(ckCache);

    _quats = getQuaternions(spice);
    _avvs = getAngularVelocities(spice);
    _times = getTimes(spice);

    _startTime = _times[0];
    _endTime = _times[size(_times)-1];

    // Load necesary kernels (IAK for Cassini, mainly)
    _kernels.Load("FK,SCLK,LSK,IAK");

    //  Here's where all the heavy lifting occurs.
    SMatSeq lmats, rmats;
    SVector sclks;
    getRotationMatrices(cube, *camera, ckCache, lmats, rmats, sclks);
    SMatrix ckQuats, ckAvvs;
    convert(_quats, _avvs, lmats, rmats, ckQuats, ckAvvs);

    // Compute small increment to pad each end
    const double Epsilon(3.0e-3);
    double topSclk = ETtoSCLK(camera->naifSclkCode(), _times[0] - Epsilon);
    double botSclk = ETtoSCLK(camera->naifSclkCode(),
                              _times[size(_times)-1] + Epsilon);

    // Pad the top and bottom of the CK data.  This copies the top and bottom
    //  contents of the data - that will work for us.
    ckQuats = expand(1, 1, ckQuats);
    if ( size(ckAvvs) > 0 ) ckAvvs = expand(1, 1, ckAvvs);
    sclks   = expand(1, 1, sclks);

    //  Finally, adjust the top and bottom times by pad time
    sclks[0]             = topSclk;
    sclks[size(sclks)-1] = botSclk;

    // Replace contents with converted quaternions, angular velocities (if
    // they exist) and sclk times.
    _quats = ckQuats;
    _avvs =  ckAvvs;
    _times = sclks;

    _startTime = SCLKtoET(camera->naifSclkCode(),_times[0]);
    _endTime = SCLKtoET(camera->naifSclkCode(),_times[size(_times)-1]);

    _utcStartTime = toUTC(startTime());
    _utcEndTime   = toUTC(endTime());
    _kernels.UnLoad("FK,SCLK,LSK,IAK");

  } catch ( IException &ie  ) {
    ostringstream mess;
    mess << "Failed to construct CK content from ISIS file " << _fname;
    throw IException(ie, IException::User, mess.str(), _FILEINFO_);
  }

  return;
}


SpiceSegment::SMatrix SpiceSegment::getQuaternions(const SMatrix &spice) const {
  int nrecs = size(spice);
  SMatrix quats(nrecs, 4);

  for ( int i = 0 ; i < nrecs ; i++ ) {
    for ( int j = 0 ; j < 4 ; j++ ) {
      quats[i][j] = spice[i][j];
    }
  }
  return (quats);
}

SpiceSegment::SMatrix SpiceSegment::getAngularVelocities(const SMatrix &spice)
                                                      const {
  int nrecs = size(spice);
  int fields = spice.dim2();
  //  Must have 8 fields
  if ( fields != 8 ) {
    return (SMatrix(0,0));
  }

  SMatrix avvs(nrecs, 3);
  for ( int i = 0 ; i < nrecs ; i++ ) {
    for ( int j = 0 ; j < 3 ; j++ ) {
      avvs[i][j] = spice[i][j+4];
    }
  }
  return (avvs);
}


SpiceSegment::SVector SpiceSegment::getTimes(const SMatrix &spice) const {
  int nrecs = size(spice);
  SVector etdp(nrecs);
  int tcol = spice.dim2() - 1;

  for ( int i = 0 ; i < nrecs ; i++ ) {
    etdp[i] = spice[i][tcol];
  }
  return (etdp);
}

bool SpiceSegment::getTimeDependentFrameIds(Table &table, int &toId, int &fromId) const {
  // Load the constant and time-based frame traces and mission frame ids
  std::vector<int> tdfids;
  if ( table.Label().hasKeyword("TimeDependentFrames") ) {
    PvlKeyword labelTimeFrames = table.Label()["TimeDependentFrames"];
    for (int i=0; i<labelTimeFrames.size(); i++) {
      tdfids.push_back(toInt(labelTimeFrames[i]));
    }
  }
  else {
    toId = fromId = 0;
    return (false);
  }

  //  Return the needed IDs
  toId = tdfids[0];
  fromId = tdfids[tdfids.size()-1];
  return (true);
}

QString SpiceSegment::getFrameName(int frameid) const {
  SpiceChar frameBuf[40];
  frmnam_c ( (SpiceInt) frameid, sizeof(frameBuf), frameBuf);
  return (QString(frameBuf));
}

SpiceSegment::SMatrix SpiceSegment::getConstantRotation(Table &table) const {
  // Get constant rotation matrix from label
  SMatrix crot(3,3);
  try {
    PvlKeyword conrot = table.Label()["ConstantRotation"];
    SVector rot(9, crot[0]);
    for (int i=0; i < 9 ; i++) {  //  Loop count ensures valid matrices
      rot[i] = toDouble(conrot[i]);
    }
  } catch ( IException &ie ) {
    ostringstream mess;
    mess << "Failed to get rotation (via ConstantRotation keyword) from table "
         << table.Name() << " label. "
         << "  Most likely outdated ISIS file - may need to rerun spiceinit.";
    throw IException(ie, IException::User, mess.str(), _FILEINFO_);
  }
  return (crot);
}

SpiceSegment::SMatrix SpiceSegment::getIdentityRotation(const int &nelements) const {
  // Get constant rotation matrix from label
  SMatrix irot(nelements, nelements, 0.0);
  for (int i=0; i < nelements ; i++) {
    irot[i][i] = 1.0;
  }
  return (irot);
}


/**
 * @brief Retrieve state rotation matrix from kernels
 *
 * This method is invoked when a dynamic frame is encountered.  This condition
 * requires the reloading of kernels to resolve time dependent rotations
 * (MESSENGER is one such instrument).
 *
 * There are two ways the state rotation matrix can be determined.  One is with
 * the sxform_c NAIF routine that returns a 6x6 state matrix sufficient to
 * transform quaternions and angular velocity vectors if they exist in the
 * CK kernel. If angular velocites do not exist in the CK, then pxform_c is used
 * to get the 3x3 rotation matrix and then rav2fx_c is used to create the 6x6
 * state matrix with a constant angular velocity (set to 0.0).
 *
 * At any rate, a 6x6 state matrix is returned upon success.
 *
 * @param frame1 Frame name of quaternion as stored in ISIS blob
 * @param frame2 Frame name of desired state
 * @param etTime Empheris time to acquire state rotation for
 *
 * @return SpiceSegment::SMatrix A 6x6 state rotation matrix
 */
SpiceSegment::SMatrix SpiceSegment::computeStateRotation(const QString &frame1,
                                                         const QString &frame2,
                                                         double etTime) const {
  SMatrix state(6,6);
  NaifStatus::CheckErrors();
  try {
    // Get pointing w/AVs
    sxform_c(frame1.toAscii().data(), frame2.toAscii().data(), etTime,
             (SpiceDouble (*)[6]) state[0]);
    NaifStatus::CheckErrors();
  } catch ( IException & ) {
    try {
      SMatrix rot(3,3);
      pxform_c(frame1.toAscii().data(), frame2.toAscii().data(), etTime,
               (SpiceDouble (*)[3]) rot[0]);
      NaifStatus::CheckErrors();
      SVector av(3, 0.0);
      rav2xf_c((SpiceDouble (*)[3]) rot[0], &av[0],
               (SpiceDouble (*)[6]) state[0]);
    } catch ( IException &ie ) {
      ostringstream mess;
      mess << "Could not get dynamic state for time " << etTime;
      throw IException(ie, IException::User, mess.str(), _FILEINFO_);
    }
  }
  return (state);
}


void SpiceSegment::getRotationMatrices(Cube &cube, Camera &camera, Table &table,
                                       SMatSeq &lmats, SMatSeq &rmats,
                                       SVector &sclks) {


  int LtoId, LfromId;
  if ( !getTimeDependentFrameIds(table, LtoId, LfromId) ) {
    QString mess = "Cannot determine time dependent frames! - perhaps a spiceinit is in order.";
    throw IException(IException::User, mess, _FILEINFO_);
  }

  int toId = camera.CkFrameId();
  int fromId = camera.CkReferenceId();

  // Set CK instrument code
  _instCode = toId;

  // Load SPICE and extract necessary contents
  Spice mySpice(*cube.label(), true);  // load w/out tables

  QString CLtoId = getFrameName(LtoId);
  QString CtoId = getFrameName(toId);
  _instFrame = CtoId;
  QString CfromId = getFrameName(fromId);
  _refFrame = CfromId;
  QString CLfromId = getFrameName(LfromId);

  SMatSeq lmat(size(_times)), rmat(size(_times)), avr(size(_times));
  for ( int i = 0 ; i < size(_times) ; i++ ) {
    SMatrix left = computeStateRotation(CtoId, CLtoId, _times[i]);
    SMatrix right = computeStateRotation(CfromId, CLfromId, _times[i]);
    lmat[i] = left;
    rmat[i] = right;
  }

  lmats = lmat;
  rmats = rmat;
  sclks = convertTimes(camera.naifSclkCode(), _times);

  return;
}

const SpiceSegment::SMatrix &SpiceSegment::getMatrix(const SpiceSegment::SMatSeq &seq,
                                                     const int &nth) const {
  if ( (nth < size(seq)) && (nth >= 0)  ) {
    return (seq[nth]);
  }
  return (seq[0]);
}

SpiceSegment::SVector SpiceSegment::convertTimes(
                                          int sclkCode,
                                          const SpiceSegment::SVector &etTimes
                                                ) {
  SVector sclks(size(etTimes));
  for ( int i  = 0 ; i < size(etTimes) ; i++ ) {
    sce2c_c(sclkCode, etTimes[i], &sclks[i]);
  }

  //  Determine the tick rate in case we need to create a type 2 CK
  SpiceDouble et0, et1;
  sct2e_c(sclkCode, sclks[0], &et0);
  sct2e_c(sclkCode, sclks[0]+1.0, &et1);
  _tickRate = fabs(et1 - et0);

  _utcStartTime = toUTC(startTime());
  _utcEndTime   = toUTC(endTime());

  return (sclks);
}





void SpiceSegment::convert(const SpiceSegment::SMatrix &quats,
                           const SpiceSegment::SMatrix &avvs,
                           const SpiceSegment::SMatSeq &lmats,
                           const SpiceSegment::SMatSeq &rmats,
                           SpiceSegment::SMatrix &ckQuats,
                           SpiceSegment::SMatrix &ckAvvs) const {

  ckQuats = SMatrix(quats.dim1(), quats.dim2());
  ckAvvs = SMatrix(avvs.dim1(), avvs.dim2());

  SpiceDouble m[3][3];
  SpiceDouble xform[6][6];
  SpiceDouble mout[6][6];
  SpiceDouble avZero[3] = { 0.0, 0.0, 0.0 };
  SpiceDouble avRav[3];
  ConstSpiceDouble *avIn(&avZero[0]);
  SpiceDouble *avOut(&avRav[0]); // For no AVs
  bool hasAv = (size(avvs) > 0);

  for ( int i = 0 ; i < size(quats) ; i++ ) {
    //  Handle option angular velocities
    if ( hasAv ) {
      avIn = avvs[i];
      avOut = ckAvvs[i];
    }

    // Convert quaternion to rotation and then to state matrix
    q2m_c(quats[i], m);
    rav2xf_c(m, avIn, xform);

    // Do the left and right multiplies
    mxmg_c(getMatrix(lmats, i)[0], xform, 6, 6, 6, mout);
    mxmg_c(mout, getMatrix(rmats, i)[0], 6, 6, 6, xform);

    // Transform to output format
    xf2rav_c(xform, m, avOut);  // Transfers AV to output ckAvvs
    m2q_c(m, ckQuats[i]);       // Transfers quaternion
  }
  return;
}


QString SpiceSegment::getComment() const {
  ostringstream comment;

  FileName fname(_fname);

  comment <<
"\n-----------------------------------------------------------------------\n" <<
"  File:       " << fname.name() << endl <<
"  ProductId:  " << _name << endl <<
"  StartTime:  " << _utcStartTime << endl <<
"  EndTime:    " << _utcEndTime << endl <<
"  Instrument: " << _instId << endl <<
"  Target:     " << _target << endl <<
"  InstFrame:  " << _instFrame << endl <<
"  RefFrame:   " << _refFrame << endl <<
"  Records:    " << size() << endl;

  QString hasAV = (size(_avvs) > 0) ? "YES" : "NO";
  comment <<
"  HasAV:      " << hasAV << endl;

  comment <<
"  CamVersion: " << _camVersion << endl;
  QStringList klist = _kernels.getKernelList();
  if ( klist.size() > 0 ) {
    comment <<
"  Kernels:    \n";
    for ( int i = 0 ; i < klist.size() ; i++  ) {
      comment <<
"    " << klist[i] << endl;
    }
  }

  return (QString(comment.str().c_str()));
}

void SpiceSegment::init() {
  _camVersion = 1;
  _name = _fname = "";
  _startTime = _endTime  = 0.0;
  _utcStartTime = _utcEndTime = "";
  _instId = _target = "UNKNOWN";
  _instCode = 0;
  _instFrame = _refFrame = "";
  _quats = _avvs = SMatrix(0,0);
  _times = SVector(0);
  _tickRate = 0.0;
  return;
}

SpiceSegment::SMatrix SpiceSegment::load(Table &table) {

//  Allocate the internal cache and transfer.
  //  Makes some assumptions about the format of the SPICE table in that
  //  all fields are double.
  int nrecs = table.Records();
  TableRecord &rec = table[0];
  int nvals = rec.Fields();

  // Ensure the table has the expected format, error out if not valid.
  if ( !((nvals == 8) || (nvals == 5)) ) {
    ostringstream mess;
    mess << "SPICE (CK) Table " << table.Name()
         << " must have 8 (with angular velocities) or 5 fields but has "
         << nvals;
    throw IException(IException::User, mess.str(), _FILEINFO_);
  }

  // Extract contents
  SMatrix spice = SMatrix(nrecs,nvals);
  for ( int i = 0 ; i < nrecs ; i++ ) {
    TableRecord &rec = table[i];
    for ( int f = 0 ; f < rec.Fields() ; f++ ) {
      TableField &field = rec[f];
      spice[i][f] = (double) field;
    }
  }
  return (spice);
}


/**
 * @brief Add elements to top and bottom of a matrix
 *
 * This method is to expand a matrix to add additional records for padding
 * purposes. The parameter ntop indicate the number to add to the top of the
 * matrix. nbot indicates the number to add to the bottom
 *
 * Elements added to the top have the contents of the first element of the
 * input matrix copied to it. Elements added to the bottom have the last
 * element copied to it.
 *
 * The new matrix has the contents of the original copied to it place
 * immediately after the number of elements added to it.
 *
 * @author Kris Becker - 4/6/2011
 *
 * @param ntop Number of elements to add to the top
 * @param vec  number of elements to add to the bottom
 * @param matrix Matrix to add elements to
 *
 *  @return SpiceSegment::SMatrix Expanded matrix
 */
SpiceSegment::SMatrix SpiceSegment::expand(int ntop, int nbot,
                                           const SpiceSegment::SMatrix &matrix)
                                           const {
  //  Add lines to matrix at top and bottom
  int ndim(matrix.dim1());
  ntop = std::max(0, ntop);
  nbot = std::max(0, nbot);
  int nlines(ndim+ntop+nbot);
  SMatrix mat(nlines, matrix.dim2());


  // Duplicate top lines from first input matrix line
  for (int n = 0 ; n < ntop ; n++) {
    for (int s = 0 ; s < matrix.dim2() ; s++) {
      mat[n][s] = matrix[0][s];
    }
  }

  // Copy the contents of the input matrix to the output
  for (int n = 0 ; n < ndim ; n++) {
    for (int s = 0 ; s < matrix.dim2() ; s++) {
      mat[n+ntop][s] = matrix[n][s];
    }
  }

  // Duplicate bottom lines from last input matrix lines
  for (int n = 0 ; n < nbot ; n++) {
    for (int s = 0 ; s < matrix.dim2() ; s++) {
      mat[nlines-1-n][s] = matrix[ndim-1][s];
    }
  }

  return (mat);
}

/**
 * @brief Add elements to top and bottom of a vector
 *
 * This method is to expand a vector to add additional records for padding
 * purposes. The parameter ntop indicate the number to add to the top of the
 * vector. nbot indicates the number to add to the bottom
 *
 * Elements added to the top have the contents of the first element of the
 * input vector copied to it. Elements added to the bottom have the last
 * element copied to it.
 *
 * The new vector has the contents of the original copied to it place
 * immediately after the number of elements added to it.
 *
 * @author Kris Becker - 4/6/2011
 *
 * @param ntop Number of elements to add to the top
 * @param vec  number of elements to add to the bottom
 * @param vector Vector to add elements to
 *
 *  @return SpiceSegment::SVector Expanded vector
 */
SpiceSegment::SVector SpiceSegment::expand(int ntop, int nbot,
                                           const SpiceSegment::SVector &vec)
                                           const {
  //  Add lines to matrix at top and bottom
  int ndim(vec.dim1());
  ntop = std::max(0, ntop);
  nbot = std::max(0, nbot);
  int nvals(ndim+ntop+nbot);
  SVector myvec(nvals);

  // Duplicate top elements to expanded elements
  for (int n = 0 ; n < ntop ; n++) {
    myvec[n] = vec[0];
  }

  // Copy elements from input vector to output
  for (int n = 0 ; n < vec.dim1() ; n++) {
    myvec[n+ntop] = vec[n];
  }


  // Duplicate bottom elements to expanded elements
  for (int n = 0 ; n < nbot ; n++) {
    myvec[nvals-1-n] = vec[ndim-1];
  }

  return (myvec);
}

double SpiceSegment::SCLKtoET(SpiceInt scCode, double sclk) const {
  SpiceDouble et;
  sct2e_c(scCode, sclk, &et);
  return (et);
}

double SpiceSegment::ETtoSCLK(SpiceInt scCode, double et) const {
  SpiceDouble sclk;
  sce2c_c(scCode, et, &sclk);
  return (sclk);
}

QString SpiceSegment::toUTC(const double &et) const {
  const int UTCLEN = 80;
  char utcout[UTCLEN];
  et2utc_c(et, "ISOC", 3, UTCLEN, utcout);
  return (QString(utcout));
}

};  // namespace Isis

