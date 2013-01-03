#include <iostream>

#include <QFileInfo>

#include "IException.h"
#include "Brick.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "LineManager.h"
#include "Pvl.h"
#include "Preference.h"
#include "Histogram.h"
#include "SpecialPixel.h"
#include "Statistics.h"

using namespace std;
using namespace Isis;


int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  try {
    void Report(Cube & c);
    cerr << "Unit test for Cube" << endl;

    cerr << "Constructing cube ... " << endl;
    Cube out;
    Report(out);

    // Test create and write methods
    cerr << "Creating 32-bit cube ... " << endl;
    out.setDimensions(150, 200, 2);
    out.create("IsisCube_00");
    Report(out);

    cerr << "Write cube ... " << endl;
    LineManager line(out);
    long j = 0;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) j;
        j++;
      }
      j--;
      out.write(line);
    }

    // Copy returns the resulting Cube, we don't care about it (but we need it to flush) so delete
    delete out.copy("IsisCube_01", CubeAttributeOutput());
    out.close();

    // Test the open and read methods
    cerr << "Opening cube ... " << endl;
    Cube in;
    in.open("IsisCube_01");
    Report(in);

    cerr << "Comparing cube ... " << endl;
    LineManager inLine(in);
    j = 0;
    for(inLine.begin(); !inLine.end(); inLine++) {
      in.read(inLine);
      for(int i = 0; i < inLine.size(); i++) {
        if(inLine[i] != (double) j) {
          cerr << "Problem at"
               << " line " << inLine.Line()
               << " sample " << i + 1
               << " band " << inLine.Band() << ":  "
               << inLine[i] << " != " << double(j) << endl;
          return 1;
        }
        j++;
      }
      j--;
    }
    in.close();
    cerr << endl;

    // Test other options for output
    cerr << "Creating 8-bit cube ... " << endl;
    Cube out2;
    out2.setDimensions(150, 200, 1);
    out2.setLabelsAttached(false);
    out2.setBaseMultiplier(200.0, -1.0);
//  out2.SetByteOrder(Msb);
    out2.setByteOrder(ISIS_LITTLE_ENDIAN ? Msb : Lsb);
    out2.setFormat(Cube::Bsq);
    out2.setLabelSize(1000);
    out2.setPixelType(UnsignedByte);
    out2.create("IsisCube_02");

    j = 0;
    LineManager oline(out2);
    for(oline.begin(); !oline.end(); oline++) {
      for(int i = 0; i < oline.size(); i++) {
        oline[i] = (double) j;
      }
      out2.clearIoCache();
      out2.write(oline);
      j++;
    }
    out2.close();

    cerr << "Comparing cube ... " << endl;
    Cube in2;
    try {
      in2.open("IsisCube_02");
    }
    catch (IException &e) {
      e.print();
    }
    Report(in2);
    j = 0;
    LineManager inLine2(in2);
    for(inLine2.begin(); !inLine2.end(); inLine2++) {
      in2.read(inLine2);
      for(int i = 0; i < inLine2.size(); i++) {
        if(inLine2[i] != (double) j) {
          cerr << "Problem at line " << inLine2.Line()
               << " sample " << i + 1 << ":  "
               << inLine2[i] << " != " << double(j) << endl;
          return 1;
        }
      }
      in2.clearIoCache();
      j++;
    }
    in2.close();


    // Test other options for output
    cerr << "Creating 16-bit cube ... " << endl;
    Cube out3;
    out3.setDimensions(150, 200, 2);
    out3.setBaseMultiplier(30000.0, -1.0);
//  out3.SetByteOrder(Msb);
    out2.setByteOrder(ISIS_LITTLE_ENDIAN ? Msb : Lsb);
    out3.setPixelType(SignedWord);
    out3.create("IsisCube_03");

    j = 0;
    LineManager oline3(out3);
    for(oline3.begin(); !oline3.end(); oline3++) {
      for(int i = 0; i < oline3.size(); i++) {
        oline3[i] = (double) j;
        j++;
      }
      out3.write(oline3);
    }
    out3.close();

    cerr << "Comparing cube ... " << endl;
    Cube in3;
    in3.open("IsisCube_03");
    Report(in3);
    j = 0;
    LineManager inLine3(in3);
    for(inLine3.begin(); !inLine3.end(); inLine3++) {
      in3.read(inLine3);
      in3.clearIoCache();
      for(int i = 0; i < inLine3.size(); i++) {
        if(inLine3[i] != (double) j) {
          cerr << "Problem at line " << inLine3.Line()
               << " sample " << i + 1 << " band " << inLine3.Band() << ":  "
               << inLine3[i] << " != " << double(j) << endl;
          return 1;
        }
        j++;
      }
    }
    in3.close();


    in.open("IsisCube_01");

    // Test Histogram object on a single band, 1 by default
    cerr << "Testing histogram method, band 1 ... " << endl;
    Histogram *bandOneHist = in.histogram();
    cerr << "Average:        " << bandOneHist->Average() << endl;
    cerr << "Standard Dev:   " << bandOneHist->StandardDeviation() << endl;
    cerr << "Mode:           " << bandOneHist->Mode() << endl;
    cerr << "Total Pixels:   " << bandOneHist->TotalPixels() << endl;
    cerr << "Null Pixels:    " << bandOneHist->NullPixels() << endl;
    cerr << endl;
    delete bandOneHist;
    bandOneHist = NULL;

    // Test histogram object on all bands
    cerr << "Testing histogram method, all bands ... " << endl;
    Histogram *allBandsHistogram = in.histogram(0);
    cerr << "Average:        " << allBandsHistogram->Average() << endl;
    cerr << "Standard Dev:   " << allBandsHistogram->StandardDeviation() << endl;
    cerr << "Mode:           " << allBandsHistogram->Mode() << endl;
    cerr << "Total Pixels:   " << allBandsHistogram->TotalPixels() << endl;
    cerr << "Null Pixels:    " << allBandsHistogram->NullPixels() << endl;
    cerr << endl;
    delete allBandsHistogram;
    allBandsHistogram = NULL;

    // Check error for too few (negative) bands
    try {
      in.histogram(-1);
    }
    catch (IException &e) {
      e.print();
    }

    // Test statistics object on a single band, 1 by default
    cerr << "Testing statistics method, band 1 ... " << endl;
    Statistics *bandOneStats = in.statistics();
    cerr << "Average:        " << bandOneStats->Average() << endl;
    cerr << "Standard Dev:   " << bandOneStats->StandardDeviation() << endl;
    cerr << "Total Pixels:   " << bandOneStats->TotalPixels() << endl;
    cerr << "Null Pixels:    " << bandOneStats->NullPixels() << endl;
    cerr << endl;
    delete bandOneStats;
    bandOneStats = NULL;

    // Test statistics object on all bands
    cerr << "Testing statistics method, all bands ... " << endl;
    Statistics *allBandsStats = in.statistics(0);
    cerr << "Average:        " << allBandsStats->Average() << endl;
    cerr << "Standard Dev:   " << allBandsStats->StandardDeviation() << endl;
    cerr << "Total Pixels:   " << allBandsStats->TotalPixels() << endl;
    cerr << "Null Pixels:    " << allBandsStats->NullPixels() << endl;
    cerr << endl;
    delete allBandsStats;
    allBandsStats = NULL;

    // Check error for too few (negative) bands
    try {
      in.statistics(-1);
    }
    catch (IException &e) {
      e.print();
    }

    cerr << endl;

    cerr << "Virtual band tests" << endl;  // Virtual Band tests

    cerr << "Nbands = " << in.bandCount() << endl;
    cerr << "Band 1 = " << in.physicalBand(1) << endl;
    cerr << "Band 2 = " << in.physicalBand(2) << endl;
    in.close();
    cerr << endl;

    QList<QString> vbands;
    vbands.push_back("2");
    in.setVirtualBands(vbands);
    in.open("IsisCube_01");
    cerr << "Nbands = " << in.bandCount() << endl;
    cerr << "Band 1 = " << in.physicalBand(1) << endl;
    cerr << endl;


    //  Test ReOpen
    cerr << "ReOpen tests" << endl;
    Report(in);
    in.reopen("rw");
    Report(in);
    in.reopen("r");
    Report(in);


    // Test reading past cube boundaries.
    // First create a new cube for us to test and fill it with ones.
    cerr << "Testing reading past cube boundaries ... " << endl;
    cerr << "Constructing cube ... " << endl;
    Cube boundaryTestCube;
    boundaryTestCube.setDimensions(10, 10, 2);
    boundaryTestCube.create("IsisCube_boundary");
    Report(boundaryTestCube);
    LineManager boundaryLine(boundaryTestCube);
    j = 0;
    for(boundaryLine.begin(); !boundaryLine.end(); boundaryLine++) {
      for(int i = 0; i < boundaryLine.size(); i++) {
        boundaryLine[i] = 1.0;
        j++;
      }
      j--;
      boundaryTestCube.write(boundaryLine);
    }

    // Now read past the cube boundaries and compare the results to what we
    // expect. All valid positions in the brick should be filled with ones since
    // our cube is entirely filled with ones, and any parts that fall outside of
    // the cube should be nulls.
    cerr << "Reading completely within cube boundaries ... " << endl;
    Brick readBrick(1, 1, 2, boundaryTestCube.pixelType());
    readBrick.SetBasePosition(1, 1, 1);
    boundaryTestCube.read(readBrick);

    cerr << "Comparing results ... " << endl;
    for(int i = 0; i < readBrick.size(); i++) {
      if (readBrick[i] != 1.0) {
        cerr << "Not all values in brick were 1.0.\n";
        return 1;
      }
    }
    cerr << endl;

    cerr << "Reading completely outside band boundaries ... " << endl;
    readBrick.SetBasePosition(1, 1, -1);
    boundaryTestCube.read(readBrick);

    cerr << "Comparing results ... " << endl;
    for(int i = 0; i < readBrick.size(); i++) {
      if (readBrick[i] != Null) {
        cerr << "Not all values in brick were Null.\n";
        return 1;
      }
    }
    cerr << endl;

    // Read before the bands start in the cube.
    cerr << "Reading partially within band boundaries ... " << endl;
    readBrick.SetBasePosition(1, 1, 0);
    boundaryTestCube.read(readBrick);

    cerr << "Comparing results ... " << endl;
    if (readBrick[0] != Null) {
      cerr << "Value outside cube boundary was not Null.\n";
      return 1;
    }
    if (readBrick[1] != 1.0) {
      cerr << "Value inside cube boundary was not 1.0.\n";
      return 1;
    }

    // Read after the bands start in the cube.
    readBrick.SetBasePosition(1, 1, 2);
    boundaryTestCube.read(readBrick);
    if (readBrick[0] != 1.0) {
      cerr << "Value inside cube boundary was not 1.0.\n";
      return 1;
    }
    if (readBrick[1] != Null) {
      cerr << "Value outside cube boundary was not Null.\n";
      return 1;
    }
    cerr << endl;

    boundaryTestCube.close();

    // Test reading outside a cube with virtual bands.
    cerr << "Testing reading past cube boundaries with virtual bands ... \n";
    QList<QString> virtualBands;
    virtualBands.push_back("2");
    boundaryTestCube.setVirtualBands(virtualBands);
    boundaryTestCube.open("IsisCube_boundary");

    cerr << "Reading completely outside virtual band boundaries ... " << endl;
    readBrick.SetBasePosition(1, 1, 2);
    boundaryTestCube.read(readBrick);

    cerr << "Comparing results ... " << endl;
    for(int i = 0; i < readBrick.size(); i++) {
      if (readBrick[i] != Null) {
        cerr << "Not all values in brick were Null.\n";
        return 1;
      }
    }

    readBrick.SetBasePosition(1, 1, 1000);
    boundaryTestCube.read(readBrick);
    for(int i = 0; i < readBrick.size(); i++) {
      if (readBrick[i] != Null) {
        cerr << "Not all values in brick were Null.\n";
        return 1;
      }
    }

    readBrick.SetBasePosition(1, 1, -1);
    boundaryTestCube.read(readBrick);
    for(int i = 0; i < readBrick.size(); i++) {
      if (readBrick[i] != Null) {
        cerr << "Not all values in brick were Null.\n";
        return 1;
      }
    }
    cerr << endl;

    // Read before the bands start in the cube.
    cerr << "Reading partially within virtual band boundaries ... " << endl;
    readBrick.SetBasePosition(1, 1, 0);
    boundaryTestCube.read(readBrick);

    cerr << "Comparing results ... " << endl;
    if (readBrick[0] != Null) {
      cerr << "Value outside cube boundary was not Null.\n";
      return 1;
    }
    if (readBrick[1] != 1.0) {
      cerr << "Value inside cube boundary was not 1.0.\n";
      return 1;
    }

    // Read after the bands start in the cube.
    readBrick.SetBasePosition(1, 1, 1);
    boundaryTestCube.read(readBrick);
    if (readBrick[0] != 1.0) {
      cerr << "Value inside cube boundary was not 1.0.\n";
      return 1;
    }
    if (readBrick[1] != Null) {
      cerr << "Value outside cube boundary was not Null.\n";
      return 1;
    }

    // Resize the brick to be have many more bands than the cube, and position
    // it before the start of the bands. We should get nulls, then some values,
    // then more nulls.
    readBrick.Resize(1, 1, 20);
    readBrick.SetBasePosition(1, 1, -10);
    boundaryTestCube.read(readBrick);
    for (int i = 0; i < readBrick.size(); i++) {
      if (i == 11) {
        if (readBrick[i] != 1.0)
          cerr << "Value inside cube boundary was not 1.0.\n";
      }
      else {
        if (readBrick[i] != Null)
          cerr << "Value outside cube boundary was not Null.\n";
      }
    }
    cerr << endl;
    boundaryTestCube.close();


    // Check errors
    cerr << "Testing errors ... " << endl;
    try {
      in.open("blah");
    }
    catch (IException &e) {
      e.print();
    }

    try {
      in.create("blah");
    }
    catch (IException &e) {
      e.print();
    }

    try {
      in.write(inLine3);
    }
    catch (IException &e) {
      e.print();
    }

    try {
      Cube in;
      in.open("blah");
    }
    catch (IException &e) {
      e.print();
    }

    try {
      in.physicalBand(2);
    }
    catch (IException &e) {
      e.print();
    }

    try {
      in.physicalBand(0);
    }
    catch (IException &e) {
      e.print();
    }

    try {
      Cube in;
      in.read(inLine3);
    }
    catch (IException &e) {
      e.print();
    }

    try {
      Cube in;
      in.write(inLine3);
    }
    catch (IException &e) {
      e.print();
    }

    try {
      Cube out;
      out.create("IsisCube_04");
      out.close();
    }
    catch (IException &e) {
      e.print();
    }

    try {
      Cube out;
      out.setLabelSize(15);
      out.setDimensions(1, 1, 1);
      out.create("IsisCube_04");
      out.close();
    }
    catch (IException &e) {
      e.print();
    }

    try {
      Cube out;
      out.setDimensions(1000000, 1000000, 9);
      out.create("IsisCube_05");
      out.close();
    }
    catch (IException &e) {
      e.print();
    }
    try {
      Cube in;
      in.open("IsisCube_01", "a");
    }
    catch (IException &e) {
      e.print();
    }
    try {
      Cube in;
      in.setDimensions(0, 0, 0);
    }
    catch (IException &e) {
      e.print();
    }
    try {
      Cube in;
      in.setDimensions(1, 0, 0);
    }
    catch (IException &e) {
      e.print();
    }
    try {
      Cube in;
      in.setDimensions(1, 1, 0);
    }
    catch (IException &e) {
      e.print();
    }

    Cube in4;
    try {
      in4.open("$base/testData/isisTruth.cub");
    }
    catch (IException &e) {
      e.print();
    }

    try {
      in4.reopen("rw");
    }
    catch (IException &e) {
      QString error = e.toString();
      error = error.replace(QRegExp("\\[[^\\]]*\\]"), "[...]");
      cerr << error.toStdString() << endl;
    }

    in4.setPixelType(None);
    try {
      in4.setDimensions(1, 1, 1);
      in4.create("shouldntExist.cub");
    }
    catch (IException &e) {
      e.print();
    }
  }
  catch (IException &e) {
    e.print();
  }

  cerr << endl << "Test creating an ecub" << endl;
  {
    Cube externalData;
    externalData.setExternalDnData("$base/testData/isisTruth.cub");
    externalData.create("isisTruth_external.ecub");
    externalData.putGroup(PvlGroup("TestGroup"));
    cerr << *externalData.label() << endl;

    Brick readBrick(3, 3, 2, externalData.pixelType());
    readBrick.SetBasePosition(1, 1, 1);
    externalData.read(readBrick);
    for (int index = 0; index < readBrick.size(); index++) {
      if (readBrick[index] == Null) {
        cerr << "N ";
      }
      else {
        cerr << readBrick[index] << " ";
      }
    }
    cerr << endl;

    try {
      externalData.write(readBrick);
    }
    catch (IException &e) {
      e.print();
    }
  }

  cerr << endl << "Test creating an ecub from an ecub" << endl;
  {
    Cube externalData;
    externalData.setExternalDnData("isisTruth_external.ecub");
    externalData.create("isisTruth_external2.ecub");
    cerr << *externalData.label() << endl;

    Brick readBrick(3, 3, 2, externalData.pixelType());
    readBrick.SetBasePosition(1, 1, 1);
    externalData.read(readBrick);
    for (int index = 0; index < readBrick.size(); index++) {
      if (readBrick[index] == Null) {
        cerr << "N ";
      }
      else {
        cerr << readBrick[index] << " ";
      }
    }
    cerr << endl;

    try {
      externalData.write(readBrick);
    }
    catch (IException &e) {
      e.print();
    }
  }

  cerr << endl << "Test reading an ecub" << endl;
  {
    Cube externalData;
    externalData.open("isisTruth_external");
    externalData.putGroup(PvlGroup("TestGroup2"));
    cerr << *externalData.label() << endl;

    Brick readBrick(3, 3, 2, externalData.pixelType());
    readBrick.SetBasePosition(1, 1, 1);
    externalData.read(readBrick);
    for (int index = 0; index < readBrick.size(); index++) {
      if (readBrick[index] == Null) {
        cerr << "N ";
      }
      else {
        cerr << readBrick[index] << " ";
      }
    }
    cerr << endl;

    try {
      externalData.write(readBrick);
    }
    catch (IException &e) {
      e.print();
    }
  }

  cerr << endl << "Test reading an ecub that points to another ecub" << endl;
  {
    Cube externalData;
    externalData.open("isisTruth_external2");
    cerr << *externalData.label() << endl;

    Brick readBrick(3, 3, 2, externalData.pixelType());
    readBrick.SetBasePosition(1, 1, 1);
    externalData.read(readBrick);
    for (int index = 0; index < readBrick.size(); index++) {
      if (readBrick[index] == Null) {
        cerr << "N ";
      }
      else {
        cerr << readBrick[index] << " ";
      }
    }
    cerr << endl;

    try {
      externalData.write(readBrick);
    }
    catch (IException &e) {
      e.print();
    }
  }

  cerr << endl << "Test reading an ecub that points to detached lbl" << endl;
  {
    Cube externalData;
    externalData.setExternalDnData("IsisCube_02.lbl");
    externalData.create("isisTruth_external3.ecub");
    cerr << *externalData.label() << endl;

    Brick readBrick(3, 3, 2, externalData.pixelType());
    readBrick.SetBasePosition(1, 1, 1);
    externalData.read(readBrick);
    for (int index = 0; index < readBrick.size(); index++) {
      if (readBrick[index] == Null) {
        cerr << "N ";
      }
      else {
        cerr << readBrick[index] << " ";
      }
    }
    cerr << endl;

    try {
      externalData.write(readBrick);
    }
    catch (IException &e) {
      e.print();
    }
  }

  cerr << endl << "Test copying an ecub that points to detached lbl" << endl;
  {
    Cube externalData;
    externalData.open("isisTruth_external3.ecub");
    Cube *copiedCube = externalData.copy("isisTruth_external3.copy.ecub",
                                         CubeAttributeOutput("+External"));
    cerr << *copiedCube->label() << endl;

    Brick readBrick(3, 3, 2, copiedCube->pixelType());
    readBrick.SetBasePosition(1, 1, 1);
    copiedCube->read(readBrick);
    for (int index = 0; index < readBrick.size(); index++) {
      if (readBrick[index] == Null) {
        cerr << "N ";
      }
      else {
        cerr << readBrick[index] << " ";
      }
    }
    cerr << endl;

    try {
      copiedCube->write(readBrick);
    }
    catch (IException &e) {
      e.print();
    }
  }

  remove("IsisCube_00.cub");
  remove("IsisCube_01.cub");
  remove("IsisCube_02.cub");
  remove("IsisCube_02.lbl");
  remove("IsisCube_03.cub");
  remove("IsisCube_04.cub");
  remove("IsisCube_05.cub");
  remove("IsisCube_boundary.cub");
  remove("isisTruth_external.ecub");
  remove("isisTruth_external2.ecub");
  remove("isisTruth_external3.ecub");
  remove("isisTruth_external3.copy.ecub");

  return 0;
}


void Report(Cube &c) {
  cerr << "File   = " << IString(QFileInfo(c.fileName()).fileName()) << endl;
  cerr << "Samps  = " << c.sampleCount() << endl;
  cerr << "Lines  = " << c.lineCount() << endl;
  cerr << "Bands  = " << c.bandCount() << endl;
  cerr << "Base   = " << c.base() << endl;
  cerr << "Mult   = " << c.multiplier() << endl;
  cerr << "Type   = " << c.pixelType() << endl;
//  cerr << "Order  = " << c.ByteOrder() << endl; // Needs to be system independent
  cerr << "Atchd  = " << c.labelsAttached() << endl;
  cerr << "Format = " << c.format() << endl;
  cerr << "Open   = " << c.isOpen() << endl;
  try {
    cerr << "R/O    = ";
    cerr.flush();
    cerr << c.isReadOnly();
  }
  catch (IException &e) {
    cerr << "N/A";
  }

  cerr << endl;

  try {
    cerr << "R/W    = ";
    cerr.flush();
    cerr << c.isReadWrite();
  }
  catch (IException &e) {
    cerr << "N/A";
  }

  cerr << endl;
  cerr << "Lbytes = " << c.labelSize() << endl;
  cerr << endl;
}
