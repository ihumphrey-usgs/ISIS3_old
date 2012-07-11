#include "Isis.h"

#include "Camera.h"
#include "CameraDetectorMap.h"
#include "CameraFactory.h"
#include "CSVReader.h"
#include "FileList.h"
#include "iTime.h"
#include "LineScanCameraDetectorMap.h"
#include "Pipeline.h"
#include "ProgramLauncher.h"
#include "SpecialPixel.h"
#include "TextFile.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

vector<iString> redFiles;
vector<iString> tempFiles;
int firstFilter;
int numFiles;
FileName FindRed(FileList &inList, int n);
void ProcessNoprojFiles(Pipeline &p);

void GetCropLines(const string inFile, double eTime1, double eTime2, int & line1, int & line2, int & numLines);
void GetEphemerisTimeFromJitterFile(const string jitterFile, double & eTime1, double & eTime2);

// avgOffsets[i][Sample = 0, Line = 1]
// where i is in the table just above
// lineOff's declaration
double avgOffsets[9][2];

/*
      lineoff table
------------------------
 i | ccd-ccd | lineoff[i]
------------------------
 0 |   0-1   |  574
 1 |   1-2   | -622
 2 |   2-3   |  620
 3 |   3-4   | -586
 4 |   4-5   |  584
 5 |   5-6   | -600
 6 |   6-7   |  597
 7 |   7-8   | -576
 8 |   8-9   |  607
*/
const double lineOff[9] = {
  574,
  -622,
  620,
  -586,
  584,
  -600,
  597,
  -576,
  607
};

void IsisMain() {
  // Initialize globals
  redFiles.clear();
  tempFiles.clear();

  // Get user interface
  UserInterface &ui = Application::GetUserInterface();

  FileList inputList(ui.GetFileName("FROMLIST"));

  // We dont actually need all 10... so dont do this exception
  //if(inputList.size() != 10) {
  //  iString msg = "Input list file must have 10 entries, one for each CCD (0 to 9)";
  //  throw IException(IException::User, msg, _FILEINFO_);
  //}

  int masterFileNum = ui.GetInteger("MASTER");

  numFiles = inputList.size();

  // This will initialize our global variables
  FindRed(inputList, masterFileNum);

  if(masterFileNum - firstFilter > numFiles || masterFileNum - firstFilter < 0) {
    iString msg = "Input list does not contain the MASTER [RED" + iString(masterFileNum) + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  Pipeline matchfilePipeline("hijitter - match");

  matchfilePipeline.SetInputFile(FindRed(inputList, masterFileNum));
  matchfilePipeline.SetOutputFile(FileName("$TEMPORARY/matchMaster.cub"));
  tempFiles.push_back(FileName("$TEMPORARY/matchMaster.cub").expanded());

  matchfilePipeline.KeepTemporaryFiles(false);

  matchfilePipeline.AddToPipeline("cubeatt");
  matchfilePipeline.Application("cubeatt").SetInputParameter("FROM", true);
  matchfilePipeline.Application("cubeatt").SetOutputParameter("TO", "copy");

  matchfilePipeline.AddToPipeline("spiceinit");
  matchfilePipeline.Application("spiceinit").SetInputParameter("FROM", false);
  matchfilePipeline.Application("spiceinit").AddConstParameter("ATTACH", "NO");

  matchfilePipeline.AddToPipeline("spicefit");
  matchfilePipeline.Application("spicefit").SetInputParameter("FROM", false);

  matchfilePipeline.Run();

  Pipeline p("hijitter");

  p.SetInputListFile("FROM");
  p.SetOutputFile(FileName("$TEMPORARY/noproj"));

  for(int i = 0; i < numFiles; i++) {
    tempFiles.push_back(
      FileName("$TEMPORARY/noproj.FROM" + iString(i + 1) + ".noproj.cub").expanded()
    );
  }

  p.KeepTemporaryFiles(false);

  p.AddToPipeline("cubeatt");
  p.Application("cubeatt").SetInputParameter("FROM", true);
  p.Application("cubeatt").SetOutputParameter("TO", "copy");

  p.AddToPipeline("spiceinit");
  p.Application("spiceinit").SetInputParameter("FROM", false);
  p.Application("spiceinit").AddConstParameter("ATTACH", "NO");

  p.AddToPipeline("appjit");
  p.Application("appjit").SetInputParameter("FROMLIST", PipelineApplication::LastAppOutputListNoMerge, false);
  p.Application("appjit").AddParameter("JITTER", "JITTER");
  p.Application("appjit").AddParameter("DEGREE", "DEGREE");

  if (ui.WasEntered("JITTERCK"))  p.AddPause();

  p.AddToPipeline("noproj");
  p.Application("noproj").SetInputParameter("FROM", true);
  p.Application("noproj").AddConstParameter("MATCH", FileName("$TEMPORARY/matchMaster.cub").expanded());
  p.Application("noproj").SetOutputParameter("TO", FileName("$TEMPORARY/noproj").expanded());

  p.Prepare();

  iString masterFile = p.Application("cubeatt").GetOutputs()[masterFileNum - firstFilter];
  p.Application("appjit").AddConstParameter("MASTER", masterFile);
  p.Run();

  if (ui.WasEntered("JITTERCK"))  p.Run();

  // the outputs are temporary files
  for(int redNum = 0; redNum < numFiles; redNum++)
    tempFiles.push_back(FileName("$TEMPORARY/noproj.FROM" + iString(redNum + 1) + ".cub").expanded());

  // Do some calculations, delete the final outputs from the pipeline
  ProcessNoprojFiles(p);

  p.SetOutputListFile("TO");
  p.Application("noproj").SetOutputParameter("TO", "jitter");

  p.Prepare();

  masterFile = p.Application("cubeatt").GetOutputs()[masterFileNum - firstFilter];
  p.Application("appjit").AddConstParameter("MASTER", masterFile);

  p.Run();

  if (ui.WasEntered("JITTERCK")) {
    iString params = "FROM=" + masterFile + " TO=" + ui.GetFileName("JITTERCK");

    try {
      ProgramLauncher::RunIsisProgram("ckwriter", params);
    }
    catch(IException &e) {
      iString message = "Creation of the output ck, " +
        ui.GetFileName("JITTERCK") + " failed.";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    p.Run();
  }

  // Crop the lines using the jitter file if crop is ebabled
  if(ui.GetBoolean("CROP")) {
    double eTime1, eTime2;
    string jitterFile = ui.GetAsString("JITTER");

    GetEphemerisTimeFromJitterFile(jitterFile, eTime1, eTime2);

    for(int i=0; i<numFiles; i++) {
      int line1=0, line2=0;
      int numLines=0;

      GetCropLines(p.FinalOutput(i).c_str(), eTime1, eTime2, line1, line2, numLines);
      Pipeline pcrop;
      pcrop.KeepTemporaryFiles(false);

      iString tag = "crop" + iString(i);
      string inFile(p.FinalOutput(i).c_str());
      string outFile = "temp_"+tag+".cub";

      pcrop.SetInputFile(FileName(inFile));
      pcrop.SetOutputFile(FileName(outFile));

      pcrop.AddToPipeline("crop", tag);
      pcrop.Application(tag).SetInputParameter ("FROM",   false);
      pcrop.Application(tag).SetOutputParameter("TO",     "crop");
      pcrop.Application(tag).AddConstParameter ("LINE",   line1);
      pcrop.Application(tag).AddConstParameter ("NLINES", numLines);
      pcrop.Run();

      remove(inFile.c_str());
      rename(outFile.c_str(), inFile.c_str());
    }
  }

  for(unsigned int tempFile = 0; tempFile < tempFiles.size(); tempFile++)
    remove(tempFiles[tempFile].c_str());

  tempFiles.clear();
  redFiles.clear();
}

/**
 * This method will initialize global variables when it is
 * called for the first time and will return the filename of CCD n given
 * the file list.
 *
 * @param inList Input file list
 * @param n Red CCD to return
 *
 * @return FileName Name of the CCD file
 */
FileName FindRed(FileList &inList, int n) {
  iString nonMroFile = "";

  if(n > 9 || n < 0) {
    iString msg = "Parameter n must be [0-9] but found [" + iString(n) + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  if(!redFiles.empty() && redFiles[n].empty()) {
    iString msg = "Filter [RED" + iString(n) + "] is not in the input list";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if(!redFiles.empty()) return redFiles[n];
  redFiles.resize(10);
  numFiles = 0;

  int lastRedNum = -1;
  for (int i = 0; nonMroFile.empty() && i < inList.size(); i++) {
    try {
      FileName currentFileName(inList[i]);
      Pvl labels(currentFileName.expanded());
      PvlGroup &inst = labels.FindGroup("Instrument", Pvl::Traverse);

      string redNum = ((string)inst["CcdId"]).substr(3);
      int redNumber = (int)(iString)redNum;

      if(redNumber < 0 || redNumber > 9) {
        iString msg = "CcdId value of [" + redNum + "] found in [" + inList[i].toString() + "] not supported";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      if(lastRedNum == -1) {
        lastRedNum = redNumber;
        firstFilter = redNumber;
        numFiles ++;
      }
      else if(lastRedNum + 1 == redNumber) {
        lastRedNum = redNumber;
        numFiles ++;
      }
      else {
        iString msg = "The input file list must be in order from RED0 to RED9";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      redFiles[redNumber] = inList[i].toString();
    }
    catch(IException &e) {
      nonMroFile = inList[i].toString();
    }
  }

  if(!nonMroFile.empty()) {
    iString message = "File [" + nonMroFile + "] is not a valid MRO cube";
    throw IException(IException::User, message, _FILEINFO_);
  }

  // Look for missing files
  bool foundFirstFile = false;
  bool foundLastFile = false;
  for(unsigned int i = 0; i < redFiles.size(); i++) {
    if(!foundFirstFile && !redFiles[i].empty()) {
      foundFirstFile = true;
    }
    else if(foundFirstFile && !foundLastFile && redFiles[i].empty()) {
      foundLastFile = true;
    }
    else if(foundFirstFile && foundLastFile && !redFiles[i].empty()) {
      iString msg = "Filter [RED" + iString((int)i) + "] is not consecutive";
    }
  }

  return FindRed(inList, n);
}

void ProcessNoprojFiles(Pipeline &p) {
  UserInterface &ui = Application::GetUserInterface();

  // This will be decremented on error, it's easier this way
  int count = numFiles - 1;

  for(int i = 0; i < numFiles - 1; i++) {
    iString tempDir = FileName("$TEMPORARY").expanded();
    iString flatFileLoc = tempDir + "/first" + iString(firstFilter + i) + "-" + iString(firstFilter + i + 1) + ".flat";

    iString params = "FROM=" + tempDir + "/noproj.FROM" + iString(i + 1) + ".cub";
    params += " MATCH=" + tempDir + "/noproj.FROM" + iString(i + 2) + ".cub";
    params += " REGDEF=" + ui.GetFileName("REGDEF");
    params += " FLAT=" + flatFileLoc;

    try {
      ProgramLauncher::RunIsisProgram("hijitreg", params);
    }
    catch(IException &e) {
      count --;
      continue;
    }

    // Read offsets

    TextFile flatFile(flatFileLoc);
    tempFiles.push_back(flatFileLoc);

    string line;
    avgOffsets[i][0] = Isis::Null;
    avgOffsets[i][1] = Isis::Null;

    try {
      while(flatFile.GetLine(line, false) &&
            (avgOffsets[i][0] == Isis::Null || avgOffsets[i][1] == Isis::Null)) {
        line = iString(line).Compress();
        string::size_type pos = line.find("Average Sample Offset: ");

        if(pos != string::npos) {
          // cut off text before our number (start pos + strlen + 1)
          line = line.substr(pos + strlen("Average Sample Offset: "));

          // cut off text after our number
          line = line.substr(0, line.find(" "));

          avgOffsets[i][0] = (double)(iString)line;
          pos = string::npos;
        }

        pos = line.find("Average Line Offset: ");

        if(pos != string::npos) {
          // cut off text before our number (start pos + strlen + 1)
          line = line.substr(pos + strlen("Average Line Offset: "));

          // cut off text after our number
          line = line.substr(0, line.find(" "));

          avgOffsets[i][1] = (double)(iString)line;
          pos = string::npos;
        }
      }
    }
    catch(IException &e) {
      //iString msg = "Unable to find average sample/line offsets in hijitreg results for CCDs [" + iString(i) + "-" + iString(i+1) + "]";
      //throw iException::Message(iException::Programmer, msg, _FILEINFO_);
      count --;
    }
  }

  if(count <= 0) {
    iString msg = "Unable to calculate average sample/line offsets from hijitreg results";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  Pvl labels(FileName("$TEMPORARY/noproj.FROM1.cub").expanded());
  Camera *cam = CameraFactory::Create(labels);

  double lineRate = cam->DetectorMap()->LineRate();

  delete cam;
  cam = NULL;

  // pitchRate (radians/sec) = 0.000001 *
  // sum(averageLineOffset[i]/(lineOff[i] * lineRate) / 9.,
  // where i = (0,8), averageLineOffset[i] is the lineOffset from first0-1.flat,
  // lineOff[i] is listed in table below.
  double pitchRate = 0.0;

  // yaw (radians) = sum( arctan(averageSampleOffset[i])/lineOff[i]) / 9.,
  // where i = (0,8), the angles are small enough that the arctan is optional,
  // averageSampleOffset[i] is the sampleOffset from first0-1.flat,
  double yaw = 0.0;

  for(int i = 0; i < count; i++) {
    if(IsSpecial(avgOffsets[i][0]) || IsSpecial(avgOffsets[i][1])) continue;

    pitchRate += 0.000001 * (avgOffsets[i][1] / (lineOff[i] * lineRate)) / (double)count;
    yaw += atan(avgOffsets[i][0] / lineOff[i]) / (double)count;
  }

  p.Application("appjit").AddConstParameter("PITCHRATE", iString(pitchRate));
  p.Application("appjit").AddConstParameter("YAW", iString(yaw));
}

/**
 * Get the start and end ephemeris time from the jitter file
 *
 * @author Sharmila Prasad (12/20/2011)
 *
 * @param jitterFile - jitter file
 * @param eTime1 - Start time
 * @param eTime2 - End time
 */
void GetEphemerisTimeFromJitterFile(const string jitterFile, double & eTime1, double & eTime2) {
  CSVReader jitter(jitterFile, true, 0, ' ', false, true);
  int iRows = jitter.rows();
  CSVReader::CSVAxis csvArr;

  eTime1=0, eTime2=0;

  double temp=0;
  for (int j=0; j<iRows; j++) {
    csvArr = jitter.getRow(j);
    int iArrSize = csvArr.dim();
    for (int i=0; i<iArrSize; i++) {
      csvArr[i].TrimHead(" \n,");
      csvArr[i].TrimTail(" \n,\t\r");
      temp = iString(csvArr[i]).ToDouble();
      if(!i && temp == 0) {
        break;
      }
      // Ephemeris Time
      if(i==2) {
        if (eTime1 == 0) {
          eTime1 = temp;
        }
        else {
          eTime2 = temp;
        }
      }
    }
  }
}

/**
 * GetCropLines for an image given the start and end ephemeris times
 *
 * @author Sharmila Prasad (12/21/2011)
 *
 * @param inFile   - Input image file
 * @param eTime1   - Ephemeris start time
 * @param eTime2   - Ephemeris end time
 * @param line1    - Calculated start line for cropping
 * @param line2    - Calculated end line for cropping
 * @param numLines - Number of lines in the image to be cropped
 */
void GetCropLines(const string inFile, double eTime1, double eTime2, int & line1, int & line2, int & numLines){

  Cube *inCube = new Cube;
  inCube->open(inFile);

  Camera *cam = inCube->getCamera();

  iTime etTime = cam->Spice::cacheStartTime();
  double etStart = etTime.Et();
  double lineRate = ((LineScanCameraDetectorMap*) cam->DetectorMap())->LineRate();

  int imgLines = inCube->getLineCount();
  line1 = int ((eTime1 - etStart) / lineRate + 0.5 );
  line2 = int ((eTime2 - etStart) / lineRate + 0.5 );

  if (line2 > imgLines)
    numLines = imgLines - line1 + 1;
  else
    numLines = line2 - line1 + 1;

  inCube->close();
  delete(inCube);
}

