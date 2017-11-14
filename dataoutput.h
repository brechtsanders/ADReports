#ifndef INCLUDED_DATAOUTPUT_H
#define INCLUDED_DATAOUTPUT_H

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#ifdef USE_XLSXIO
#include <xlsxio_write.h>
#endif

#ifdef USE_XLSXIO
#define DATAOUTPUT_FORMAT_HELP_LIST "TSV/TXT/HTML/XML/XLSX"
#else
#define DATAOUTPUT_FORMAT_HELP_LIST "TSV/TXT/HTML/XML"
#endif

class DataOutputBase* CreateDataOutput (const char* dstformat, const char* dstfilename);

class DataOutputBase
{
 public:
  virtual ~DataOutputBase ();
  virtual void AddColumn (const char* name, int width = 0) = 0;
  virtual void AddRow () = 0;
  virtual void AddData (int value) = 0;
  virtual void AddData (int64_t value) = 0;
  virtual void AddData (double value) = 0;
  virtual void AddData (const char* value) = 0;
};

class DataOutputTSV : public DataOutputBase
{
 protected:
  FILE* dst;
  bool owndst;
  int col;
 public:
  DataOutputTSV ();
  DataOutputTSV (const char* filename);
  virtual ~DataOutputTSV ();
  static const char* GetDefaultExtension () { return ".tsv"; }
  virtual void AddColumn (const char* name, int width = 0);
  virtual void AddRow ();
  virtual void AddData (int value);
  virtual void AddData (int64_t value);
  virtual void AddData (double value);
  virtual void AddData (const char* value);
};

class DataOutputTXT : public DataOutputBase
{
 protected:
  FILE* dst;
  bool owndst;
  int col;
  std::vector<unsigned int> colwidths;
 public:
  DataOutputTXT ();
  DataOutputTXT (const char* filename);
  virtual ~DataOutputTXT ();
  static const char* GetDefaultExtension () { return ".txt"; }
  virtual void AddColumn (const char* name, int width = 0);
  virtual void AddRow ();
  virtual void AddData (int value);
  virtual void AddData (int64_t value);
  virtual void AddData (double value);
  virtual void AddData (const char* value);
};

class DataOutputHTML : public DataOutputBase
{
 protected:
  FILE* dst;
  bool owndst;
  std::string tablename;
  int col;
  std::vector<unsigned int> colwidths;
  std::vector<std::string> colnames;
  bool headersent;
  void SendHeaderIfNeeded ();
 public:
  DataOutputHTML ();
  DataOutputHTML (const char* filename, const char* title = NULL);
  virtual ~DataOutputHTML ();
  static const char* GetDefaultExtension () { return ".html"; }
  virtual void AddColumn (const char* name, int width = 0);
  virtual void AddRow ();
  virtual void AddData (int value);
  virtual void AddData (int64_t value);
  virtual void AddData (double value);
  virtual void AddData (const char* value);
};

class DataOutputXML : public DataOutputBase
{
 protected:
  FILE* dst;
  bool owndst;
  std::string tablename;
  int col;
  bool headersent;
  std::vector<unsigned int> colwidths;
  std::vector<std::string> colnames;
  static std::string ConvertToXMLData (std::string str);
  void SendHeaderIfNeeded ();
 public:
  DataOutputXML ();
  DataOutputXML (const char* filename, const char* worksheetname = NULL);
  virtual ~DataOutputXML ();
  static const char* GetDefaultExtension () { return ".xml"; }
  virtual void AddColumn (const char* name, int width = 0);
  virtual void AddRow ();
  virtual void AddData (int value);
  virtual void AddData (int64_t value);
  virtual void AddData (double value);
  virtual void AddData (const char* value);
};

#ifdef USE_XLSXIO
class DataOutputXLSX : public DataOutputBase
{
 protected:
  xlsxiowriter dst;
 public:
  DataOutputXLSX (const char* filename);
  DataOutputXLSX (const char* filename, const char* sheetname);
  virtual ~DataOutputXLSX ();
  static const char* GetDefaultExtension () { return ".xlsx"; }
  virtual void AddColumn (const char* name, int width = 0);
  virtual void AddRow ();
  virtual void AddData (int value);
  virtual void AddData (int64_t value);
  virtual void AddData (double value);
  virtual void AddData (const char* value);
};
#endif

#endif //INCLUDED_DATAOUTPUT_H
