#ifndef INCLUDED_LDAPCONNECTION_H
#define INCLUDED_LDAPCONNECTION_H

#if USE_WINLDAP
#include <windows.h>
#include <winldap.h>
#else
#include <ldap.h>
#ifndef NO_PAGED_LDAP
#define NO_PAGED_LDAP
#endif
#endif

#define LDAP_COMMAND_LINE_PARAMETERS "[-h host[:port]] [-u user -p password] [-b searchbase] [-l user]"
#define LDAP_COMMAND_LINE_HELP \
    "  -h host[:port] \tLDAP host (and optionally port) to connect to (default\n" \
    "                 \tis the default Active Directory LDAP server)\n" \
    "  -S             \tUse SSL encryption for the LDAP communication\n" \
    "  -u user        \tLDAP authentication user login (default is to\n" \
    "                 \tauthenticate as the currently logged on domain user)\n" \
    "  -p password    \tLDAP authentication password (default is to\n" \
    "                 \tauthenticate as the currently logged on domain user)\n" \
    "  -b searchbase  \tLDAP search base (default is current domain's default\n" \
    "                 \tnaming context)\n"

char* get_current_login ();

class LDAPConnection
{
 protected:
  friend class LDAPResponse;
  char* ldaphost;
  int ldapsecure;
  char* ldapuser;
  char* ldappass;
  char* ldapsearchbase;
  LDAP* ldapconnection;
 public:
  LDAPConnection ();
  ~LDAPConnection ();
  bool ProcessCommandLineParameter (int argc, char** argv, int& index);
  const char* Open ();
  void Close();
  //std::string GetValue (const char* name, const char* line_join = "\n");
  //inline std::string GetValue (std::string name, const char* line_join = "\n") { return GetValue(name.c_str(), line_join); }
  //inline std::string GetValue (std::string name, std::string line_join) { return GetValue(name.c_str(), line_join.c_str()); }
  class LDAPResponse* Search (const char* searchfilter, const char** attrs = NULL);
  void SetSearchBase (const char* searchbase);
  const char* GetSearchBase () { return ldapsearchbase; }
};

class LDAPResponse
{
  friend class LDAPConnection;
 public:
  typedef bool (*attribute_callback_fn) (const char* attributename, void* userdata);
 protected:
  LDAPConnection* parent;
  LDAPMessage* ldapresponse;
  LDAPMessage* currentldapresponse;
#ifndef NO_PAGED_LDAP
  LDAPSearch* pagedldapsearch;
  LDAPResponse (LDAPConnection* ldap, LDAPMessage* response, LDAPSearch* pagedsearch);
#else
  LDAPResponse (LDAPConnection* ldap, LDAPMessage* response);
#endif
 public:
  ~LDAPResponse ();
  bool Rewind ();
  bool Next ();
  int IterateAttributes (attribute_callback_fn callback, void* userdata = NULL);
  char* GetDN ();
  char* GetUFNDN ();
  //void ShowAttribute (const char* name, const char* separator = "\n");
  char* GetAttribute (const char* name, const char* separator = "\n");
  long long GetAttributeInt (const char* name);
  void* GetAttributeBin (const char* name, size_t* len);
};

#endif //INCLUDED_LDAPCONNECTION_H
