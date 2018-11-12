#
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
AC_INIT(generate_si_cache, 1.0, ssunil@tataelxsi.co.in)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(src/si_cache_app.cpp)

AC_CHECK_HEADERS([libxml/encoding.h libxml/xmlwriter.h],
	[gsc_found_libxml_headers=yes; break;])

AC_SEARCH_LIBS(
    [xmlNewTextWriterFilename], [xml2], [gsc_found_libxml_lib=yes], [gsc_found_libxml_lib=no], [])

AS_IF([test "x$gsc_found_libxml_headers" != "xyes" -o "x$gsc_found_libxml_lib" != "xyes"],
	[AC_MSG_ERROR([LibXml2 is not installed in this machine. Please install and try again.])])

AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_CXX

AC_OUTPUT(Makefile src/Makefile)
