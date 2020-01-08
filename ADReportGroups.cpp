#include <stdio.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#include <lmaccess.h>
#else
#endif
#include <string>
#include <vector>
#include "adreports_version.h"
#include "ldapconnection.h"
#include "dataoutput.h"
#include "adformats.h"

void show_help()
{
  printf(
    "ADReportGroups v" ADREPORTS_VERSION_STRING " - generate Active Directory group reports\n" \
    "Credits: " ADREPORTS_CREDITS "\n" \
    "Usage:  ADReportGroups " LDAP_COMMAND_LINE_PARAMETERS " [-f format] [-o file] [-l user] [-g] [-d] [-c days] [-q ldapfilter]\n" \
    "Parameters:\n" \
    LDAP_COMMAND_LINE_HELP \
    "  -f format      \tOutput format (" DATAOUTPUT_FORMAT_HELP_LIST ")\n" \
    "  -o file        \tOutput file (default is standard output)\n" \
    "  -l user        \tInclude all groups this user is a member of (may be\n" \
    "                 \tspecified multiple times, default is all groups)\n" \
    "  -g             \tSecurity groups only\n" \
    "  -d             \tDistribution groups only\n" \
    "  -c days        \tShow only groups created in the last number of days\n" \
    "  -q ldapfilter  \tLDAP filter\n" \
    "\n"
  );
}

int main (int argc, char *argv[])
{
  char* dstformat = NULL;
  char* dstfilename = NULL;
  std::vector<std::string> users;
  bool securitygroupsonly = false;
  bool distributiongroupsonly = false;
  int createdlastdays = -1;
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
          case 'l' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              return false;
            users.push_back(param);
            break;
          case 'g' :
            securitygroupsonly = true;
            break;
          case 'd' :
            distributiongroupsonly = true;
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
  std::string searchfilter = "(&(objectCategory=group)(objectClass=group)";
  if (securitygroupsonly) {
    searchfilter += "(groupType:1.2.840.113556.1.4.803:=2147483648)"; //LDAP_MATCHING_RULE_BIT_AND, ADS_GROUP_TYPE_SECURITY_ENABLED (0x80000000)
  }
  if (distributiongroupsonly) {
    searchfilter += "(!(groupType:1.2.840.113556.1.4.803:=2147483648))"; //LDAP_MATCHING_RULE_BIT_AND, ADS_GROUP_TYPE_SECURITY_ENABLED (0x80000000)
  }
  //TO DO: when not selecting either only security groups or only distribution groups only security groups are listed
  //if (!securitygroupsonly && !distributiongroupsonly) {
  //  searchfilter += "(|((groupType:1.2.840.113556.1.4.803:=2147483648))(!(groupType:1.2.840.113556.1.4.803:=2147483648)))"; //LDAP_MATCHING_RULE_BIT_AND, ADS_GROUP_TYPE_SECURITY_ENABLED (0x80000000)
  //}
  if (users.size() > 0) {
    std::vector<std::string>::iterator user;
    searchfilter += "(|";
    for (user = users.begin(); user != users.end(); user++) {
      searchfilter += "(member:1.2.840.113556.1.4.1941:=";  //LDAP_MATCHING_RULE_IN_CHAIN
      searchfilter += *user;
      searchfilter += ")";
    }
    searchfilter += ")";
  }
  if (createdlastdays > 0) {
    searchfilter += "(whenCreated>=";
    searchfilter += time2timestamp(now - createdlastdays * 24 * 60 * 60);
    searchfilter += ")";
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
    dst->AddColumn("Name", 40);
    dst->AddColumn("Created", 19);
    dst->AddColumn("LastChanged", 19);
    dst->AddColumn("GroupScope", 12);
    dst->AddColumn("GroupType", 12);
    dst->AddColumn("ADName", 128);
    dst->AddColumn("Description", 48);
    dst->AddColumn("PrimaryEmail", 48);
    if (result->Rewind()) {
      char* s;
      std::string type;
      do {
        dst->AddRow();
        dst->AddData(s = result->GetAttribute("cn"));
        free(s);
        dst->AddData(format_timestamp(s = result->GetAttribute("whenCreated")));
        free(s);
        dst->AddData(format_timestamp(s = result->GetAttribute("whenChanged")));
        free(s);
        type = "";
        if (result->GetAttributeInt("groupType") & 0x00000002)
          type += " global";
        if (result->GetAttributeInt("groupType") & 0x00000004)
          type += " domain local";
        if (result->GetAttributeInt("groupType") & 0x00000008)
          type += " universal";
        if (type.length() > 0)
          type.erase(0, 1);
        dst->AddData(type.c_str());
        type = (result->GetAttributeInt("groupType") & 0x80000000 ? "security" : "distribution");
        if (result->GetAttributeInt("groupType") & 0x00000001)
          type += " (system)";
        if (result->GetAttributeInt("groupType") & 0x00000010)
          type += " APP_BASIC";
        if (result->GetAttributeInt("groupType") & 0x00000020)
          type += " APP_QUERY";
        dst->AddData(type.c_str());
        dst->AddData((s = result->GetDN()));
        free(s);
        dst->AddData(replace_line_breaks_with_spaces(s = result->GetAttribute("description")));
        free(s);
        dst->AddData(get_primary_smtp_address(s = result->GetAttribute("proxyAddresses")));
        free(s);
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
