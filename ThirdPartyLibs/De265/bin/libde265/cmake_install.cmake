# Install script for directory: C:/Esenthel/ThirdPartyLibs/De265/libde265

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/libde265")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/Debug/libde265.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/Release/libde265.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/MinSizeRel/libde265.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/RelWithDebInfo/libde265.lib")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/Debug/libde265.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/Release/libde265.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/MinSizeRel/libde265.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/RelWithDebInfo/libde265.dll")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libde265" TYPE FILE FILES
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/acceleration.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/alloc_pool.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/bitstream.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/cabac.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/configparam.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/contextmodel.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/de265.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/deblock.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/decctx.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/dpb.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/en265.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/fallback-dct.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/fallback-motion.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/fallback.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/image-io.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/image.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/intrapred.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/md5.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/motion.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/nal-parser.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/nal.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/pps.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/quality.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/refpic.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/sao.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/scan.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/sei.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/slice.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/sps.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/threads.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/transform.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/util.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/visualize.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/vps.h"
    "C:/Esenthel/ThirdPartyLibs/De265/libde265/vui.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libde265/libde265Config.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libde265/libde265Config.cmake"
         "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/CMakeFiles/Export/lib/cmake/libde265/libde265Config.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libde265/libde265Config-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libde265/libde265Config.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libde265" TYPE FILE FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/CMakeFiles/Export/lib/cmake/libde265/libde265Config.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libde265" TYPE FILE FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/CMakeFiles/Export/lib/cmake/libde265/libde265Config-debug.cmake")
  endif()
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libde265" TYPE FILE FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/CMakeFiles/Export/lib/cmake/libde265/libde265Config-minsizerel.cmake")
  endif()
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libde265" TYPE FILE FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/CMakeFiles/Export/lib/cmake/libde265/libde265Config-relwithdebinfo.cmake")
  endif()
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libde265" TYPE FILE FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/CMakeFiles/Export/lib/cmake/libde265/libde265Config-release.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libde265" TYPE FILE FILES "C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/libde265ConfigVersion.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/encoder/cmake_install.cmake")
  include("C:/Esenthel/ThirdPartyLibs/De265/bin/libde265/x86/cmake_install.cmake")

endif()

