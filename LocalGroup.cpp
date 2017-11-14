#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <lmaccess.h>

int main (int argc, char *argv[])
{
  LPWSTR hostname;
  NET_API_STATUS status;
  DWORD n;
  DWORD total;
  ULONG_PTR resumehandle;
  DWORD i;
/*
  //list all local groups
  LOCALGROUP_INFO_0* groupinfo;
  resumehandle = 0;
  do {
    status = NetLocalGroupEnum(hostname, 0, (BYTE**)&groupinfo, 8192, &n, &total, &resumehandle);
    if ((status != ERROR_SUCCESS && status != ERROR_MORE_DATA) || groupinfo == NULL) {
      return 1;
    }
    for (i = 0; i < n; i++) {
      wprintf(L"%s\n", groupinfo[i].lgrpi0_name);
    }
  } while (status == ERROR_MORE_DATA);
*/
  //hostname = NULL;
  //hostname = L"SBEWEPAS013";
  //output header
  wprintf(L"Host,AdministratorsMember\n");
  //iterate systems supplied on command line (or local system if none supplied)
  char** currenthost = &argv[1];
  do {
    //convert hostname to wide string
    if (*currenthost) {
      int len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, *currenthost, -1, NULL, 0);
      hostname = (WCHAR*)malloc(len * sizeof(WCHAR));
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, *currenthost, -1, hostname, len);
    } else {
      hostname = NULL;
    }
    //list members of local Administrators group
    LOCALGROUP_MEMBERS_INFO_3* memberinfo;
    resumehandle = 0;
    do {
      status = NetLocalGroupGetMembers(hostname, L"Administrators", 3, (BYTE**)&memberinfo, 8192, &n, &total, &resumehandle);
      if ((status != ERROR_SUCCESS && status != ERROR_MORE_DATA) || memberinfo == NULL) {
        return 2;
      }
      for (i = 0; i < n; i++) {
        wprintf(L"%s,%s\n", (hostname ? hostname : L""), memberinfo[i].lgrmi3_domainandname);
      }
    } while (status == ERROR_MORE_DATA);
    if (*currenthost) {
      free(hostname);
      currenthost++;
    }
  } while (*currenthost);
  return 0;
}

//http://www.codeproject.com/KB/applications/collectsid.aspx
