#define GUIHELPERS

#include "Isis.h"
#include <sstream>
#include "PvlGroup.h"
#include "UserInterface.h"
#include "iString.h"

using namespace std;
using namespace Isis;

void PrintPvl();
void LoadPvl();

map <string, void *> GuiHelpers() {
  map <string, void *> helper;
  helper ["PrintPvl"] = (void *) PrintPvl;
  helper ["LoadPvl"] = (void *) LoadPvl;
  return helper;
}

//functions in the code
void addPhoModel(Pvl &pvl, Pvl &outPvl);
void addAtmosModel(Pvl &pvl, Pvl &outPvl);

// Helper function to print the input pvl file to session log
void PrintPvl() {
  UserInterface &ui = Application::GetUserInterface();

  // Write file out to log
  string inFile(ui.GetFileName("FROMPVL"));
  Pvl inPvl;
  inPvl.Read(ui.GetFileName("FROMPVL"));
  string Ostring = "***** Output of [" + inFile + "] *****";
  Application::GuiLog(Ostring);
  Application::GuiLog(inPvl);
}

// Load the values from the input PVL to a string to be displayed
// onto the UI.
void LoadKeyValue(const PvlKeyword & key, string & val){
  int size = key.Size();
  val = "";
  for (int i=0; i<size; i++) {
    if (i > 0) {
      val += ", ";
    }
    val += key[i];
  }
}

// Data from the UI is output to a PVL
// Converts the string into double value
void OutputKeyValue(PvlKeyword & key, string val){
  key.Clear();
  size_t found = val.find(",");
  while(found != string::npos) {
    key += iString(val.substr(0, found)).ToDouble();
    val = val.substr(found+1);
    found = val.find(",");
  }
  key += iString(val.substr(0, found)).ToDouble();
}

// Helper function to load the input pvl file into the GUI
void LoadPvl() {
  std::stringstream os;
  string keyVal;
  UserInterface &ui = Application::GetUserInterface();
  string inFile(ui.GetFileName("FROMPVL"));
  Pvl inPvl;
  inPvl.Read(inFile);
  iString phtName = ui.GetAsString("PHTNAME");
  phtName = phtName.UpCase();
  iString atmName = ui.GetAsString("ATMNAME");
  atmName = atmName.UpCase();
  if (phtName == "NONE" && atmName == "NONE") {
    string message = "A photometric model or an atmospheric model must be specified before it can be loaded from the PVL";
    throw IException(IException::User, message, _FILEINFO_);
  }
  if (phtName != "NONE") {
    if (inPvl.HasObject("PhotometricModel")) {
      PvlObject phtObj = inPvl.FindObject("PhotometricModel");
      iString phtVal;
      if (phtObj.HasGroup("Algorithm")) {
        PvlObject::PvlGroupIterator phtGrp = phtObj.BeginGroup();
        bool wasFound = false;
        if (phtGrp->HasKeyword("PHTNAME")) {
          phtVal = (string)phtGrp->FindKeyword("PHTNAME");
        } else if (phtGrp->HasKeyword("NAME")) {
          phtVal = (string)phtGrp->FindKeyword("NAME");
        } else {
          phtVal = "NONE";
        }
        phtVal = phtVal.UpCase();
        if (phtName == phtVal) {
          wasFound = true;
        }
        if (ui.WasEntered("PHTNAME") && !wasFound) {
          phtName = ui.GetAsString("PHTNAME");
          phtName = phtName.UpCase();
          while (phtGrp != phtObj.EndGroup()) {
            if (phtGrp->HasKeyword("PHTNAME") || phtGrp->HasKeyword("NAME")) {
              if (phtGrp->HasKeyword("PHTNAME")) {
                phtVal = (string)phtGrp->FindKeyword("PHTNAME");
              } else if (phtGrp->HasKeyword("NAME")) {
                phtVal = (string)phtGrp->FindKeyword("NAME");
              } else {
                phtVal = "NONE";
              }
              phtVal = phtVal.UpCase();
              if (phtName == phtVal) {
                wasFound = true;
                break;
              }
            }
            phtGrp++;
          }
        }
        if (wasFound) {
          ui.Clear("PHTNAME");
          ui.Clear("THETA");
          ui.Clear("WH");
          ui.Clear("HG1");
          ui.Clear("HG2");
          ui.Clear("HH");
          ui.Clear("B0");
          ui.Clear("BH");
          ui.Clear("CH");
          ui.Clear("L");
          ui.Clear("K");
          ui.Clear("PHASELIST");
          ui.Clear("KLIST");
          ui.Clear("LLIST");
          ui.Clear("PHASECURVELIST");
          if (phtGrp->HasKeyword("PHTNAME")) {
            phtVal = (string)phtGrp->FindKeyword("PHTNAME");
          } else if (phtGrp->HasKeyword("NAME")) {
            phtVal = (string)phtGrp->FindKeyword("NAME");
          } else {
            phtVal = "NONE";
          }
          phtVal = phtVal.UpCase();
          if (phtVal == "HAPKEHEN" || phtVal == "HAPKELEG") {
            if (phtGrp->HasKeyword("THETA")) {
              PvlKeyword thetaKey = phtGrp->FindKeyword("THETA");
              LoadKeyValue(thetaKey, keyVal);
              ui.PutAsString("THETA", keyVal);
            }
            if (phtGrp->HasKeyword("WH")) {
              PvlKeyword whKey = phtGrp->FindKeyword("WH");
              LoadKeyValue(whKey, keyVal);
              ui.PutAsString("WH", keyVal);
            }
            if (phtGrp->HasKeyword("HH")) {
              PvlKeyword hhKey = phtGrp->FindKeyword("HH");
              LoadKeyValue(hhKey, keyVal);
              ui.PutAsString("HH", keyVal);
            }
            if (phtGrp->HasKeyword("B0")) {
              PvlKeyword b0Key = phtGrp->FindKeyword("B0");
              LoadKeyValue(b0Key, keyVal);
              ui.PutAsString("B0", keyVal);
            }
            if (phtVal == "HAPKEHEN") {
              if (phtGrp->HasKeyword("HG1")) {
                PvlKeyword hg1Key = phtGrp->FindKeyword("HG1");
                LoadKeyValue(hg1Key, keyVal);
                ui.PutAsString("HG1", keyVal);
              }
              if (phtGrp->HasKeyword("HG2")) {
                PvlKeyword hg2Key = phtGrp->FindKeyword("HG2");
                LoadKeyValue(hg2Key, keyVal);
                ui.PutAsString("HG2", keyVal);
              }
            }
            if (phtVal == "HAPKELEG") {
              if (phtGrp->HasKeyword("BH")) {
                PvlKeyword bhKey = phtGrp->FindKeyword("BH");
                LoadKeyValue(bhKey, keyVal);
                ui.PutAsString("BH", keyVal);
              }
              if (phtGrp->HasKeyword("CH")) {
                PvlKeyword chKey = phtGrp->FindKeyword("CH");
                LoadKeyValue(chKey, keyVal);
                ui.PutAsString("CH", keyVal);
              }
            }
          } else if (phtVal == "MINNAERT") {
            if (phtGrp->HasKeyword("K")) {
              PvlKeyword k = phtGrp->FindKeyword("K");
              LoadKeyValue(k, keyVal);
              ui.PutAsString("K", keyVal);
            }
          } else if (phtVal == "LUNARLAMBERTEMPIRICAL" || phtVal == "MINNAERTEMPIRICAL") {
            if (phtGrp->HasKeyword("PHASELIST")) {
              PvlKeyword phaselist = phtGrp->FindKeyword("PHASELIST");
              LoadKeyValue(phaselist, keyVal);
              ui.PutAsString("PHASELIST", keyVal);
            }
            if (phtGrp->HasKeyword("PHASECURVELIST")) {
              PvlKeyword phasecurvelist = phtGrp->FindKeyword("PHASECURVELIST");
              LoadKeyValue(phasecurvelist, keyVal);
              ui.PutAsString("PHASECURVELIST", keyVal);
            }
            if (phtVal == "LUNARLAMBERTEMPIRICAL") {
              if (phtGrp->HasKeyword("LLIST")) {
                PvlKeyword llist = phtGrp->FindKeyword("LLIST");
                LoadKeyValue(llist, keyVal);
                ui.PutAsString("LLIST", keyVal);
              }
            }
            if (phtVal == "MINNAERTEMPIRICAL") {
              if (phtGrp->HasKeyword("KLIST")) {
                PvlKeyword kList = phtGrp->FindKeyword("KLIST");
                LoadKeyValue(kList, keyVal);
                ui.PutAsString("KLIST", keyVal);
              }
            }
          } else if (phtVal == "LUNARLAMBERT") {
            if (phtGrp->HasKeyword("L")) {
              PvlKeyword l = phtGrp->FindKeyword("L");
              LoadKeyValue(l, keyVal);
              ui.PutAsString("L", keyVal);
            }
          } else if (phtVal != "LAMBERT" && phtVal != "LOMMELSEELIGER" &&
                     phtVal != "LUNARLAMBERTMCEWEN") {
            string message = "Unsupported photometric model [" + phtVal + "].";
            throw IException(IException::User, message, _FILEINFO_);
          }
          ui.PutAsString("PHTNAME", phtVal);
        }
      }
    }
  }
  if (atmName == "NONE") {
    return;
  }
  if (inPvl.HasObject("AtmosphericModel")) {
    PvlObject atmObj = inPvl.FindObject("AtmosphericModel");
    iString atmVal;
    if (atmObj.HasGroup("Algorithm")) {
      PvlObject::PvlGroupIterator atmGrp = atmObj.BeginGroup();
      bool wasFound = false;
      if (atmGrp->HasKeyword("ATMNAME")) {
        atmVal = (string)atmGrp->FindKeyword("ATMNAME");
      } else if (atmGrp->HasKeyword("NAME")) {
        atmVal = (string)atmGrp->FindKeyword("NAME");
      } else {
        atmVal = "NONE";
      }
      atmVal = atmVal.UpCase();
      if (atmName == atmVal) {
        wasFound = true;
      }
      if (ui.WasEntered("ATMNAME") && !wasFound) {
        while (atmGrp != atmObj.EndGroup()) {
          if (atmGrp->HasKeyword("ATMNAME") || atmGrp->HasKeyword("NAME")) {
            if (atmGrp->HasKeyword("ATMNAME")) {
              atmVal = (string)atmGrp->FindKeyword("ATMNAME");
            } else if (atmGrp->HasKeyword("NAME")) {
              atmVal = (string)atmGrp->FindKeyword("NAME");
            } else {
              atmVal = "NONE";
            }
            atmVal = atmVal.UpCase();
            if (atmName == atmVal) {
              wasFound = true;
              break;
            }
          }
          atmGrp++;
        }
      }
      if (!wasFound) {
        return;
      }
      ui.Clear("ATMNAME");
      ui.Clear("HNORM");
      ui.Clear("BHA");
      ui.Clear("TAU");
      ui.Clear("TAUREF");
      ui.Clear("WHA");
      ui.Clear("HGA");
      if (atmGrp->HasKeyword("ATMNAME")) {
        atmVal = (string)atmGrp->FindKeyword("ATMNAME");
      } else if (atmGrp->HasKeyword("NAME")) {
        atmVal = (string)atmGrp->FindKeyword("NAME");
      } else {
        atmVal = "NONE";
      }
      atmVal = atmVal.UpCase();
      if (atmVal == "ANISOTROPIC1" || atmVal == "ANISOTROPIC2" ||
          atmVal == "HAPKEATM1" || atmVal == "HAPKEATM2" ||
          atmVal == "ISOTROPIC1" || atmVal == "ISOTROPIC2") {
        if (atmGrp->HasKeyword("HNORM")) {
          double hnorm = atmGrp->FindKeyword("HNORM");
          os.str("");
          os << hnorm;
          ui.PutAsString("HNORM", os.str());
        }
        if (atmGrp->HasKeyword("TAU")) {
          double tau = atmGrp->FindKeyword("TAU");
          os.str("");
          os << tau;
          ui.PutAsString("TAU", os.str());
        }
        if (atmGrp->HasKeyword("TAUREF")) {
          double tauref = atmGrp->FindKeyword("TAUREF");
          os.str("");
          os << tauref;
          ui.PutAsString("TAUREF", os.str());
        }
        if (atmGrp->HasKeyword("WHA")) {
          double wha = atmGrp->FindKeyword("WHA");
          os.str("");
          os << wha;
          ui.PutAsString("WHA", os.str());
        }
        if (atmGrp->HasKeyword("NULNEG")) {
          string nulneg = (string)atmGrp->FindKeyword("NULNEG");
          if (nulneg.compare("YES")) {
            ui.PutString("NULNEG", "YES");
          } else if (nulneg.compare("NO")) {
            ui.PutString("NULNEG", "NO");
          } else {
            string message = "The NULNEG value is invalid - must be set to YES or NO";
            throw IException(IException::User, message, _FILEINFO_);
          }
        }
      }
      if (atmVal == "ANISOTROPIC1" || atmVal == "ANISOTROPIC2") {
        if (atmGrp->HasKeyword("BHA")) {
          double bha = atmGrp->FindKeyword("BHA");
          os.str("");
          os << bha;
          ui.PutAsString("BHA", os.str());
        }
      }
      if (atmVal == "HAPKEATM1" || atmVal == "HAPKEATM2") {
        if (atmGrp->HasKeyword("HGA")) {
          double hga = atmGrp->FindKeyword("HGA");
          os.str("");
          os << hga;
          ui.PutAsString("HGA", os.str());
        }
      }

      if (atmVal != "ANISOTROPIC1" && atmVal != "ANISOTROPIC2" &&
          atmVal != "HAPKEATM1" && atmVal != "HAPKEATM2" &&
          atmVal != "ISOTROPIC1" && atmVal != "ISOTROPIC2") {
        string message = "Unsupported atmospheric model [" + atmVal + "].";
        throw IException(IException::User, message, _FILEINFO_);
      }
      ui.PutAsString("ATMNAME", atmVal);
    }
  }
}

void IsisMain() {
  // Get the output file name from the GUI and write the pvl
  // to the file. If no extension is given, '.pvl' will be used.
  UserInterface &ui = Application::GetUserInterface();
  FileName out = ui.GetFileName("TOPVL");
  string output = ui.GetFileName("TOPVL");
  if(out.extension() == "") {
    output += ".pvl";
  }

  //The PVL to be written out
  Pvl p;
  Pvl op;

  if (ui.WasEntered("FROMPVL")) {
    string input = ui.GetFileName("FROMPVL");
    p.Read(input);
  }

  //Check to make sure that a model was specified
  if (ui.GetAsString("PHTNAME") == "NONE" && ui.GetAsString("ATMNAME") == "NONE") {
    string message = "A photometric model or an atmospheric model must be specified before running this program";
    throw IException(IException::User, message, _FILEINFO_);
  }

  //Add the different models to the PVL
  if (ui.GetAsString("PHTNAME") != "NONE") {
    addPhoModel(p, op);
  }

  if (ui.GetAsString("ATMNAME") != "NONE") {
    addAtmosModel(p, op);
  }

  op.Write(output);
}

//Function to add photometric model to the PVL
void addPhoModel(Pvl &pvl, Pvl &outPvl) {
  UserInterface &ui = Application::GetUserInterface();

  bool wasFound = false;
  iString phtName = ui.GetAsString("PHTNAME");
  phtName = phtName.UpCase();
  iString phtVal;
  PvlObject pvlObj;
  PvlGroup pvlGrp;
  if (pvl.HasObject("PhotometricModel")) {
    pvlObj = pvl.FindObject("PhotometricModel");
    if (pvlObj.HasGroup("Algorithm")) {
      PvlObject::PvlGroupIterator pvlGrp = pvlObj.BeginGroup();
      if (pvlGrp->HasKeyword("PHTNAME")) {
        phtVal = (string)pvlGrp->FindKeyword("PHTNAME");
      } else if (pvlGrp->HasKeyword("NAME")) {
        phtVal = (string)pvlGrp->FindKeyword("NAME");
      } else {
        phtVal = "NONE";
      }
      phtVal = phtVal.UpCase();
      if (phtName == phtVal) {
        wasFound = true;
      }
      if (!wasFound) {
        while (pvlGrp != pvlObj.EndGroup()) {
          if (pvlGrp->HasKeyword("PHTNAME") || pvlGrp->HasKeyword("NAME")) {
            if (pvlGrp->HasKeyword("PHTNAME")) {
              phtVal = (string)pvlGrp->FindKeyword("PHTNAME");
            } else if (pvlGrp->HasKeyword("NAME")) {
              phtVal = (string)pvlGrp->FindKeyword("NAME");
            } else {
              phtVal = "NONE";
            }
            phtVal = phtVal.UpCase();
            if (phtName == phtVal) {
              wasFound = true;
              break;
            }
          }
          pvlGrp++;
        }
      }
    }
    if (wasFound) {
      outPvl.AddObject(pvlObj);
    } else {
      outPvl.AddObject(PvlObject("PhotometricModel"));
      outPvl.FindObject("PhotometricModel").AddGroup(PvlGroup("Algorithm"));
      outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
             AddKeyword(PvlKeyword("PHTNAME",phtName),Pvl::Replace);
    }
  } else {
    outPvl.AddObject(PvlObject("PhotometricModel"));
    outPvl.FindObject("PhotometricModel").AddGroup(PvlGroup("Algorithm"));
    outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
           AddKeyword(PvlKeyword("PHTNAME",phtName),Pvl::Replace);
  }

  //Get the photometric model and any parameters specific to that
  //model and write it to the algorithm group

  //Hapke Photometric Models
  if(phtName == "HAPKEHEN" || phtName == "HAPKELEG") {
    if (ui.WasEntered("THETA")) {
      PvlKeyword thetaKey("THETA");
      OutputKeyValue(thetaKey, ui.GetString("THETA"));
      outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
             AddKeyword(thetaKey,Pvl::Replace);
    } else {
      if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                  HasKeyword("THETA")) {
        string message = "The " + phtName + " Photometric model requires a value for the THETA parameter.";
        message += "The normal range for THETA is: 0 <= THETA <= 90";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("WH")) {
      PvlKeyword whKey("WH");
      OutputKeyValue(whKey, ui.GetString("WH"));
      outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
             AddKeyword(whKey, Pvl::Replace);
    } else {
      if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                  HasKeyword("WH")) {
        string message = "The " + phtName + " Photometric model requires a value for the WH parameter.";
        message += "The normal range for WH is: 0 < WH <= 1";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("HH")) {
      PvlKeyword hhKey("HH");
      OutputKeyValue(hhKey, ui.GetString("HH"));
      outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
             AddKeyword(hhKey, Pvl::Replace);
    } else {
      if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                  HasKeyword("HH")) {
        string message = "The " + phtName + " Photometric model requires a value for the HH parameter.";
        message += "The normal range for HH is: 0 <= HH";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("B0")) {
      PvlKeyword b0Key("B0");
      OutputKeyValue(b0Key, ui.GetString("B0"));
      outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
             AddKeyword(b0Key, Pvl::Replace);
    } else {
      if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                  HasKeyword("B0")) {
        string message = "The " + phtName + " Photometric model requires a value for the B0 parameter.";
        message += "The normal range for B0 is: 0 <= B0";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (phtName == "HAPKEHEN") {
      if (ui.WasEntered("HG1")) {
        PvlKeyword hg1Key("HG1");
        OutputKeyValue(hg1Key, ui.GetString("HG1"));
        outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
               AddKeyword(hg1Key, Pvl::Replace);
      } else {
        if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                    HasKeyword("HG1")) {
          string message = "The " + phtName + " Photometric model requires a value for the HG1 parameter.";
          message += "The normal range for HG1 is: -1 < HG1 < 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      if (ui.WasEntered("HG2")) {
        PvlKeyword hg2Key("HG2");
        OutputKeyValue(hg2Key, ui.GetString("HG2"));
        outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
               AddKeyword(hg2Key, Pvl::Replace);
      } else {
        if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                    HasKeyword("HG2")) {
          string message = "The " + phtName + " Photometric model requires a value for the HG2 parameter.";
          message += "The normal range for HG2 is: 0 <= HG2 <= 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
    } else {
      if (ui.WasEntered("BH")) {
        PvlKeyword bhKey("BH");
        OutputKeyValue(bhKey, ui.GetString("BH"));
        outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
               AddKeyword(bhKey, Pvl::Replace);
      } else {
        if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                    HasKeyword("BH")) {
          string message = "The " + phtName + " Photometric model requires a value for the BH parameter.";
          message += "The normal range for BH is: -1 <= BH <= 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      if (ui.WasEntered("CH")) {
        PvlKeyword chKey("CH");
        OutputKeyValue(chKey, ui.GetString("CH"));
        outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
               AddKeyword(chKey, Pvl::Replace);
      } else {
        if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                    HasKeyword("CH")) {
          string message = "The " + phtName + " Photometric model requires a value for the CH parameter.";
          message += "The normal range for CH is: -1 <= CH <= 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
    }

  }
  //Lunar Lambert Empirical and Minnaert Empirical Photometric Models
  else if (phtName == "LUNARLAMBERTEMPIRICAL" || phtName == "MINNAERTEMPIRICAL") {
    if (ui.WasEntered("PHASELIST")) {
      PvlKeyword phaseListKey("PHASELIST");
      OutputKeyValue(phaseListKey, ui.GetString("PHASELIST"));
      outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
             AddKeyword(phaseListKey, Pvl::Replace);
    } else {
      if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                    HasKeyword("PHASELIST")) {
        string message = "The " + phtName + " Photometric model requires a value for the PHASELIST parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("PHASECURVELIST")) {
      PvlKeyword phCurveListKey("PHASECURVELIST");
      OutputKeyValue(phCurveListKey, ui.GetString("PHASECURVELIST"));
      outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
             AddKeyword(phCurveListKey, Pvl::Replace);
    } else {
      if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                  HasKeyword("PHASECURVELIST")) {
        string message = "The " + phtName + " Photometric model requires a value for the PHASECURVELIST parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (phtName == "LUNARLAMBERTEMPIRICAL") {
      if (ui.WasEntered("LLIST")) {
        PvlKeyword lListKey("LLIST");
        OutputKeyValue(lListKey, ui.GetString("LLIST"));
        outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
               AddKeyword(lListKey, Pvl::Replace);
      } else {
        if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                    HasKeyword("LLIST")) {
          string message = "The " + phtName + " Photometric model requires a value for the LLIST parameter.";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
    } else {
      if (ui.WasEntered("KLIST")) {
        PvlKeyword kListKey("KLIST");
        OutputKeyValue(kListKey, ui.GetString("KLIST"));
        outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
               AddKeyword(kListKey, Pvl::Replace);
      } else {
        if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                    HasKeyword("KLIST")) {
          string message = "The " + phtName + " Photometric model requires a value for the KLIST parameter.";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
    }
  }
  //Lunar Lambert Photometric Model
  else if (phtName == "LUNARLAMBERT") {
    if (ui.WasEntered("L")) {
      PvlKeyword lKey("L");
      OutputKeyValue(lKey, ui.GetString("L"));
      outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
             AddKeyword(lKey,Pvl::Replace);
    } else {
      if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                  HasKeyword("L")) {
        string message = "The " + phtName + " Photometric model requires a value for the L parameter.";
        message += "The L parameter has no limited range";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
  }
  //Minnaert Photometric Model
  else if (phtName == "MINNAERT") {
    if (ui.WasEntered("K")) {
      PvlKeyword kKey("K");
      OutputKeyValue(kKey, ui.GetString("K"));
      outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
             AddKeyword(kKey, Pvl::Replace);
    } else {
      if (!outPvl.FindObject("PhotometricModel").FindGroup("Algorithm").
                  HasKeyword("K")) {
        string message = "The " + phtName + " Photometric model requires a value for the K parameter.";
        message += "The normal range for K is: 0 <= K";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
  }
}

//Function to add atmospheric model to the PVL
void addAtmosModel(Pvl &pvl, Pvl &outPvl) {
  UserInterface &ui = Application::GetUserInterface();

  bool wasFound = false;
  iString atmName = ui.GetAsString("ATMNAME");
  atmName = atmName.UpCase();
  iString atmVal;
  PvlObject pvlObj;
  PvlGroup pvlGrp;
  if (pvl.HasObject("AtmosphericModel")) {
    pvlObj = pvl.FindObject("AtmosphericModel");
    if (pvlObj.HasGroup("Algorithm")) {
      PvlObject::PvlGroupIterator pvlGrp = pvlObj.BeginGroup();
      if (pvlGrp->HasKeyword("ATMNAME")) {
        atmVal = (string)pvlGrp->FindKeyword("ATMNAME");
      } else if (pvlGrp->HasKeyword("NAME")) {
        atmVal = (string)pvlGrp->FindKeyword("NAME");
      } else {
        atmVal = "NONE";
      }
      atmVal = atmVal.UpCase();
      if (atmName == atmVal) {
        wasFound = true;
      }
      if (!wasFound) {
        while (pvlGrp != pvlObj.EndGroup()) {
          if (pvlGrp->HasKeyword("ATMNAME") || pvlGrp->HasKeyword("NAME")) {
            if (pvlGrp->HasKeyword("ATMNAME")) {
              atmVal = (string)pvlGrp->FindKeyword("ATMNAME");
            } else if (pvlGrp->HasKeyword("NAME")) {
              atmVal = (string)pvlGrp->FindKeyword("NAME");
            } else {
              atmVal = "NONE";
            }
            atmVal = atmVal.UpCase();
            if (atmName == atmVal) {
              wasFound = true;
              break;
            }
          }
          pvlGrp++;
        }
      }
    }
    if (wasFound) {
      outPvl.AddObject(pvlObj);
    } else {
      outPvl.AddObject(PvlObject("AtmosphericModel"));
      outPvl.FindObject("AtmosphericModel").AddGroup(PvlGroup("Algorithm"));
      outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
             AddKeyword(PvlKeyword("ATMNAME",atmName),Pvl::Replace);
    }
  } else {
    outPvl.AddObject(PvlObject("AtmosphericModel"));
    outPvl.FindObject("AtmosphericModel").AddGroup(PvlGroup("Algorithm"));
    outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
           AddKeyword(PvlKeyword("ATMNAME",atmName),Pvl::Replace);
  }

  //Get the atmospheric model and any parameters specific to that
  //model and write it to the algorithm group

  if (atmName == "ANISOTROPIC1" || atmName == "ANISOTROPIC2" ||
      atmName == "HAPKEATM1" || atmName == "HAPKEATM2" ||
      atmName == "ISOTROPIC1" || atmName == "ISOTROPIC2") {
    if (ui.WasEntered("HNORM")) {
       PvlKeyword hnormKey("HNORM");
       OutputKeyValue(hnormKey, ui.GetString("HNORM"));
       outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
             AddKeyword(hnormKey, Pvl::Replace);
    } else {
      if (!outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
                  HasKeyword("HNORM")) {
        string message = "The " + atmName + " Atmospheric model requires a value for the HNORM parameter.";
        message += "The normal range for HNORM is: 0 <= HNORM";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("TAU")) {
      PvlKeyword tauKey("TAU");
      OutputKeyValue(tauKey, ui.GetString("TAU"));
      outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
             AddKeyword(tauKey, Pvl::Replace);
    } else {
      if (!outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
                  HasKeyword("TAU")) {
        string message = "The " + atmName + " Atmospheric model requires a value for the TAU parameter.";
        message += "The normal range for TAU is: 0 <= TAU";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("TAUREF")) {
      PvlKeyword taurefKey("TAUREF");
      OutputKeyValue(taurefKey, ui.GetString("TAUREF"));
      outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
             AddKeyword(taurefKey,Pvl::Replace);
    } else {
      if (!outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
                  HasKeyword("TAUREF")) {
        string message = "The " + atmName + " Atmospheric model requires a value for the TAUREF parameter.";
        message += "The normal range for TAUREF is: 0 <= TAUREF";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("WHA")) {
      PvlKeyword whaKey("WHA");
      OutputKeyValue(whaKey, ui.GetString("WHA"));
      outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
             AddKeyword(whaKey, Pvl::Replace);
    } else {
      if (!outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
                  HasKeyword("WHA")) {
        string message = "The " + atmName + " Atmospheric model requires a value for the WHA parameter.";
        message += "The normal range for WHA is: 0 < WHA < 1";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.GetString("NULNEG") == "YES") {
      outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
             AddKeyword(PvlKeyword("NULNEG","YES"),Pvl::Replace);
    } else if (ui.GetString("NULNEG") == "NO") {
      outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
             AddKeyword(PvlKeyword("NULNEG","NO"),Pvl::Replace);
    } else {
      if (!outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
                  HasKeyword("NULNEG")) {
        string message = "The " + atmName + " Atmospheric model requires a value for the NULNEG parameter.";
        message += "The valid values for NULNEG are: YES, NO";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
  }
  if (atmName == "ANISOTROPIC1" || atmName == "ANISOTROPIC2") {
    if (ui.WasEntered("BHA")) {
      PvlKeyword bhaKey("BHA");
      OutputKeyValue(bhaKey, ui.GetString("BHA"));
      outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
             AddKeyword(bhaKey, Pvl::Replace);
    } else {
      if (!outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
                  HasKeyword("BHA")) {
        string message = "The " + atmName + " Atmospheric model requires a value for the BHA parameter.";
        message += "The normal range for BHA is: -1 <= BHA <= 1";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
  }
  if (atmName == "HAPKEATM1" || atmName == "HAPKEATM2") {
    if (ui.WasEntered("HGA")) {
      PvlKeyword hgaKey("HGA");
      OutputKeyValue(hgaKey, ui.GetString("HGA"));
      outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
             AddKeyword(hgaKey, Pvl::Replace);
    } else {
      if (!outPvl.FindObject("AtmosphericModel").FindGroup("Algorithm").
                  HasKeyword("HGA")) {
        string message = "The " + atmName + " Atmospheric model requires a value for the HGA parameter.";
        message += "The normal range for HGA is: -1 < HGA < 1";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
  }
}
