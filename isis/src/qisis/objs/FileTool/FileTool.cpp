#include "FileTool.h"

#include <QApplication>
#include <QFileDialog>
#include <QImage>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPrinter>
#include <QPrintDialog>

#include <cmath>

#include "Brick.h"
#include "BrowseDialog.h"
#include "CubeAttribute.h"
#include "Enlarge.h"
#include "FileDialog.h"
#include "Interpolator.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "OriginalLabel.h"
#include "Portal.h"
#include "ProcessRubberSheet.h"
#include "ProcessByLine.h"
#include "Pvl.h"
#include "Reduce.h"
#include "SaveAsDialog.h"
#include "Workspace.h"

namespace Isis {
  /**
   * Constructs a FileTool object.
   *
   * @param parent
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Added "What's this?"
   *            and shortcut to "Save" action
   */
  FileTool::FileTool(QWidget *parent) : Tool(parent) {
    p_parent = parent;
    p_dir = "/thisDirDoesNotExist!";
    p_open = new QAction(parent);
    p_open->setShortcut(Qt::CTRL + Qt::Key_O);
    p_open->setText("&Open...");
    p_open->setIcon(QPixmap(toolIconDir() + "/fileopen.png"));
    p_open->setToolTip("Open cube");
    QString whatsThis =
      "<b>Function:</b> Open an <i>Isis cube</i> in new viewport \
       <p><b>Shortcut:</b>  Ctrl+O\n</p> \
       <p><b>Hint:</b> Use Ctrl or Shift in file dialog to open \
       multiple cubes</p>";
    p_open->setWhatsThis(whatsThis);
    connect(p_open, SIGNAL(activated()), this, SLOT(open()));

    p_browse = new QAction(parent);
    p_browse->setShortcut(Qt::CTRL + Qt::Key_B);
    p_browse->setText("&Browse...");
    p_browse->setToolTip("Browse cubes");
    whatsThis =
      "<b>Function:</b> Browse a <i>Isis cubes</i> in new viewport \
       <p><b>Shortcut:</b>  Ctrl+B\n</p>";
    p_browse->setWhatsThis(whatsThis);
    connect(p_browse, SIGNAL(activated()), this, SLOT(browse()));

    p_save = new QAction(parent);
    p_save->setShortcut(Qt::CTRL + Qt::Key_S);
    p_save->setText("&Save");
    p_save->setIcon(QPixmap(toolIconDir() + "/filesave.png"));
    p_save->setToolTip("Save");
    whatsThis =
      "<b>Function:</b> Save changes to the current Cube \
       <p><b>Shortcut:</b> Ctrl+S</p>";
    p_save->setWhatsThis(whatsThis);
    connect(p_save, SIGNAL(activated()), this, SLOT(confirmSave()));
    p_save->setEnabled(false);

    p_saveAs = new QAction(parent);
    p_saveAs->setText("Save &As...");
    p_saveAs->setIcon(QPixmap(toolIconDir() + "/filesaveas.png"));
    p_saveAs->setToolTip("Save As");
    whatsThis =
      "<b>Function:</b> Save the current Cube to the specified location";
    p_saveAs->setWhatsThis(whatsThis);
    connect(p_saveAs, SIGNAL(activated()), this, SLOT(saveAs()));
    p_saveAs->setEnabled(false);
    
    p_saveInfo = new QAction(parent);
    p_saveInfo->setText("Save &Info...");
    p_saveInfo->setIcon(QPixmap(toolIconDir() + "/filesaveas.png"));
    p_saveInfo->setToolTip("Save Info");
    whatsThis =
      "<b>Function:</b> Save the current Cube's Whatsthis Info to the specified location";
    p_saveInfo->setWhatsThis(whatsThis);
    connect(p_saveInfo, SIGNAL(activated()), this, SLOT(saveInfo()));
    p_saveInfo->setEnabled(false);
    
    p_exportView = new QAction(parent);
    p_exportView->setText("Export View");
    p_exportView->setIcon(QPixmap(toolIconDir() + "/fileexport.png"));
    p_exportView->setToolTip("Export View");
    whatsThis =
      "<b>Function:</b> Save visible contents of the active \
       viewport as a png, jpg, tiff \
       <p><b>Hint:</b>  Your local installation of Qt may not support \
       all formats.  Reinstall Qt if necessary</p>";
    p_exportView->setWhatsThis(whatsThis);
    connect(p_exportView, SIGNAL(activated()), this, SLOT(exportView()));
    p_exportView->setEnabled(false);

    p_print = new QAction(parent);
    p_print->setText("&Print...");
    p_print->setShortcut(Qt::CTRL + Qt::Key_P);
    p_print->setIcon(QPixmap(toolIconDir() + "/fileprint.png"));
    p_print->setToolTip("Print");
    whatsThis =
      "<b>Function:</b> Print visible contents of the active viewport \
      <p><b>Shortcut:</b> Ctrl+P</b>";
    p_print->setWhatsThis(whatsThis);
    connect(p_print, SIGNAL(activated()), this, SLOT(print()));
    p_print->setEnabled(false);

    p_closeAll = new QAction(parent);
    p_closeAll->setText("&Close All...");
    p_closeAll->setToolTip("Close All");
    whatsThis =
      "<b>Function:</b> Close all cube viewports.";
    p_closeAll->setWhatsThis(whatsThis);

    p_exit = new QAction(parent);
    p_exit->setShortcut(Qt::CTRL + Qt::Key_Q);
    p_exit->setText("E&xit");
    p_exit->setIcon(QPixmap(toolIconDir() + "/fileclose.png"));
    whatsThis =
      "<b>Function:</b>  Quit qview \
      <p><b>Shortcut:</b> Ctrl+Q</p>";
    p_exit->setWhatsThis(whatsThis);
    connect(p_exit, SIGNAL(activated()), this, SLOT(exit()));

    p_lastDir.clear();
    p_lastViewport = NULL;

    p_saveAsDialog = NULL;
    activate(true);
  }

  /**
   * Adds the file tool's actions to the menu
   *
   * @param menu
   */
  void FileTool::addTo(QMenu *menu) {
    menu->addAction(p_open);
    menu->addAction(p_browse);
    menu->addAction(p_save);
    menu->addAction(p_saveAs);
    menu->addAction(p_saveInfo);
    menu->addAction(p_exportView);
    menu->addAction(p_print);
    menu->addAction(p_closeAll);
    menu->addAction(p_exit);
  }

  /**
   * Connects the fileSelected signal to the workspace's addCubeViewport slot
   *
   * @param ws
   */
  void FileTool::addTo(Workspace *ws) {
    p_workSpace = ws;
    Tool::addTo(ws);
    connect(this, SIGNAL(fileSelected(QString)),
            ws, SLOT(addCubeViewport(QString)));

    connect(p_closeAll, SIGNAL(activated()), ws, SLOT(closeAllSubWindows()));
  }

  /**
   * Adds the file tool's actions to the permanent toolbar
   *
   * @param perm
   */
  void FileTool::addToPermanent(QToolBar *perm) {
    perm->addAction(p_open);
    perm->addAction(p_exportView);
    perm->addAction(p_print);
    perm->addAction(p_exit);
  }

  /**
   * This method allows the user to navigate and open a cube with a file dialog.
   *
   */
  void FileTool::open() {
    //Set up the list of filters that are default with this dialog.
    if(!p_filterList.contains("Isis cubes (*.cub)")) {
      p_filterList.append("Isis cubes (*.cub)");
      p_filterList.append("All files (*)");
    }
    if(!p_dir.exists()) {
      p_dir = QDir::current();
    }

    FileDialog *fileDialog = new FileDialog("Open", p_filterList, p_dir, (QWidget *)parent());
    fileDialog->show();
    connect(fileDialog, SIGNAL(fileSelected(QString)),
            p_workSpace, SLOT(addCubeViewport(QString)));
  }

  /**
   * This method allows the user to navigate and browse cubes with a file dialog .
   *
   */
  void FileTool::browse() {
    //Set up the list of filters that are default with this dialog.
    if(!p_filterList.contains("Isis cubes (*.cub)")) {
      p_filterList.append("Isis cubes (*.cub)");
      p_filterList.append("All files (*)");
    }
    if(!p_dir.exists()) {
      p_dir = QDir::current();
    }
    BrowseDialog *browser = new BrowseDialog("Browse", p_filterList, p_dir, (QWidget *)parent());
    browser->show();
    connect(browser, SIGNAL(fileSelected(QString)),
            p_workSpace, SLOT(addBrowseView(QString)));
  }

  /**
   * This method asks the user to confirm if they want to finalize their saved
   * changes.
   *
   */
  void FileTool::confirmSave() {
    if(QMessageBox::question(
          p_parent,
          tr("Save File?"),
          tr("Are you sure you want to save this file?"),
          tr("&Save"), tr("&Cancel"),
          QString(), 0, 1)) {
      return;
    }
    else {
      save();
    }
  }

  /**
   * This method saves any changes made to the current cube, these
   * changes are finalized! There is no undoing once a save has
   * been made.
   *
   */
  void FileTool::save() {
    //If the current viewport is null (safety check), return from this method
    if(cubeViewport() == NULL) {
      QMessageBox::information((QWidget *)parent(), "Error", "No active cube to save");
      return;
    }
    //Emit a signal to notify other objects that this cube has been saved
    emit saveChanges(cubeViewport());
    //Disable the save action
    p_save->setEnabled(false);

    //Essentially, closing the cube saves it's changes, and we want to keep it open,
    //so reopen the current cube!
    cubeViewport()->cube()->reopen("rw");
  }
  
  /**
   * SaveAs Action - Displays the FileDialog with the filterlist (*.cub) to select
   * the output cube. This dialog additionally displays radio buttons for choices 
   * FullImage, ExportAsIs, ExportFullRes. These choices are located at the bottom 
   * of the dialog.
   * FullImage     - copies the entire image into the user specified output file 
   * ExportAsIs    - copies the image as displayed in the qview app window 
   * ExportFullRes - copies the image as displayed in the qview app window but with 
   *                 full resolution
   *  
   * @author Sharmila Prasad (4/8/2011)
   */
  void FileTool::saveAs() {
    //If the current viewport is null (safety check), return from this method
    if(cubeViewport() == NULL) {
      QMessageBox::information((QWidget *)parent(), "Error", "No active cube to save");
      return;
    }
    //Set up the list of filters that are default with this dialog.
    if(!p_filterList.contains("Isis cubes (*.cub)")) {
      p_filterList.append("Isis cubes (*.cub)");
    }
    if(!p_dir.exists()) {
      p_dir = QDir(p_lastDir);
    }
    if(p_saveAsDialog) {
      delete p_saveAsDialog;
      p_saveAsDialog = NULL;
    }

    p_saveAsDialog = new SaveAsDialog("Save As", p_filterList, p_dir, (QWidget *)parent());
    connect(p_saveAsDialog, SIGNAL(fileSelected(QString)), this, SLOT(saveAsCubeByOption(QString)));

    p_saveAsDialog->show();
  }

  /**
   * Save input image as a cube into specified output file as FullImage or 
   * ExportAsIs or ExportFullRes option
   * 
   * @author Sharmila Prasad (4/8/2011)
   * 
   * @param psOutFile - user specified output file 
   */
  void FileTool::saveAsCubeByOption(QString psOutFile) {
    //If the current viewport is null (safety check), return from this method
    if(cubeViewport() == NULL) {
      QMessageBox::information((QWidget *)parent(), "Error",
                               "No active cube to save");
      return;
    }

    //If the filename is empty, return
    if(psOutFile.isEmpty() || (p_saveAsDialog==NULL)){
      QMessageBox::information((QWidget *)parent(), "Error",
                               "No output file selected");
      return;
    }

    //Check if the output file is already opened
    QVector< MdiCubeViewport *> *vwportList = p_workSpace->cubeViewportList();
    QVector<MdiCubeViewport *>::iterator it;
    for (it = vwportList->begin(); it != vwportList->end(); ++it){
      if(QString((*it)->cube()->getFilename().c_str()) ==  psOutFile) {
        QMessageBox::information((QWidget *)parent(), "Error",
            "Output File is already open\n\""+ psOutFile + "\"");
        return;
      }
    }

    //If the filename is the same as the current cube's filename, just save it
    if(p_saveAsDialog->getSaveAsType() == SaveAsDialog::FullImage &&
       psOutFile.toStdString() == cubeViewport()->cube()->getFilename()) {
      save();
      return;
    }

    //Save the current cube's changes by reopening it, and open an input cube
    //from the current cube's location
    Cube *icube = new Cube(); 
    icube->open(cubeViewport()->cube()->getFilename(), "rw");
    Cube *ocube = NULL;

    if(p_saveAsDialog->getSaveAsType() != SaveAsDialog::ExportAsIs) {
      //Create the output cube
      ocube = new Cube;
    }

    if(p_saveAsDialog->getSaveAsType() == SaveAsDialog::FullImage) {
      copyCubeDetails(psOutFile, icube, ocube, icube->getSampleCount(),
                      icube->getLineCount(), icube->getBandCount());
      saveAsFullImage(icube, ocube);
      ocube->close();
    }
    else {
      // start and end Samples and Lines
      double dStartSample=0, dEndSample=0, dStartLine=0, dEndLine=0;
      p_lastViewport->getCubeArea(dStartSample, dEndSample, dStartLine, dEndLine);
      
      if(p_saveAsDialog->getSaveAsType() == SaveAsDialog::ExportFullRes || p_lastViewport->scale() == 1) {
        int numSamples = (int)ceil(dEndSample-dStartSample);
        int numLines = (int)ceil(dEndLine-dStartLine);
        copyCubeDetails(psOutFile, icube, ocube, numSamples, numLines, icube->getBandCount());
        saveAs_FullResolution(icube, ocube, numSamples, numLines);
        ocube->close();
      }
      else if(p_saveAsDialog->getSaveAsType() == SaveAsDialog::ExportAsIs ) {
        saveAs_AsIs(icube, psOutFile);
      }
    }

    emit(fileSelected(psOutFile));

    //Disable the save action
    p_save->setEnabled(false);

    p_lastDir = psOutFile;
  }
  
  /**
   * For AsIs option, save the enlarged input image visible in the viewport window
   * using the Enlarge functionality 
   *  
   * @author Sharmila Prasad (4/26/2011) 
   *  
   * @param pInCube - Input Cube
   * @param pOutCube - Output Cube 
   */
  void FileTool::saveAsEnlargedCube(Cube *icube,
                                    const QString & psOutFile) {
    double dScale = p_lastViewport->scale();

    // start and end Samples and Lines
    double dStartSample=0, dEndSample=0, dStartLine=0, dEndLine=0;
    p_lastViewport->getCubeArea(dStartSample, dEndSample, dStartLine, dEndLine);

    int ons, onl;
    ons = (int)ceil((dEndSample-dStartSample) * dScale);
    onl = (int)ceil((dEndLine-dStartLine) * dScale);

    try {
      ProcessRubberSheet p;
      p.SetInputCube (icube);
      Cube *ocube = p.SetOutputCube(psOutFile.toStdString(), CubeAttributeOutput(" "),
                       ons , onl, icube->getBandCount());
      
      Interpolator *interp = new Interpolator(Interpolator::NearestNeighborType);
      
      Enlarge *imgEnlarge  = new Enlarge(icube, dScale, dScale);
      imgEnlarge->SetInputArea((int)dStartSample, (int)dEndSample, (int)dStartLine, (int)dEndLine);

      p.StartProcess(*imgEnlarge, *interp);
      imgEnlarge->UpdateOutputLabel(ocube);
      p.EndProcess();
      
      delete imgEnlarge;
      delete interp;
      
      ocube->close();
    } catch(iException &e) {
      QMessageBox::critical((QWidget *)parent(),
                            "Error", "Cannot open file, please check permissions");
      e.Clear();
    }
  }

  /**
   * For AsIs option, save the reduced input image visible in the viewport window
   * using the Reduce functionality 
   * 
   * @author Sharmila Prasad (4/26/2011) 
   *  
   * @param pInCube - Input Cube
   * @param psOutFile - Output filename 
   */
  void FileTool::saveAsReducedCube(Cube *icube,
                                   const QString & psOutFile) {
    double dScale = p_lastViewport->scale();
    //dScale *= 10;
    // start and end Samples and Lines
    double dStartSample=0, dEndSample=0, dStartLine=0, dEndLine=0;
    p_lastViewport->getCubeArea(dStartSample, dEndSample, dStartLine, dEndLine);

    double ins, inl;
    ins = dEndSample-dStartSample;
    inl = dEndLine-dStartLine;
    
    int ons, onl;
    ons = (int)ceil(ins * dScale);
    onl = (int)ceil(inl * dScale);

    CubeAttributeInput cai(icube->getFilename());
    vector<string> bands = cai.Bands();
    int inb = bands.size();

    if(inb == 0) {
      inb = cubeViewport()->cube()->getBandCount();
      for(int i = 1; i <= inb; i++) {
        bands.push_back((iString)i);
      }
    }
    
    ProcessByLine p;
    p.SetInputCube (icube);
    Cube *ocube = NULL;
    try {
      ocube = p.SetOutputCube(psOutFile.toStdString(), CubeAttributeOutput(""), ons+1, onl+1, inb);
      // Our processing routine only needs 1
      // the original set was for info about the cube only
      p.ClearInputCubes();
    }
    catch(iException &e) {
      // If there is a problem, catch it and close the cube so it isn't open next time around
      icube->close();
      throw e;
    }

    Cube *tempcube=new Cube;
    tempcube->open(cubeViewport()->cube()->getFilename(), "r");
    Nearest *near = new Nearest(tempcube, bands, ins/ons, inl/onl);
    near->setInputBoundary((int)dStartSample, (int)dEndSample, (int)dStartLine, (int)dEndLine);
    
    p.StartProcessInPlace(*near);
    near->UpdateOutputLabel(ocube);
    p.EndProcess();
    
    delete near;
    near=NULL;
  }

  /**
   * AsIs option, save the input image visible in the viewport window Enlarged/Reduced
   * 
   * @author Sharmila Prasad (4/26/2011)
   * 
   * @param pInCube - Input Cube
   * @param pOutCube - Output Cube
   */
  void FileTool::saveAs_AsIs(Cube *icube, const QString & psOutFile) {
    double dScale = p_lastViewport->scale();

    // Enlarge the cube area
    if(dScale > 1) {
      saveAsEnlargedCube(icube, psOutFile);
    }
    // Reduce the cube area
    else {
      saveAsReducedCube(icube, psOutFile);
    }
  }

  /**
   * Copy input image details into the output given output images's dimension.
   * Info like instrument, history are transferred to output image 
   * 
   * @param icube        - input image
   * @param ocube        - output image
   * @param outAtt       - output cube attributes
   * @param piNumSamples - out samples
   * @param piNumLines   - out lines
   * @param piNumBands   - out bands
   *  
   * @history 2011-05-11 Sharmila Prasad - Isolated from original SaveAs function so that
   *                                           it can be used by different SaveAs options 
   */
  void FileTool::copyCubeDetails(const QString & psOutFile, Cube *icube,
      Cube *ocube, int piNumSamples, int piNumLines, int piNumBands) {
    //Create the default output attribute with the output filename
    CubeAttributeOutput outAtt(psOutFile.toStdString());

    //Propagate all labels, tables, blobs, etc from the input to output cube
    try {
      ocube->setDimensions(piNumSamples, piNumLines, piNumBands);
      ocube->setByteOrder(outAtt.ByteOrder());
      ocube->setFormat(outAtt.FileFormat());
      ocube->setLabelsAttached(outAtt.AttachedLabel());

      if(outAtt.PropagatePixelType()) {
        ocube->setPixelType(icube->getPixelType());
      }
      else {
        ocube->setPixelType(outAtt.PixelType());
      }

      if(outAtt.PropagateMinimumMaximum()) {
        if(ocube->getPixelType() == Real) {
          ocube->setBaseMultiplier(0.0, 1.0);
        }
        else if(ocube->getPixelType() >= icube->getPixelType()) {
          double base = icube->getBase();
          double mult = icube->getMultiplier();
          ocube->setBaseMultiplier(base, mult);
        }
        else if((ocube->getPixelType() != Real) &&
                (ocube->getPixelType() != UnsignedByte) &&
                (ocube->getPixelType() != SignedWord)) {
          std::string msg = "Looks like your refactoring to add different pixel types";
          msg += " you'll need to make changes here";
          throw iException::Message(iException::Programmer, msg, _FILEINFO_);
        }
        else {
          std::string msg = "You've chosen to reduce your output PixelType for [" +
                            psOutFile.toStdString() + "] you must specify the output pixel range too";
          throw iException::Message(iException::User, msg, _FILEINFO_);
        }
      }
      else {
        // Not propagating so either the user entered or the programmer did
        ocube->setMinMax(outAtt.Minimum(), outAtt.Maximum());
      }

      int needLabBytes = icube->getLabelSize(true) + (1024 * 6);
      if(needLabBytes > ocube->getLabelSize()) {
        ocube->setLabelSize(needLabBytes);
      }

      // Allocate the cube
      ocube->create(psOutFile.toStdString());

      // Transfer labels from the first input cube
      PvlObject &incube = icube->getLabel()->FindObject("IsisCube");
      PvlObject &outcube = ocube->getLabel()->FindObject("IsisCube");
      for(int i = 0; i < incube.Groups(); i++) {
        outcube.AddGroup(incube.Group(i));
      }

      // Transfer tables from the first input cube
      Pvl &inlab = *icube->getLabel();
      for(int i = 0; i < inlab.Objects(); i++) {
        if(inlab.Object(i).IsNamed("Table")) {
          Blob t((std::string)inlab.Object(i)["Name"], inlab.Object(i).Name());
          icube->read(t);
          ocube->write(t);
        }
      }

      // Transfer blobs from the first input cube
      inlab = *icube->getLabel();
      for(int i = 0; i < inlab.Objects(); i++) {
        if(inlab.Object(i).IsNamed("Polygon")) {
          Blob t((std::string)inlab.Object(i)["Name"], inlab.Object(i).Name());
          icube->read(t);
          ocube->write(t);
        }
      }

      // Transfer tables from the first input cube
      inlab = *icube->getLabel();
      for(int i = 0; i < inlab.Objects(); i++) {
        if(inlab.Object(i).IsNamed("OriginalLabel")) {
          OriginalLabel ol;
          icube->read(ol);
          ocube->write(ol);
        }
      }
    }
    catch(iException &e) {
      delete ocube;
      throw;
    }
  }
  
  /**
   * This method essentially creates a new cube, copies the
   * current cube (and any changes made to it) to the new cube,
   * reverses all changes NOT saved to the current cube and closes
   * it. Finally it sets the cubeviewport's cube to the new saved
   * cube.
   *  
   * @param pInCube  - input image
   * @param pOutCube - output image 
   */
  void FileTool::saveAsFullImage(Cube *icube, Cube *ocube) {
    //Start the copy process line by line
    Brick ibrick(*icube, icube->getSampleCount(), 1, 1);
    Brick obrick(*ocube, ocube->getSampleCount(), 1, 1);

    int numBricks;
    if(ibrick.Bricks() > obrick.Bricks()) numBricks = ibrick.Bricks();
    else numBricks = obrick.Bricks();

    // Loop and let the app programmer work with the bricks
    ibrick.begin();
    obrick.begin();
    for(int i = 0; i < numBricks; i++) {
      icube->read(ibrick);
      //Copy the contents to the output cube
      copy(ibrick, obrick);
      ocube->write(obrick);
      ibrick++;
      obrick++;
    }
  }

  /**
   * Full Resolution option, save the input image visible in the viewport window 
   * Enlarged/Reduced in full resolution
   * 
   * @author Sharmila Prasad (4/26/2011)
   * 
   * @param pInCube     - input image
   * @param pOutCube    - output image 
   * @param pNumSamples - out samples
   * @param pNumLines   - out lines
   */
  void FileTool::saveAs_FullResolution(Cube *pInCube,
      Cube *pOutCube, int pNumSamples, int pNumLines) {
    // start and end Samples and Lines
    double dStartSample=0, dEndSample=0, dStartLine=0, dEndLine=0;
    p_lastViewport->getCubeArea(dStartSample, dEndSample, dStartLine, dEndLine);
    int iNumBands   = pInCube->getBandCount();
    
    Portal iPortal (pNumSamples, 1, pInCube->getPixelType());
    Portal oPortal (pNumSamples, 1, pOutCube->getPixelType());
    
    for(int iBand=1; iBand<=iNumBands; iBand++) {
      int ol=1;
      for(int iLine=(int)dStartLine; iLine<=(int)dEndLine; iLine++) {
        iPortal.SetPosition(dStartSample, iLine, iBand);
        pInCube->read(iPortal);

        oPortal.SetPosition(1, ol++, iBand);
        pOutCube->read(oPortal);
        
        oPortal.Copy(iPortal);
        pOutCube->write(oPortal);
      }
    }
  }
  
  /**
   * Saves the whatsthis info of the cubeviewport to 
   * user specified output file 
   * 
   * @author Sharmila Prasad (4/6/2011)
   */
  void FileTool::saveInfo(void)
  {
    if(cubeViewport() == NULL) {
      QMessageBox::information((QWidget *)parent(), "Error", "No active cube to save info");
      return;
    }
    
    //Get the new cube's filename
    QString output =
      QFileDialog::getSaveFileName((QWidget *)parent(),
                                   "Choose output file",
                                   p_lastDir,
                                   QString("PVL Files (*.pvl)"));
    
    //If the filename is empty, return
    if(output.isEmpty()) {
      return;
    }
    else if(!output.endsWith(".pvl")) {
      output += ".pvl";
    }

    Pvl whatsThisPvl;
    cubeViewport()->getAllWhatsThisInfo(whatsThisPvl);
    whatsThisPvl.Write(output.toStdString());
  }
  
  /**
   * This method copies from the input buffer to the output buffer
   *
   * @param in
   * @param out
   */
  void FileTool::copy(Buffer &in, Buffer &out) {
    out.Copy(in);
  }

  /**
   * This slot emits a signal to discard all changes to the
   * current viewport
   *
   */
  void FileTool::discard() {
    emit discardChanges(cubeViewport());
  }

  /**
   * This method allows the user to export the current view as an image file.
   *
   */
  void FileTool::exportView() {
    if(cubeViewport() == NULL) {
      QMessageBox::information((QWidget *)parent(), "Error", "No active cube to export");
      return;
    }

    QString output =
      QFileDialog::getSaveFileName((QWidget *)parent(),
                                   QString("Choose output file"),
                                   p_lastDir,
                                   QString("PNG (*.png);;JPG (*.jpg);;TIF (*.tif)"));
    if(output.isEmpty()) return;

    p_lastDir = output;

    QString format = QFileInfo(output).suffix();

    if(format.isEmpty()) {
      if(output.endsWith('.')) {
        output.append(QString("png"));
      }
      else {
        output.append(QString(".png"));
      }
    }
    else if(format.compare("png", Qt::CaseInsensitive) &&
            format.compare("jpg", Qt::CaseInsensitive) &&
            format.compare("tif", Qt::CaseInsensitive)) {

      QMessageBox::information((QWidget *)parent(), "Error", format + " is an invalid extension.");
      return;
    }

    QPixmap pm = QPixmap::grabWidget(cubeViewport()->viewport());

    //if (!cubeViewport()->pixmap().save(output,format.toStdString().c_str())) {

    if(!pm.save(output)) {
      QMessageBox::information((QWidget *)parent(), "Error", "Unable to save " + output);
      return;
    }
  }

  /**
   * This method allows the user to print the current viewport.
   *
   */
  void FileTool::print() {
    // Is there anything to print
    if(cubeViewport() == NULL) {
      QMessageBox::information((QWidget *)parent(), "Error", "No active cube to print");
      return;
    }

    // Initialize a printer
    static QPrinter *printer = NULL;
    if(printer == NULL) printer = new QPrinter;
    printer->setPageSize(QPrinter::Letter);
    printer->setColorMode(QPrinter::GrayScale);
    if(cubeViewport()->isColor()) printer->setColorMode(QPrinter::Color);

    QPrintDialog printDialog(printer, (QWidget *)parent());
    if(printDialog.exec() == QDialog::Accepted) {
      // Get display widget as a pixmap and convert to an image
      QPixmap pixmap = QPixmap::grabWidget(cubeViewport()->viewport());
      QImage img = pixmap.toImage();

      // C++ Gui Programmign with Qt, page 201
      QPainter painter(printer);
      QRect rect = painter.viewport();
      QSize size = img.size();
      size.scale(rect.size(), Qt::KeepAspectRatio);
      painter.setViewport(rect.x(), rect.y(),
                          size.width(), size.height());
      painter.setWindow(img.rect());
      painter.drawImage(0, 0, img);
    }
  }

  /**
   * Try to close all open cubes and save/discard if necessary.
   */
  bool FileTool::closeAll() {
    //  Close all cubes
    // We must create a temporary list.  If not the actual
    // list size gets modified when a close occurs and not all
    // windows were being closed.
    MdiCubeViewport *d;
    QVector< MdiCubeViewport * > tempList(*cubeViewportList());
    for(int i = 0; i < (int)tempList.size(); i++) {
      d = tempList.at(i);
      //Set the current viewport to the one being closed
      setCubeViewport(d);

      //If the user cancels the close operation, delete any viewports
      //that WERE closed and set the viewportlist to the temp list and return
      if(!d->parentWidget()->close()) {
        //tempList.erase(tempList.begin(), tempList.begin() + i);
        //cubeViewportList()->assign(tempList.begin(), tempList.end());
        return false;
      }
    }
    return true;
  }

  /**
   * Exit the program, this slot called when the exit is chosen from the File menu
   *
   * @internal
   * @history  2007-02-13  Tracie Sucharski,  Close all cubes before exiting
   */
  void FileTool::exit() {
    if(closeAll()) {
      /*This is OK to cast the p_parent because we know it's sub-subclassed from
       *MainWindow and we know that MainWindow has a close method*/
      ((MainWindow *)p_parent)->close();
      qApp->quit();
    }
  }

  /**
   * This slot enables or disables save and save as.
   *
   * @param enable
   */
  void FileTool::enableSave(bool enable) {
    p_save->setEnabled(enable);
  }

  /**
   * This method is called when the tool is updated.
   *
   */
  void FileTool::updateTool() {
    if(cubeViewport() == NULL) {
      if(p_lastViewport != NULL) {
        p_lastViewport = NULL;
      }
      p_print->setEnabled(false);
      p_save->setEnabled(false);
      p_exportView->setEnabled(false);
      p_saveAs->setEnabled(false);
      p_saveInfo->setEnabled(false);
    }
    else {
      if(p_lastViewport == NULL) {
        //Set the last viewport to the current viewport and connect signals to save and discard
        p_lastViewport = cubeViewport();
        connect(p_lastViewport, SIGNAL(saveChanges()), this, SLOT(save()));
        connect(p_lastViewport, SIGNAL(discardChanges()), this, SLOT(discard()));
      }
      else {
        if(p_lastViewport != cubeViewport()) {
          //If the viewport has changes made to it enable the save action
          if(cubeViewport()->windowTitle().endsWith("*")) {
            p_save->setEnabled(true);
          }
          //Else disable it
          else {
            p_save->setEnabled(false);
          }
          //disconnect signals from the old viewport and connect them to the new viewport
          disconnect(p_lastViewport, SIGNAL(saveChanges()), this, SLOT(save()));
          disconnect(p_lastViewport, SIGNAL(discardChanges()), this, SLOT(discard()));
          p_lastViewport = cubeViewport();
          connect(p_lastViewport, SIGNAL(saveChanges()), this, SLOT(save()));
          connect(p_lastViewport, SIGNAL(discardChanges()), this, SLOT(discard()));
        }
      }
      p_print->setEnabled(true);
      p_exportView->setEnabled(true);
      p_saveAs->setEnabled(true);
      p_saveInfo->setEnabled(true);
    }
  }
}
