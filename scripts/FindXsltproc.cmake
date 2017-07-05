# Authors: Frank Stappers 
# Copyright: see the accompanying file COPYING or copy at
# https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Documentation requires xsltproc

find_file(XSLTPROC xsltproc)
if( XSLTPROC )
  message( STATUS "xsltproc found: ${XSLTPROC}" )
endif( XSLTPROC )