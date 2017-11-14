ADReports
=========
Generate reports from Active Directory (AD).

Description
-----------
ADReports provides tools to generate reports from Active Directory (AD) via LDAP.
These reports are useful to manage Active Directory, but also to improve compliancy for audits (e.g. due diligence, Sarbanes–Oxley Act (SOX)).

Goal
----
The library was written with the following goals in mind:
- portable across different platforms (Windows, *nix)
- cross platform (using either native Windows LDAP or OpenLDAP)
- command line to allow easy scheduling of reports

Dependancies
------------
This project has the following depencancies:
- [XLSX I/O](https://brechtsanders.github.io/xlsxio/) (optional)
- [OpenLDAP](http://www.openldap.org/software/download/) (optional on Windows)

Examples
--------

- Create HTML file with report containing all enabled users that have not logged in during the past 45 days
```
ADReportUsers /f HTML /o ADReportUsers_NotRecentlyLoggedIn.html /l -45 /e
```
- Create XML file with report containing all users in the specified OU
```
ADReportUsers /f XML /o ADReportUsers_RecycleBin.html /b "OU=DeleteMe,OU=Company,DC=DOMAIN,DC=LOCAL"
```

License
-------
XLSX I/O is released under the terms of the GNU General Public License, see COPYING.txt.
