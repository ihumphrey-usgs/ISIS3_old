#include "IsisDebug.h"

#include "iException.h"
#include "MosaicMainWindow.h"
#include "Preference.h"
#include "PvlGroup.h"
#include "QIsisApplication.h"

void startMonitoringMemory();
void stopMonitoringMemory();

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
#ifdef CWDEBUG
  startMonitoringMemory();
#endif

  try {
    QApplication *app = new Qisis::QIsisApplication(argc, argv);
    QApplication::setApplicationName("qmos");

    // check for forcing of gui style
//     PvlGroup &uiPref = Isis::Preference::Preferences().FindGroup(
//                                "UserInterface");
//     if(uiPref.HasKeyword("GuiStyle")) {
//       string style = uiPref["GuiStyle"];
//       QApplication::setStyle((Isis::iString) style);
//     }

    MosaicMainWindow *mainWindow = new MosaicMainWindow("qmos");

    mainWindow->show();

    if(argc == 2) {
      mainWindow->loadProject(argv[1]);
    }
    else if(argc > 2) {
      std::cerr << "Usage: qmos [project file]" << std::endl;
      return 1;
    }

    int status = app->exec();

    delete mainWindow;
    delete app;

    return status;
  }
  catch(iException &e) {
    e.Report();
  }

}


void startMonitoringMemory() {
#ifdef CWDEBUG
#ifndef NOMEMCHECK
  MyMutex *mutex = new MyMutex();
  std::fstream *alloc_output = new std::fstream("/dev/null");
  Debug(make_all_allocations_invisible_except(NULL));
  ForAllDebugChannels(if(debugChannel.is_on()) debugChannel.off());
  Debug(dc::malloc.on());
  Debug(libcw_do.on());
  Debug(libcw_do.set_ostream(alloc_output));
  Debug(libcw_do.set_ostream(alloc_output, mutex));
  atexit(stopMonitoringMemory);
#endif
#endif
}


void stopMonitoringMemory() {
#ifdef CWDEBUG
#ifndef NOMEMCHECK
  Debug(
    alloc_filter_ct alloc_filter;
    std::vector<std::string> objmasks;
    objmasks.push_back("libc.so*");
    objmasks.push_back("libstdc++*");
    std::vector<std::string> srcmasks;
    srcmasks.push_back("*new_allocator.h*");
    srcmasks.push_back("*set_ostream.inl*");
    alloc_filter.hide_objectfiles_matching(objmasks);
    alloc_filter.hide_sourcefiles_matching(srcmasks);
    alloc_filter.hide_unknown_locations();
    delete libcw_do.get_ostream();
    libcw_do.set_ostream(&std::cout);
    list_allocations_on(libcw_do, alloc_filter);
    dc::malloc.off();
    libcw_do.off()
  );
#endif
#endif
}

