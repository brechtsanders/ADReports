#ifndef INCLUDED_ADREPORTS_VERSION_H
#define INCLUDED_ADREPORTS_VERSION_H

/*! \brief version number constants
 * \name   ADREPORTS_VERSION_*
 * \{
 */
/*! \brief major version number */
#define ADREPORTS_VERSION_MAJOR 1
/*! \brief minor version number */
#define ADREPORTS_VERSION_MINOR 4
/*! \brief micro version number */
#define ADREPORTS_VERSION_MICRO 5
/*! @} */

/*! \cond PRIVATE */
#define ADREPORTS_VERSION_STRINGIZE_(major, minor, micro) #major"."#minor"."#micro
#define ADREPORTS_VERSION_STRINGIZE(major, minor, micro) ADREPORTS_VERSION_STRINGIZE_(major, minor, micro)
/*! \endcond */

/*! \brief string with dotted version number \hideinitializer */
#define ADREPORTS_VERSION_STRING ADREPORTS_VERSION_STRINGIZE(ADREPORTS_VERSION_MAJOR, ADREPORTS_VERSION_MINOR, ADREPORTS_VERSION_MICRO)

#endif
