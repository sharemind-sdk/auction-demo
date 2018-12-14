#
# Copyright (C) Cybernetica
#
# Research/Commercial License Usage
# Licensees holding a valid Research License or Commercial License
# for the Software may use this file according to the written
# agreement between you and Cybernetica.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3.0 as published by the Free Software
# Foundation and appearing in the file LICENSE.GPL included in the
# packaging of this file.  Please review the following information to
# ensure the GNU General Public License version 3.0 requirements will be
# met: http://www.gnu.org/copyleft/gpl-3.0.html.
#
# For further information, please contact us at sharemind@cyber.ee.
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
PROJECT(AUCTION)

INCLUDE("${CMAKE_CURRENT_SOURCE_DIR}/config.local" OPTIONAL)
INCLUDE("${CMAKE_CURRENT_BINARY_DIR}/config.local" OPTIONAL)

ADD_COMPILE_OPTIONS(
  "-std=c++14" "-Wall" "-Wextra" "-O2"
  "$<$<NOT:$<STREQUAL:$<CONFIGURATION>,Release>>:-ggdb>"
  "$<$<NOT:$<STREQUAL:$<CONFIGURATION>,Release>>:-fno-omit-frame-pointer>"
  "$<$<STREQUAL:$<CONFIGURATION>,Release>:-DNDEBUG>"
  "$<$<STREQUAL:$<CONFIGURATION>,Release>:-fomit-frame-pointer>"
)

SET(Boost_USE_STATIC_RUNTIME OFF)
FIND_PACKAGE(Boost COMPONENTS program_options system REQUIRED)
FIND_PACKAGE(LogHard REQUIRED)
FIND_PACKAGE(SharemindLibController REQUIRED)

INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
    ${LogHard_INCLUDE_DIRS}
    ${SharemindLibController_INCLUDE_DIRS}
)

ADD_DEFINITIONS(
    ${LogHard_DEFINITIONS}
    ${SharemindLibController_DEFINITIONS}
    "-D__STDC_LIMIT_MACROS"
)

SET(AUCTION_LIBRARIES
    ${Boost_LIBRARIES}
    ${LogHard_LIBRARIES}
    ${SharemindLibController_LIBRARIES}
)

FILE(GLOB_RECURSE AUCTION_SECREC "${CMAKE_CURRENT_SOURCE_DIR}/secrec/*.sc")

SET(AUCTION_FILES ${AUCTION_SECREC})

ADD_CUSTOM_TARGET("include_Auction_Files_in_IDE"
                  SOURCES ${AUCTION_FILES})

ADD_EXECUTABLE(auction-result src/result.cpp)
TARGET_LINK_LIBRARIES(auction-result ${AUCTION_LIBRARIES})
INSTALL(TARGETS auction-result RUNTIME DESTINATION "bin")

ADD_EXECUTABLE(auction-bid src/bid.cpp)
TARGET_LINK_LIBRARIES(auction-bid ${AUCTION_LIBRARIES})
INSTALL(TARGETS auction-bid RUNTIME DESTINATION "bin")
