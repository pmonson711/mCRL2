
// Copyright: see the accompanying file COPYING.
#define BOOST_UNITS_STRINGIZE_IMPL(x) #x
#define BOOST_UNITS_STRINGIZE(x) BOOST_UNITS_STRINGIZE_IMPL(x)

#define BOOST_UNITS_HEADER BOOST_UNITS_STRINGIZE(BOOST_UNITS_HEADER_NAME)

#include BOOST_UNITS_HEADER

int main() {}