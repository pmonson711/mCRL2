// Author(s): Thomas Neele
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCRL2_SMT_TRANSLATE_EXPRESSION_H
#define MCRL2_SMT_TRANSLATE_EXPRESSION_H

#include "mcrl2/core/identifier_string.h"
#include "mcrl2/data/data_equation.h"
#include "mcrl2/data/data_expression.h"
#include "mcrl2/data/function_symbol.h"
#include "mcrl2/smt2/native_translation.h"
#include "mcrl2/smt2/translate_sort.h"
#include "mcrl2/smt2/utilities.h"

namespace mcrl2
{
namespace smt
{

template <typename T, typename OutputStream>
void translate_data_expression(const T& x, OutputStream& o, const native_translations& nt);

namespace detail
{

template <template <class> class Traverser, class OutputStream>
struct translate_data_expression_traverser: public Traverser<translate_data_expression_traverser<Traverser, OutputStream> >
{
  typedef Traverser<translate_data_expression_traverser<Traverser, OutputStream> > super;
  using super::enter;
  using super::leave;
  using super::apply;

  OutputStream& out;
  const native_translations& m_native;

  translate_data_expression_traverser(OutputStream& out_, const native_translations& nt)
    : out(out_)
    , m_native(nt)
  {}


  /**
   * \brief Declare variables to be used in binder such as exists or forall and print the declaration to out
   * \return An expression that constrains the domains of Pos and Nat variables
   */
  data::data_expression declare_variables_binder(const data::variable_list& vars)
  {
    data::data_expression result = data::sort_bool::true_();
    out << "(";
    for(const data::variable& var: vars)
    {
      out << "(" << translate_identifier(var.name()) << " ";
      translate_sort_expression(var.sort(), out, m_native);
      out << ")";
      if(var.sort() == data::sort_pos::pos())
      {
        result = data::lazy::and_(result, greater_equal(var, data::sort_pos::c1()));
      }
      else if(var.sort() == data::sort_nat::nat())
      {
        result = data::lazy::and_(result, greater_equal(var, data::sort_nat::c0()));
      }
    }
    out << ")";
    return result;
  }

  void apply(const data::application& v)
  {
    auto find_result = m_native.find_native_translation(v);
    if(find_result != m_native.expressions.end())
    {
      out << find_result->second(v) << " ";
    }
    else
    {
      out << "(";
      super::apply(v);
      out << ") ";
    }
  }

  void apply(const data::function_symbol& v)
  {
    auto find_result = m_native.symbols.find(v);
    if(find_result != m_native.symbols.end())
    {
      out << find_result->second << " ";
    }
    else
    {
      out << translate_identifier(v.name()) << " ";
    }
  }

  void apply(const data::variable& v)
  {
    out << translate_identifier(v.name()) << " ";
  }

  void apply(const data::forall& v)
  {
    out << "(forall ";
    data::data_expression vars_conditions = declare_variables_binder(v.variables());
    out << " ";
    super::apply(data::lazy::implies(vars_conditions, v.body()));
    out << ")";
  }

  void apply(const data::exists& v)
  {
    out << "(exists ";
    data::data_expression vars_conditions = declare_variables_binder(v.variables());
    out << " ";
    super::apply(data::lazy::and_(vars_conditions, v.body()));
    out << ")";
  }
};

template <template <class> class Traverser, class OutputStream>
translate_data_expression_traverser<Traverser, OutputStream>
make_translate_data_expression_traverser(OutputStream& out, const native_translations& nt)
{
  return translate_data_expression_traverser<Traverser, OutputStream>(out, nt);
}

} // namespace detail

template <typename T, typename OutputStream>
void translate_data_expression(const T& x, OutputStream& o, const native_translations& nt)
{
  detail::make_translate_data_expression_traverser<data::data_expression_traverser>(o, nt).apply(x);
}

template <typename T, typename OutputStream>
void translate_assertion(const T& x, OutputStream& o, const native_translations& nt)
{
  o << "(assert ";
  translate_data_expression(x, o, nt);
  o << ")\n";
}

template <typename Container, typename OutputStream>
void translate_variable_declaration(const Container& vars, OutputStream& o, const native_translations& nt)
{
  data::data_expression vars_conditions = data::sort_bool::true_();
  for(const data::variable& v: vars)
  {
    o << "(declare-fun " << translate_identifier(v.name()) << " (";
    data::sort_expression_list domain = data::is_function_sort(v.sort()) ?
      atermpp::down_cast<data::function_sort>(v.sort()).domain() : data::sort_expression_list();
    for(const data::sort_expression& s: domain)
    {
      translate_sort_expression(s, o, nt);
      o << " ";
      if(s == data::sort_pos::pos())
      {
        vars_conditions = data::lazy::and_(vars_conditions, greater_equal(v, data::sort_pos::c1()));
      }
      else if(s == data::sort_nat::nat())
      {
        vars_conditions = data::lazy::and_(vars_conditions, greater_equal(v, data::sort_nat::c0()));
      }
    }
    o << ") ";
    translate_sort_expression(v.sort().target_sort(), o, nt);
    o << ")\n";
  }
  translate_assertion(vars_conditions, o, nt);
}

} // namespace smt
} // namespace mcrl2

#endif