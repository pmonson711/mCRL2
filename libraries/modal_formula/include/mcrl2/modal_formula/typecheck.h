// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/modal_formula/typecheck.h
/// \brief add your file description here.

#ifndef MCRL2_MODAL_FORMULA_TYPECHECK_H
#define MCRL2_MODAL_FORMULA_TYPECHECK_H

#include "mcrl2/data/bag.h"
#include "mcrl2/data/fbag.h"
#include "mcrl2/data/fset.h"
#include "mcrl2/data/int.h"
#include "mcrl2/data/list.h"
#include "mcrl2/data/nat.h"
#include "mcrl2/data/pos.h"
#include "mcrl2/data/real.h"
#include "mcrl2/data/set.h"
#include "mcrl2/data/typecheck.h"
#include "mcrl2/lps/typecheck.h"
#include "mcrl2/modal_formula/builder.h"
#include "mcrl2/modal_formula/is_monotonous.h"
#include "mcrl2/modal_formula/normalize_sorts.h"
#include "mcrl2/modal_formula/state_formula.h"
#include "mcrl2/modal_formula/detail/state_variable_context.h"
#include "mcrl2/modal_formula/detail/typecheck_assignments.h"
#include "mcrl2/process/typecheck.h"
#include "mcrl2/utilities/text_utility.h"

namespace mcrl2
{

namespace action_formulas
{

namespace detail
{

struct typecheck_builder: public action_formula_builder<typecheck_builder>
{
  typedef action_formula_builder<typecheck_builder> super;
  using super::apply;

  data::data_type_checker& m_data_type_checker;
  data::detail::variable_context m_variable_context;
  const process::detail::action_context& m_action_context;

  typecheck_builder(data::data_type_checker& data_typechecker,
                    const data::detail::variable_context& variable_context,
                    const process::detail::action_context& action_context
                   )
    : m_data_type_checker(data_typechecker),
      m_variable_context(variable_context),
      m_action_context(action_context)
  {}

  process::action typecheck_action(const core::identifier_string& name, const data::data_expression_list& parameters)
  {
    return process::typecheck_action(name, parameters, m_data_type_checker, m_variable_context, m_action_context);
  }

  action_formula apply(const data::data_expression& x)
  {
    return m_data_type_checker.typecheck_data_expression(x, data::untyped_sort(), m_variable_context);
  }

  action_formula apply(const action_formulas::at& x)
  {
    data::data_expression new_time = m_data_type_checker.typecheck_data_expression(x.time_stamp(), data::sort_real::real_(), m_variable_context);
    return at((*this).apply(x.operand()), new_time);
  }

  action_formula apply(const process::untyped_multi_action& x)
  {
    // If x has size 1, first try to type check it as a data expression.
    if (x.actions().size() == 1)
    {
      const data::untyped_data_parameter& y = x.actions().front();
      try
      {
        return data::typecheck_untyped_data_parameter(m_data_type_checker, y.name(), y.arguments(), data::sort_bool::bool_(), m_variable_context);
      }
      catch (mcrl2::runtime_error& e)
      {
        // skip
      }
    }
    // Type check it as a multi action
    process::action_list new_arguments;
    for (const data::untyped_data_parameter& a: x.actions())
    {
      new_arguments.push_front(typecheck_action(a.name(), a.arguments()));
    }
    new_arguments = atermpp::reverse(new_arguments);
    return action_formulas::multi_action(new_arguments);
  }

  action_formula apply(const action_formulas::forall& x)
  {
    try
    {
      auto m_variable_context_copy = m_variable_context;
      m_variable_context.add_context_variables(x.variables(), m_data_type_checker);
      action_formula body = (*this).apply(x.body());
      m_variable_context = m_variable_context_copy;
      return forall(x.variables(), body);
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nwhile typechecking " + action_formulas::pp(x));
    }
  }

  action_formula apply(const action_formulas::exists& x)
  {
    try
    {
      auto m_variable_context_copy = m_variable_context;
      m_variable_context.add_context_variables(x.variables(), m_data_type_checker);
      action_formula body = (*this).apply(x.body());
      m_variable_context = m_variable_context_copy;
      return exists(x.variables(), body);
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nwhile typechecking " + action_formulas::pp(x));
    }
  }
};

inline
typecheck_builder make_typecheck_builder(
                    data::data_type_checker& data_typechecker,
                    const data::detail::variable_context& variables,
                    const process::detail::action_context& actions
                   )
{
  return typecheck_builder(data_typechecker, variables, actions);
}

} // namespace detail

template <typename ActionLabelContainer = std::vector<state_formulas::variable>, typename VariableContainer = std::vector<data::variable> >
action_formula typecheck(const action_formula& x,
                         const data::data_specification& dataspec,
                         const VariableContainer& variables,
                         const ActionLabelContainer& actions
                        )
{
  data::data_type_checker data_typechecker(dataspec);
  data::detail::variable_context variable_context;
  variable_context.add_context_variables(variables, data_typechecker);
  process::detail::action_context action_context;
  action_context.add_context_action_labels(actions, data_typechecker);
  return detail::make_typecheck_builder(data_typechecker, variable_context, action_context).apply(action_formulas::normalize_sorts(x, data_typechecker.typechecked_data_specification()));
}

inline
action_formula typecheck(const action_formula& x, const lps::specification& lpsspec)
{
  return typecheck(x, lpsspec.data(), lpsspec.global_variables(), lpsspec.action_labels());
}

} // namespace action_formulas

namespace regular_formulas
{

namespace detail
{

struct typecheck_builder: public regular_formula_builder<typecheck_builder>
{
  typedef regular_formula_builder<typecheck_builder> super;
  using super::apply;

  data::data_type_checker& m_data_type_checker;
  const data::detail::variable_context& m_variable_context;
  const process::detail::action_context& m_action_context;

  typecheck_builder(data::data_type_checker& data_typechecker,
                    const data::detail::variable_context& variables,
                    const process::detail::action_context& actions
                   )
    : m_data_type_checker(data_typechecker),
      m_variable_context(variables),
      m_action_context(actions)
  {}

  data::data_expression make_fbag_union(const data::data_expression& left, const data::data_expression& right)
  {
    const data::sort_expression& s = atermpp::down_cast<data::application>(left).head().sort();
    const data::container_sort& cs = atermpp::down_cast<data::container_sort>(s);
    return data::sort_fbag::union_(cs.element_sort(), left, right);
  }

  data::data_expression make_bag_union(const data::data_expression& left, const data::data_expression& right)
  {
    const data::sort_expression& s = atermpp::down_cast<data::application>(left).head().sort();
    const data::container_sort& cs = atermpp::down_cast<data::container_sort>(s);
    return data::sort_bag::union_(cs.element_sort(), left, right);
  }

  data::data_expression make_fset_union(const data::data_expression& left, const data::data_expression& right)
  {
    const data::sort_expression& s = atermpp::down_cast<data::application>(left).head().sort();
    const data::container_sort& cs = atermpp::down_cast<data::container_sort>(s);
    return data::sort_fset::union_(cs.element_sort(), left, right);
  }

  data::data_expression make_set_union(const data::data_expression& left, const data::data_expression& right)
  {
    const data::sort_expression& s = atermpp::down_cast<data::application>(left).head().sort();
    const data::container_sort& cs = atermpp::down_cast<data::container_sort>(s);
    return data::sort_set::union_(cs.element_sort(), left, right);
  }

  data::data_expression make_plus(const data::data_expression& left, const data::data_expression& right)
  {
    if (data::sort_real::is_real(left.sort()) || data::sort_real::is_real(right.sort()))
    {
      return data::sort_real::plus(left, right);
    }
    else if (data::sort_int::is_int(left.sort()) || data::sort_int::is_int(right.sort()))
    {
      return data::sort_int::plus(left, right);
    }
    else if (data::sort_nat::is_nat(left.sort()) || data::sort_nat::is_nat(right.sort()))
    {
      return data::sort_nat::plus(left, right);
    }
    else if (data::sort_pos::is_pos(left.sort()) || data::sort_pos::is_pos(right.sort()))
    {
      return data::sort_pos::plus(left, right);
    }
    else if (data::sort_bag::is_union_application(left) || data::sort_bag::is_union_application(right))
    {
      return make_bag_union(left, right);
    }
    else if (data::sort_fbag::is_union_application(left) || data::sort_fbag::is_union_application(right))
    {
      return make_fbag_union(left, right);
    }
    else if (data::sort_set::is_union_application(left) || data::sort_set::is_union_application(right))
    {
      return make_set_union(left, right);
    }
    else if (data::sort_fset::is_union_application(left) || data::sort_fset::is_union_application(right))
    {
      return make_fset_union(left, right);
    }
    throw mcrl2::runtime_error("could not typecheck " + data::pp(left) + " + " + data::pp(right));
  }

  data::data_expression make_element_at(const data::data_expression& left, const data::data_expression& right) const
  {
    const data::sort_expression& s = atermpp::down_cast<data::application>(left).head().sort();
    const data::container_sort& cs = atermpp::down_cast<data::container_sort>(s);
    return data::sort_list::element_at(cs.element_sort(), left, right);
  }

  regular_formula apply(const regular_formulas::untyped_regular_formula& x)
  {
    regular_formula left = (*this).apply(x.left());
    regular_formula right = (*this).apply(x.right());
    if (data::is_data_expression(left) && data::is_data_expression(right))
    {
      if (x.name() == core::identifier_string("."))
      {
        return make_element_at(atermpp::down_cast<data::data_expression>(left), atermpp::down_cast<data::data_expression>(right));
      }
      else
      {
        return make_plus(atermpp::down_cast<data::data_expression>(left), atermpp::down_cast<data::data_expression>(right));
      }
    }
    else
    {
      if (x.name() == core::identifier_string("."))
      {
        return seq(left, right);
      }
      else
      {
        return alt(left, right);
      }
    }
  }

  regular_formula apply(const action_formulas::action_formula& x)
  {
    return action_formulas::detail::make_typecheck_builder(m_data_type_checker, m_variable_context, m_action_context).apply(x);
  }
};

inline
typecheck_builder make_typecheck_builder(
                    data::data_type_checker& data_typechecker,
                    const data::detail::variable_context& variables,
                    const process::detail::action_context& actions
                   )
{
  return typecheck_builder(data_typechecker, variables, actions);
}

} // namespace detail

template <typename ActionLabelContainer = std::vector<state_formulas::variable>, typename VariableContainer = std::vector<data::variable> >
regular_formula typecheck(const regular_formula& x,
                          const data::data_specification& dataspec,
                          const VariableContainer& variables,
                          const ActionLabelContainer& actions
                         )
{
  data::data_type_checker data_typechecker(dataspec);
  data::detail::variable_context variable_context;
  variable_context.add_context_variables(variables, data_typechecker);
  process::detail::action_context action_context;
  action_context.add_context_action_labels(actions, data_typechecker);
  return detail::make_typecheck_builder(data_typechecker, variable_context, action_context).apply(regular_formulas::normalize_sorts(x, data_typechecker.typechecked_data_specification()));
}

inline
regular_formula typecheck(const regular_formula& x, const lps::specification& lpsspec)
{
  return typecheck(x, lpsspec.data(), lpsspec.global_variables(), lpsspec.action_labels());
}

} // namespace regular_formulas

namespace state_formulas
{

namespace detail
{

struct typecheck_builder: public state_formula_builder<typecheck_builder>
{
  typedef state_formula_builder<typecheck_builder> super;
  using super::apply;

  data::data_type_checker& m_data_type_checker;
  data::detail::variable_context m_variable_context;
  const process::detail::action_context& m_action_context;
  detail::state_variable_context m_state_variable_context;

  typecheck_builder(data::data_type_checker& data_typechecker,
                    const data::detail::variable_context& variable_context,
                    const process::detail::action_context& action_context,
                    const detail::state_variable_context& state_variable_context
                   )
    : m_data_type_checker(data_typechecker),
      m_variable_context(variable_context),
      m_action_context(action_context),
      m_state_variable_context(state_variable_context)
  {}

  void check_sort_declared(const data::sort_expression& s, const state_formula& x)
  {
    try
    {
      m_data_type_checker.check_sort_is_declared(s);
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\ntype error occurred while typechecking " + state_formulas::pp(x));
    }
  }

  state_formula apply(const data::data_expression& x)
  {
    return m_data_type_checker.typecheck_data_expression(x, data::sort_bool::bool_(), m_variable_context);
  }

  state_formula apply(const state_formulas::forall& x)
  {
    try
    {
      auto m_variable_context_copy = m_variable_context;
      m_variable_context.add_context_variables(x.variables(), m_data_type_checker);
      state_formula body = (*this).apply(x.body());
      m_variable_context = m_variable_context_copy;
      return forall(x.variables(), body);
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nwhile typechecking " + state_formulas::pp(x));
    }
  }

  state_formula apply(const state_formulas::exists& x)
  {
    try
    {
      auto m_variable_context_copy = m_variable_context;
      m_variable_context.add_context_variables(x.variables(), m_data_type_checker);
      state_formula body = (*this).apply(x.body());
      m_variable_context = m_variable_context_copy;
      return exists(x.variables(), body);
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nwhile typechecking " + state_formulas::pp(x));
    }
  }

  state_formula apply(const state_formulas::may& x)
  {
    return may(regular_formulas::detail::make_typecheck_builder(m_data_type_checker, m_variable_context, m_action_context).apply(x.formula()), (*this).apply(x.operand()));
  }

  state_formula apply(const state_formulas::must& x)
  {
    return must(regular_formulas::detail::make_typecheck_builder(m_data_type_checker, m_variable_context, m_action_context).apply(x.formula()), (*this).apply(x.operand()));
  }

  state_formula apply(const state_formulas::delay_timed& x)
  {
    data::data_expression new_time = m_data_type_checker.typecheck_data_expression(x.time_stamp(), data::sort_real::real_(), m_variable_context);
    return delay_timed(new_time);
  }

  state_formula apply(const state_formulas::yaled_timed& x)
  {
    data::data_expression new_time = m_data_type_checker.typecheck_data_expression(x.time_stamp(), data::sort_real::real_(), m_variable_context);
    return yaled_timed(new_time);
  }

  state_formula apply_untyped_parameter(const core::identifier_string& name, const data::data_expression_list& arguments)
  {
    data::sort_expression_list expected_sorts = m_state_variable_context.matching_state_variable_sorts(name, arguments);
    data::data_expression_list new_arguments;
    auto q1 = expected_sorts.begin();
    auto q2 = arguments.begin();
    for (; q1 != expected_sorts.end(); ++q1, ++q2)
    {
      new_arguments.push_front(m_data_type_checker.typecheck_data_expression(*q2, *q1, m_variable_context));
    }
    new_arguments = atermpp::reverse(new_arguments);
    return state_formulas::variable(name, new_arguments);
  }

  state_formula apply(const state_formulas::variable& x)
  {
    return apply_untyped_parameter(x.name(), x.arguments());
  }

  state_formula apply(const data::untyped_data_parameter& x)
  {
    return apply_untyped_parameter(x.name(), x.arguments());
  }

  data::variable_list assignment_variables(const data::assignment_list& x) const
  {
    std::vector<data::variable> result;
    for (const data::assignment& a: x)
    {
      result.push_back(a.lhs());
    }
    return data::variable_list(result.begin(), result.end());
  }

  template <typename MuNuFormula>
  state_formula apply_mu_nu(const MuNuFormula& x, bool is_mu)
  {
    for (const data::assignment& a: x.assignments())
    {
      check_sort_declared(a.lhs().sort(), x);
    }

    data::assignment_list new_assignments = detail::typecheck_assignments(x.assignments(), m_variable_context, m_data_type_checker);

    // add the assignment variables to the context
    auto m_variable_context_copy = m_variable_context;
    data::variable_list x_variables = assignment_variables(x.assignments());
    m_variable_context.add_context_variables(x_variables, m_data_type_checker);

    // add x to the state variable context
    auto m_state_variable_context_copy = m_state_variable_context;
    m_state_variable_context.add_state_variable(x.name(), x_variables, m_data_type_checker);

    // typecheck the operand
    state_formula new_operand = (*this).apply(x.operand());

    // restore the context
    m_variable_context = m_variable_context_copy;
    m_state_variable_context = m_state_variable_context_copy;

    if (is_mu)
    {
      return mu(x.name(), new_assignments, new_operand);
    }
    else
    {
      return nu(x.name(), new_assignments, new_operand);
    }
  }

  state_formula apply(const state_formulas::nu& x)
  {
    return apply_mu_nu(x, false);
  }

  state_formula apply(const state_formulas::mu& x)
  {
    return apply_mu_nu(x, true);
  }
};

inline
typecheck_builder make_typecheck_builder(
                    data::data_type_checker& data_typechecker,
                    const data::detail::variable_context& variable_context,
                    const process::detail::action_context& action_context,
                    const detail::state_variable_context& state_variable_context
                   )
{
  return typecheck_builder(data_typechecker, variable_context, action_context, state_variable_context);
}

} // namespace detail

class state_formula_type_checker
{
  protected:
    data::data_type_checker m_data_type_checker;
    data::detail::variable_context m_variable_context;
    process::detail::action_context m_action_context;
    detail::state_variable_context m_state_variable_context;

  public:
    /** \brief     Type check a state formula.
     *  Throws a mcrl2::runtime_error exception if the expression is not well typed.
     *  \param[in] d A state formula that has not been type checked.
     *  \param[in] check_monotonicity Check whether the formula is monotonic, in the sense that no fixed point
     *             variable occurs in the scope of an odd number of negations.
     *  \return    a state formula where all untyped identifiers have been replace by typed ones.
     **/
    template <typename ActionLabelContainer = std::vector<state_formulas::variable>, typename VariableContainer = std::vector<data::variable> >
    state_formula_type_checker(const data::data_specification& dataspec,
                               const ActionLabelContainer& action_labels = ActionLabelContainer(),
                               const VariableContainer& variables = VariableContainer()
                              )
      : m_data_type_checker(dataspec)
    {
      m_variable_context.add_context_variables(variables, m_data_type_checker);
      m_action_context.add_context_action_labels(action_labels, m_data_type_checker);
    }

    //check correctness of the state formula in state_formula using
    //the process specification or LPS in spec as follows:
    //1) determine the types of actions according to the definitions
    //   in spec
    //2) determine the types of data expressions according to the
    //   definitions in spec
    //3) check for name conflicts of data variable declarations in
    //   forall, exists, mu and nu quantifiers
    //4) check for monotonicity of fixpoint variables
    state_formula operator()(const state_formula& x, bool check_monotonicity)
    {
      mCRL2log(log::verbose) << "type checking state formula..." << std::endl;

      state_formula result = detail::make_typecheck_builder(m_data_type_checker, m_variable_context, m_action_context, m_state_variable_context).apply(state_formulas::normalize_sorts(x, m_data_type_checker.typechecked_data_specification()));
      if (check_monotonicity && !is_monotonous(result))
      {
        throw mcrl2::runtime_error("state formula is not monotonic: " + state_formulas::pp(result));
      }
      return result;
    }
};

/** \brief     Type check a state formula.
 *  Throws an exception if something went wrong.
 *  \param[in] formula A state formula that has not been type checked.
 *  \post      formula is type checked.
 **/
inline
state_formula type_check_state_formula(const state_formula& x, const lps::specification& lpsspec, bool check_monotonicity = true)
{
  try
  {
    state_formula_type_checker type_checker(lpsspec.data(), lpsspec.action_labels(), lpsspec.global_variables());
    return type_checker(x, check_monotonicity);
  }
  catch (mcrl2::runtime_error& e)
  {
    throw mcrl2::runtime_error(std::string(e.what()) + "\ncould not type check modal formula " + state_formulas::pp(x));
  }
}

} // namespace state_formulas

} // namespace mcrl2

#endif // MCRL2_MODAL_FORMULA_TYPECHECK_H
