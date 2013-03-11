/**
 * @file
 * $Revision: 1.10 $
 * $Date: 2010/01/04 18:01:31 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software
 *   and related material nor shall the fact of distribution constitute any such
 *   warranty, and no responsibility is assumed by the USGS in connection
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

#include "IString.h"
#include "Message.h"
#include "IException.h"
#include "PvlTranslationManager.h"

using namespace std;
namespace Isis {

  PvlTranslationManager::PvlTranslationManager(const QString &transFile) {
    AddTable(transFile);
  }

  /**
   * Constructs and initializes a TranslationManager object
   *
   * @param inputLabel The Pvl holding the input label.
   *
   * @param transFile The translation file to be used to tranlate keywords in
   *                  the input label.
   */
  PvlTranslationManager::PvlTranslationManager(Isis::Pvl &inputLabel,
      const QString &transFile) {
    p_fLabel = inputLabel;

    // Internalize the translation table
    AddTable(transFile);
  }

  /**
   * Constructs and initializes a TranslationManager object
   *
   * @param inputLabel The Pvl holding the input label.
   *
   * @param transStrm A stream containing the tranlation table to be used to
   *                  tranlate keywords in the input label.
   */
  PvlTranslationManager::PvlTranslationManager(Isis::Pvl &inputLabel,
      std::istream &transStrm) {
    p_fLabel = inputLabel;

    // Internalize the translation table
    AddTable(transStrm);
  }

  /**
   * Returns a translated value. The output name is used to find the input
   * group, keyword, default and tranlations in the translation table. If the
   * keyword does not exist in the input label, the input default if
   * available will be used as the input value. This input value
   * is then used to search all of the translations. If a match is
   * found the translated value is returned.
   *
   * @param nName The output name used to identify the input keyword to be
   *              translated.
   *
   * @param findex The index into the input keyword array.  Defaults to 0
   *
   * @return string
   */
  QString PvlTranslationManager::Translate(QString nName, int findex) {
    const Isis::PvlContainer *con;
    int inst = 0;
    PvlKeyword grp;

    while((grp = InputGroup(nName, inst++)).name() != "") {
      if((con = GetContainer(grp)) != NULL) {
        if(con->hasKeyword(InputKeywordName(nName))) {
          return PvlTranslationTable::Translate(nName,
                                                (*con)[InputKeywordName(nName)][findex]);
        }
      }
    }

    return Isis::PvlTranslationTable::Translate(nName);
  }

  /**
   * Translate the requested output name to output values using the input name
   * and values or default value
   *
   * @param nName The output name used to identify the input keyword to be
   *              translated.
   *
   * @return Isis::PvlKeyword
   */
  Isis::PvlKeyword PvlTranslationManager::DoTranslation(
    const QString nName) {
    const Isis::PvlContainer *con = NULL;
    Isis::PvlKeyword key;

    int inst = 0;
    PvlKeyword grp;

    while((grp = InputGroup(nName, inst++)).name() != "") {
      if((con = GetContainer(grp)) != NULL) {
        if(con->hasKeyword(InputKeywordName(nName))) {
          key.setName(OutputName(nName));

          for(int v = 0; v < (*con)[(InputKeywordName(nName))].size(); v++) {
            key.addValue(Isis::PvlTranslationTable::Translate(nName,
                         (*con)[InputKeywordName(nName)][v]),
                         (*con)[InputKeywordName(nName)].unit(v));
          }

          return key;
        }
      }
    }

    return Isis::PvlKeyword(OutputName(nName),
                            PvlTranslationTable::Translate(nName, ""));
  }



  // Automatically translate all the output names found in the translation table
  // If a output name does not translate an error will be thrown by one
  // of the support members
  // Store the translated key, value pairs in the argument pvl
  void PvlTranslationManager::Auto(Isis::Pvl &outputLabel) {
    // Attempt to translate every group in the translation table
    for(int i = 0; i < TranslationTable().groups(); i++) {
      Isis::PvlGroup &g = TranslationTable().group(i);
      if(IsAuto(g.name())) {
        try {
          Isis::PvlContainer *con = CreateContainer(g.name(), outputLabel);
          (*con) += PvlTranslationManager::DoTranslation(g.name());
        }
        catch(IException &e) {
          if(!IsOptional(g.name())) {
            throw;
          }
        }
      }
    }
  }

  /**
   * Returns the ith input value assiciated with the output name argument.
   *
   * @param nName The output name used to identify the input keyword.
   *
   * @param findex The index into the input keyword array.  Defaults to 0
   *
   * @throws Isis::IException::Programmer
   */
  const PvlKeyword &PvlTranslationManager::InputKeyword(
    const QString nName) const {

    int instanceNumber = 0;
    PvlKeyword inputGroupKeyword = InputGroup(nName, instanceNumber);
    bool anInputGroupFound = false;

    while(inputGroupKeyword.name() != "") {
      const PvlContainer *containingGroup = GetContainer(inputGroupKeyword);
      if(containingGroup != NULL) {
        anInputGroupFound = true;

        if(containingGroup->hasKeyword(InputKeywordName(nName))) {
          return containingGroup->findKeyword(InputKeywordName(nName));
        }
      }

      instanceNumber ++;
      inputGroupKeyword = InputGroup(nName, instanceNumber);
    }

    if(anInputGroupFound) {
      QString msg = "Unable to find input keyword [" + InputKeywordName(nName) +
                   "] for output name [" + nName + "] in file [" + TranslationTable().fileName() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else {
      QString container = "";

      for(int i = 0; i < InputGroup(nName).size(); i++) {
        if(i > 0) container += ",";

        container += InputGroup(nName)[i];
      }

      QString msg = "Unable to find input group [" + container +
                   "] for output name [" + nName + "] in file [" + TranslationTable().fileName() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Indicates if the input keyword corresponding to the output name exists in
   * the label
   *
   * @param nName The output name used to identify the input keyword.
   */
  bool PvlTranslationManager::InputHasKeyword(const QString nName) {

    // Set the current position in the input label pvl
    // by finding the input group corresponding to the output group
    const Isis::PvlContainer *con;
    int inst = 0;
    //while ((con = GetContainer(InputGroup(nName, inst++))) != NULL) {
    //if ((con = GetContainer (InputGroup(nName))) != NULL) {

    PvlKeyword grp;
    while((grp = InputGroup(nName, inst++)).name() != "") {
      if((con = GetContainer(grp)) != NULL) {
        if(con->hasKeyword(InputKeywordName(nName))) return true;
      }
    }

    return false;
  }

  /*
   * Indicates if the input group corresponding to the output name exists in
   * the label
   *
   * @param nName The output name used to identify the input keyword.

   bool PvlTranslationManager::InputHasGroup (const QString nName) {

     if (GetContainer (InputGroup(nName)) != NULL) {
       return true;
     }

     return false;
   }
  */

  // Return a container from the input label according tund
  const Isis::PvlContainer *PvlTranslationManager::GetContainer(
    const PvlKeyword &inputGroup) const {


    // Return the root container if "ROOT" is the ONLY thing in the list
    if(inputGroup.size() == 1 &&
        PvlKeyword::stringEqual(inputGroup[0], "ROOT")) {
      return &p_fLabel;
    }

    const Isis::PvlObject *currentObject = &p_fLabel;

    // Search for object containing our solution
    int objectIndex;
    for(objectIndex = 0;
        objectIndex < inputGroup.size() - 1;
        objectIndex ++) {
      if(currentObject->hasObject(inputGroup[objectIndex])) {
        currentObject = &currentObject->findObject(inputGroup[objectIndex]);
      }
      else {
        return NULL;
      }
    }

    // Our solution can be an object or a group
    if(currentObject->hasObject(inputGroup[objectIndex])) {
      return &currentObject->findObject(inputGroup[objectIndex]);
    }
    else if(currentObject->hasGroup(inputGroup[objectIndex])) {
      return &currentObject->findGroup(inputGroup[objectIndex]);
    }
    else {
      return NULL;
    }
  }


  // Create the requsted container and any containers above it and
  // return a reference to the container
  // list is an Isis::PvlKeyword with an array of container types an their names
  Isis::PvlContainer *PvlTranslationManager::CreateContainer(const QString nName,
      Isis::Pvl &pvl) {

    // Get the array of Objects/Groups from the OutputName keyword
    Isis::PvlKeyword np = OutputPosition(nName);

    Isis::PvlObject *obj = &pvl;

    // Look at every pair in the output position
    for(int c = 0; c < np.size(); c += 2) {
      // If this pair is an object
      if(np[c].toUpper() == "OBJECT") {
        // If the object doesn't exist create it
        if(!obj->hasObject(np[c+1])) {
          obj->addObject(np[c+1]);
        }
        obj = &(obj->findObject(np[c+1]));
      }
      // If this pair is a group
      else if(np[c].toUpper() == "GROUP") {
        // If the group doesn't exist create it
        if(!obj->hasGroup(np[c+1])) {
          obj->addGroup(np[c+1]);
        }
        return (Isis::PvlContainer *) & (obj->findGroup(np[c+1]));

      }
    }

    return (Isis::PvlContainer *) obj;
  }
} // end namespace isis
