#include "Isis.h"

#include "CameraPointInfo.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Displacement.h"
#include "Filename.h"
#include "iException.h"
#include "iString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Progress.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "SerialNumberList.h"
#include "Statistics.h"
#include "SurfacePoint.h"
#include "TextFile.h"
#include "UserInterface.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <string>

using namespace std;
using namespace Isis;

iString CheckValue(double value);
iString CheckValue(iString value);
void Write(PvlGroup *point, const ControlMeasure &cm);

// Allows for column names to be written on the first pass
bool isFirst;
bool append;

TextFile *txt = NULL;

// For control measure related data and labels
iString measureInfo;
QString measureLabels;

void IsisMain() {
  isFirst = true;
  append = false;
  bool outside = false;
  bool errors = false;
  measureInfo = "";
  measureLabels = "";

  Progress prog;

  // Get user entered information
  UserInterface & ui = Application::GetUserInterface();
  ControlNet cnet(ui.GetFilename("CNET"));
  SerialNumberList serials(ui.GetFilename("FROMLIST"));
  append = ui.GetBoolean("APPEND");

  if(cnet.GetNumMeasures() == 0) {
    string msg = "Your control network must contain at least one point";
    throw iException::Message(iException::User, msg, _FILEINFO_);
  }

  prog.SetMaximumSteps(cnet.GetNumMeasures());

  // If append is true, output will be appended or a new file created
  if (append) {
    // Check to see if its a new file or we open an existing file
    Filename file(ui.GetFilename("FLATFILE"));
    if (!file.Exists()) {
      // It is new, so we aren't appending
      // Set this because it is used elsewhere
      append = false;
    }
    txt = new TextFile(ui.GetFilename("FLATFILE"), "append");
  }
  // Without append, if the files exists it will be overwritten
  else {
    txt = new TextFile(ui.GetFilename("FLATFILE"), "overwrite");
  }

  PvlGroup * grp = NULL;
  CameraPointInfo camPoint;

  outside = ui.GetBoolean("ALLOWOUTSIDE");
  errors = ui.GetBoolean("ALLOWERRORS");

  // Loop through all points in controlnet
  for (int i = 0; i < cnet.GetNumPoints(); i++) {
    const ControlPoint * cpoint = cnet[i];

    if (isFirst && !append) {
      measureLabels += "ControlPointId,";
      measureLabels += "PointType,";
      measureLabels += "ChooserName,";
      measureLabels += "DateTime,";
      measureLabels += "EditLock,";
      measureLabels += "Ignored,";

      measureLabels += "AprioriSurfacePointSource,";
      measureLabels += "AprioriSurfacePointSourceFile,";

      measureLabels += "AprioriRadiusSource,";
      measureLabels += "AprioriRadiusSourceFile,";

      measureLabels += "AprioriX,";
      measureLabels += "AprioriY,";
      measureLabels += "AprioriZ,";
      measureLabels += "AprioriXSigma,";
      measureLabels += "AprioriYSigma,";
      measureLabels += "AprioriZSigma,";
      measureLabels += "AprioriLatitude,";
      measureLabels += "AprioriLongitude,";
      measureLabels += "AprioriLocalRadius,";
      measureLabels += "AprioriLatitudeSigma,";
      measureLabels += "AprioriLongitudeSigma,";
      measureLabels += "AprioriLocalRadiusSigma,";
      measureLabels += "AprioriLatitudeSigmaDistance,";
      measureLabels += "AprioriLongitudeSigmaDistance,";

      measureLabels += "AdjustedX,";
      measureLabels += "AdjustedY,";
      measureLabels += "AdjustedZ,";
      measureLabels += "AdjustedXSigma,";
      measureLabels += "AdjustedYSigma,";
      measureLabels += "AdjustedZSigma,";
      measureLabels += "AdjustedLatitude,";
      measureLabels += "AdjustedLongitude,";
      measureLabels += "AdjustedLocalRadius,";
      measureLabels += "AdjustedLatitudeSigma,";
      measureLabels += "AdjustedLongitudeSigma,";
      measureLabels += "AdjustedLocalRadiusSigma,";
      measureLabels += "AdjustedLatitudeSigmaDistance,";
      measureLabels += "AdjustedLongitudeSigmaDistance,";

      measureLabels += "MinimumResidual,";
      measureLabels += "MaximumResidual,";
      measureLabels += "AverageResidual,";
      measureLabels += "MinimumSampleResidual,";
      measureLabels += "MaximumSamlpeResidual,";
      measureLabels += "MinimumLineResidual,";
      measureLabels += "MaximumLineResidual,";
    }

    // Always add data
    measureInfo.clear();
    measureInfo += cpoint->GetId() + ",";
    measureInfo += cpoint->GetPointTypeString() + ",";
    measureInfo += iString(cpoint->GetChooserName()) + ",";
    measureInfo += iString(cpoint->GetDateTime()) + ",";
    measureInfo += iString(cpoint->IsEditLocked()) + ",";
    measureInfo += iString(cpoint->IsIgnored()) + ",";

    measureInfo += iString(cpoint->GetSurfacePointSourceString()) + ",";
    measureInfo += iString(cpoint->GetAprioriSurfacePointSourceFile()) + ",";
    measureInfo += iString(cpoint->GetRadiusSourceString()) + ",";
    measureInfo += iString(cpoint->GetAprioriRadiusSourceFile()) + ",";

    SurfacePoint Asp = cpoint->GetAprioriSurfacePoint();
    measureInfo += iString(CheckValue(Asp.GetX().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(Asp.GetY().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(Asp.GetZ().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(Asp.GetXSigma().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(Asp.GetYSigma().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(Asp.GetZSigma().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(Asp.GetLatitude().GetPlanetocentric(Angle::Degrees))) + ",";
    measureInfo += iString(CheckValue(Asp.GetLongitude().GetPositiveEast(Angle::Degrees))) + ",";
    measureInfo += iString(CheckValue(Asp.GetLocalRadius().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(Asp.GetLatSigma().GetDegrees())) + ",";
    measureInfo += iString(CheckValue(Asp.GetLonSigma().GetDegrees())) + ",";
    measureInfo += iString(CheckValue(Asp.GetLocalRadiusSigma().GetKilometers())) + ",";
    try { measureInfo += iString(CheckValue(Asp.GetLatSigmaDistance().GetKilometers())) + ","; }
    catch (iException e) {
      e.Clear();
      measureInfo += ",";
    }
    try { measureInfo += iString(CheckValue(Asp.GetLonSigmaDistance().GetKilometers())) + ","; }
    catch (iException e) {
      e.Clear();
      measureInfo += ",";
    }

    SurfacePoint sp = cpoint->GetAdjustedSurfacePoint();
    measureInfo += iString(CheckValue(sp.GetX().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(sp.GetY().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(sp.GetZ().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(sp.GetXSigma().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(sp.GetYSigma().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(sp.GetZSigma().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(sp.GetLatitude().GetPlanetocentric(Angle::Degrees))) + ",";
    measureInfo += iString(CheckValue(sp.GetLongitude().GetPositiveEast(Angle::Degrees))) + ",";
    measureInfo += iString(CheckValue(sp.GetLocalRadius().GetKilometers())) + ",";
    measureInfo += iString(CheckValue(sp.GetLatSigma().GetDegrees())) + ",";
    measureInfo += iString(CheckValue(sp.GetLonSigma().GetDegrees())) + ",";
    measureInfo += iString(CheckValue(sp.GetLocalRadiusSigma().GetKilometers())) + ",";
    try { measureInfo += iString(CheckValue(sp.GetLatSigmaDistance().GetKilometers())) + ","; }
    catch (iException e) {
      e.Clear();
      measureInfo += ",";
    }
    try { measureInfo += iString(CheckValue(sp.GetLonSigmaDistance().GetKilometers())) + ","; }
    catch (iException e) {
      e.Clear();
      measureInfo += ",";
    }

    measureInfo += iString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetResidualMagnitude).Minimum())) + ",";
    measureInfo += iString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetResidualMagnitude).Maximum())) + ",";
    measureInfo += iString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetResidualMagnitude).Average())) + ",";
    measureInfo += iString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetSampleResidual).Minimum())) + ",";
    measureInfo += iString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetSampleResidual).Maximum())) + ",";
    measureInfo += iString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetLineResidual).Minimum())) + ",";
    measureInfo += iString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetLineResidual).Maximum())) + ",";
    
    // Loop through all measures in controlpoint
    for(int j = 0; j < cpoint->GetNumMeasures(); j++) {

      const ControlMeasure * cmeasure = (*cpoint)[j];

      // Set and then get CameraPointInfo information
      camPoint.SetCube(serials.Filename(cmeasure->GetCubeSerialNumber()));

      grp = camPoint.SetImage(cmeasure->GetSample(), cmeasure->GetLine(), outside, errors);
      // Shouldn't ever happen, but, being safe...
      if(grp == NULL) {
        string msg = "You shouldn't have gotten here. Errors in CameraPointInfo class";
        throw iException::Message(iException::Programmer, msg, _FILEINFO_);
      }
      Write(grp, *cmeasure);
      delete grp;
      grp = NULL;
    }

    // Making progress!
    prog.CheckStatus();
  }

  // All done, clean up
  prog.CheckStatus();
  if(txt != NULL) {
    delete txt;
    txt = NULL;
  }
}

// This function is meant to check a value, and, if it is a special pixel
// return " " instead of the special pixel value.
iString CheckValue(double value) {
  if (IsSpecial(value)) {
    return iString("");
  }
  else {
    return CheckValue(iString(value));
  }
}

iString CheckValue(iString value) {
  if (value == iString(Isis::Null) ||
      value == iString(Isis::Hrs) || 
      value == iString(Isis::His) || 
      value == iString(Isis::Lrs) || 
      value == iString(Isis::Lis)) {
    return iString("");
  }
  else {
    return value;
  }
}

iString CheckValue(QString value) {
  if (value == QString::number(Isis::Null) ||
      value == QString::number(Isis::Hrs) || 
      value == QString::number(Isis::His) || 
      value == QString::number(Isis::Lrs) || 
      value == QString::number(Isis::Lis)) {
    return iString("");
  }
  else {
    return value.toStdString();
  }  
}

// Write each PvlGroup out to file
void Write(PvlGroup *point, const ControlMeasure &cm) {

  // QStrings are used QString here because of ControlMeasure returning
  // QStrings. There is some monkey motion involving ingesting doubles,
  // this is necessary as iString will accept doubles but QString won't.

  QString output = "";
  QVector < iString > dataNames;

  // Do we have errors?
  int maxCount = 0;
  bool errors = point->HasKeyword("Error");
  if(errors) {
    maxCount = point->Keywords() - 1;
  }
  else {
    maxCount = point->Keywords();
  }

  // If its first and not appending, write the column labels
  if(isFirst && !append) {

    QList< QStringList > printableMeasureData = cm.PrintableClassData();
    QStringList nameValuePair;
    foreach (nameValuePair, printableMeasureData) {
      output += nameValuePair.at(0) + ",";
    }
    
    // point information
    for(int i = 0; i < maxCount; i++) {
      if((*point)[i].Size() == 3) {
        output += QString((*point)[i].Name().c_str()) + "X,";
        output += QString((*point)[i].Name().c_str()) + "Y,";
        output += QString((*point)[i].Name().c_str()) + "Z,";
      }
      else {
        output += QString((*point)[i].Name().c_str()) + ",";
      }
    }

    // control measure information
    //dataNames = cm.GetMeasureDataNames();
    //for(int i = 0; i < dataNames.size(); i++) {
    //  output += iString(dataNames[i] + ",");
    //}


    if(errors) output += QString((*point)[maxCount].Name().c_str());
    isFirst = false;
    measureLabels += output;
    txt->PutLine(measureLabels.toStdString());
  }
  output.clear();
  measureLabels.clear();

  QList< QStringList > printableMeasureData = cm.PrintableClassData();
  QStringList nameValuePair;
  foreach (nameValuePair, printableMeasureData) {
    output += CheckValue(nameValuePair.at(1)).ToQt() + ",";
  }

  // Write out date values
  // point information
  for(int i = 0; i < maxCount; i++) {
    if((*point)[i].Size() == 3) {
      output += QString(CheckValue((*point)[i][0])) + ",";
      output += QString(CheckValue((*point)[i][1])) + ",";
      output += QString(CheckValue((*point)[i][2])) + ",";
    }
    else {
      output += QString(CheckValue((*point)[i][0])) + ",";
    }
  }

  //dataNames = cm.GetMeasureDataNames();
  //for(int i = 0; i < dataNames.size(); i++) {
  //  output += iString(cm.GetMeasureData(dataNames[i])) + ",";
  //}

  

  if(errors) output += QString((*point)[maxCount][0]);

  // Meaure info comes first
  QString pri = "";
  pri += measureInfo.ToQt();
  pri += output;

  txt->PutLine(pri.toStdString());

  output.clear();
  pri.clear();
}
