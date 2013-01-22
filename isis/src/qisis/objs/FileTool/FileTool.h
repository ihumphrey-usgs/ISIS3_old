#ifndef FileTool_h
#define FileTool_h

#include "Tool.h"

#include <QDir>
#include <QStringList>

class QAction;

namespace Isis {
  class Brick;
  class Buffer;
  class Cube;
  class CubeAttributeOutput;
}

namespace Isis {
  class MdiCubeViewport;
  class SaveAsDialog;

  /**
   * @author ????-??-?? Unknown
   *
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Added "What's this?"
   *              and shortcut to "Save" action
   *   @history 2010-06-26 Eric Hyer - Now uses MdiCubeViewport
   *   @history 2011-04-05 Sharmila Prasad - Added SaveInfo option to save
   *              the current cubeviewport's whatsthis info
   *   @history 2011-05-11 Sharmila Prasad - Added SaveAsDialog to select the output
   *                   cube and options to save FullImage, ExportAsIs, ExportFullRes
   *   @history  2012-02-01 Sharmila Prasad - Fixed bug #0000681 - reduce in ISIS 3.3.0
   *                   is ignoring the +N band identifier
   *   @history  2012-05-24 Steven Lambright - Minor changes to support prompting to save on exit
   *                            once again (this has been broken for a very long time). The prompt
   *                            now appears if you have edited your file but not saved it - not when
   *                            clicking "Save." This was a minimal fix (I left a lot of problems
   *                            to be solved at a later date). Fixes #854.
   *   @history  2012-06-04 Steven Lambright - Fixed a problem with the save button. References
   *                            #854.
   *   @history  2013-01-07 Tracie Sucharski - Added AlphaCube to output cube for saving the
   *                            full resolution viewport image.  Fixes # 706.
   *   @history  2013-01-09 Tracie Sucharski - Fixed saveAsCubeByOption which was not creating the
   *                            cube for the case where the scale was 1 and saveAs_FullResolution
   *                            was being called without an initialized ocube.  Fixes #1386.
   *   @history  2013-01-17 Tracie Sucharski - Fixed some round-off errors when calculating
   *                            output lines/samples.  Fixed printing of Results pvl group where
   *                            samples and lines were swapped.  References #1385.
   */
  class FileTool : public Tool {
      Q_OBJECT

    public:
      FileTool(QWidget *parent);
      void addTo(QMenu *menu);
      void addTo(Workspace *ws);
      void addToPermanent(QToolBar *perm);
      //! Returns the open action
      QAction *openAction() {
        return p_open;
      };
      //! Returns the save as action
      QAction *saveAction() {
        return p_saveAs;
      };
      QStringList p_filterList; //!< Filter List
      QDir p_dir;               //!< Directory
      QStringList p_fileList;   //!< File list

    signals:
      void fileSelected(QString); //!< This signal is called when a file is selected
      /**
       * This signal is called when changes should be saved
       *
       * @param vp
       */
      void saveChanges(CubeViewport *vp);
      /**
       * This signal is called when changes should be discarded
       *
       * @param vp
       */
      void discardChanges(CubeViewport *vp);

    public slots:
      virtual void open();
      virtual void browse();
      void print();
      virtual void save();
      virtual void saveAs();
      virtual void saveInfo(); //!< Saves the whatsthis info of the cubeviewport
      virtual void exportView();
      virtual bool closeAll();
      virtual void exit();
      void enableSave(bool enable);
      void discard();
      void saveAsCubeByOption(QString); //!< Save as Isis Cube (FullImage, AsIs, FullRes)

    protected:
      //! Returns the menu name for the file tool
      QString menuName() const {
        return "&File";
      };
      //! Updates the tool
      void updateTool();

    private:
      static void copy(Buffer &in, Buffer &out);
      QAction *p_open;   //!< Action to open a file
      QAction *p_browse; //!< Action to browse and open files
      QAction *p_print;  //!< Action to print the current view
      QAction *p_save;   //!< Action to save the current cube
      QAction *p_saveAs; //!< Action save the current cube as a user specified file
      QAction *p_saveInfo;   //!< Action to save the current cube's Whatsthis info
      QAction *p_exportView; //!< Action to export the view as a picture
      QAction *p_closeAll;   //!< Action to close all windows
      QAction *p_exit;       //!< Action to exit qview
      QWidget *p_parent;     //!< The parent widget of this object
      QString p_lastDir;     //!< The last directory opened
      Workspace *p_workSpace;          //!< The workspace being used
      MdiCubeViewport *p_lastViewport; //!< The last cubeviewport that was used
      SaveAsDialog *p_saveAsDialog;    //!< SaveAs Dialog with different save options

      //! Save Image in its entirety to an output file
      void saveAsFullImage(Cube *icube, Cube *ocube);

      //! Copy input cube details into output file given its dimensions
      void copyCubeDetails(const QString & psFileName, Cube *icube,
           Cube *ocube, int piNumSamples, int piNumLines, int piNumBands);

      //! Save image AsIs (As viewed in the viewport window) into output file
      void saveAs_AsIs(Cube *icube, const QString & psOutFile);

      //! Save image Full Resolution (image viewed in the viewport window) into output
      void saveAs_FullResolution(Cube *pInCube, Cube *pOutCube,
                                 int pNumSamples, int pNumLines);

      //! Save image AsIs Enlarged into output
      void saveAsEnlargedCube(Cube *icube, const QString & psOutFile);

      //! Save image AsIs Reduced into output
      void saveAsReducedCube (Cube *icube, const QString & psOutFile);
  };
};

#endif
