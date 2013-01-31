#ifndef MosaicControlNetTool_h
#define MosaicControlNetTool_h

#include "MosaicTool.h"

class QDialog;
class QLabel;
class QPushButton;

namespace Isis {
  class ControlNet;
  class ControlNetGraphicsItem;
  class CubeDisplayProperties;

  /**
   * //TODO: Remove debug printout & comment
   * @brief Handles Control Net displays
   *
   * @ingroup Visualization Tools
   *
   * @author ????-??-?? Christopher Austin
   *
   * @internal
   *   @history 2010-06-24 Christopher Austin - Added |x| functionality and
   *                           fixed control net loading
   *   @history 2011-05-07 Steven Lambright - Refactored.
   *   @history 2011-05-10 Steven Lambright - Reduced useless code, open cnet
   *                           button is now always enabled.
   *   @history 2011-05-10 Steven Lambright - Added label for currently open
   *                           network.
   *   @history 2011-09-27 Steven Lambright - Improved user documentation. Made
   *                           the open control network button more obvious.
   *   @history 2013-01-02 Steven Lambright - Implemented movement arrow colorization. This is a
   *                           quick and dirty implementation designed to get the most basic
   *                           functionality working with minimal options. Added the enum
   *                           MovementColorSource and the methods setMovementArrowColorSource(),
   *                           movementArrowColorSource(), maxMovementColorMeasureCount(),
   *                           maxMovementColorResidualMagnitude(), toString(),
   *                           and fromMovementColorSourceString(). Fixes #479.
   *   @history 2013-01-31 Steven Lambright - Removed some debugging statements that were left
   *                           around from the last change. Fixes #1459.
   */
  class MosaicControlNetTool : public MosaicTool {
      Q_OBJECT

    public:
      /**
       * This enum defines how to draw the movement arrows (arrows from CP A Priori location to
       *   adjusted location). These settings include whether the arrows are shown and how to color
       *   them.
       *
       * NOTE: It's important to start at zero. Also, if you add to this enumeration, be sure to
       *       update NUM_MOVEMENT_COLOR_SOURCE_VALUES.
       */
      enum MovementColorSource {
        //! Do not show movement arrows
        NoMovement = 0,
        //! Show black movement arrows
        NoColor,
        //! Show movement arrows colored by measure count
        MeasureCount,
        //! Show movement arrows colored by residual magnitude
        ResidualMagnitude
      };
      //! This is the count of possible values of MovementColorSource (useful for loops).
      static const int NUM_MOVEMENT_COLOR_SOURCE_VALUES = 4;

      MosaicControlNetTool(MosaicSceneWidget *);
      ~MosaicControlNetTool();

      void addToMenu(QMenu *menu);

      PvlObject toPvl() const;
      void fromPvl(const PvlObject &obj);
      QString projectPvlObjectName() const;

      void setMovementArrowColorSource(MovementColorSource, int, double);
      MovementColorSource movementArrowColorSource() const;

      int maxMovementColorMeasureCount() const;
      double maxMovementColorResidualMagnitude() const;

      static QString toString(MovementColorSource);
      static MovementColorSource fromMovementColorSourceString(QString);

    public slots:

    protected:
      QAction *getPrimaryAction();
      QWidget *getToolBarWidget();

    private slots:
      void configMovement();
      void updateTool();
      void openControlNet();
      void displayConnectivity();
      void displayControlNet();
      void closeNetwork();
      void loadNetwork();
      void randomizeColors();

      void objectDestroyed(QObject *);

    private:
      void createDialog();

      CubeDisplayProperties *
          takeDisplay(QString sn, QList< CubeDisplayProperties *> &displays);

      QPushButton *m_loadControlNetButton;
      QPushButton *m_displayControlNetButton;
      QPushButton *m_displayConnectivity;
      QPushButton *m_configMovement;
      QPushButton *m_closeNetwork;
      QPushButton *m_randomizeColors;
      QAction *m_connectivity;
      ControlNet *m_controlNet;
      ControlNetGraphicsItem *m_controlNetGraphics;
      QLabel *m_controlNetFileLabel;
      QString m_controlNetFile;

      //! This defines the drawing mode of the apriori to adjusted arrows
      MovementColorSource m_movementArrowColorSource;
      //! This is the measure count at which we start coloring the movement arrows
      int m_measureCount;
      //! This is the residual magnitude at which we coloring the movement arrows
      double m_residualMagnitude;
  };
};

#endif

