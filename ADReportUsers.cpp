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
    "ADReportGroups v" ADREPORTS_VERSION_STRING " - generate Active Directory user reports\n" \
    "Credits: " ADREPORTS_CREDITS "\n" \
    "Usage:  ADReportUsers " LDAP_COMMAND_LINE_PARAMETERS " [-f format] [-o file] [-g group] [-c days] [-x days] [-l days] [-n days] [-e] [-d] [-t] [-q ldapfilter]\n" \
    "Parameters:\n" \
    LDAP_COMMAND_LINE_HELP \
    "  -f format      \tOutput format (" DATAOUTPUT_FORMAT_HELP_LIST ")\n" \
    "  -o file        \tOutput file (default is standard output)\n" \
    "  -g group       \tInclude all members of this group (may be specified\n" \
    "                 \tmultiple times, default is all users)\n" \
    "  -c days        \tShow only users created in the last number of days\n" \
    "  -x days        \tShow only users expiring in the next number of days\n" \
    "                 \t(may be negative to specify a date in the past)\n" \
    "  -l days        \tShow only users logged on in the last number of days\n" \
    "                 \t(not logged on if negative)\n" \
    "  -n days        \tShow only users that have changed their password in\n" \
    "                 \tthe last number of days (or not if negative)\n" \
    "  -e             \tShow only enabled users\n" \
    "  -d             \tShow only disabled users\n" \
    "  -t             \tInclude trust accounts\n" \
    "  -q ldapfilter  \tAdditional LDAP filter\n" \
    "\n"
  );
}

int main (int argc, char *argv[])
{
  char* dstformat = NULL;
  char* dstfilename = NULL;
  std::vector<std::string> groups;
  int createdlastdays = -1;
  int expiresnextdays = 0x7FFF;
  int loggedonlastdays = 0;
  int changedpasswordlastdays = 0;
  bool enabledonly = false;
  bool disabledonly = false;
  bool includetrustaccounts = false;
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
            if (!param) {
              paramerror = true;
              break;
	    }
            dstformat = strdup(param);
            break;
          case 'o' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param) {
              paramerror = true;
              break;
	    }
            dstfilename = strdup(param);
            break;
          case 'g' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param) {
              paramerror = true;
              break;
	    }
            groups.push_back(param);
            break;
          case 'c' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param) {
              paramerror = true;
              break;
	    }
            createdlastdays = atoi(param);
            break;
          case 'x' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param) {
              paramerror = true;
              break;
	    }
            expiresnextdays = atoi(param);
            break;
          case 'l' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param) {
              paramerror = true;
              break;
	    }
            loggedonlastdays = atoi(param);
            break;
          case 'n' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param) {
              paramerror = true;
              break;
	    }
            changedpasswordlastdays = atoi(param);
            break;
          case 'e' :
            enabledonly = true;
            break;
          case 'd' :
            disabledonly = true;
            break;
          case 't' :
            includetrustaccounts = true;
            break;
          case 'q' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param) {
              paramerror = true;
              break;
	    }
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
  //SetConsoleOutputCP(CP_UTF8);//set console to UTF-8
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
/*
  //if ((result = ldap->Search("distinguishedName=CN=Domain Admins,CN=Users,DC=ISIS,DC=LOCAL")) == NULL) {
  //if ((result = ldap->Search("memberOf:1.2.840.113556.1.4.1941:=CN=Denied RODC Password Replication Group,CN=Users,DC=ISIS,DC=LOCAL")) == NULL) {
  //if ((result = ldap->Search("(&(userAccountControl:1.2.840.113556.1.4.803:=2)(memberOf:1.2.840.113556.1.4.1941:=CN=Denied RODC Password Replication Group,CN=Users,DC=ISIS,DC=LOCAL))")) == NULL) {
  //if ((result = ldap->Search("memberOf:1.2.840.113556.1.4.1941:=CN=Domain Admins,CN=Users,DC=ISIS,DC=LOCAL")) == NULL) {
  //                              (!(userAccountControl:1.2.840.113556.1.4.803:=2))
*/
  std::string searchfilter = "(&(objectCategory=person)(objectClass=user)";
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
  if (expiresnextdays != 0x7FFF) {
    searchfilter += "(&(!(accountExpires=9223372036854775807))(!(accountExpires=0))(accountExpires<=";
    searchfilter += time2timevalue(now + expiresnextdays * 24 * 60 * 60);
    searchfilter += "))";
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
  if (changedpasswordlastdays != 0) {
    if (changedpasswordlastdays > 0) {
      searchfilter += "(&(!(pwdLastSet=9223372036854775807))(!(pwdLastSet=0))(pwdLastSet>=";
      searchfilter += time2timevalue(now - changedpasswordlastdays * 24 * 60 * 60);
      searchfilter += "))";
    } else {
      searchfilter += "(|(pwdLastSet=9223372036854775807)(pwdLastSet=0) (&(!(pwdLastSet=*))(whenCreated<=";
      searchfilter += time2timestamp(now - -changedpasswordlastdays * 24 * 60 * 60);
      searchfilter += ")) (pwdLastSet<=";
      searchfilter += time2timevalue(now - -changedpasswordlastdays * 24 * 60 * 60);
      searchfilter += "))";
    }
  }
  if (enabledonly) {
    searchfilter += "(!(userAccountControl:1.2.840.113556.1.4.803:=2))";  //LDAP_MATCHING_RULE_BIT_AND, UF_ACCOUNTDISABLED
  }
  if (disabledonly) {
    searchfilter += "(userAccountControl:1.2.840.113556.1.4.803:=2)";  //LDAP_MATCHING_RULE_BIT_AND, UF_ACCOUNTDISABLED
  }
  if (!includetrustaccounts) {
    searchfilter += "(!(userAccountControl:1.2.840.113556.1.4.803:=2048))";  //LDAP_MATCHING_RULE_BIT_AND, INTERDOMAIN_TRUST_ACCOUNT
    //searchfilter += "(!(userAccountControl:1.2.840.113556.1.4.803:=4096))";  //LDAP_MATCHING_RULE_BIT_AND, WORKSTATION_TRUST_ACCOUNT
    //searchfilter += "(!(userAccountControl:1.2.840.113556.1.4.803:=8192))";  //LDAP_MATCHING_RULE_BIT_AND, SERVER_TRUST_ACCOUNT
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
    dst->AddColumn("Name", 40);
    dst->AddColumn("Expires", 19);
    dst->AddColumn("LastLogon", 19);
    dst->AddColumn("LastSetPassword", 19);
    dst->AddColumn("LastBadPassword", 19);
    dst->AddColumn("LockOut", 19);
    dst->AddColumn("Active", 23);
    dst->AddColumn("Created", 19);
    dst->AddColumn("LastChanged", 19);
    dst->AddColumn("Login", 18);
    dst->AddColumn("Logons", 8);
    dst->AddColumn("ADName", 128);
    dst->AddColumn("FirstName", 20);
    dst->AddColumn("LastName", 20);
    dst->AddColumn("Email", 48);
    dst->AddColumn("HomeDirectory", 48);
    dst->AddColumn("Description", 48);
    dst->AddColumn("Notes", 48);
    dst->AddColumn("PasswordExpires", 15);
    dst->AddColumn("FunctionTitle", 24);
    dst->AddColumn("Department", 24);
    dst->AddColumn("Company", 24);
    dst->AddColumn("Phone", 15);
    dst->AddColumn("Mobile", 15);
    dst->AddColumn("Fax", 15);
    dst->AddColumn("Address", 48);
    dst->AddColumn("Postal", 6);
    dst->AddColumn("Location", 20);
    dst->AddColumn("Country", 14);
    dst->AddColumn("EmployeeID", 10);
    dst->AddColumn("EmployeeType", 12);
    dst->AddColumn("Manager", 128);
    dst->AddColumn("PrimaryEmail", 48);
    dst->AddColumn("MailboxCreated", 19);
    dst->AddColumn("MailboxSoftDeleted", 19);
    dst->AddColumn("ExchangeHomeServer", 48);
    dst->AddColumn("ExchangeArchiveDatabase", 48);
    dst->AddColumn("VoiceEnabled", 12);
    dst->AddColumn("VoiceAddress", 48);
    dst->AddColumn("VoiceURI", 20);
    dst->AddColumn("VoiceExtension", 14);
    dst->AddColumn("VoiceOptionFlags", 24);
    if (result->Rewind()) {
      char* s;
      do {
        dst->AddRow();
        dst->AddData(s = result->GetAttribute("cn"));
        free(s);
        dst->AddData(format_time(timevalue2time(result->GetAttributeInt("accountExpires"))));
        dst->AddData(format_timevalue(result->GetAttributeInt("lastLogonTimestamp")));
        dst->AddData(format_timevalue(result->GetAttributeInt("pwdLastSet")));
        dst->AddData(format_timevalue(result->GetAttributeInt("badPasswordTime")));
        dst->AddData(format_timevalue(result->GetAttributeInt("lockoutTime")));
        {
          time_t expiration = timevalue2time(result->GetAttributeInt("accountExpires"));
          long long accountctrl = result->GetAttributeInt("userAccountControl");
          char activedata[32];
          strcpy(activedata, (accountctrl & UF_ACCOUNTDISABLE ? "Disabled" : "Enabled"));
          if (expiration && expiration <= now)
            strcat(activedata, "+Expired");
          if (accountctrl & UF_LOCKOUT /*|| result->GetAttributeInt("lockoutTime") != 0*/)
            strcat(activedata, "+Locked");
          dst->AddData(activedata);
        }
        dst->AddData(format_timestamp(s = result->GetAttribute("whenCreated")));
        free(s);
        dst->AddData(format_timestamp(s = result->GetAttribute("whenChanged")));
        free(s);
        dst->AddData((s = result->GetAttribute("sAMAccountName")));
        free(s);
        dst->AddData((int)result->GetAttributeInt("logonCount"));
        //dst->AddData((s = result->GetAttribute("distinguishedName")));
        dst->AddData((s = result->GetDN()));
        free(s);
        dst->AddData((s = result->GetAttribute("givenName")));
        free(s);
        dst->AddData((s = result->GetAttribute("sn")));
        free(s);
        dst->AddData((s = result->GetAttribute("mail")));
        free(s);
        dst->AddData((s = result->GetAttribute("homeDirectory")));
        free(s);
        dst->AddData(replace_line_breaks_with_spaces(s = result->GetAttribute("description")));
        free(s);
        dst->AddData(replace_line_breaks_with_spaces(s = result->GetAttribute("info")));
        free(s);
        dst->AddData(result->GetAttributeInt("userAccountControl") & UF_DONT_EXPIRE_PASSWD ? "Never" : "Yes");
        dst->AddData((s = result->GetAttribute("title")));
        free(s);
        dst->AddData((s = result->GetAttribute("department")));
        free(s);
        dst->AddData((s = result->GetAttribute("company")));
        free(s);
        dst->AddData((s = result->GetAttribute("telephoneNumber")));
        free(s);
        dst->AddData((s = result->GetAttribute("mobile")));
        free(s);
        dst->AddData((s = result->GetAttribute("facsimileTelephoneNumber")));
        free(s);
        dst->AddData((s = result->GetAttribute("streetAddress")));
        free(s);
        dst->AddData((s = result->GetAttribute("postalCode")));
        free(s);
        dst->AddData((s = result->GetAttribute("l")));
        free(s);
        dst->AddData((s = result->GetAttribute("co")));
        free(s);
        dst->AddData((s = result->GetAttribute("employeeID")));
        free(s);
        dst->AddData((s = result->GetAttribute("employeeType")));
        free(s);
        dst->AddData((s = result->GetAttribute("manager")));
        free(s);
        dst->AddData(get_primary_smtp_address(s = result->GetAttribute("proxyAddresses")));
        free(s);
        dst->AddData(format_time(timeint2time(result->GetAttributeInt("msExchWhenMailboxCreated"))));
        dst->AddData(format_time(timeint2time(result->GetAttributeInt("msExchWhenSoftDeletedTime"))));
        dst->AddData((s = result->GetAttribute("msExchHomeServerName")));
        free(s);
        dst->AddData((s = result->GetAttribute("msExchArchiveDatabaseLink")));
        free(s);
        s = result->GetAttribute("msRTCSIP-UserEnabled");
        dst->AddData(s && strcmp(s, "TRUE") == 0 ? "Yes" : (s && strcmp(s, "FALSE") == 0 ? "No" : s));
        free(s);
        dst->AddData((s = result->GetAttribute("msRTCSIP-PrimaryUserAddress")));
        free(s);
        dst->AddData((s = result->GetAttribute("msRTCSIP-Line")));
        free(s);
        dst->AddData((s = result->GetAttribute("msRTCSIP-UserExtension")));
        free(s);
        {
          int optionflags = result->GetAttributeInt("msRTCSIP-OptionFlags");
          char fieldresult[100];
#define VOICE_OPTIONFLAG_IM                                                   0x0001
#define VOICE_OPTIONFLAG_REMOTE_CALL_CONTROL                                  0x0010
#define VOICE_OPTIONFLAG_ALLOW_ORGANIZE_MEETING_WITH_ANONYMOUS_PARTICIPANTS   0x0040
#define VOICE_OPTIONFLAG_UC                                                   0x0080
#define VOICE_OPTIONFLAG_ENABLED_FOR_ENHANCED_PRESENCE                        0x0100
#define VOICE_OPTIONFLAG_REMOTE_CALL_CONTROL_DUAL_MODE                        0x0200
          *fieldresult = 0;
          if (optionflags & VOICE_OPTIONFLAG_IM) {
            if (*fieldresult)
              strcat(fieldresult, ",");
            strcat(fieldresult, "IM");
          }
          if (optionflags & VOICE_OPTIONFLAG_UC) {
            if (*fieldresult)
              strcat(fieldresult, ",");
            strcat(fieldresult, "UC");
          }
          if (optionflags & VOICE_OPTIONFLAG_ENABLED_FOR_ENHANCED_PRESENCE) {
            if (*fieldresult)
              strcat(fieldresult, ",");
            strcat(fieldresult, "EnhancedPresence");
          }
          if (optionflags & VOICE_OPTIONFLAG_ALLOW_ORGANIZE_MEETING_WITH_ANONYMOUS_PARTICIPANTS) {
            if (*fieldresult)
              strcat(fieldresult, ",");
            strcat(fieldresult, "MeetingWithAnonymousParticipants");
          }
          if (optionflags & VOICE_OPTIONFLAG_REMOTE_CALL_CONTROL) {
            if (*fieldresult)
              strcat(fieldresult, ",");
            strcat(fieldresult, "RemoteCallControl");
          }
          if (optionflags & VOICE_OPTIONFLAG_REMOTE_CALL_CONTROL_DUAL_MODE) {
            if (*fieldresult)
              strcat(fieldresult, ",");
            strcat(fieldresult, "RemoteCallControlDualMode");
          }
          dst->AddData(fieldresult);
        }
      } while (result->Next());
    }
    delete result;
  }
/*
Lockout-Time Attribute
This attribute value is only reset when the account is logged onto successfully. This means that this value may be non zero, yet the account is not locked out.
To accurately determine if the account is locked out, you must add the Lockout-Duration to this time and compare the result to the current time, accounting for local time zones and daylight savings time.
*/
/*/
  if ((result = ldap->Search("CN=Builtin")) == NULL) {
    printf("Error\n");
  } else {
    char* s = result->GetAttribute("lockoutDuration");
    free(s);
    delete result;
  }
/*/

  //clean up
  ldap->Close();
  delete ldap;
  delete dst;
  return 0;
}

/*
Domain Admins
Enterprise Admins
Schema Admins

/g "CN=Domain Admins,CN=Users,DC=ISIS,DC=LOCAL" /g "CN=Enterprise Admins,CN=Users,DC=ISIS,DC=LOCAL" /g "CN=Schema Admins,CN=Users,DC=ISIS,DC=LOCAL"
*/







/*
char* join_fields (CSV_FILE* csv_file, DataFields* data)
{
  int i;
  char* result = NULL;
  if (!data)
    return NULL;
  for (i = 0; i < data->FieldCount(); i++) {
    char* val = strdup((*data)[i] ? (*data)[i] : "");
    char* p;
    //convert CR+LF to single linefeed
    p = val;
    while ((p = strstr(p, "\r\n")) != NULL) {
      strcpy(p, p + 1);
      p++;
    }
    //convert linefeed to carriage return (if needed)
    p = val;
    if (strcmp(csv_file->newline, "\n") == 0)
      while ((p = strchr(p, '\n')) != NULL)
        *p++ = '\r';
    //quote data ?
    if (strchr(val, csv_file->quote) || strchr(val, csv_file->separator) ||
        strchr(val, '\r') || strchr(val, '\n') || strchr(val, '\t')) {
      //double all quotes
      int l;
      p = val;
      while ((p = strchr(p, csv_file->quote)) != NULL) {
        char* new_val = (char*)malloc(strlen(val) + 2);
        l = (p - val) + 1;
        memcpy(new_val, val, l);
        strcpy(new_val + l, p);
        free(val);
        val = new_val;
        p = val + l + 1;
      }
      //quotes before and after
      l = strlen(val);
      p = (char*)malloc(l + 3);
      p[0] = csv_file->quote;
      memcpy(p + 1, val, l);
      p[l + 1] = csv_file->quote;
      p[l + 2] = 0;
      free(val);
      val = p;
    }
    char* new_result = (char*)malloc((result ? strlen(result) + 1 : 0) + strlen(val) + 1);
    p = new_result;
    if (result) {
      p = stpcpy(p, result);
      *p++ = csv_file->separator;
    }
    p = stpcpy(p, val);
    *p = 0;
    free(result);
    result = new_result;
  }
  return result;
}
*/
