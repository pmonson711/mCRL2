// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/rewriters/data_rewriter.h
/// \brief add your file description here.

#ifndef MCRL2_PBES_REWRITERS_DATA_REWRITER_H
#define MCRL2_PBES_REWRITERS_DATA_REWRITER_H

#include "mcrl2/data/substitutions/no_substitution.h"
#include "mcrl2/pbes/builder.h"

namespace mcrl2 {

namespace pbes_system {

namespace detail {

template <typename DataRewriter, typename SubstitutionFunction>
const data::data_expression data_rewrite(const data::data_expression& x, const DataRewriter& R, SubstitutionFunction& sigma)
{
  mCRL2log(log::debug2) << "data_rewrite " << x << sigma << " -> " << R(x, sigma) << std::endl;
  return R(x, sigma);
}

template <typename DataRewriter, typename SubstitutionFunction>
void data_rewrite(data::data_expression& result, const data::data_expression& x, const DataRewriter& R, SubstitutionFunction& sigma)
{
  mCRL2log(log::debug2) << "data_rewrite " << x << sigma << " -> " << R(x, sigma) << std::endl;
  R(result, x, sigma);
}

template <typename DataRewriter>
const data::data_expression data_rewrite(const data::data_expression& x, const DataRewriter& R, data::no_substitution&)
{
  mCRL2log(log::debug2) << "data_rewrite " << x << "[]" << " -> " << R(x) << std::endl;
  return R(x);
}

template <typename DataRewriter>
void data_rewrite(data::data_expression& result, const data::data_expression& x, const DataRewriter& R, data::no_substitution&)
{
  mCRL2log(log::debug2) << "data_rewrite " << x << "[]" << " -> " << R(x) << std::endl;
  result = R(x);
}

template <template <class> class Builder, class Derived, class DataRewriter, class SubstitutionFunction = data::no_substitution>
struct add_data_rewriter: public Builder<Derived>
{
  typedef Builder<Derived> super;
  using super::apply;

  const DataRewriter& R;
  SubstitutionFunction& sigma;

  add_data_rewriter(const DataRewriter& R_, SubstitutionFunction& sigma_)
    : R(R_), sigma(sigma_)
  {}

  template <class T>
  void apply(T& result, const data::data_expression& x)
  {
    data_rewrite(atermpp::reference_cast<data::data_expression>(result), x, R, sigma);
    // data_rewrite(reinterpret_cast<data::data_expression&>(result), x, R, sigma);
    // result = data_rewrite(x, R, sigma);
  }

  template <class T>
  void apply(T& result, const propositional_variable_instantiation& x)
  {
    /* std::vector<data::data_expression> d;
    for (const data::data_expression& e: x.parameters())
    {
      data::data_expression r;
      r = data_rewrite(e, R, sigma);
      d.push_back(r);
    } 
    make_propositional_variable_instantiation(result, x.name(), data::data_expression_list(d.begin(), d.end())); */
    make_propositional_variable_instantiation(
              result, 
              x.name(), 
              [this, &x](data::data_expression_list& r) -> void
                  { atermpp::make_term_list<data::data_expression>(
                               r, 
                               x.parameters().begin(),
                               x.parameters().end(),
                               [this](data::data_expression& r1, const data::data_expression& arg) -> void
                                     { data_rewrite(r1, arg, R, sigma); } ) ;
                  }); 
  } 
};

template <typename Derived, typename DataRewriter, typename SubstitutionFunction>
struct data_rewriter_builder: public add_data_rewriter<pbes_system::pbes_expression_builder, Derived, DataRewriter, SubstitutionFunction>
{
  typedef add_data_rewriter<pbes_system::pbes_expression_builder, Derived, DataRewriter, SubstitutionFunction> super;
  using super::enter;
  using super::leave;

  data_rewriter_builder(const DataRewriter& R, SubstitutionFunction& sigma)
    : super(R, sigma)
  {}
};

template <template <class, class, class> class Builder, class DataRewriter, class SubstitutionFunction>
struct apply_rewriter_builder: public Builder<apply_rewriter_builder<Builder, DataRewriter, SubstitutionFunction>, DataRewriter, SubstitutionFunction>
{
  typedef Builder<apply_rewriter_builder<Builder, DataRewriter, SubstitutionFunction>, DataRewriter, SubstitutionFunction> super;
  using super::enter;
  using super::leave;

  apply_rewriter_builder(const DataRewriter& datar, SubstitutionFunction& sigma)
    : super(datar, sigma)
  {}
};

template <template <class, class, class> class Builder, class DataRewriter, class SubstitutionFunction>
apply_rewriter_builder<Builder, DataRewriter, SubstitutionFunction>
make_apply_rewriter_builder(const DataRewriter& datar, SubstitutionFunction& sigma)
{
  return apply_rewriter_builder<Builder, DataRewriter, SubstitutionFunction>(datar, sigma);
}

} // namespace detail

/// \brief A rewriter that applies a data rewriter to data expressions in a term.
template <typename DataRewriter>
struct data_rewriter
{
  typedef pbes_expression term_type;
  typedef data::variable variable_type;

  const DataRewriter& R;

  data_rewriter(const DataRewriter& R_)
    : R(R_)
  {}

  pbes_expression operator()(const pbes_expression& x) const
  {
    data::no_substitution sigma;
    pbes_expression result;
    detail::make_apply_rewriter_builder<detail::data_rewriter_builder>(R, sigma).apply(result, x);
    return result;
  }

  template <typename SubstitutionFunction>
  pbes_expression operator()(const pbes_expression& x, SubstitutionFunction& sigma) const
  {
    pbes_expression result;
    detail::make_apply_rewriter_builder<detail::data_rewriter_builder>(R, sigma).apply(result, x);
    return result;
  }
};

} // namespace pbes_system

} // namespace mcrl2

#endif // MCRL2_PBES_REWRITERS_DATA_REWRITER_H
