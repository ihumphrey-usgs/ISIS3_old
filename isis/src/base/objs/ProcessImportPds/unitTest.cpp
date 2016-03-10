#include "Isis.h"

#include <QFile>

#include "ProcessImportPds.h"
#include "Application.h"
#include "IString.h"
#include "OriginalLabel.h"
#include "Statistics.h"

using namespace std;
using namespace Isis;
/**
 * @internal
 *   @history 2012-05-08 Tracie Sucharski - Moved test data to /usgs/cpks/mer/testData and
 *                         /usgs/cpkgs/clementine1/testData.  Added test for invalid label.
 */
void IsisMain() {

  Isis::Preference::Preferences(true);
  void ReportError(QString err);

  // Test an IMAGE file
  try {
    cout << "Testing PDS file containing an ^IMAGE pointer" << endl;
    Isis::ProcessImportPds p;
    Isis::Pvl plab;
    p.SetPdsFile("$mer/testData/mer.img", "$mer/testData/mer.img", plab);
    p.SetOutputCube("TO");
    p.StartProcess();
    p.EndProcess();

    cout << plab << endl;
    Isis::Process p2;
    Isis::CubeAttributeInput att;
    QString file = Isis::Application::GetUserInterface().GetFileName("TO");
    Isis::Cube *cube = p2.SetInputCube(file, att);
    Isis::Statistics *stat = cube->statistics();
    cout << stat->Average() << endl;
    cout << stat->Variance() << endl;
    p2.EndProcess();
    Isis::OriginalLabel ol(file);
    Isis::Pvl label = ol.ReturnLabels();
    cout << label << endl;
    QFile::remove(file);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  // Test a QUBE file
  try {
    cout << endl;
    cout << "Testing PDS file containing a ^QUBE pointer" << endl;
    Isis::ProcessImportPds p;
    Isis::Pvl plab;
    p.SetPdsFile("$mer/testData/mer.lab", "", plab);
    p.SetOutputCube("TO");
    p.StartProcess();
    p.OmitOriginalLabel();
    p.EndProcess();

    cout << plab << endl;
    Isis::Process p2;
    Isis::CubeAttributeInput att;
    QString file = Isis::Application::GetUserInterface().GetFileName("TO");
    Isis::Cube *cube = p2.SetInputCube(file, att);
    Isis::Statistics *stat = cube->statistics();
    cout << stat->Average() << endl;
    cout << stat->Variance() << endl;
    p2.EndProcess();

    // Check input file error
    try {
      Isis::OriginalLabel ol(file);
    }
    catch(Isis::IException &e) {
      ReportError(e.toString());
    }
    QFile::remove(file);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  // Test an Isis2 file
  try {
    cout << endl;
    cout << "Testing Isis2 file" << endl;
    Isis::ProcessImportPds p;
    Isis::Pvl plab;
    p.SetPdsFile("$clementine1/testData/clemuvvis_Isis2.cub",
                 "$clementine1/testData/clemuvvis_Isis2.cub", plab);
    p.SetOutputCube("TO");
    p.StartProcess();
    Isis::Pvl ilab;
    p.TranslateIsis2Labels(ilab);
    p.EndProcess();

    cout << ilab << endl;
    QString file = Isis::Application::GetUserInterface().GetFileName("TO");
    QFile::remove(file);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try{// this file is saved locally since it is not needed in the data area for 
      // the rest of isis
    cout << "Testing PDS file containing an ^IMAGE pointer and ^TABLE pointer" << endl;
    Isis::ProcessImportPds p;
    Isis::Pvl plab;
    p.SetPdsFile("data/pdsImageWithTables.lbl", "data/pdsImageWithTables.img", plab);
    p.SetOutputCube("TO");
    p.ImportTable("SUN_POSITION_TABLE");
    p.StartProcess();
    p.EndProcess();
    
    cout << plab << endl;
    Isis::Process p2;
    Isis::CubeAttributeInput att;
    QString file = Isis::Application::GetUserInterface().GetFileName("TO");
    Isis::Cube *cube = p2.SetInputCube(file, att);
    Pvl isisCubeLab = *(cube->label());
    (isisCubeLab.findObject("IsisCube").findObject("Core")["StartByte"]).setValue("");
    (isisCubeLab.findObject("Table")["StartByte"]).setValue("");
    (isisCubeLab.findObject("Table")["Bytes"]).setValue("");
    (isisCubeLab.findObject("History")["StartByte"]).setValue("");
    (isisCubeLab.findObject("History")["Bytes"]).setValue("");
    (isisCubeLab.findObject("OriginalLabel")["StartByte"]).setValue("");
    (isisCubeLab.findObject("OriginalLabel")["Bytes"]).setValue("");
    cout << isisCubeLab << endl;
    Isis::Statistics *stat = cube->statistics();
    cout << stat->Average() << endl;
    cout << stat->Variance() << endl;
    p2.EndProcess();
    Isis::OriginalLabel ol(file);
    Isis::Pvl label = ol.ReturnLabels();
    cout << label << endl;
    QFile::remove(file);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  //  Test an invalid label file
  try {
    cout << endl;
    cout << "Testing file with invalid Pds label" << endl;
    Isis::ProcessImportPds p;
    Isis::Pvl plab;
    p.SetPdsFile("$galileo/testData/1213r.img", "$galileo/testData/1213r.img", plab);
  }
  catch (Isis::IException &e) {
    e.print();
  }
}

/**
 * Reports error messages from Isis:iException without full paths of filenames
 * @param err Error QString of iException
 * @author Jeannie Walldren
 * @internal
 *   @history 2011-08-05 Jeannie Backer - Copied from Cube class.
 */
void ReportError(QString err) {
  cout << err.replace(QRegExp("\\[[^\\]]*\\]"), "[]") << endl;
}

