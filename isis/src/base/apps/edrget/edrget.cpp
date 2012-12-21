#include "Isis.h"

#include <iostream>

#include <QtCore>

#include "httpget.h"
#include "ftpget.h"
#include "UserInterface.h"
#include "ProgramLauncher.h"
#include "IString.h"
#include "IException.h"

using namespace Isis;
using namespace std;

void IsisMain() {

  // Get the file name from the GUI
  UserInterface &ui = Application::GetUserInterface();
  QString guiURL = ui.GetString("URL");
  QString guiPath;
  if(ui.WasEntered("TOPATH")) {
    guiPath = ui.GetString("TOPATH");
  }
  QUrl qurl(guiURL);
  //test if scheme is ftp and set port
  if(qurl.scheme().toLower() == "ftp") {
    qurl.setPort(21);

    if(ui.IsInteractive()) {
      QString parameters = "URL=" + guiURL;
      if(ui.WasEntered("TOPATH")) {
        parameters += " TOPATH=" + guiPath;
      }
      ProgramLauncher::RunIsisProgram("edrget", parameters);
    }
    else {

      FtpGet getter;
      QObject::connect(&getter, SIGNAL(done()), QCoreApplication::instance(), SLOT(quit()));
      //a false getFile return means no error and we sould execute the get.
      if(!getter.getFile(qurl, guiPath))  QCoreApplication::instance()->exec();
      //if error occurred throw could not acquire
      if(getter.error()) {
        QString localFileName;
        if(ui.WasEntered("TOPATH")) {
          localFileName += guiPath;
          localFileName += "/";
        }
        localFileName +=  QFileInfo(qurl.path()).fileName();
        QString localFileNameStr(localFileName);
        QFile::remove(localFileNameStr);
        QString msg = "Could not acquire [" + guiURL + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
  }
  //test is scheme is http and set port
  else if(qurl.scheme().toLower() == "http") {
    qurl.setPort(80);

    if(ui.IsInteractive()) {
      QString parameters = "URL=" + guiURL;
      if(ui.WasEntered("TOPATH")) {
        parameters += " TOPATH=" + guiPath;
      }
      ProgramLauncher::RunIsisProgram("edrget", parameters);
    }
    else {
      HttpGet getter;
      QObject::connect(&getter, SIGNAL(done()), QCoreApplication::instance(), SLOT(quit()));
      //a false getFile return means no error and we sould execute the get.
      if(!getter.getFile(qurl, guiPath)) QCoreApplication::instance()->exec();
      //if error occurred then throw could not acquire
      if(getter.error()) {
        QString localFileName;
        if(ui.WasEntered("TOPATH")) {
          localFileName += guiPath;
          localFileName += "/";
        }
        QString localFileNameStr(localFileName);
        QFile::remove(localFileNameStr);
        QString msg = "Could not acquire [" + guiURL + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
  }
  //if scheme is not ftp or http throw error
  else {
    QString msg = "Scheme [" + qurl.scheme() + "] not found, must be 'ftp' or 'http'";
    throw IException(IException::User, msg, _FILEINFO_);
  }
}

