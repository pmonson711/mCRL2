//  Copyright 2007 Jeroen van der Wulp. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
/// \file include/tipi/utility/generic_visitor.tcc

#include "tipi/utility/generic_visitor.h"

namespace utility {
  template < typename S, typename R >
  typename abstract_visitor< R >::visitable_type_tree  visitor< S, R >::visitable_types;
}
