#include <stdio.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#include <lmaccess.h>
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
    "ADGetPhoto v" ADREPORTS_VERSION_STRING " - extract Active Directory photos\n" \
    "Credits: " ADREPORTS_CREDITS "\n" \
    "Usage:  ADGetPhoto " LDAP_COMMAND_LINE_PARAMETERS " [-c] [-l login] [-q ldapfilter]\n" \
    "Parameters:\n" \
    LDAP_COMMAND_LINE_HELP \
    "  -c             \tExtract photo of current user (default = all users)\n" \
    "  -l login       \tLogin name of user to extract photo of\n" \
    "  -q ldapfilter  \tLDAP filter\n" \
    "\n"
  );
}

int main (int argc, char *argv[])
{
  std::string login;
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
          case 'c' :
            login = get_current_login ();
            break;
          case 'l' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              return false;
            login = strdup(param);
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

  //do LDAP stuff
  const char* ldaperrmsg;
  if ((ldaperrmsg  = ldap->Open()) != NULL) {
    fprintf(stderr, "Error opening LDAP connection: %s\n", ldaperrmsg);
    return 2;
  }
  std::string searchfilter = "(&(objectCategory=person)(objectClass=user)";
  if (login.size() > 0) {
    searchfilter += "(sAMAccountName=";
    searchfilter += login;
    searchfilter += ")";
  }
  if (!ldapfilter.empty()) {
    searchfilter += "(";
    searchfilter += ldapfilter;
    searchfilter += ")";
  }
  searchfilter += ")";
/////printf("%s\n", searchfilter.c_str());
  if ((result = ldap->Search(searchfilter.c_str())) == NULL) {
    fprintf(stderr, "LDAP search failed\n");
  } else {
    if (result->Rewind()) {
      std::string filename;
      char* s;
      size_t len;
      FILE* f;
      do {
        s = result->GetAttribute("sAMAccountName");
        printf("%s\n", s);
        filename = std::string(s) + ".jpg";
        free(s);
        if ((s = (char*)result->GetAttributeBin("thumbnailPhoto", &len)) != NULL) {
          if ((f = fopen(filename.c_str(), "wb")) != NULL) {
            fwrite(s, len, 1, f);
            fclose(f);
          }
          free(s);
        }
      } while (result->Next());
    }
    delete result;
  }

  //clean up
  ldap->Close();
  delete ldap;
  return 0;
}
