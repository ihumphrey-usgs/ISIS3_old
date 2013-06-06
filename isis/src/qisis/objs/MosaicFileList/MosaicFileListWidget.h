#ifndef MosaicFileListWidget_H
#define MosaicFileListWidget_H

#include <QWidget>

class QProgressBar;
class QSettings;
class QTreeWidgetItem;

namespace Isis {
  class CubeDisplayProperties;
  class MosaicTreeWidget;
  class MosaicTreeWidgetItem;
  class PvlObject;

  /**
   * @brief A colored, grouped cube list
   *
   * @author 2011-07-29 Steven Lambright
   *
   * @internal
   *   @history 2011-07-29 Steven Lambright - Expansion state is now stored in
   *                           the project file. This change will cause older
   *                           versions of qmos to fail to read newer project
   *                           files. References #275.
   *   @history 2011-08-12 Steven Lambright - Added export options,
   *                           references #342
   *   @history 2011-08-29 Steven Lambright - Reworded save file list export
   *                           action, references #342
   *   @history 2011-09-27 Steven Lambright - Improved user documentation
   *   @history 2012-12-27 Steven Lambright - Added support for projects that
   *                           are missing column visibility states; this is
   *                           to support old projects (made before Phase Angle
   *                           was added as a column). References #1093.
   *   @history 2013-03-19 Steven Lambright - Added setDefaultFileListCols().
   */
  class MosaicFileListWidget : public QWidget {
      Q_OBJECT
    public:
      MosaicFileListWidget(QSettings &settings, QWidget *parent = 0);
      virtual ~MosaicFileListWidget();

      QProgressBar *getProgress();
      void fromPvl(PvlObject &pvl);
      PvlObject toPvl() const;

      QList<QAction *> getViewActions();
      QList<QAction *> getExportActions();

      void setDefaultFileListCols();

      static QWidget * getLongHelp(QWidget * fileListContainer = NULL);

    public slots:
      void addCubes(QList<CubeDisplayProperties *>);

    private slots:
      void saveList();

    private:
      MosaicTreeWidgetItem *takeItem(QString filename,
                                     QList<QTreeWidgetItem *> &items);
      MosaicTreeWidget *p_tree; //!< Tree item associated with this mosaic item
  };
}

#endif

