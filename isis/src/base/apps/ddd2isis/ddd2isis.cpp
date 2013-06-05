#include "Isis.h"
#include "ProcessImport.h"
#include "UserInterface.h"
#include "SpecialPixel.h"
#include "FileName.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  ProcessImport p;
  IString from = ui.GetFileName("FROM");
  EndianSwapper swp("MSB");
  int nsamples = 0, nlines = 0, nbands = 1, noffset = 0;

  union {
    char readChars[4];
    long readLong;
    float readFloat;
  } readBytes;

  ifstream fin;
  fin.open(from.c_str(), ios::in | ios::binary);
  if(!fin.is_open()) {
    string msg = "Cannot open input file [" + from + "]";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  /**
   *  0-rel byte offset   value
   *       0          32-bit integer magic number
   *       4          32-bit integer number of image lines
   *       8          32-bit integer number of bytes per image line
   *      12          32-bit integer number of bits per image elements
   *      16          32-bit integer currently unused
   *      20          32-bit integer number of bytes to start of image data
   *      24          ASCII label up to 1000 characters long
   *                  The label is NUL-terminated
   *
   */

  // Verify the magic number
  readBytes.readLong = 0;
  fin.seekg(0);
  fin.read(readBytes.readChars, 4);
  readBytes.readFloat = swp.Float(readBytes.readChars);

  if(readBytes.readLong != 0x67B) {
    string msg = "Input file [" + from + "] does not appear to be in ddd format";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  fin.read(readBytes.readChars, 4);
  readBytes.readFloat = swp.Float(readBytes.readChars);
  nlines = (int)readBytes.readLong;

  fin.read(readBytes.readChars, 4);
  readBytes.readFloat = swp.Float(readBytes.readChars);
  nsamples = (int)readBytes.readLong;

  fin.read(readBytes.readChars, 4);
  readBytes.readFloat = swp.Float(readBytes.readChars);

  if(fin.fail() || fin.eof()) {
    string msg = "An error ocurred when reading the input file [" + from + "]";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  switch(readBytes.readLong) {
    case 8:
      p.SetPixelType(Isis::UnsignedByte);
      break;
    case 16:
      p.SetPixelType(Isis::UnsignedWord);
      break;
    case 32:
      p.SetPixelType(Isis::Real);
      break;
    default:
      IString msg = "Unsupported bit per pixel count [" + IString((int)readBytes.readLong) + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
  }

  nsamples /= (readBytes.readLong / 8);

  fin.read(readBytes.readChars, 4);
  readBytes.readFloat = swp.Float(readBytes.readChars);

  fin.read(readBytes.readChars, 4);
  readBytes.readFloat = swp.Float(readBytes.readChars);
  noffset = (int)readBytes.readLong;
  if (noffset < 1024) {
    noffset = 1024;
  }

  fin.close();

  p.SetDimensions(nsamples, nlines, nbands);
  p.SetFileHeaderBytes(noffset);
  p.SetByteOrder(Isis::Msb);
  p.SetInputFile(ui.GetFileName("FROM"));
  p.SetOutputCube("TO");

  p.StartProcess();
  p.EndProcess();

  return;
}

