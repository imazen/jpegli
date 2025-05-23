# Copyright (c) the JPEG XL Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if((SANITIZER STREQUAL "asan") OR (SANITIZER STREQUAL "msan"))
  set(BUILD_TESTING OFF)
endif()

# Highway
set(HWY_SYSTEM_GTEST ON CACHE INTERNAL "")
set(HWY_FORCE_STATIC_LIBS ON CACHE INTERNAL "")
set(HWY_ENABLE_CONTRIB OFF CACHE INTERNAL "")
set(HWY_ENABLE_EXAMPLES OFF CACHE INTERNAL "")
set(HWY_ENABLE_TESTS OFF CACHE INTERNAL "")
if((SANITIZER STREQUAL "asan") OR (SANITIZER STREQUAL "msan"))
  set(HWY_ENABLE_INSTALL OFF CACHE INTERNAL "")
endif()
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/highway/CMakeLists.txt" AND
    NOT JPEGXL_FORCE_SYSTEM_HWY)
  add_subdirectory(highway)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/highway/LICENSE"
                 ${PROJECT_BINARY_DIR}/LICENSE.highway COPYONLY)
else()
  find_package(HWY 1.0.7)
  if (NOT HWY_FOUND)
    message(FATAL_ERROR
        "Highway library (hwy) not found. Install libhwy-dev or download it "
        "to third_party/highway from https://github.com/google/highway . "
        "Highway is required to build JPEG XL. You can run "
        "${PROJECT_SOURCE_DIR}/deps.sh to download this dependency.")
  endif()
  if(JPEGXL_DEP_LICENSE_DIR)
    configure_file("${JPEGXL_DEP_LICENSE_DIR}/libhwy-dev/copyright"
                   ${PROJECT_BINARY_DIR}/LICENSE.highway COPYONLY)
  endif()  # JPEGXL_DEP_LICENSE_DIR
endif()

# *cms
if (JPEGXL_ENABLE_SKCMS)
  if( NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/skcms/skcms.h" )
    message(FATAL_ERROR "Please run ${PROJECT_SOURCE_DIR}/deps.sh to fetch the "
            "build dependencies.")
  endif()
  include(skcms.cmake)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/skcms/LICENSE"
                 ${PROJECT_BINARY_DIR}/LICENSE.skcms COPYONLY)
endif ()
if (NOT JPEGXL_ENABLE_SKCMS)
  if( NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lcms/.git" OR JPEGXL_FORCE_SYSTEM_LCMS2 )
    find_package(LCMS2 2.12)
    if ( NOT LCMS2_FOUND )
      message(FATAL_ERROR "Please install lcms2 or run git submodule update --init")
    endif ()
  else()
    include(lcms2.cmake)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/lcms/COPYING"
                   ${PROJECT_BINARY_DIR}/LICENSE.lcms COPYONLY)
  endif()
endif()

# libpng
if (JPEGXL_BUNDLE_LIBPNG AND EMSCRIPTEN)
  if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libpng/CMakeLists.txt")
  message(FATAL_ERROR "Please run ${PROJECT_SOURCE_DIR}/deps.sh to fetch the "
          "build dependencies.")
  endif()
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/libpng/scripts/pnglibconf.h.prebuilt" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/libpng")
  file(RENAME "${CMAKE_CURRENT_SOURCE_DIR}/libpng/pnglibconf.h.prebuilt" "${CMAKE_CURRENT_SOURCE_DIR}/libpng/pnglibconf.h")
  set(ZLIB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/zlib/")
  set(ZLIB_LIBRARIES "")
  set(PNG_FOUND YES PARENT_SCOPE)
  set(PNG_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libpng/" PARENT_SCOPE)
  set(PNG_LIBRARIES "" PARENT_SCOPE)
elseif (JPEGXL_BUNDLE_LIBPNG)
  if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libpng/CMakeLists.txt")
    message(FATAL_ERROR "Please run ${PROJECT_SOURCE_DIR}/deps.sh to fetch the "
            "build dependencies.")
  endif()
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/libpng/scripts/pnglibconf.h.prebuilt" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/libpng")
  file(RENAME "${CMAKE_CURRENT_SOURCE_DIR}/libpng/pnglibconf.h.prebuilt" "${CMAKE_CURRENT_SOURCE_DIR}/libpng/pnglibconf.h")
  add_subdirectory(zlib)
  set(PNG_STATIC ON CACHE BOOL "")
  set(PNG_EXECUTABLES OFF CACHE BOOL "")
  set(PNG_BUILD_ZLIB ON CACHE BOOL "")
  set(PNG_TESTS OFF CACHE BOOL "")
  set(SKIP_INSTALL_ALL ON CACHE BOOL "")
  set(ZLIB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/zlib/")
  set(ZLIB_LIBRARIES zlibstatic)
  add_subdirectory(libpng EXCLUDE_FROM_ALL)
  set(PNG_FOUND YES PARENT_SCOPE)
  set(PNG_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libpng/" PARENT_SCOPE)
  set(PNG_LIBRARIES png_static PARENT_SCOPE)
  set_property(TARGET png_static PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_property(TARGET zlibstatic PROPERTY POSITION_INDEPENDENT_CODE ON)
  if(JPEGXL_DEP_LICENSE_DIR)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libpng/LICENSE"
                   ${PROJECT_BINARY_DIR}/LICENSE.libpng COPYONLY)
  endif()
else()
  find_package(PNG)
  if(PNG_FOUND AND JPEGXL_DEP_LICENSE_DIR)
    configure_file("${JPEGXL_DEP_LICENSE_DIR}/zlib1g-dev/copyright"
                   ${PROJECT_BINARY_DIR}/LICENSE.zlib COPYONLY)
    configure_file("${JPEGXL_DEP_LICENSE_DIR}/libpng-dev/copyright"
                   ${PROJECT_BINARY_DIR}/LICENSE.libpng COPYONLY)
  endif()  # JPEGXL_DEP_LICENSE_DIR
endif()

# sjpeg
if (JPEGXL_ENABLE_SJPEG)
  if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sjpeg/CMakeLists.txt")
    message(FATAL_ERROR "Please run ${PROJECT_SOURCE_DIR}/deps.sh to fetch the "
            "build dependencies.")
  endif()
  include(sjpeg.cmake)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sjpeg/COPYING"
                 ${PROJECT_BINARY_DIR}/LICENSE.sjpeg COPYONLY)
endif ()
