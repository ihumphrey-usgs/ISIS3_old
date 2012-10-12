#include "IsisDebug.h"

#include "AbstractMeasureItem.h"

#include <QMessageBox>
#include <QString>
#include <QVariant>

#include "CnetDisplayProperties.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlPoint.h"
#include "IException.h"


#include "TableColumn.h"
#include "TableColumnList.h"


namespace Isis
{
  namespace CnetViz
  {
    QString AbstractMeasureItem::getColumnName(Column col)
    {
      switch (col)
      {
        case PointId:
          return "Point ID";
        case ImageId:
          return "Image ID";
        case Sample:
          return "Sample";
        case Line:
          return "Line";
        case EditLock:
          return "Edit Locked";
        case Ignored:
          return "Ignored";
        case Reference:
          return "Reference";
        case Type:
          return "Measure Type";
        case Obsolete_Eccentricity:
          return "Obsolete_Eccentricity";
        case GoodnessOfFit:
          return "Goodness of Fit";
        case MinPixelZScore:
          return "Minimum Pixel Z-Score";
        case MaxPixelZScore:
          return "Maximum Pixel Z-Score";
        case SampleShift:
          return "Sample Shift";
        case LineShift:
          return "Line Shift";
        case SampleSigma:
          return "Sample Sigma";
        case LineSigma:
          return "Line Sigma";
        case APrioriSample:
          return "A Priori Sample";
        case APrioriLine:
          return "A Priori Line";
        case Diameter:
          return "Diameter";
        case JigsawRejected:
          return "Rejected by Jigsaw";
        case ResidualSample:
          return "Residual Sample";
        case ResidualLine:
          return "Residual Line";
        case ResidualMagnitude:
          return "Residual Magnitude";
      }

      ASSERT(0);
      return QString();
    }


    AbstractMeasureItem::Column AbstractMeasureItem::getColumn(
        QString columnTitle)
    {
      for (int i = 0; i < COLS; i++)
      {
        if (columnTitle == getColumnName((Column) i))
          return (Column) i;
      }

      IString msg = "Column title [" + columnTitle + "] does not match any of "
          "the defined column types";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }


    TableColumnList * AbstractMeasureItem::createColumns()
    {
      TableColumnList * columnList = new TableColumnList;

      columnList->append(new TableColumn(getColumnName(PointId), true, false));
      columnList->append(new TableColumn(getColumnName(ImageId), true,
                                            true));
      columnList->append(new TableColumn(getColumnName(Sample), true, false));
      columnList->append(new TableColumn(getColumnName(Line), true, false));
      columnList->append(new TableColumn(getColumnName(EditLock), false,
                                            false));
      columnList->append(new TableColumn(getColumnName(Ignored), false, true));
      columnList->append(new TableColumn(getColumnName(Reference), true, true));
      columnList->append(new TableColumn(getColumnName(Type), false, false));
      columnList->append(new TableColumn(getColumnName(Obsolete_Eccentricity), true,
                                            false));
      columnList->append(new TableColumn(getColumnName(GoodnessOfFit), true,
                                            false));
      columnList->append(new TableColumn(getColumnName(MinPixelZScore), true,
                                            false));
      columnList->append(new TableColumn(getColumnName(MaxPixelZScore), true,
                                            false));
      columnList->append(new TableColumn(getColumnName(SampleShift), true,
                                            false));
      columnList->append(new TableColumn(getColumnName(LineShift), true,
                                            false));
      columnList->append(new TableColumn(getColumnName(SampleSigma), false,
                                            false));
      columnList->append(new TableColumn(getColumnName(LineSigma), false,
                                            false));
      columnList->append(new TableColumn(getColumnName(APrioriSample), true,
                                            false));
      columnList->append(new TableColumn(getColumnName(APrioriLine), true,
                                            false));
      columnList->append(new TableColumn(getColumnName(Diameter), false,
                                            false));
      columnList->append(new TableColumn(getColumnName(JigsawRejected), true,
                                            false));
      columnList->append(new TableColumn(getColumnName(ResidualSample), true,
                                            false));
      columnList->append(new TableColumn(getColumnName(ResidualLine), true,
                                            false));
      columnList->append(new TableColumn(getColumnName(ResidualMagnitude),
                                            true, false));

      return columnList;
    }

    AbstractMeasureItem::AbstractMeasureItem(ControlMeasure * cm,
        int avgCharWidth, AbstractTreeItem * parent)
        : AbstractTreeItem(parent)
    {
      ASSERT(cm);
      measure = cm;
      calcDataWidth(avgCharWidth);

      connect(measure, SIGNAL(destroyed(QObject *)), this, SLOT(sourceDeleted()));
    }


    AbstractMeasureItem::~AbstractMeasureItem()
    {
      measure = NULL;
    }


    QVariant AbstractMeasureItem::getData() const
    {
      return getData(getColumnName(ImageId));
    }


    QVariant AbstractMeasureItem::getData(QString columnTitle) const
    {
      if (measure)
      {
        Column column = getColumn(columnTitle);

        switch ((Column) column)
        {
          case PointId:
            return QVariant((QString) measure->Parent()->GetId());
          case ImageId:
            return QVariant(CnetDisplayProperties::getInstance()->getImageName(
                (QString) measure->GetCubeSerialNumber()));
          case Sample:
            return QVariant(measure->GetSample());
          case Line:
            return QVariant(measure->GetLine());
          case EditLock:
            if (measure->IsEditLocked())
              return QVariant("Yes");
            else
              return QVariant("No");
          case Ignored:
            if (measure->IsIgnored())
              return QVariant("Yes");
            else
              return QVariant("No");
          case Reference:
            if (measure->Parent()->GetRefMeasure() == measure)
              return QVariant("Yes");
            else
              return QVariant("No");
          case Type:
            return QVariant(
                (QString)measure->MeasureTypeToString(measure->GetType()));
          case Obsolete_Eccentricity:
            return QVariant(
                measure->GetLogData(
                    ControlMeasureLogData::Obsolete_Eccentricity).GetNumericalValue());
          case GoodnessOfFit:
            return QVariant(
                measure->GetLogData(
                    ControlMeasureLogData::GoodnessOfFit).GetNumericalValue());
          case MinPixelZScore:
            return QVariant(
                measure->GetLogData(ControlMeasureLogData::MinimumPixelZScore).
                GetNumericalValue());
          case MaxPixelZScore:
            return QVariant(
                measure->GetLogData(ControlMeasureLogData::MaximumPixelZScore).
                GetNumericalValue());
          case SampleShift:
            return QVariant(measure->GetSampleShift());
          case LineShift:
            return QVariant(measure->GetLineShift());
          case SampleSigma:
            return QVariant(measure->GetSampleSigma());
          case LineSigma:
            return QVariant(measure->GetLineSigma());
          case APrioriSample:
            return QVariant(measure->GetAprioriSample());
          case APrioriLine:
            return QVariant(measure->GetAprioriLine());
          case Diameter:
            return QVariant(measure->GetDiameter());
          case JigsawRejected:
            if (measure->IsRejected())
              return QVariant("Yes");
            else
              return QVariant("No");
          case ResidualSample:
            return QVariant(measure->GetSampleResidual());
          case ResidualLine:
            return QVariant(measure->GetLineResidual());
          case ResidualMagnitude:
            return QVariant(
                measure->GetResidualMagnitude());
        }
      }

      return QVariant();
    }


    void AbstractMeasureItem::setData(QString const & columnTitle,
                                      QString const & newData)
    {
      if (measure)
      {
        Column column = getColumn(columnTitle);

        switch ((Column) column)
        {
          case PointId:
            // PointId is not editable in the measure table
            break;
          case ImageId:
            measure->SetCubeSerialNumber(
                CnetDisplayProperties::getInstance()->getSerialNumber(newData));
            break;
          case Sample:
            measure->SetCoordinate(catchNull(newData),
                measure->GetLine());
            break;
          case Line:
            measure->SetCoordinate(measure->GetSample(),
                catchNull(newData));
            break;
          case EditLock:
            if (newData == "Yes")
              measure->SetEditLock(true);
            else
              measure->SetEditLock(false);
            break;
          case Ignored:
            if (newData == "Yes")
              measure->SetIgnored(true);
            else
              if (newData == "No")
                measure->SetIgnored(false);
            break;
          case Reference:
            // A measure's reference status should never be editable. It should
            // only be changed through the point.
            break;
          case Type:
            measure->SetType(measure->StringToMeasureType(
                CnetDisplayProperties::getInstance()->getSerialNumber(
                newData)));
            break;
          case Obsolete_Eccentricity:
            setLogData(measure, ControlMeasureLogData::Obsolete_Eccentricity, newData);
            break;
          case GoodnessOfFit:
            setLogData(measure, ControlMeasureLogData::GoodnessOfFit, newData);
            break;
          case MinPixelZScore:
            setLogData(measure, ControlMeasureLogData::MinimumPixelZScore,
                newData);
            break;
          case MaxPixelZScore:
            setLogData(measure, ControlMeasureLogData::MaximumPixelZScore,
                newData);
            break;
          case SampleShift:
            // This is not editable anymore.
            break;
          case LineShift:
            // This is not editable anymore.
            break;
          case SampleSigma:
            measure->SetSampleSigma(catchNull(newData));
            break;
          case LineSigma:
            measure->SetLineSigma(catchNull(newData));
            break;
          case APrioriSample:
            measure->SetAprioriSample(catchNull(newData));
            break;
          case APrioriLine:
            measure->SetAprioriLine(catchNull(newData));
            break;
          case Diameter:
            measure->SetDiameter(catchNull(newData));
            break;
          case JigsawRejected:
            // jigsaw rejected is not editable!
            break;
          case ResidualSample:
            measure->SetResidual(
              catchNull(newData), measure->GetLineResidual());
            break;
          case ResidualLine:
            measure->SetResidual(
              measure->GetSampleResidual(), catchNull(newData));
            break;
          case ResidualMagnitude:
            // residual magnitude is not editable!
            break;
        }
      }
    }


    // Returns true if the data at the given column is locked (i.e. is
    // edit-locked). If the measure is edit-locked, all columns except the edit
    // lock column should be uneditable. If the measure's parent point is
    // edit-locked, none of the columns should be editable as it should only be
    // unlocked from the parent point.
    bool AbstractMeasureItem::isDataEditable(QString columnTitle) const {
      bool parentLocked = !measure->Parent() ||
                           measure->Parent()->IsEditLocked();
      bool locked = measure->IsEditLocked() || parentLocked;

      if (getColumn(columnTitle) == EditLock && !parentLocked) {
        locked = false;
      }

      return !locked;
    }


    void AbstractMeasureItem::deleteSource()
    {
      if (measure)
      {
        if (measure->Parent()->IsEditLocked()) {
          IString msg = "Measures in point [" +
              getFormattedData(getColumnName(PointId)) +
              "] cannot be deleted because point is edit locked";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        else if (measure->IsEditLocked()) {
          IString msg = "Measure [" + getFormattedData() + "] in point [" +
              getFormattedData(getColumnName(PointId)) +
              "] cannot be deleted because measure is edit locked";
          throw IException(IException::User, msg, _FILEINFO_);
        }
  //       else if (measure->Parent()->GetRefMeasure() == measure) {
  //         IString msg = "Measure [" + getData() + "] in point [" +
  //             getData(getColumnName(PointId)) + "] cannot be deleted because "
  //             "it is the reference";
  //         throw iException::Message(iException::User, msg, _FILEINFO_);
  //       }

        ControlMeasure * tempMeasure = measure;
        measure = NULL;
        tempMeasure->Parent()->Delete(tempMeasure);
      }
    }


    AbstractTreeItem::InternalPointerType AbstractMeasureItem::getPointerType()
    const
    {
      return AbstractTreeItem::Measure;
    }


    void * AbstractMeasureItem::getPointer() const
    {
      return measure;
    }


    bool AbstractMeasureItem::hasMeasure(ControlMeasure * m) const
    {
      return measure == m;
    }


    void AbstractMeasureItem::sourceDeleted() {
      measure = NULL;
    }


    void AbstractMeasureItem::setLogData(ControlMeasure * measure,
        int measureLogDataEnum, const QString & value)
    {
      ASSERT(measure);

      QString newDataStr = value.toLower();
      ControlMeasureLogData::NumericLogDataType type =
        (ControlMeasureLogData::NumericLogDataType) measureLogDataEnum;

      if (newDataStr == "null")
      {
        measure->DeleteLogData(type);
      }
      else
      {
        measure->SetLogData(ControlMeasureLogData(type,
            value.toDouble()));
      }
    }
  }
}

