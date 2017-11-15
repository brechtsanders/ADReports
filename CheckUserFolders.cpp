#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <lmaccess.h>
#else
#define UF_ACCOUNTDISABLE 2
#define UF_LOCKOUT 16
#endif
#include <string>
#include <vector>
#include "adreports_version.h"
#include "ldapconnection.h"
#include "dataoutput.h"
#include "adformats.h"

#ifdef __WIN32
  #define PATH_SEPARATOR '\\'
#else
  #define PATH_SEPARATOR '/'
#endif

void show_help()
{
  printf(
    "CheckUserFolders v" ADREPORTS_VERSION_STRING " - generate Active Directory home folder reports\n" \
    "Usage:  CheckUserFolders " LDAP_COMMAND_LINE_PARAMETERS " [-f format] [-o file] [-q ldapfilter] path ...\n" \
    "Parameters:\n" \
    LDAP_COMMAND_LINE_HELP \
    "  -f format      \tOutput format (" DATAOUTPUT_FORMAT_HELP_LIST ")\n" \
    "  -o file        \tOutput file (default is standard output)\n" \
    "  -q ldapfilter  \tLDAP filter\n" \
    "  path           \tOne or more directories to scan\n" \
    "\n"
  );
}

int main (int argc, char *argv[])
{
  char* dstformat = NULL;
  char* dstfilename = NULL;
  std::vector<std::string> paths;
  std::string ldapfilter;
  LDAPConnection* ldap = new LDAPConnection;
  LDAPResponse* result;

  //parse command line parameters
  {
    int i = 0;
    const char* param;
    while (++i < argc) {
      if (!argv[i][0] || (argv[i][0] != '/' && argv[i][0] != '-')) {
        paths.push_back(argv[i]);
      } else if (!ldap->ProcessCommandLineParameter(argc, argv, i)) {
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
            paths.push_back(argv[i]);
            break;
        }
      }
    }
  }
  if (paths.size() == 0) {
    show_help();
    return 0;
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
  ldap->Open();
  std::string searchfilter;
  std::string searchfilterbase = "(&(objectCategory=person)(objectClass=user)";
  if (!ldapfilter.empty()) {
    searchfilterbase += "(";
    searchfilterbase += ldapfilter;
    searchfilterbase += ")";
  }
  //searchfilter += "(sAMAccountName=)";
  //searchfilter += ")";

  std::vector<std::string>::iterator path;
  for (path = paths.begin(); path != paths.end(); path++) {
    DIR* dir;
    struct dirent* entry;
    struct stat status;
    std::string fullname;
    //int pathlen = path->length();
    dst->AddColumn("Folder", 40);
    dst->AddColumn("Login", 18);
    dst->AddColumn("Name", 18);
    dst->AddColumn("Active", 23);
    dst->AddColumn("Expires", 19);
    dst->AddColumn("LastLogon", 19);
    dst->AddColumn("Logons", 8);
    dst->AddColumn("HomeDirectory", 48);
    dst->AddColumn("ADName", 128);
    if ((dir = opendir(path->c_str())) != NULL) {
      while ((entry = readdir(dir)) != NULL) {
        if (memcmp(entry->d_name, ".", 2) != 0 && memcmp(entry->d_name, "..", 3) != 0) {
          fullname = *path + PATH_SEPARATOR + entry->d_name;
          if (stat(fullname.c_str(), &status) == 0) {
            if (S_ISDIR(status.st_mode)) {
              searchfilter = searchfilterbase + "(sAMAccountName=" + entry->d_name + "))";
              //searchfilter = searchfilterbase + ")";
              dst->AddRow();
              dst->AddData(fullname.c_str());
              if ((result = ldap->Search(searchfilter.c_str())) != NULL && result->Rewind()) {
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
                  dst->AddData(s = result->GetAttribute("sAMAccountName"));
                  free(s);
                  dst->AddData(s = result->GetAttribute("cn"));
                  free(s);
                  dst->AddData(activedata);
                  dst->AddData(format_time(expiration));
                  dst->AddData(format_timevalue(result->GetAttributeInt("lastLogonTimestamp")));
                  dst->AddData((int64_t)result->GetAttributeInt("logonCount"));
                  dst->AddData(s = result->GetAttribute("homeDirectory"));
                  free(s);
                  dst->AddData((s = result->GetDN()));
                  free(s);
                } while (result->Next());
              } else {
                int i;
                for (i = 0; i < 8; i++)
                  dst->AddData("");
              }
            }
          }
        }
      }
      closedir(dir);
    }
  }

  //clean up
  ldap->Close();
  delete ldap;
  delete dst;
  return 0;
}
