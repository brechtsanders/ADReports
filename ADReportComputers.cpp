#include <stdio.h>
#include <ctype.h>
#include <string>
#include <vector>
#include "adreports_version.h"
#include "ldapconnection.h"
#include "dataoutput.h"
#include "adformats.h"

void show_help()
{
  printf(
    "ADReportComputers v" ADREPORTS_VERSION_STRING " - generate Active Directory computer reports\n" \
    "Credits: " ADREPORTS_CREDITS "\n" \
    "Usage:  ADReportComputers " LDAP_COMMAND_LINE_PARAMETERS " [-f format] [-o file] [-g group] [-c days] [-l days] [-e] [-d] [-q ldapfilter]\n" \
    "Parameters:\n" \
    LDAP_COMMAND_LINE_HELP \
    "  -f format      \tOutput format (" DATAOUTPUT_FORMAT_HELP_LIST ")\n" \
    "  -o file        \tOutput file (default is standard output)\n" \
    "  -g group       \tInclude all members of this group (may be specified\n" \
    "                 \tmultiple times, default is all computers)\n" \
    "  -c days        \tShow only computers created in the last number of days\n" \
    "  -l days        \tShow only computers logged on in the last number of days\n" \
    "                 \t(not logged on if negative)\n" \
    "  -e             \tShow only enabled computers\n" \
    "  -d             \tShow only disabled computers\n" \
    "  -q ldapfilter  \tLDAP filter\n" \
    "\n"
  );
}

int main (int argc, char *argv[])
{
  char* dstformat = NULL;
  char* dstfilename = NULL;
  std::vector<std::string> groups;
  int createdlastdays = -1;
  int loggedonlastdays = 0;
  bool enabledonly = false;
  bool disabledonly = false;
  std::string ldapfilter;
  LDAPConnection* ldap = new LDAPConnection;
  LDAPResponse* result;

  //parse command line parameters
  {
    int i = 0;
    const char* param;
    bool paramerror = false;
    while (!paramerror && ++i < argc) {
      if (!argv[i][0] || (argv[i][0] != '/' && argv[i][0] != '-')) {
        paramerror = true;
        break;
      }
      if (!ldap->ProcessCommandLineParameter(argc, argv, i)) {
        param = NULL;
        switch (tolower(argv[i][1])) {
          case '?' :
            show_help();
            return 0;
          case 'f' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              return false;
            dstformat = strdup(param);
            break;
          case 'o' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              return false;
            dstfilename = strdup(param);
            break;
          case 'g' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              return false;
            groups.push_back(param);
            break;
          case 'c' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              return false;
            createdlastdays = atoi(param);
            break;
          case 'l' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              return false;
            loggedonlastdays = atoi(param);
            break;
          case 'e' :
            enabledonly = true;
            break;
          case 'd' :
            disabledonly = true;
            break;
          case 'q' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              return false;
            ldapfilter = param;
            break;
          default :
            paramerror = true;
            break;
        }
      }
    }
    if (paramerror) {
      fprintf(stderr, "Invalid command line parameters\n");
      return 1;
    }
  }

  //initialize
  time_t now = time(NULL);
  DataOutputBase* dst = CreateDataOutput(dstformat, dstfilename);
  if (dstfilename)
    free(dstfilename);
  if (dstformat) {
    free(dstformat);
    if (!dst) {
      fprintf(stderr, "Invalid output format\n");
      return 1;
    }
  }

  //do LDAP stuff
  const char* ldaperrmsg;
  if ((ldaperrmsg  = ldap->Open()) != NULL) {
    fprintf(stderr, "Error opening LDAP connection: %s\n", ldaperrmsg);
    return 2;
  }
  std::string searchfilter = "(&(objectCategory=computer)(objectClass=computer)";
  if (groups.size() > 0) {
    std::vector<std::string>::iterator group;
    searchfilter += "(|";
    for (group = groups.begin(); group != groups.end(); group++) {
      searchfilter += "(memberOf:1.2.840.113556.1.4.1941:=";  //LDAP_MATCHING_RULE_IN_CHAIN
      searchfilter += *group;
      searchfilter += ")";
    }
    searchfilter += ")";
  }
  if (createdlastdays > 0) {
    searchfilter += "(whenCreated>=";
    searchfilter += time2timestamp(now - createdlastdays * 24 * 60 * 60);
    searchfilter += ")";
  }
  if (loggedonlastdays != 0) {
    if (loggedonlastdays > 0) {
      searchfilter += "(&(!(lastLogonTimestamp=9223372036854775807))(!(lastLogonTimestamp=0))(lastLogonTimestamp>=";
      searchfilter += time2timevalue(now - loggedonlastdays * 24 * 60 * 60);
      searchfilter += "))";
    } else {
      searchfilter += "(|(lastLogonTimestamp=9223372036854775807)(lastLogonTimestamp=0) (&(!(lastLogonTimestamp=*))(whenCreated<=";
      searchfilter += time2timestamp(now - -loggedonlastdays * 24 * 60 * 60);
      searchfilter += ")) (lastLogonTimestamp<=";
      searchfilter += time2timevalue(now - -loggedonlastdays * 24 * 60 * 60);
      searchfilter += "))";
    }
  }
  if (enabledonly) {
    searchfilter += "(!(userAccountControl:1.2.840.113556.1.4.803:=2))";  //LDAP_MATCHING_RULE_BIT_AND, UF_ACCOUNTDISABLED
  }
  if (disabledonly) {
    searchfilter += "(userAccountControl:1.2.840.113556.1.4.803:=2)";  //LDAP_MATCHING_RULE_BIT_AND, UF_ACCOUNTDISABLED
  }
  if (!ldapfilter.empty()) {
    searchfilter += "(";
    searchfilter += ldapfilter;
    searchfilter += ")";
  }
  searchfilter += ")";
//printf("%s\n", searchfilter.c_str());
  if ((result = ldap->Search(searchfilter.c_str())) == NULL) {
    fprintf(stderr, "LDAP search failed\n");
  } else {
    dst->AddColumn("Name", 24);
    dst->AddColumn("OperatingSystem", 32);
    dst->AddColumn("ServicePack", 15);
    dst->AddColumn("HotFix", 6);
    dst->AddColumn("Version", 10);
    dst->AddColumn("Description", 24);
    dst->AddColumn("Location", 24);
    dst->AddColumn("Active", 23);
    dst->AddColumn("LastLogon", 19);
    dst->AddColumn("Created", 19);
    dst->AddColumn("LastChanged", 19);
    dst->AddColumn("ADName", 128);
    dst->AddColumn("ManagedBy", 128);
    dst->AddColumn("Role", 4);
    if (result->Rewind()) {
      char* s;
      do {
        time_t expiration = timevalue2time(result->GetAttributeInt("accountExpires"));
        long long accountctrl = result->GetAttributeInt("userAccountControl");
        char activedata[32];
        strcpy(activedata, (accountctrl & UF_ACCOUNTDISABLE ? "Disabled" : "Enabled"));
        if (expiration && expiration <= now)
          strcat(activedata, "+Expired");
        if (accountctrl & UF_LOCKOUT /*|| result->GetAttributeInt("lockoutTime") != 0*/)
          strcat(activedata, "+Locked");
        dst->AddRow();
        dst->AddData(s = result->GetAttribute("cn"));
        free(s);
        dst->AddData(s = result->GetAttribute("operatingSystem"));
        free(s);
        dst->AddData(s = result->GetAttribute("operatingSystemServicePack"));
        free(s);
        dst->AddData(s = result->GetAttribute("operatingSystemHotfix"));
        free(s);
        dst->AddData(s = result->GetAttribute("operatingSystemVersion"));
        free(s);
        dst->AddData(replace_line_breaks_with_spaces(s = result->GetAttribute("description")));
        free(s);
        dst->AddData(s = result->GetAttribute("location"));
        free(s);
        dst->AddData(activedata);
        dst->AddData(format_timevalue(result->GetAttributeInt("lastLogonTimestamp")));
        dst->AddData(format_timestamp(s = result->GetAttribute("whenCreated")));
        free(s);
        dst->AddData(format_timestamp(s = result->GetAttribute("whenChanged")));
        free(s);
        dst->AddData((s = result->GetDN()));
        free(s);
        dst->AddData(s = result->GetAttribute("managedBy"));
        free(s);
        dst->AddData(result->GetAttributeInt("userAccountControl") & 0x00002000 || result->GetAttributeInt("primaryGroupID") == 516 ? "DC" : "");
      } while (result->Next());
    }
    delete result;
  }

  //clean up
  ldap->Close();
  delete ldap;
  delete dst;
  return 0;
}
