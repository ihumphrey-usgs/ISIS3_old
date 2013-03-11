#include "Isis.h"
#include "ProcessByLine.h"
#include "Pixel.h"
#include "LineManager.h"
#include "SpecialPixel.h"
#include "Cube.h"
#include "Table.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "SubArea.h"

using namespace std;
using namespace Isis;

int minSample, maxSample, numSamples;
int minLine, maxLine, numLines;
int curBand, numBands;
bool cropNulls, cropHrs, cropLrs, cropHis, cropLis;
LineManager *in;
Cube cube;

void FindPerimeter(Buffer &in);
void SpecialRemoval(Buffer &out);


void IsisMain() {
  maxSample = 0;
  maxLine = 0;
  curBand = 1;

  // Open the input cube
  UserInterface &ui = Application::GetUserInterface();
  QString from = ui.GetAsString("FROM");
  CubeAttributeInput inAtt(from);
  cube.setVirtualBands(inAtt.bands());
  from = ui.GetFileName("FROM");
  cube.open(from);

  cropNulls = ui.GetBoolean("NULL");
  cropHrs = ui.GetBoolean("HRS");
  cropLrs = ui.GetBoolean("LRS");
  cropHis = ui.GetBoolean("HIS");
  cropLis = ui.GetBoolean("LIS");

  minSample = cube.sampleCount() + 1;
  minLine = cube.lineCount() + 1;
  numBands = cube.bandCount();

  // Setup the input cube
  ProcessByLine p1;
  p1.SetInputCube("FROM");
  p1.Progress()->SetText("Finding Perimeter");

  // Start the first pass
  p1.StartProcess(FindPerimeter);
  p1.EndProcess();

  if(minSample == cube.sampleCount() + 1) {
    cube.close();
    QString msg = "There are no valid pixels in the [FROM] cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  numSamples = maxSample - (minSample - 1);
  numLines = maxLine - (minLine - 1);

  // Setup the output cube
  ProcessByLine p2;
  p2.SetInputCube("FROM");
  p2.PropagateTables(false);
  p2.Progress()->SetText("Removing Special Pixels");
  Cube *ocube = p2.SetOutputCube("TO", numSamples, numLines, numBands);
  p2.ClearInputCubes();

  // propagate tables manually
  Pvl &inLabels = *cube.label();

  // Loop through the labels looking for object = Table
  for(int labelObj = 0; labelObj < inLabels.objects(); labelObj++) {
    PvlObject &obj = inLabels.object(labelObj);

    if(obj.name() != "Table") continue;

    // Read the table into a table object
    Table table(obj["Name"], from);

    ocube->write(table);
  }

  // Construct a label with the results
  PvlGroup results("Results");
  results += PvlKeyword("InputLines", toString(cube.lineCount()));
  results += PvlKeyword("InputSamples", toString(cube.sampleCount()));
  results += PvlKeyword("StartingLine", toString(minLine));
  results += PvlKeyword("StartingSample", toString(minSample));
  results += PvlKeyword("EndingLine", toString(maxLine));
  results += PvlKeyword("EndingSample", toString(maxSample));
  results += PvlKeyword("OutputLines", toString(numLines));
  results += PvlKeyword("OutputSamples", toString(numSamples));

  // Create a buffer for reading the input cube
  in = new LineManager(cube);

  // Start the second pass
  p2.StartProcess(SpecialRemoval);

  // Update the Mapping, Instrument, and AlphaCube groups in the output
  // cube label
  SubArea s;
  s.SetSubArea(cube.lineCount(), cube.sampleCount(), minLine, minSample, minLine + numLines - 1,
               minSample + numSamples - 1, 1.0, 1.0);
  s.UpdateLabel(&cube, ocube, results);

  p2.EndProcess();
  cube.close();

  delete in;
  in = NULL;

  // Write the results to the log
  Application::Log(results);
}

// Process each line to find the min and max lines and samples
void FindPerimeter(Buffer &in) {
  for(int i = 0; i < in.size(); i++) {
    // Do nothing until we find a valid pixel, or a pixel we do not want to crop off
    if((Pixel::IsValid(in[i])) || (Pixel::IsNull(in[i]) && !cropNulls) ||
        (Pixel::IsHrs(in[i]) && !cropHrs) || (Pixel::IsLrs(in[i]) && !cropLrs) ||
        (Pixel::IsHis(in[i]) && !cropHis) || (Pixel::IsLis(in[i]) && !cropLis)) {
      // The current line has a valid pixel and is greater than max, so make it the new max line
      if(in.Line() > maxLine) maxLine = in.Line();

      // This is the first line to contain a valid pixel, so it's the min line
      if(in.Line() < minLine) minLine = in.Line();

      int cur_sample = i + 1;

      // We process by line, so the min sample is the valid pixel with the lowest index in all line arrays
      if(cur_sample < minSample) minSample = cur_sample;

      // Conversely, the max sample is the valid pixel with the highest index
      if(cur_sample > maxSample) maxSample = cur_sample;
    }
  }
}

// Using the min and max values, create a new cube without the extra specials
void SpecialRemoval(Buffer &out) {
  int iline = minLine + (out.Line() - 1);
  in->SetLine(iline, curBand);
  cube.read(*in);

  // Loop and move appropriate samples
  for(int i = 0; i < out.size(); i++) {
    out[i] = (*in)[(minSample - 1) + i];
  }

  if(out.Line() == numLines) curBand++;
}
