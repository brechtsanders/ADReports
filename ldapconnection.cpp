#ifndef USE_WINLDAP
#define LDAP_DEPRECATED 1       //needed for ldap_init (TO DO: replace with ldap_initialize)
#endif
#include "ldapconnection.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef USE_WINLDAP
#include <winber.h>
#else
#include <unistd.h>             //needed for getlogin_r
#ifndef _WIN32
#include <bits/local_lim.h>     //needed for LOGIN_NAME_MAX
#endif
#include <lber.h>
#endif

#ifdef USE_WINLDAP
#define WINAPIASCII(fn) fn##A
#define LDAPRESULTTYPE ULONG
#else
#define WINAPIASCII(fn) fn
#define LDAPRESULTTYPE int
#endif

//the following defines influence how this module is built
//  UNICODE         build with UNICODE support
//  NO_PAGED_LDAP   build without LDAP paged results

#define LDAP_PAGE_SIZE 250

#ifndef UNICODE

#define RESULT_STRDUP(s) strdup((char*)s)

#else

#define RESULT_STRDUP(s) strdupWtoUTF8((wchar_t*)s)

char* strdupWtoUTF8 (const wchar_t* data)
{
  int n;
  char* result;
  if (!data)
    return NULL;
  if ((n = WideCharToMultiByte(CP_UTF8, 0, data, -1, NULL, 0, NULL, NULL)) > 0) {
    result = (char*)malloc(n);
    WideCharToMultiByte(CP_UTF8, 0, data, -1, result, n, NULL, NULL);
  } else {
    result = strdup("[CONVERSION ERROR]");
  }
  return result;
}

wchar_t* strdupUTF8toW (const char* data)
{
  int n;
  wchar_t* result;
  if (!data)
    return NULL;
  if ((n = MultiByteToWideChar(CP_UTF8, 0, data, -1, NULL, 0)) > 0) {
    result = (wchar_t*)malloc(n * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, data, -1, result, n);
  } else {
    result = wcsdup(L"[CONVERSION ERROR]");
  }
  return result;
}

#endif

char* get_current_login ()
{
#ifdef _WIN32
  //get currently logged on user
  TCHAR* ldapaccountname = NULL;
  DWORD ldapaccountnamelen = 0;
  if (GetUserName(ldapaccountname, &ldapaccountnamelen) ||
      GetLastError() != ERROR_INSUFFICIENT_BUFFER || ldapaccountnamelen == 0 ||
      (ldapaccountname = (TCHAR*)malloc(sizeof(TCHAR) * ldapaccountnamelen)) == NULL ||
      !GetUserName(ldapaccountname, &ldapaccountnamelen) ||
      !ldapaccountname || !*ldapaccountname) {
    return NULL;
  } else {
#ifndef UNICODE
    return (char*)ldapaccountname;
#else
    char* result = strdupWtoUTF8(ldapaccountname);
    free(ldapaccountname);
    return result;
#endif
  }
#else
  char* result;
  if ((result = (char*)malloc(LOGIN_NAME_MAX + 1)) != NULL) {
    if (getlogin_r(result, LOGIN_NAME_MAX) != 0)
      free(result);
    else
      result = (char*)realloc(result, strlen(result) + 1);
  }
  return result;
#endif
}

LDAPConnection::LDAPConnection ()
: ldaphost (NULL), ldapuser(NULL), ldappass(NULL), ldapsearchbase(NULL), ldapconnection(NULL)/*, ldapresponse(NULL)*/
{
}

LDAPConnection::~LDAPConnection ()
{
  Close();
  free(ldaphost);
  free(ldapuser);
  free(ldappass);
  free(ldapsearchbase);
}

bool LDAPConnection::ProcessCommandLineParameter (int argc, char** argv, int& index)
{
  const char* param;
  param = NULL;
  switch (tolower(argv[index][1])) {
    case 'h' :
      if (argv[index][2])
        param = argv[index] + 2;
      else if (index + 1 < argc && argv[index + 1])
        param = argv[++index];
      if (!param)
        return false;
      ldaphost = strdup(param);
      return true;
    case 'u' :
      if (argv[index][2])
        param = argv[index] + 2;
      else if (index + 1 < argc && argv[index + 1])
        param = argv[++index];
      if (!param)
        return false;
      ldapuser = strdup(param);
      return true;
    case 'p' :
      if (argv[index][2])
        param = argv[index] + 2;
      else if (index + 1 < argc && argv[index + 1])
        param = argv[++index];
      if (!param)
        return false;
      ldappass = strdup(param);
      return true;
    case 'b' :
      if (argv[index][2])
        param = argv[index] + 2;
      else if (index + 1 < argc && argv[index + 1])
        param = argv[++index];
      if (!param)
        return false;
      ldapsearchbase = strdup(param);
      return true;
    default :
      return false;
  }
}

const char* LDAPConnection::Open ()
{
  LDAPRESULTTYPE msgid;
  //connect to default LDAP server
  if ((ldapconnection = WINAPIASCII(ldap_init)(ldaphost, 0)) == NULL)
    return "Error opening LDAP connection";
  //set options on connection blocks to specify LDAP version 3
  //ldapconnection->ld_lberoptions = 0;
  LDAPRESULTTYPE version = LDAP_VERSION3;
  ldap_set_option(ldapconnection, LDAP_OPT_PROTOCOL_VERSION, &version);
  //bind using specified or current credentials
  if (ldapuser || ldappass)
    msgid = WINAPIASCII(ldap_simple_bind_s)(ldapconnection, (char*)(ldapuser ? ldapuser : ""), (char*)(ldappass ? ldappass : "")); //to do: replace with ldap_sasl_bind_s
  else
    msgid = WINAPIASCII(ldap_bind_s)(ldapconnection, NULL, NULL, LDAP_AUTH_NEGOTIATE);
  if (msgid != LDAP_SUCCESS) {
    const char* errmsg = WINAPIASCII(ldap_err2string)(msgid);
    Close();
    return (errmsg ? errmsg : "Error binding LDAP");
  }
  //get default search base if not supplied
  if (!ldapsearchbase) {
    LDAPMessage* response;
    static char* baseattrs[] = {(char*)"defaultNamingContext", NULL};
    if (WINAPIASCII(ldap_search_ext_s)(ldapconnection, NULL, LDAP_SCOPE_BASE, NULL, baseattrs, 0, NULL, NULL, NULL, 0, &response) == LDAP_SUCCESS) {
      char** values;
      if ((values = WINAPIASCII(ldap_get_values)(ldapconnection, response, (char*)"defaultNamingContext")) != NULL) {
        if (*values)
          ldapsearchbase = strdup(*values);
        WINAPIASCII(ldap_value_free)(values);
      }
    }
    if (!ldapsearchbase) {
      Close();
      return "Error getting default naming context from LDAP";
    }
  }
  return NULL;
}

void LDAPConnection::Close ()
{
  if (ldapconnection) {
    ldap_unbind(ldapconnection);
    ldapconnection = NULL;
  }
}

class LDAPResponse* LDAPConnection::Search (const char* searchfilter, const char** attrs)
{
  LDAPMessage* ldapresponse = NULL;
  //start LDAP search
#ifndef NO_PAGED_LDAP
  LDAPSearch* pagedsearch = WINAPIASCII(ldap_search_init_page)(ldapconnection, ldapsearchbase, LDAP_SCOPE_SUBTREE, (char*)searchfilter, (char**)attrs, 0, NULL, NULL, 0, 0, NULL);
  if (pagedsearch)
    return new LDAPResponse(this, ldapresponse, pagedsearch);
#else
  LDAPRESULTTYPE status = WINAPIASCII(ldap_search_ext_s)(ldapconnection, ldapsearchbase, LDAP_SCOPE_SUBTREE, (char*)searchfilter, (char**)attrs, 0, NULL, NULL, NULL, 0, &ldapresponse);
  if (status == LDAP_SUCCESS || status == LDAP_PARTIAL_RESULTS)
    return new LDAPResponse(this, ldapresponse);
  fprintf(stderr, "LDAP error: %s\n", WINAPIASCII(ldap_err2string)(status));
  fprintf(stderr, "Filter: %s\n", searchfilter);
#endif
  if (ldapresponse)
    ldap_msgfree(ldapresponse);
  return NULL;
}

void LDAPConnection::SetSearchBase (const char* searchbase)
{
  if (ldapsearchbase)
    free(ldapsearchbase);
  ldapsearchbase = strdup(searchbase);
}

////////////////////////////////////////////////////////////////////////

#ifndef NO_PAGED_LDAP
LDAPResponse::LDAPResponse (LDAPConnection* ldap, LDAPMessage* response, LDAPSearch* pagedsearch)
: parent(ldap), ldapresponse(response), pagedldapsearch(pagedsearch)
{
  if (ldap_get_next_page_s(parent->ldapconnection, pagedldapsearch, NULL, LDAP_PAGE_SIZE, NULL, &ldapresponse) != LDAP_SUCCESS) {
    currentldapresponse = NULL;
    return;
  }
  currentldapresponse = ldap_first_entry(parent->ldapconnection, ldapresponse);
}
#else
LDAPResponse::LDAPResponse (LDAPConnection* ldap, LDAPMessage* response)
: parent(ldap), ldapresponse(response)
{
  currentldapresponse = ldap_first_entry(parent->ldapconnection, ldapresponse);
}
#endif

LDAPResponse::~LDAPResponse ()
{
  if (ldapresponse)
    ldap_msgfree(ldapresponse);
}

bool LDAPResponse::Rewind ()
{
  if (!ldapresponse)
    return false;
  return ((currentldapresponse = ldap_first_entry(parent->ldapconnection, ldapresponse)) != NULL);
}

bool LDAPResponse::Next ()
{
  if (!currentldapresponse)
    return NULL;
  currentldapresponse = ldap_next_entry(parent->ldapconnection, currentldapresponse);
#ifndef NO_PAGED_LDAP
  if (!currentldapresponse) {
    if (ldap_get_next_page_s(parent->ldapconnection, pagedldapsearch, NULL, 250, NULL, &ldapresponse) == LDAP_SUCCESS)
      currentldapresponse = ldap_first_entry(parent->ldapconnection, ldapresponse);
  }
#endif
  return (currentldapresponse != NULL);
}

int LDAPResponse::IterateAttributes (attribute_callback_fn callback, void* userdata)
{
  int count = 0;
  bool notdone = true;
  if (!ldapresponse)
    return 0;
  LDAPMessage* entry = ldap_first_entry(parent->ldapconnection, ldapresponse);
  while (notdone && entry) {
    BerElement* pos;
    char* attributename = WINAPIASCII(ldap_first_attribute)(parent->ldapconnection, entry, &pos);
    while (notdone && attributename) {
      notdone = (*callback)(attributename, userdata);
      WINAPIASCII(ldap_memfree)(attributename);
      attributename = WINAPIASCII(ldap_next_attribute)(parent->ldapconnection, entry, pos);
    }
    if (pos)
      ber_free(pos, 0);
    entry = ldap_next_entry(parent->ldapconnection, ldapresponse);
  }
  return count;
}

char* LDAPResponse::GetDN ()
{
  if (!currentldapresponse)
    return NULL;
#ifdef USE_WINLDAP
  char* result;
  PTCHAR dn;
  if ((dn = ldap_get_dn(parent->ldapconnection, currentldapresponse)) == NULL)
    return NULL;
  result = RESULT_STRDUP(dn);
  ldap_memfree(dn);
  return result;
#else
  char* result;
  char* dn;
  if ((dn = WINAPIASCII(ldap_get_dn)(parent->ldapconnection, currentldapresponse)) == NULL)
    return NULL;
  result = strdup(dn);
  ldap_memfree(dn);
  return result;
#endif
}

char* LDAPResponse::GetUFNDN ()
{
  if (!currentldapresponse)
    return NULL;
#ifdef USE_WINLDAP
  PTCHAR dn;
  PTCHAR ufn;
  char* result;
  if ((dn = ldap_get_dn(parent->ldapconnection, currentldapresponse)) == NULL)
    return NULL;
  if ((ufn = ldap_dn2ufn(dn)) == NULL)
    ufn = dn;
  else
    ldap_memfree(dn);
  result = RESULT_STRDUP((char*)ufn);
  ldap_memfree(ufn);
  return result;
#else
  char* dn;
  char* ufn;
  char* result;
  if ((dn = WINAPIASCII(ldap_get_dn)(parent->ldapconnection, currentldapresponse)) == NULL)
    return NULL;
  if ((ufn = WINAPIASCII(ldap_dn2ufn)(dn)) == NULL)
    ufn = dn;
  else
    ldap_memfree(dn);
  result = strdup(ufn);
  ldap_memfree(ufn);
  return result;
#endif
}

/*
void LDAPResponse::ShowAttribute (const char* name, const char* separator)
{
  TCHAR** ldapvalues;
  TCHAR** p;
  if (!currentldapresponse)
    return;
  if ((ldapvalues = ldap_get_values(parent->ldapconnection, currentldapresponse, name)) != NULL) {
    p = ldapvalues;
    while (*p) {
      if (p != ldapvalues)
        printf("%s", separator);
      printf("%s", *p);
      p++;
    }
    ldap_value_free(ldapvalues);
  }
}
*/

char* LDAPResponse::GetAttribute (const char* name, const char* separator)
{
  char* result = NULL;
  if (!currentldapresponse)
    return NULL;
#if !defined(UNICODE) || !defined(USE_WINLDAP)
  char** ldapvalues;
  char** p;
  if ((ldapvalues = WINAPIASCII(ldap_get_values)(parent->ldapconnection, currentldapresponse, name)) != NULL) {
    p = ldapvalues;
    while (*p) {
      if (p != ldapvalues) {
        result = (char*)realloc(result, (result ? strlen(result) : 0) + strlen(separator) + strlen(*p) + 1);
        strcat(result, separator);
      } else {
        result = (char*)malloc(strlen(*p) + 1);
        *result = 0;
      }
      strcat(result, *p);
      p++;
    }
    ldap_value_free(ldapvalues);
  }
#else
  TCHAR** ldapvalues;
  TCHAR** p;
  char* s;
  WCHAR* namew = strdupUTF8toW(name);
  if ((ldapvalues = ldap_get_values(parent->ldapconnection, currentldapresponse, namew)) != NULL) {
    p = ldapvalues;
    while (*p) {
      s = strdupWtoUTF8(*p);
      if (p != ldapvalues) {
        result = (char*)realloc(result, (result ? strlen(result) : 0) + strlen(separator) + strlen(s) + 1);
        strcat(result, separator);
      } else {
        result = (char*)malloc(strlen(s) + 1);
        *result = 0;
      }
      strcat(result, s);
      free(s);
      p++;
    }
    ldap_value_free(ldapvalues);
  }
  free(namew);
#endif
  return result;
}

long long LDAPResponse::GetAttributeInt (const char* name)
{
  long long result = 0;
  char** ldapvalue;
  if (!currentldapresponse)
    return 0;
  if ((ldapvalue = WINAPIASCII(ldap_get_values)(parent->ldapconnection, currentldapresponse, (char*)name)) != NULL) {
    result = atoll(*ldapvalue);
    WINAPIASCII(ldap_value_free)(ldapvalue);
  }
  return result;
}

void* LDAPResponse::GetAttributeBin (const char* name, size_t* len)
{
  void* result = NULL;
  struct berval** ldapvalues;
  struct berval** p;
  if (!currentldapresponse)
    return NULL;
  if ((ldapvalues = WINAPIASCII(ldap_get_values_len)(parent->ldapconnection, currentldapresponse, (char*)name)) != NULL) {
    p = ldapvalues;
    while (*p) {
      result = malloc((*p)->bv_len);
      memcpy(result, (*p)->bv_val, (*p)->bv_len);
      if (len)
        *len = (*p)->bv_len;
      break;//abort after first binary object if there are multiple
      //p++;
    }
    ldap_value_free_len(ldapvalues);
  }
  return result;
}
