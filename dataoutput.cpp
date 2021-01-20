#include "dataoutput.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#ifdef USE_XLSXIO
#include <unistd.h>
#endif

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#define strcasecmp stricmp
#endif

////////////////////////////////////////////////////////////////////////

std::string ConvertToHTMLData (std::string str)
{
	std::string::size_type pos = 0;
	while (str[pos] == ' ') {
    str.replace(pos, 1, "&nbsp;");
		pos += 6;
		if (str[pos] == ' ')
      pos++;
	}
	while (str[pos]) {
		switch (str[pos]) {
			case '&' :
				str.replace(pos, 1, "&amp;");
				pos += 5;
				break;
			case '\"' :
				str.replace(pos, 1, "&quot;");
				pos += 6;
				break;
			case '<' :
				str.replace(pos, 1, "&lt;");
				pos += 4;
				break;
			case '>' :
				str.replace(pos, 1, "&gt;");
				pos += 4;
				break;
			case ' ' :
        if (str[pos + 1] == ' ' || !str[pos + 1]) {
          str.replace(pos, 1, "&nbsp;");
          pos += 6;
        } else {
          pos++;
        }
				break;
			default:
/*
				//convert ASCII to UTF-8 (spread across 2 bytes: 110zzzxx 10xxxxxx)
				if ((unsigned char)str[pos] >= 0x80) {
					const char utf8[] = {((unsigned char)str[pos] >> 6) | 0xC0, ((unsigned char)str[pos] & 0x3F) | 0x80, 0};
					str.replace(pos, 1, utf8);
					pos += 2;
					break;
				}
*/
				pos++;
				break;
		}
	}
	return str;
}

std::string getbasename (const char* filename, unsigned int maxlen = 0)
{
  std::string result = (filename ? filename : "");
  std::string::size_type pos;
  //strip path
  if ((pos = result.rfind("/", result.length())) != std::string::npos)
    result.erase(0, pos + 1);
#ifdef _WIN32
  if ((pos = result.rfind(":", result.length())) != std::string::npos)
    result.erase(0, pos + 1);
  if ((pos = result.rfind("\\", result.length())) != std::string::npos)
    result.erase(0, pos + 1);
#endif
  //strip extension
  if ((pos = result.find(".", 0)) != std::string::npos)
    result.erase(pos);
  if (maxlen > 0 && result.length() > maxlen)
    result.erase(maxlen);
  return result;
}

////////////////////////////////////////////////////////////////////////

class DataOutputBase* CreateDataOutput (const char* dstformat, const char* dstfilename)
{
  if (!dstformat || !*dstformat || strcasecmp(dstformat, "TSV") == 0)
    return new DataOutputTSV(dstfilename);
  if (strcasecmp(dstformat, "TXT") == 0)
    return new DataOutputTXT(dstfilename);
  if (strcasecmp(dstformat, "HTML") == 0)
    //return new DataOutputHTML(dstfilename);
    return new DataOutputHTML(dstfilename, getbasename(dstfilename).c_str());
  if (strcasecmp(dstformat, "XML") == 0)
    return new DataOutputXML(dstfilename, getbasename(dstfilename, 31).c_str());
#ifdef USE_XLSXIO
  if (strcasecmp(dstformat, "XLSX") == 0)
    return new DataOutputXLSX(dstfilename, getbasename(dstfilename).c_str());
#endif
  return NULL;
}

////////////////////////////////////////////////////////////////////////

DataOutputBase::~DataOutputBase ()
{
}

////////////////////////////////////////////////////////////////////////

DataOutputTSV::DataOutputTSV()
: dst(NULL), owndst(false), col(0)
{
  dst = stdout;
}

DataOutputTSV::DataOutputTSV(const char* filename)
: dst(NULL), owndst(false), col(0)
{
  if (filename && *filename && strcmp(filename, "-") != 0) {
    dst = fopen(filename, "w");
    owndst = true;
  } else {
    dst = stdout;
  }
}

DataOutputTSV::~DataOutputTSV()
{
  if (col)
    fprintf(dst, "\n");
  if (owndst)
    fclose(dst);
}

void DataOutputTSV::AddColumn (const char* name, int width)
{
  AddData(name);
}

void DataOutputTSV::AddRow ()
{
  fprintf(dst, "\n");
  col = 0;
}

void DataOutputTSV::AddData (int value)
{
  if (col++)
    fprintf(dst, "\t");
  fprintf(dst, "%i", value);
}

void DataOutputTSV::AddData (int64_t value)
{
  if (col++)
    fprintf(dst, "\t");
  fprintf(dst, "%" PRIi64, value);
}

void DataOutputTSV::AddData (double value)
{
  if (col++)
    fprintf(dst, "\t");
  fprintf(dst, "%.32G", value);
}

void DataOutputTSV::AddData (const char* value)
{
  if (col++)
    fprintf(dst, "\t");
  if (value)
    fprintf(dst, "%s", value);
}

////////////////////////////////////////////////////////////////////////

DataOutputTXT::DataOutputTXT()
: dst(NULL), owndst(false), col(0)
{
  dst = stdout;
}

DataOutputTXT::DataOutputTXT(const char* filename)
: dst(NULL), owndst(false), col(0)
{
  if (filename && *filename && strcmp(filename, "-") != 0) {
    dst = fopen(filename, "w");
    owndst = true;
  } else {
    dst = stdout;
  }
}

DataOutputTXT::~DataOutputTXT()
{
  if (col)
    fprintf(dst, "\n");
  if (owndst)
    fclose(dst);
}

void DataOutputTXT::AddColumn (const char* name, int width)
{
  colwidths.push_back(width);
  AddData(name);
}

void DataOutputTXT::AddRow ()
{
  fprintf(dst, "\n");
  col = 0;
}

void DataOutputTXT::AddData (int value)
{
  if (col++)
    fprintf(dst, "  ");
  if (colwidths[col - 1] > 0)
    fprintf(dst, "%*i", (int)colwidths[col - 1], value);
  else
    fprintf(dst, "%i", value);
}

void DataOutputTXT::AddData (int64_t value)
{
  if (col++)
    fprintf(dst, "  ");
  if (colwidths[col - 1] > 0)
    fprintf(dst, "%*" PRIi64, (int)colwidths[col - 1], value);
  else
    fprintf(dst, "%" PRIi64, value);
}

void DataOutputTXT::AddData (double value)
{
  if (col++)
    fprintf(dst, "  ");
  if (colwidths[col - 1] > 2)
    fprintf(dst, "%*.*G", (int)colwidths[col - 1], (int)colwidths[col - 1] - 2, value);
  else
    fprintf(dst, "%.32G", value);
}

void DataOutputTXT::AddData (const char* value)
{
  if (col++)
    fprintf(dst, "  ");
  if (colwidths[col - 1] > 0)
    fprintf(dst, "%*s", (int)-colwidths[col - 1], (value ? value : ""));
  else
    fprintf(dst, "%s", (value ? value : ""));
}

////////////////////////////////////////////////////////////////////////

void DataOutputHTML::SendHeaderIfNeeded ()
{
  if (!headersent && dst) {
    fprintf(dst,
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
      "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n" \
      "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n" \
      "<head>\n" \
      "<title>%s</title>\n" \
      "<meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\"/>\n" \
      "<style type=\"text/css\">\n" \
      "body { background-color: #FFFFFF; color: #000000; margin: 0px; padding: 0px; font: 10pt Consolas, Courier New, Courier, sans-serif; }\n" \
      "@media handheld { body { margin: 0px; padding: 0px; font: 7pt sans-serif; } }\n" \
      "table { border: 1px solid #DDDDDD; border-spacing: 0px; border-collapse: collapse; }\n" \
      "th { padding-left: 0.5ex; padding-right: 0.5ex; border: 1px solid #DDDDDD; background-color: #F8F8F8; font-weight: bold; text-align: left; }\n" \
      "td { padding-left: 0.5ex; padding-right: 0.5ex; border: 1px dotted #DDDDDD; }\n" \
      "</style>\n" \
      "</head>\n" \
      "<body>\n" \
      "<table>\n" \
      "<tr><head>", ConvertToHTMLData(tablename).c_str()
    );
/*
    std::vector<std::string>::iterator colname;
    for (colname = colnames.begin(); colname != colnames.end(); colname++) {
      fprintf(dst, "<th>%s</th>", ConvertToHTMLData(*colname).c_str());
    }
*/
    unsigned int i;
    std::string value;
    for (i = 0; i < colnames.size(); i++) {
      value = ConvertToHTMLData(colnames[i]);
      if (value.empty())
        value = "&nbsp;";
      if (colwidths[i] > 0)
        fprintf(dst, "<th style=\"min-width: %u.5ex\">%s</th>", colwidths[i] + 1, value.c_str());
      else
        fprintf(dst, "<th>%s</th>", value.c_str());
    }
    fprintf(dst, "</head>");
    headersent = true;
    col = 0;
  }
}

DataOutputHTML::DataOutputHTML()
: dst(NULL), owndst(false), col(0), headersent(false)
{
  dst = stdout;
}

DataOutputHTML::DataOutputHTML(const char* filename, const char* title)
: dst(NULL), owndst(false), col(0), headersent(false)
{
  if (filename && *filename && strcmp(filename, "-") != 0) {
    dst = fopen(filename, "wb");
    owndst = true;
  } else {
    dst = stdout;
  }
  tablename = (title ? title : "");
}

DataOutputHTML::~DataOutputHTML()
{
  if (!headersent)
    SendHeaderIfNeeded();
  if (col)
    fprintf(dst, "\n");
  fprintf(dst, "</tr>\n" \
    "</table>\n" \
    "</body>\n" \
    "</html>\n"
  );
  if (owndst)
    fclose(dst);
}

void DataOutputHTML::AddColumn (const char* name, int width)
{
  colwidths.push_back(width);
  colnames.push_back(name);
}

void DataOutputHTML::AddRow ()
{
  if (!headersent)
    SendHeaderIfNeeded();
  fprintf(dst, "</tr>\n<tr>");
  col = 0;
}

void DataOutputHTML::AddData (int value)
{
  col++;
  fprintf(dst, "<td align=\"right\">%i</td>", value);
}

void DataOutputHTML::AddData (int64_t value)
{
  col++;
  fprintf(dst, "<td align=\"right\">%" PRIi64 "</td>", value);
}

void DataOutputHTML::AddData (double value)
{
  col++;
  fprintf(dst, "<td align=\"right\">%.32G</td>", value);
}

void DataOutputHTML::AddData (const char* value)
{
  col++;
  if (value && *value)
    fprintf(dst, "<td>%s</td>", ConvertToHTMLData(value).c_str());
  else
    fprintf(dst, "<td>&nbsp;</td>");
}

////////////////////////////////////////////////////////////////////////

std::string DataOutputXML::ConvertToXMLData (std::string str)
{
	std::string::size_type pos = 0;
	while (str[pos] == ' ') {
    str.replace(pos, 1, "&nbsp;");
		pos += 6;
		if (str[pos] == ' ')
      pos++;
	}
	while (str[pos]) {
		switch (str[pos]) {
			case '&' :
				str.replace(pos, 1, "&amp;");
				pos += 5;
				break;
			case '\"' :
				str.replace(pos, 1, "&quot;");
				pos += 6;
				break;
			case '<' :
				str.replace(pos, 1, "&lt;");
				pos += 4;
				break;
			case '>' :
				str.replace(pos, 1, "&gt;");
				pos += 4;
				break;
			case ' ' :
        if (str[pos + 1] == ' ' || !str[pos + 1]) {
          str.replace(pos, 1, "&nbsp;");
          pos += 6;
        } else {
          pos++;
        }
				break;
			default:
				//convert ASCII to UTF-8 (spread across 2 bytes: 110zzzxx 10xxxxxx)
				if ((unsigned char)str[pos] >= 0x80) {
					const char utf8[] = {(char)(((unsigned char)str[pos] >> 6) | 0xC0), (char)(((unsigned char)str[pos] & 0x3F) | 0x80), 0};
					str.replace(pos, 1, utf8);
					pos += 2;
					break;
				}
				pos++;
				break;
		}
	}
	return str;
}

void DataOutputXML::SendHeaderIfNeeded ()
{
  if (!headersent && dst) {
    fprintf(dst, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<?mso-application progid=\"Excel.Sheet\"?>\n"
      "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n"
      " xmlns:o=\"urn:schemas-microsoft-com:office:office\"\n"
      " xmlns:x=\"urn:schemas-microsoft-com:office:excel\"\n"
      " xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"\n"
      " xmlns:html=\"http://www.w3.org/TR/REC-html40\">\n"
      " <DocumentProperties xmlns=\"urn:schemas-microsoft-com:office:office\">\n"
  /*
      "  <LastAuthor>Brecht Sanders</LastAuthor>\n"
      "  <Created>2012-05-02T13:23:43Z</Created>\n"
      "  <LastSaved>2010-05-02T13:23:43Z</LastSaved>\n"
      "  <Version>11.6568</Version>\n"
  */
      " </DocumentProperties>\n"
      " <ExcelWorkbook xmlns=\"urn:schemas-microsoft-com:office:excel\">\n"
  /*
      "  <WindowHeight>12660</WindowHeight>\n"
      "  <WindowWidth>19020</WindowWidth>\n"
      "  <WindowTopX>120</WindowTopX>\n"
      "  <WindowTopY>120</WindowTopY>\n"
      "  <ProtectStructure>False</ProtectStructure>\n"
      "  <ProtectWindows>False</ProtectWindows>\n"
  */
      " </ExcelWorkbook>\n"
      " <Styles>\n"
      "  <Style ss:ID=\"Default\" ss:Name=\"Normal\">\n"
      "   <Alignment ss:Vertical=\"Top\"/>\n"
      "   <Borders/>\n"
      //"   <Font/>\n"
      "   <Font ss:FontName=\"Courier New\" x:Family=\"Swiss\"/>\n"
      "   <Interior/>\n"
      "   <NumberFormat/>\n"
      "   <Protection/>\n"
      "  </Style>\n"
      "  <Style ss:ID=\"s21\">\n"
      //"   <Font x:Family=\"Swiss\" ss:Bold=\"1\"/>\n"
      //"   <Font x:Family=\"Monospace\" ss:Bold=\"1\"/>\n"
      //"   <Font ss:FontName=\"Courier New\" x:Family=\"Swiss\" ss:Bold=\"1\"/>\n"
      //"   <Font ss:FontName=\"Courier New\" x:Family=\"Swiss\"/ ss:Bold=\"1\"/>\n"
      "   <Font ss:FontName=\"Courier New\" x:Family=\"Swiss\" ss:Bold=\"1\"/>\n"
      "  </Style>\n"
      "  <Style ss:ID=\"s22\">\n"
      "   <Alignment ss:Vertical=\"Bottom\"/>\n"
      //"   <Font x:Family=\"Swiss\" ss:Bold=\"1\"/>\n"
      //"   <Font ss:FontName=\"Courier New\" x:Family=\"Swiss\"/ ss:Bold=\"1\"/>\n"
      "   <Font ss:FontName=\"Courier New\" x:Family=\"Swiss\" ss:Bold=\"1\"/>\n"
      "  </Style>\n"
      "  <Style ss:ID=\"s23\">\n"
      //"   <Font x:Family=\"Swiss\" ss:Bold=\"0\"/>\n"
      "   <NumberFormat ss:Format=\"[$-413]dd\\ mmm\\ yyyy;@\"/>\n"
      "  </Style>\n"
      "  <Style ss:ID=\"s24\">\n"
      //"   <Font x:Family=\"Swiss\" ss:Bold=\"0\"/>\n"
      "   <NumberFormat ss:Format=\"Short Time\"/>\n"
      "  </Style>\n"
      " </Styles>\n"
      " <Worksheet ss:Name=\"%s\">\n"
      "  <WorksheetOptions xmlns=\"urn:schemas-microsoft-com:office:excel\">\n"
  /*
      "   <PageSetup>\n"
      "    <PageMargins x:Bottom=\"0.984251969\" x:Left=\"0.78740157499999996\"\n"
      "     x:Right=\"0.78740157499999996\" x:Top=\"0.984251969\"/>\n"
      "   </PageSetup>\n"
      "   <Print>\n"
      "    <ValidPrinterInfo/>\n"
      "    <PaperSizeIndex>9</PaperSizeIndex>\n"
      "    <HorizontalResolution>600</HorizontalResolution>\n"
      "    <VerticalResolution>600</VerticalResolution>\n"
      "   </Print>\n"
  */
      "   <Selected/>\n"
      "   <FreezePanes/>\n"
      "   <FrozenNoSplit/>\n"
      "   <SplitHorizontal>1</SplitHorizontal>\n"
      "   <TopRowBottomPane>1</TopRowBottomPane>\n"
      "   <ActivePane>2</ActivePane>\n"
  /*
      "   <Panes>\n"
      "    <Pane>\n"
      "     <Number>3</Number>\n"
      "     <ActiveRow>2</ActiveRow>\n"
      "     <ActiveCol>1</ActiveCol>\n"
      "    </Pane>\n"
      "   </Panes>\n"
      "   <ProtectObjects>False</ProtectObjects>\n"
      "   <ProtectScenarios>False</ProtectScenarios>\n"
  */
      "  </WorksheetOptions>\n"
      "  <Table>\n", (!tablename.empty() ? ConvertToHTMLData(tablename).c_str() : "Sheet1"));
    std::vector<unsigned int>::iterator it_width;
    for (it_width = colwidths.begin(); it_width < colwidths.end(); it_width++) {
      if (*it_width > 0)
        fprintf(dst, "   <Column ss:AutoFitWidth=\"0\" ss:Width=\"%.2f\"/>\n", (float)0.75 * (*it_width * 8 + 5));
      else
        fprintf(dst, "   <Column ss:AutoFitWidth=\"1\" ss:Width=\"64\"/>\n");
    }
    std::vector<std::string>::iterator i_name;
    fprintf(dst, "   <Row ss:StyleID=\"s22\">\n");
    for (i_name = colnames.begin(); i_name < colnames.end(); i_name++) {
      fprintf(dst, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", i_name->c_str());
    }
    fprintf(dst, "   </Row>\n");
    headersent = true;
  }
}

DataOutputXML::DataOutputXML()
: dst(NULL), owndst(false), col(0), headersent(false)
{
  dst = stdout;
  tablename = "Sheet1";
}

DataOutputXML::DataOutputXML(const char* filename, const char* worksheetname)
: dst(NULL), owndst(false), col(0), headersent(false)
{
  if (filename && *filename && strcmp(filename, "-") != 0) {
    dst = fopen(filename, "wb");
    owndst = true;
  } else {
    dst = stdout;
  }
  tablename = (worksheetname && *worksheetname ? worksheetname : "Sheet1");
}

DataOutputXML::~DataOutputXML()
{
  if (dst) {
    if (!headersent)
      SendHeaderIfNeeded();
    if (col)
      fprintf(dst, "</Row>\n");
    fprintf(dst, "  </Table>\n"
      " </Worksheet>\n"
      "</Workbook>\n");
    if (owndst)
      fclose(dst);
  }
}

void DataOutputXML::AddColumn (const char* name, int width)
{
  colwidths.push_back(width);
  colnames.push_back(name);
}

void DataOutputXML::AddRow ()
{
  if (!headersent)
    SendHeaderIfNeeded();
  if (col) {
    fprintf(dst, "</Row>\n");
    col = 0;
  }
  fprintf(dst, "<Row>");
}

void DataOutputXML::AddData (int value)
{
  col++;
  fprintf(dst, "<Cell><Data ss:Type=\"Number\">%i</Data></Cell>", value);
}

void DataOutputXML::AddData (int64_t value)
{
  col++;
  fprintf(dst, "<Cell><Data ss:Type=\"Number\">%" PRIi64 "</Data></Cell>", value);
}

void DataOutputXML::AddData (double value)
{
  col++;
  fprintf(dst, "<Cell><Data ss:Type=\"Number\">%.32G</Data></Cell>", value);
}

void DataOutputXML::AddData (const char* value)
{
  col++;
  fprintf(dst, "<Cell><Data ss:Type=\"String\">%s</Data></Cell>", value ? value : "");
}

////////////////////////////////////////////////////////////////////////

#ifdef USE_XLSXIO
#define XLSX_DETECTION_ROWS 50
#define XLSX_ROW_HEIGHT 1

DataOutputXLSX::DataOutputXLSX (const char* filename)
{
  unlink(filename);
  if ((dst = xlsxiowrite_open(filename, NULL)) != NULL) {
    xlsxiowrite_set_detection_rows(dst, XLSX_DETECTION_ROWS);
#ifdef XLSX_ROW_HEIGHT
    xlsxiowrite_set_row_height(dst, XLSX_ROW_HEIGHT);
#endif
  }
}

DataOutputXLSX::DataOutputXLSX (const char* filename, const char* sheetname)
{
  unlink(filename);
  if ((dst = xlsxiowrite_open(filename, sheetname)) != NULL) {
    xlsxiowrite_set_detection_rows(dst, XLSX_DETECTION_ROWS);
#ifdef XLSX_ROW_HEIGHT
    xlsxiowrite_set_row_height(dst, XLSX_ROW_HEIGHT);
#endif
  }
}

DataOutputXLSX::~DataOutputXLSX ()
{
  xlsxiowrite_close(dst);
}

void DataOutputXLSX::AddColumn (const char* name, int width)
{
  xlsxiowrite_add_column(dst, name, width);
}

void DataOutputXLSX::AddRow ()
{
  xlsxiowrite_next_row(dst);
}

void DataOutputXLSX::AddData (int value)
{
  xlsxiowrite_add_cell_int(dst, value);
}

void DataOutputXLSX::AddData (int64_t value)
{
  xlsxiowrite_add_cell_int(dst, value);
}

void DataOutputXLSX::AddData (double value)
{
  xlsxiowrite_add_cell_float(dst, value);
}

void DataOutputXLSX::AddData (const char* value)
{
  xlsxiowrite_add_cell_string(dst, value);
}

#endif

