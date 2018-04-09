# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

# - Find libsiofia-sip-ua
# This module defines
#  SOFIA_INCLUDE_DIR, where to find sofia.h, etc.
#  SOFIA_LIBRARIES and APRUTIL_LIBRARIES, the libraries needed to use APR.
#  SOFIA_FOUND and APRUTIL_FOUND, If false, do not try to use APR.
# also defined, but not for general use are
#  SOFIA_LIBRARY and APRUTIL_LIBRARY, where to find the APR library.

# APR first.

FIND_PATH(SOFIA_INCLUDE_DIR sip.h
  /usr/local/include/sofia-sip-1/sofia-sip
  /usr/local/include/sofia-sip-1.12/sofia-sip
  /usr/include/sofia-sip-1/sofia-sip
  /usr/include/sofia-sip-1.12/sofia-sip
)

SET(SOFIA_NAMES ${SOFIA_NAMES} sofia-sip-ua)
FIND_LIBRARY(SOFIA_LIBRARY
  NAMES ${SOFIA_NAMES}
  PATHS /usr/local/lib /usr/lib
  )

IF (SOFIA_LIBRARY AND SOFIA_INCLUDE_DIR)
    SET(SOFIA_LIBRARIES ${SOFIA_LIBRARY})
    SET(SOFIA_FOUND "YES")
ELSE (SOFIA_LIBRARY AND SOFIA_INCLUDE_DIR)
  SET(SOFIA_FOUND "NO")
ENDIF (SOFIA_LIBRARY AND SOFIA_INCLUDE_DIR)


IF (SOFIA_FOUND)
   IF (NOT SOFIA_FIND_QUIETLY)
      MESSAGE(STATUS "Found SOFIA: ${SOFIA_LIBRARIES}")
   ENDIF (NOT SOFIA_FIND_QUIETLY)
ELSE (SOFIA_FOUND)
   IF (SOFIA_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find SOFIA library")
   ENDIF (SOFIA_FIND_REQUIRED)
ENDIF (SOFIA_FOUND)

# Deprecated declarations.
SET (NATIVE_SOFIA_INCLUDE_PATH ${SOFIA_INCLUDE_DIR} )
GET_FILENAME_COMPONENT (NATIVE_SOFIA_LIB_PATH ${SOFIA_LIBRARY} PATH)

MARK_AS_ADVANCED(
  SOFIA_LIBRARY
  SOFIA_INCLUDE_DIR
  )