#ifndef MainWindow_h
#define MainWindow_h
#include <QMainWindow>
#include <QtGui>
#include <QSettings>
#include "FileName.h"

namespace Isis {
  /**
  * @brief Base class for the Qisis main windows
  *
  * @ingroup Visualization Tools
  *
  * @author ????-??-?? Stacy Alley
  *
  * @internal
  */
  class MainWindow : public QMainWindow {
      Q_OBJECT
    public:
      MainWindow(QString title, QWidget *parent = 0, Qt::WFlags flags = 0);
      virtual ~MainWindow();
      virtual void closeEvent(QCloseEvent *event);

      virtual void hideEvent(QHideEvent *event);
      virtual void readSettings();
      virtual void writeSettings() const;

    protected:
      virtual bool eventFilter(QObject *o, QEvent *e);

    private:
      std::string p_appName; //!< Application name.

  };
};

#endif

