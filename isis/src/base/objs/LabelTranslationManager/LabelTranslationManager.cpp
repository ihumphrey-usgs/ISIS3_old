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
#include "PvlTranslationTable.h"

#include "IException.h"
#include "IString.h"
#include "LabelTranslationManager.h"
#include "Message.h"
#include "Pvl.h"
#include "PvlContainer.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"

using namespace std;
namespace Isis {

  /**
   * Constructs a default LabelTranslationManager.
   */
  LabelTranslationManager::LabelTranslationManager()
      : PvlTranslationTable() {
  }


  /**
   * Constructs a LabelTranslationManager with a given translation table.
   *
   * @param transfile The translation table file.
   */
  LabelTranslationManager::LabelTranslationManager(const QString &transFile)
      : PvlTranslationTable() {
    AddTable(transFile);
  }


  /**
   * Constructs and initializes a LabelTranslationManager object
   *
   * @param transStrm A stream containing the tranlation table to be used to
   *                  tranlate keywords in the input label.
   */
  LabelTranslationManager::LabelTranslationManager(std::istream &transStrm)
      : PvlTranslationTable() {
    AddTable(transStrm);
  }


  /**
   * Destroys the LabelTranslationManager object.
   */
  LabelTranslationManager::~LabelTranslationManager() {
  }


  /**
   * Automatically translate all the output names tagged as Auto in the
   * translation table If a output name does not translate an error will be
   * thrown by one of the support members.
   *
   * The results of the translations will be stored in the outputLabel PVL
   * based on the OutputPosition keywords in the translation table.
   *
   * @param outputLabel The PVL to add the translated keywords to.
   */
  void LabelTranslationManager::Auto(Pvl &outputLabel) {
    // Attempt to translate every group in the translation table
    for(int i = 0; i < TranslationTable().groups(); i++) {
      PvlGroup &g = TranslationTable().group(i);
      if(IsAuto(g.name())) {
        try {
          PvlContainer *con = CreateContainer(g.name(), outputLabel);
          (*con) += DoTranslation(g.name());
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
   * Creates all parent PVL containers for an output keyword. If any parent
   * containers already exist then they will not be recreated.
   *
   * @param nName The name of the output keyword. The OutputPosition keyword
   *              in the translation group for nName will be used to determine
   *              which containers are made.
   * @param pvl The PVL file to create the containers in.
   *
   * @return @b PvlContainer The immediate parent container for nName.
   */
  PvlContainer *LabelTranslationManager::CreateContainer(const QString nName,
                                                         Pvl &pvl) {

    // Get the array of Objects/Groups from the OutputName keyword
    PvlKeyword np = OutputPosition(nName);

    PvlObject *obj = &pvl;

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
        return (PvlContainer *) & (obj->findGroup(np[c+1]));

      }
    }

    return (PvlContainer *) obj;
  }


  /**
   * Translate the requested output name to output values using the input name
   * and values or default value
   *
   * @param outputName The output name used to identify the input keyword to be
   *                   translated.
   *
   * @return @b PvlKeyword A keyword containing the output name and output value.
   *
   * @TODO output units
   */
  PvlKeyword LabelTranslationManager::DoTranslation(const QString outputName) {
    PvlKeyword outputKeyword( outputName, Translate(outputName) );
    return outputKeyword;
  }


  /**
 * Parses and validates a dependency specification.
 *
 * @param specification The dependency specification string.
 *
 * @return @b QStringList The dependency split into 3 components
 *                        <ol>
 *                          <li>the type (att or tag)</li>
 *                          <li>the name of what to check</li>
 *                          <li>the value to check for</li>
 *                        </ol>
 *
 * @throws IException::Unknown "Malformed dependency specification."
 * @throws IException::Unknown "Specification does not have two components
 *                              separated by [@], the type of dependency and
 *                              the name-value pair.
 * @throws IException::Unknown "Dependency type specification is invalid.
 *                              Valid types are [att] and [tag]"
 * @throws IException::Unknown "Name-value specification does not have two
 *                              components separated by [:]."
 *
 */
QStringList LabelTranslationManager::parseSpecification(QString specification) const {

  QStringList parsedSpecification;

  try {
    QStringList typeSplit = specification.split("@", QString::SkipEmptyParts);
    QStringList colonSplit = specification.split(":", QString::SkipEmptyParts);
    if (typeSplit.size() == 2) { //handle tag@elementname:value
      if (typeSplit[0].toLower() != "att" &&
          typeSplit[0].toLower() != "tag" &&
          typeSplit[0].toLower() != "new") {
        QString msg = "Dependency type specification [" + typeSplit[0] +
                      "] is invalid. Valid types are [att], [tag] and [new]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      parsedSpecification.append(typeSplit[0].toLower());

      QStringList nameValueSplit = typeSplit[1].split(":", QString::SkipEmptyParts);
      if (nameValueSplit.size() == 2) {
        parsedSpecification.append(nameValueSplit);
      }
      else if (nameValueSplit.size() == 1) {
        parsedSpecification.append(nameValueSplit);
      }
      else { //nameValueSplit is an unexpected value
        QString msg = "Malformed dependency specification [" + specification + "].";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }
    else if (colonSplit.size() == 2) { //handle elementname:value
      parsedSpecification = colonSplit;
    }
    else if (colonSplit.size() == 1 && typeSplit.size() == 1) { //handle value with no "@" or ":" characters
      parsedSpecification = colonSplit;
    }
    else { //nameValueSplit is an unexpected value
      QString msg = " [" + specification + "] has unexpected number of '@' or ':' delimiters";
      throw IException(IException::Unknown,msg, _FILEINFO_);
    }
  }

  catch (IException &e) {
    QString msg = "Malformed dependency specification [" + specification + "].";
    throw IException(e, IException::Unknown, msg, _FILEINFO_);
  }

  return parsedSpecification;
}

} // end namespace isis
