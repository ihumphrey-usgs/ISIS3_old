/**
 * @file
 * $Revision: 1.1 $
 * $Date: 2005/08/18 23:00:25 $
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
#include "ImportPdsTable.h"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <QDebug>
#include <QString>

#include "EndianSwapper.h"
#include "FileName.h"
#include "IString.h"
#include "Pvl.h"
#include "PvlObject.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"
#include "TextFile.h"


using namespace std;

namespace Isis {


  /** 
   * Default constructor. This constructor initializes the PDS table name to 
   * TABLE.
   *
   * If this constructor is used, the load() method will need to be called to
   * set the PDS label file.
   *
   * This constructor may be used for ASCII or BINARY PDS tables.
   */
  ImportPdsTable::ImportPdsTable() {
    // just inititalize the member variables
    init();
    m_tableName = "TABLE";
  }

  /**
   * @brief This constructor automatically loads the given label and table files.
   *
   * This constructor takes the name of the label file describing the PDS 
   * table, the table data file name, and the name of the PDS table object.  
   * It will extract the description of the columns and read the contents of the 
   * table data file.
   *
   * If no table file is given or an empty QString is given for the table file, 
   * the table location will be read from the label file.
   *
   * If no table name is given, the default name for the object is TABLE.
   *
   * This constructor may be used for ASCII or BINARY PDS tables.
   *
   * @param pdsLabFile Name of table label file
   * @param pdsTableFile Name of table data file
   * @param pdsTableName The name of the table object in the PDS file.
   */
  ImportPdsTable::ImportPdsTable(const QString &pdsLabFile,
                                 const QString &pdsTableFile,
                                 const QString &pdsTableName) {
    m_tableName = pdsTableName;
    load(pdsLabFile, pdsTableFile);
  }

  /**
   * Destructs the ImportPdsTable object.
   */ 
  ImportPdsTable::~ImportPdsTable() {
  }

  /**
   * @brief Loads a PDS table label and (optional) data file
   *
   * This method will load a PDS table dataset using a label file describing the
   * contents of the table data.  The caller can provide the table data file,
   * otherwise, the location of the table data is extracted from the ^TABLE_NAME
   * keyword in the provided labels.  The table data is then loaded.
   *
   * This method needs to be called if the default constructor is used. 
   * Otherwise, it is invoked in the constructor that takes the label, table 
   * file, and table name. This method may be used to overwrite the label and 
   * table file used. When it is invoked, the current contents of the object are
   * discarded.
   *
   * This method is used for ASCII or BINARY PDS tables.
   *
   * @param pdsLabFile Name of PDS table label file
   * @param pdsTableFile Name of PDS table data file to be imported into Isis 
   *                (optional)
   */
  void ImportPdsTable::load(const QString &pdsLabFile,
                            const QString &pdsTableFile) {

    init();
    QString tempTblFile;
    loadLabel(pdsLabFile, tempTblFile);
    if (!pdsTableFile.isEmpty()) tempTblFile = pdsTableFile;
    //  Vet the table filename.  Many PDS files record the filename in
    //  uppercase and in practice, the filename is in lowercase.  Do this
    //  check here.
    FileName tableFile(tempTblFile);
    try {
      int tableStartRecord = toInt(tableFile.baseName());
      tempTblFile = pdsLabFile;
      m_pdsTableStart = tableStartRecord;
    }
    catch (IException &e) {
      // if we are unable to cast the table file value to an integer, it must be a
      // file name, not a location in the label file.
      if (!tableFile.fileExists()) {
        // if the table file name doesn't exist, try lowercased version...
        FileName tableFileLowercase(tableFile.path() + "/" 
                                    + tableFile.name().toLower());
        if (!tableFileLowercase.fileExists()) {
          IString msg = "Unable to import PDS table.  Neither of the following "
                        "possible table files were found: [" 
                        + tableFile.expanded() + "]  or [" 
                        + tableFileLowercase.expanded() + "]";  
          throw IException(e, IException::Unknown, msg, _FILEINFO_);
        }
        tableFile = tableFileLowercase.expanded();
        tempTblFile = tableFile.expanded();
      }
      m_pdsTableStart = 1;
    }
    if (m_pdsTableType == "ASCII") {
      loadTable(tempTblFile);
    }
    m_pdsTableFile = tempTblFile;
    return;
  }

  /** 
   * This method determines whether the PDS table has a column with the given 
   * name.
   *
   * This method can be called for ASCII or BINARY PDS tables.
   *
   * @param colName A QString containing the column name.
   *
   * @return @b bool Indicates whether the table has the given column.
   */
  bool ImportPdsTable::hasColumn(const QString &colName) const {
     return (findColumn(colName) != 0);
  }


  /**
   * @brief Returns the name of the specifed column
   *
   * This method will return the name of a specified column by index. It also 
   * has the option to format the column name to Camel-Case.  This will remove 
   * all left and right parens, convert white space to spaces, compress 
   * consecutive spaces to only one space.  It then removes the spaces 
   * converting the next character to uppercase.
   *
   * This method can be called for ASCII or BINARY PDS tables.
   *
   * @param index      Index of colunm name to get.
   * @param formatted  Specifies to convert the name to Camel-Case if true,
   *                   otherwise leave as is in the PDS table.
   *
   * @return QString Returns the column name as requested
   */
  QString ImportPdsTable::getColumnName(const unsigned int &index,
                                            const bool &formatted) const {
    if ((int) index >= columns() - 1) {
      QString msg = "Unable to import the binary PDS table [" + m_tableName 
                    + "] into Isis. The requested column index [" 
                    + toString((int) index) + "] exceeds the last column index [" 
                    + toString(columns() - 1) + "]";
      throw IException(IException::Programmer, msg.toStdString(), _FILEINFO_);
    }
    QString name = m_coldesc[index].m_name;
    if (formatted) name = getFormattedName(name);
    return (name);
  }

  /**
   * @brief Return the names of all the columns
   *
   * This method will return the names of all columns. It also has the option to
   * format the column name to Camel-Case.  This will remove all left and right
   * parens, convert white space to spaces, compress consecutive spaces to only
   * one space. It then removes the spaces converting the next character to
   * uppercase.
   *
   * This method can be called for ASCII or BINARY PDS tables.
   *
   * @param formatted  Specifies to convert the name to Camel-Case if true,
   *                   otherwise leave as is in the PDS table.
   *
   * @return std::vector<QString> Returns vector of all column names
   */
  QStringList ImportPdsTable::getColumnNames(const bool &formatted)
                                                          const {
    QStringList colnames;
    for (int i = 0 ; i < columns() ; i++) {
      QString name = m_coldesc[i].m_name;
      if (formatted) name = getFormattedName(name);
      colnames.push_back(name);
    }
    return (colnames);
  }

  /**
   * @brief Get the type associated with the specified column
   *
   * This method returns the datatype associated with the specfied column.  If 
   * the column does not exist, an empty QString is returned.
   *
   * This method can be called for ASCII or BINARY PDS tables.
   *
   * @author kbecker (6/27/2011)
   *
   * @param colName      Name of column to get type for
   *
   * @return QString Returns the type of the column.  If the column does not
   *         exist, an empty QString is returned.
   */
  QString ImportPdsTable::getType(const QString &colName) const {
    const ColumnDescr *column = findColumn(colName);
    QString dtype("");
    if (column != 0) {
      dtype = column->m_dataType;
    }
    return (dtype);
  }

  /**
   * @brief Change the datatype for a column
   *
   * This method changes the data type the specified column.
   *
   * This method can be called for ASCII or BINARY PDS tables.
   *
   * @author kbecker (6/27/2011)
   *
   * @param colName Name of column to change
   * @param dtype   New type of column.  Support types are DOUBLE, REAL, INTEGER
   *                and CHARACTER.  Unsupported/unknown types are treated as
   *                CHARACTER.
   *
   * @return bool
   */
  bool ImportPdsTable::setType(const QString &colName,
                               const QString &dataType) {
    ColumnDescr *column = findColumn(colName);
    if (column != 0) {
      column->m_dataType = dataType.toUpper();
    }
    return (column != 0);
  }

  /**
   * @brief Populate a Table object with the PDS table and return it
   *
   * This method converts PDS table data to an ISIS table.
   *
   * This method can be called to import ASCII or BINARY PDS tables.
   *
   * @param isisTableName Name of table
   *
   * @return Table Table containing PDS table data
   */
  Table ImportPdsTable::importTable(const QString &isisTableName) {
    try {
      TableRecord record = makeRecord(m_coldesc);
      Table table(isisTableName, record);
      fillTable(table, m_coldesc, record);
      return (table);
    }
    catch (IException &e) {
      QString msg = "Unable to import the PDS table [" + m_tableName 
                    + "] from the PDS file [" + m_pdsTableFile + "] into Isis.";
      throw IException(e, IException::Unknown, msg.toStdString(), _FILEINFO_);
    
    }
  }


  /**
   * @brief Populate ISIS Table with specified column(s) from ASCII table
   *
   * This method extracts columns specified by the caller in a QString.  It is
   * typically used for a single column, but any number of columns can be
   * provided.  colnames is a comma delimited QString that contains the name of
   * the columns that will be exported in the table.
   *  
   * This method should only be called for ASCII PDS tables. If needed for 
   * BINARY tables, implementation should be added and tested.
   *
   * @param colNames String containing comma delimited column names to export
   * @param isisTableName    Name of table to create
   *
   * @return Table  Table containing the specified columns
   */
  Table ImportPdsTable::importTable(const QString &colnames,
                                    const QString &isisTableName) {
    return (importTable(colnames.split(","), isisTableName));
  }

  /**
   * @brief Populate ISIS Table with specific columns from ASCII table
   *
   * This method extracts columns specified by the caller.  If the requested
   * column does not exist, an exception is thrown.
   *
   * This method should only be called for ASCII PDS tables. If needed for 
   * BINARY tables, implementation should be added and tested.
   *
   * @param colNames Vector column names to convert to a table.
   * @param isisTableName  Name of the table to create.
   *
   * @return Table
   */
  Table ImportPdsTable::importTable(const QStringList &colnames,
                                    const QString &isisTableName) {
    ColumnTypes ctypes;
    for (int i = 0 ; i < colnames.size() ; i++) {
      const ColumnDescr *descr = findColumn(colnames[i]);
      if (!descr) {
        QString msg = "Unable to import the PDS table [" + m_tableName 
                      + "] into Isis. The requested column name [" 
                      + colnames[i] + "] does not "
                      "exist in table."; 
        throw IException(IException::Programmer, msg.toStdString(), _FILEINFO_);
      }
      ctypes.push_back(*descr);
    }

    // Create and populate the table
    TableRecord record = makeRecord(ctypes);
    Table table(isisTableName, record);
    fillTable(table, ctypes, record);
    return (table);
  }

  /** 
   * Initialize object variables.
   *
   * This method is used for ASCII or BINARY PDS tables.
   */
  void ImportPdsTable::init() {

    m_byteOrder = "";
    m_trows = 0;
    m_pdsTableStart = 0;
    m_coldesc.clear();
    m_rows.clear();
    m_pdsTableType = "";
    m_pdsTableFile = "";
    return;
  }

  /**
   * @brief Loads the contents of a PDS table label description
   *
   * The pdsLabFile parameter contains the name of a PDS label describing the
   * contents of a PDS table data file.  It will be loaded and parsed by this
   * method. The name of the table data file is returned in the pdsTableFile 
   * parameter. It is not loaded.
   *
   * This method is used for ASCII or BINARY PDS tables.
   *
   * @param pdsLabFile Name of PDS table label description file
   * @param pdsTableFile Returns the name of the PDS table data
   */
  void ImportPdsTable::loadLabel(const QString &pdsLabFile, 
                                 QString &pdsTableFile) {

    Isis::Pvl label(pdsLabFile);

    if (!label.hasObject(m_tableName)) {
      QString msg = "The PDS file " + pdsLabFile +
                    " does not have the required TABLE object, ["
                    + m_tableName +"]. The PDS label file is probably invalid";
      throw IException(IException::Unknown, msg.toStdString(), _FILEINFO_);
    }
    m_recordBytes = (int) label.findKeyword("RECORD_BYTES");
    //  Get some pertinent information from the label
    PvlObject &tabobj = label.findObject(m_tableName);
    pdsTableFile = FileName(pdsLabFile).path() + "/" 
                   + label["^" + m_tableName][0];
    m_trows = (int) tabobj.findKeyword("ROWS");
    int ncols =  (int) tabobj.findKeyword("COLUMNS");
    m_pdsTableType = QString(tabobj.findKeyword("INTERCHANGE_FORMAT"));
    if (m_pdsTableType != "ASCII" && m_pdsTableType.toUpper() != "BINARY") {
      QString msg = "Unable to import the PDS table [" + m_tableName 
                    + "] from the PDS file [" 
                    + pdsTableFile + "] into Isis. "
                    "The PDS INTERCHANGE_FORMAT [" + m_pdsTableType
                    + "] is not supported. Valid values are ASCII or BINARY.";
      throw IException(IException::User, msg.toStdString(), _FILEINFO_);
    }
    m_rowBytes = tabobj.findKeyword("ROW_BYTES");

    m_coldesc.clear();
    PvlObject::PvlObjectIterator colobj = tabobj.beginObject();
    int icol(0);
    while (colobj != tabobj.endObject()) {
      if (colobj->isNamed("COLUMN")) {
        m_coldesc.push_back(getColumnDescription(*colobj, icol));
        icol++;
      }
      ++colobj;
    }

    //  Test to ensure columns match the number listed in the label
    if (ncols != columns()) {
      ostringstream msg;
      msg << "Number of columns in the COLUMNS label keyword (" << ncols
           << ") does not match number of COLUMN objects found ("
           << columns() << ")";
  #if 0
      throw iException::Message(iException::Programmer, msg.str(), _FILEINFO_);
  #else
       cout << msg.str() << endl;
  #endif
    }
    return;
  }

  /**
   * @brief Loads the contents of a PDS table data file
   *
   * This method open and read the contents of a PDS table data file.  Values 
   * for each column are extracted from each row according to the label 
   * description. The entire table contents are stored internally. 
   *
   * Note that the table label description must already be loaded in this 
   * object. 
   *
   * This method is called by the load() method if the PDS file is ASCII.
   *
   * @see loadLabel().
   *
   * @param pdsTableFile Name of PDS table data file
   */
  void ImportPdsTable::loadTable(const QString &pdsTableFile) {

    //  Vet the filename.  Many PDS files record the filename in uppercase and
    //  in practice, the filename is in lowercase.  Do this check here.
    QString tempTblFile(pdsTableFile);
    TextFile tfile(tempTblFile);
    QString tline;
    m_rows.clear();
    int irow(0);
    while (tfile.GetLine(tline, false)) {
      if (irow >= m_trows) break;

      Columns cols;
      for (int i = 0 ; i < columns() ; i++) {
        cols.push_back(getColumnValue(tline, m_coldesc[i]));
      }
      m_rows.push_back(cols);
      irow++;
    }
    return;
  }

  /**
   * @brief Extract a column description from a COLUMN object
   *
   * This method will extract a column description from a COLUMN object.  This
   * object will typically be contained in a Pvl PDS compatable file that
   * accompanies the table data file.
   *
   * The keywords NAME, DATA_TYPE, START_BYTE and BYTES must exist.
   *
   * This method is used for ASCII or BINARY PDS tables.
   *
   * @param colobj Pvl Object containing column description
   * @param nth    Current column counter
   *
   * @return ImportPdsTable::ColumnType Returns internal struct of column
   *         description
   */
  ImportPdsTable::ColumnDescr ImportPdsTable::getColumnDescription(
      PvlObject &colobj, int nth) const {
    ColumnDescr cd;
    cd.m_name = colobj["NAME"][0];
    cd.m_colnum = nth;
    if (m_pdsTableType == "ASCII") {
      cd.m_dataType = getGenericType(colobj["DATA_TYPE"][0]).toUpper();
    }
    else {
      cd.m_dataType = colobj["DATA_TYPE"][0].toUpper();
      //cd.m_numBytes = colobj["ITEM_BYTES"];
    }
    cd.m_startByte = ((int) colobj["START_BYTE"]) - 1;   // 0-based indexing
    cd.m_numBytes = (int) colobj["BYTES"];
    return (cd);
  }

  /**
   * @brief Searches internal column descriptors for a named column
   *
   * This method converts the column names to a formatted name for consistency 
   * and then checks for the given name - with case insensitivity.  If found, a
   * pointer to the column description is returned, otherwise NULL.
   *
   * This method can be called for ASCII or BINARY PDS tables.
   *
   * @param colName Name of column to find
   *
   * @return ImportPdsTable::ColumnDescr* Pointer to column description is
   *         returned if found otherwise, NULL is returned.
   */
  ImportPdsTable::ColumnDescr *ImportPdsTable::findColumn(
      const QString &colName) {
    QString cName = getFormattedName(colName);
    ColumnTypes::iterator col = m_coldesc.begin();
    while (col != m_coldesc.end()) {
      QString c = getFormattedName(col->m_name);
      if (c.toUpper() == cName.toUpper()) { return (&(*col));  }
      col++;
    }
    return (0);
  }

  /**
   * @brief Searches internal column descriptors for a named column
   *
   * This method converts the column names to a formatted name for consistency 
   * and then checks for the given name - with case insensitivity.  If found, a
   * pointer to the column description is returned, otherwise NULL.
   *
   * This method can be called for ASCII or BINARY PDS tables.
   *
   * @param colName Name of column to find
   *
   * @return ImportPdsTable::ColumnDescr* Pointer to column description is
   *         returned if found otherwise, NULL is returned.
   */
  const ImportPdsTable::ColumnDescr *ImportPdsTable::findColumn(
      const QString &colName) const {
    QString cName = getFormattedName(colName);
    ColumnTypes::const_iterator col = m_coldesc.begin();
    while (col != m_coldesc.end()) {
      QString c = getFormattedName(col->m_name);
      if (c.toUpper() == cName.toUpper()) { return (&(*col));  }
      col++;
    }
    return (0);
  }


  /**
   * @brief Extracts a column from a QString based upon a description
   *
   * This method should not be called for BINARY PDS tables.
   *
   * @param tline  Row from table data
   * @param cdesc  Column description
   *
   * @return QString Returns the extracted column as a QString
   */
  QString ImportPdsTable::getColumnValue(const QString &tline,
                                             const ColumnDescr &cdesc) const {
    return (tline.mid(cdesc.m_startByte, cdesc.m_numBytes));
  }

  /**
   * @brief Converts a column name to a camel-case after it has been cleansed
   *
   * This method will convert a column name to camel-case after some character
   * cleaning is performed.  All white space characters as defined by the 
   * IString class are converted to spaces.  Spaces are then compressed to one 
   * space. Any left/right parens are removed.  Then the conversion to 
   * camel-case is performed.
   *
   * Camel case always converts the first character in a QString to uppercase. 
   * Any space or '_' character are removed and the following character is 
   * converted to uppercase.  All other characters are converted to lowercase. 
   *
   * This method can be called for ASCII or BINARY PDS tables.
   *
   * @param colname Column name to converty
   *
   * @return QString Returns the formatted keyword
   */
  QString  ImportPdsTable::getFormattedName(
      const QString &colname) const {
    QString cname = QString(colname).replace(QRegExp("[(),]"), " ").simplified();

    bool uppercase = true;
    QString oString;
    for (int i = 0 ; i < cname.size() ; i++) {
      if (uppercase) {
        oString.push_back(cname[i].toUpper());
        uppercase = false;
      }
      else if ( (cname[i] == ' ') || (cname[i] == '_') ) {
        uppercase = true;
      }
      else {
        oString.push_back(cname[i].toLower());
      }
    }

    return (oString);
  }


  /**
   * @brief Determine generic data type of a column
   *
   * This method will determine the generic type of a column.  In general, PDS
   * table descriptions contain types that have some prepended qualifiers to
   * column data types.  This routine removes those qualifiers and only returns
   * the generic type.
   *
   * For example, if the incoming type is MSB_INTEGER, only INTEGER will be
   * returned.
   *
   * This method is used for ASCII or BINARY PDS tables.
   *
   * @param ttype PDS data type to convert
   *
   * @return QString Generic type found in the data type.
   */
  QString ImportPdsTable::getGenericType(const QString &ttype) const {
    return ttype.split("_").last();
  }


  /**
   * @brief Creates a TableField for the column type
   *
   * This method generates a TableField with the appropriate type to contain the
   * PDS coulumn data.  It will only create three different types:  Integer,
   * Double or Text.
   *
   * All PDS data types that have INTEGER in their type are stored as an Integer
   * field type.  PDS columns with DOUBLE, REAL or FLOAT are stored as Doubles.
   * All other types are Text types. 
   *  
   * This method is called for ASCII or BINARY PDS tables. 
   *
   * @param cdesc Column description to create the TableField from
   *
   * @return TableField Returns a TableField for the column type
   */
  TableField ImportPdsTable::makeField(const ColumnDescr &cdesc) {
    QString dtype = cdesc.m_dataType;
    QString name = getFormattedName(cdesc.m_name);
    if (m_pdsTableType == "ASCII") {
      if ( dtype == "INTEGER" ) {
        return (TableField(name, TableField::Integer));
      }
      else if ( ((dtype == "DOUBLE" ) 
                 || (dtype == "REAL")
                 || (dtype == "FLOAT")) ) {
        return (TableField(name, TableField::Double));
      }
      else {
        return (TableField(name, TableField::Text, cdesc.m_numBytes));
      }
    }
    else {
      return makeFieldFromBinaryTable(cdesc);
    }
  }

  /**
   * @brief Creates a TableRecord for columns
   *
   * This method creates a TableRecord for each column in the table.  This 
   * record should be provided when creating the ISIS Table object.  It can also 
   * be used to populate the table.  TableFields are added in the order provided 
   * in ctypes. 
   *
   * This method is called for ASCII or BINARY PDS tables. 
   *  
   * @param ctypes  Columns to create fields for and add to record
   *
   * @return TableRecord Returns TableRecord of columns/fields
   */
  TableRecord ImportPdsTable::makeRecord(const ColumnTypes &ctypes) {
    TableRecord rec;
    for (unsigned int i = 0 ; i < ctypes.size() ; i++) {
      TableField field = makeField(ctypes[i]);
      rec += field;
    }
    return (rec);
  }


  /**
   * @brief Extract a TableField from a PDS column in the text row
   *
   * This routine will extract the column and convert to a TableField with the
   * appropriate type to contain the field.
   *
   * This method is only called for ASCII PDS tables. 
   *  
   * @param cols Columns for a given row
   * @param cdesc Column description used to create TableField
   * @param tfield Isis3 TableField used to determine the data type to be imported.
   *
   * @return TableField  Returns the TableField with the value from the column
   */
  TableField &ImportPdsTable::extract(const Columns &cols,
                                      const ColumnDescr &cdesc,
                                      TableField &tfield) const {
    int ith = cdesc.m_colnum;
    try {
      IString data(cols[ith]);
      if (tfield.isInteger()) {
        data.Trim(" \t\r\n");
        tfield = data.ToInteger();
      }
      else if (tfield.isDouble()) {
        data.Trim(" \t\r\n");
        tfield = data.ToDouble();
      }
      else {  // Its a text field
        QString str(tfield.size(), ' ');
        str.insert(0, data.Trim(" \t\r\n").ToQt());
        tfield = str;
      }
    }
    catch (IException &e) {
      QString msg = "Conversion failure of column " + cdesc.m_name;
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }

    return (tfield);
  }

  /**
   * @brief Extract a table record of columns from a row of columns
   *
   * This method will create a TableRecord from a list of columns.  The columns
   * are selected from the list of column descriptions parameter.
   *
   * This method is only called for ASCII PDS tables.
   *
   * @param cols Row of column data
   * @param ctypes  List of columns to extract
   * @param record  TableRecord with fields to populate
   *
   * @return TableRecord Record containing extract fields from columns
   */
  TableRecord &ImportPdsTable::extract(const Columns &cols,
                                       const ColumnTypes &ctypes,
                                       TableRecord &record) const {
    for (unsigned int i = 0 ; i < ctypes.size() ; i++) {
      extract(cols, ctypes[i], record[i]);
    }
    return (record);
  }

  /**
   * @brief Fill the ISIS Table object with PDS table data
   *
   * This method populates the ISIS Table object with selected PDS table data
   * fields.
   *
   * This method is used for ASCII or BINARY PDS tables.
   *  
   * @param table   ISIS Table object to populate
   * @param cols PDS Column map to extract data
   * @param record  ISIS TableRecord with fields to contain PDS columns
   */
  void ImportPdsTable::fillTable(Table &table, const ColumnTypes &cols,
                                 TableRecord &record) const {
    if (m_pdsTableType == "ASCII") {
      for (unsigned int i = 0 ; i < m_rows.size() ; i++) {
        try {
          table += extract(m_rows[i], cols, record);
        }
        catch (IException &e) {
          QString msg = "Failed to convert data in row [" + toString((int) i) + "]";
          throw IException(e, IException::Programmer, msg, _FILEINFO_);
        }
      }
    }
    else {
      QString tempTblFile = m_pdsTableFile;
      ifstream pdsFileStream(tempTblFile.toAscii().data(), ifstream::binary);
      if (!pdsFileStream) {
        IString msg = "Unable to open file containing PDS table [" 
                      + tempTblFile + "].";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      // read and discard the rows above the table data
      char *rowBuffer = new char[m_recordBytes];
      for (int i = 1; i < m_pdsTableStart; i++) {
        pdsFileStream.read(rowBuffer, m_recordBytes);
      }
      delete [] rowBuffer;
      rowBuffer = NULL;
      // now, import and add the records to the table
      for (int rowIndex = 0; rowIndex < m_trows; rowIndex++) {
        rowBuffer = new char[m_recordBytes];
        pdsFileStream.read(rowBuffer, m_recordBytes);
        TableRecord rec = extractBinary(rowBuffer, record);
        table += rec;
  //      TableRecord rec = extractBinary(pdsFileStream, record);
        delete [] rowBuffer;
        rowBuffer = NULL;
      }
    }
    return;
  }

  /**
   * Returns the number of columns in the table.
   *
   * This method can be called for ASCII or BINARY PDS tables.
   *
   * @return The number of columns.
   */
  int ImportPdsTable::columns() const { 
    return (m_coldesc.size()); 
  }

  /**
   * Returns the number of rows in the table.
   *
   * This method can be called for ASCII or BINARY PDS tables.
   *
   * @return The number of rows.
   */
  int ImportPdsTable::rows() const { 
    return (m_rows.size()); 
  }

  /** 
   * This method is used to set the field values for the given record. These 
   * values are extracted from the given binary buffer containing the PDS 
   * table data for the corresponding row. 
   *  
   * This method is only called for BINARY PDS tables. 
   *  
   * @param rowBuffer Buffer containing the binary information for one row of 
   *                  the table.
   * @param record The TableRecord containing fields with correct names and 
   *               types but no values. The field values will be set by this
   *               method.
   * @return TableRecord containing the field values extracted from the row 
   *         buffer.
   */
  TableRecord ImportPdsTable::extractBinary(char *rowBuffer, TableRecord &record) const {
    // for each record loop through the columns to get field values
    for (int colIndex = 0; colIndex < columns(); colIndex++) {
      QString columnName = m_coldesc[colIndex].m_name;
      for (int fieldIndex = 0 ; fieldIndex < record.Fields() ; fieldIndex++) {
        QString fieldName = record[fieldIndex].name();
        if (fieldName == columnName) {
          int startByte = m_coldesc[colIndex].m_startByte;
          int numBytes = m_coldesc[colIndex].m_numBytes;
          if (record[fieldIndex].isInteger()) {
            int columnValue;
            memmove(&columnValue, &rowBuffer[startByte], numBytes);
            EndianSwapper endianSwap(m_byteOrder);
            int fieldValue = endianSwap.Int(&columnValue);
            record[fieldIndex] = fieldValue;
          }
          else if (record[fieldIndex].isDouble()) {
            EndianSwapper endianSwap(m_byteOrder);
            double columnValue;
            memmove(&columnValue, &rowBuffer[startByte], numBytes);
            double fieldValue = endianSwap.Double(&columnValue);
            record[fieldIndex] = fieldValue;
          }
          else if (record[fieldIndex].isReal()) {
            EndianSwapper endianSwap(m_byteOrder);
            float columnValue;
            memmove(&columnValue, &rowBuffer[startByte], numBytes);
            float fieldValue = endianSwap.Float(&columnValue);
            record[fieldIndex] = fieldValue;
          }
          else if (record[fieldIndex].isText()) {
            QString fieldValue(numBytes, '\0');
            for (int byte = 0; byte < numBytes; byte++) {
              fieldValue[byte] = rowBuffer[startByte + byte];
            }
            record[fieldIndex] = fieldValue;
          }
        }
      }

    }
    return record;
  }

  /**
   * Creates an empty TableField with the appropriate type from a binary PDS
   * table column description. This method also determines whether the pds table
   * has byte order LSB or MSB.
   *
   * This method is only called for BINARY PDS tables. 
   *  
   * @param cdesc The ColumnDescr reference used to determine the column keyword
   *              values NAME, BYTES, and DATA_TYPE from the PDS file.
   *  
   * @return TableField object with the correct field name and field type, but 
   *         no value.
   */
  TableField ImportPdsTable::makeFieldFromBinaryTable(const ColumnDescr &cdesc){
    QString dataType = cdesc.m_dataType;
    // For binary tables, we will not reformat the name of the column
    QString name = cdesc.m_name;
    if (dataType == "MSB_INTEGER" || dataType == "INTEGER"    
        || dataType == "SUN_INTEGER" || dataType == "MAC_INTEGER") {
      if (cdesc.m_numBytes != 4) {
        QString msg = "Only 4 byte integer values are supported in Isis. "
                      "PDS Column [" + cdesc.m_name
                      + "] has an integer DATA_TYPE with [BYTES = " 
                      + toString(cdesc.m_numBytes) + "].";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      setPdsByteOrder("MSB");
      return TableField(name, TableField::Integer);
    }
    else if (dataType == "LSB_INTEGER" || dataType == "VAX_INTEGER"
             || dataType == "PC_INTEGER" ) {
      if (cdesc.m_numBytes != 4) {
        QString msg = "Only 4 byte integer values are supported in Isis. "
                      "PDS Column [" + cdesc.m_name
                      + "] has an integer DATA_TYPE with [BYTES = " 
                      + toString(cdesc.m_numBytes) + "].";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      setPdsByteOrder("LSB");
      return TableField(name, TableField::Integer);
    }
    else if (dataType == "FLOAT"    //IEEE_REAL alias (MSB)
             || dataType == "REAL"     //IEEE_REAL alias (MSB)
             || dataType == "SUN_REAL" //IEEE_REAL alias (MSB)
             || dataType == "MAC_REAL" //IEEE_REAL alias (MSB)
             || dataType == "IEEE_REAL" ) {
      setPdsByteOrder("MSB");
      if (cdesc.m_numBytes == 8) {
        return TableField(name, TableField::Double);
      }
      else if (cdesc.m_numBytes == 4) {
        return TableField(name, TableField::Real);
      }
      else {
        IString msg = "Only 4 byte or 8 byte real values are supported in Isis. "
                      "PDS Column [" + cdesc.m_name
                      + "] has a real DATA_TYPE with [BYTES = " 
                      + toString(cdesc.m_numBytes) + "].";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }
    else if (dataType == "PC_REAL") {
      setPdsByteOrder("LSB");
      if (cdesc.m_numBytes == 8) {
        return TableField(name, TableField::Double);
      }
      else if (cdesc.m_numBytes == 4) {
        return TableField(name, TableField::Real);
      }
      else {
        QString msg = "Only 4 byte or 8 byte real values are supported in Isis. "
                      "PDS Column [" + cdesc.m_name
                      + "] has a real DATA_TYPE with [BYTES = " 
                      + toString(cdesc.m_numBytes) + "].";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }
    else if (dataType.contains("CHARACTER")
             || dataType.contains("ASCII")
             || dataType == "DATE" || dataType == "TIME" ) {
      return TableField(name, TableField::Text, cdesc.m_numBytes);
    }
    // Isis3 tables currently don't support any of the following PDS DATA_TYPE:
    // BIT_STRING, COMPLEX, N/A, BOOLEAN, UNSIGNED_INTEGER, IBM types, some VAX types
    IString msg = "PDS Column [" + cdesc.m_name
                  + "] has an unsupported DATA_TYPE [" 
                  + dataType + "]."; 
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }

  /**
   * Sets the byte order for BINARY PDS table files.
   *
   * This method is only used for BINARY PDS tables.
   *
   * @param byteOrder The byte order of the PDS binary table. Valid values are 
   *                  "LSB" or "MSB".
   */
  void ImportPdsTable::setPdsByteOrder(QString byteOrder) {
    if (!m_byteOrder.isEmpty() && m_byteOrder != byteOrder) {
      QString msg = "Unable import the binary PDS table [" + m_tableName
                    + "]. The column DATA_TYPE values indicate differing byte "
                      "orders. ";
      throw IException(IException::Unknown, msg.toStdString(), _FILEINFO_);
    }
    m_byteOrder = byteOrder;
  }


}  //  namespace Isis

