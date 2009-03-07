// Author(s): Jeroen Keiren
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/new_data/structured_sort.h
/// \brief The class structured_sort.

#ifndef MCRL2_NEW_DATA_STRUCTURED_SORT_H
#define MCRL2_NEW_DATA_STRUCTURED_SORT_H

#include <string>
#include <iterator>
#include <boost/range/iterator_range.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include "mcrl2/atermpp/aterm_access.h"
#include "mcrl2/atermpp/aterm_appl.h"
#include "mcrl2/atermpp/aterm_list.h"
#include "mcrl2/atermpp/vector.h"
#include "mcrl2/core/identifier_string.h"
#include "mcrl2/core/detail/struct_core.h"
#include "mcrl2/new_data/sort_expression.h"
#include "mcrl2/new_data/function_symbol.h"
#include "mcrl2/new_data/data_equation.h"
#include "mcrl2/new_data/function_symbol.h"
#include "mcrl2/new_data/data_equation.h"
#include "mcrl2/new_data/variable.h"
#include "mcrl2/new_data/bool.h"
#include "mcrl2/new_data/standard.h"
#include "mcrl2/new_data/identifier_generator.h"
#include "mcrl2/new_data/data_expression_utility.h"

namespace mcrl2 {

  namespace new_data {

    class structured_sort;

    /// \brief Argument of a structured sort constructor.
    ///
    /// This comprises an optional name and a mandatory sort.
    class structured_sort_constructor_argument: public atermpp::aterm_appl
    {

      public:

        /// \brief Constructor
        ///
        structured_sort_constructor_argument()
          : atermpp::aterm_appl(core::detail::constructStructProj())
        {}

        /// \internal
        /// \brief Constructor
        ///
        /// \param[in] a A term.
        /// \pre a has the internal format of a constructor argument of a
        ///        structured sort.
        structured_sort_constructor_argument(const atermpp::aterm_appl& a)
          : atermpp::aterm_appl(a)
        {
          assert(core::detail::gsIsStructProj(a));
        }

        /// \brief Constructor
        ///
        /// \param[in] name The name of the argument.
        /// \param[in] sort The sort of the argument.
        structured_sort_constructor_argument(const std::string& name,
                                             const sort_expression& sort)
          : atermpp::aterm_appl(core::detail::gsMakeStructProj(atermpp::aterm_string(name), sort))
        {}

        /// \brief Constructor
        ///
        /// \param[in] sort The sort of the argument.
        structured_sort_constructor_argument(const sort_expression& sort)
          : atermpp::aterm_appl(core::detail::gsMakeStructProj(core::detail::gsMakeNil(), sort))
        {}

        /// \brief Returns the name of the constructor argument.
        ///
        inline
        std::string name() const
        {
          atermpp::aterm_appl n = atermpp::arg1(*this);
          if (n == core::detail::gsMakeNil())
          {
            return std::string();
          }
          else
          {
            return atermpp::aterm_string(n);
          }
        }

        /// \brief Returns the sort of the constructor argument.
        ///
        inline
        sort_expression sort() const
        {
          return atermpp::arg2(*this);
        }

    }; // class structured_sort_constructor_argument

    /// \brief List of structured_sort_constructor_argument
    typedef atermpp::term_list< structured_sort_constructor_argument > structured_sort_constructor_argument_list;
    /// \brief Vector of structured_sort_constructor_argument
    typedef atermpp::vector< structured_sort_constructor_argument >    structured_sort_constructor_argument_vector;

    /// \brief A structured sort constructor.
    ///
    /// A structured sort constructor has a mandatory name, a mandatory,
    /// non-empty list of arguments and and optional recogniser name.
    class structured_sort_constructor: public atermpp::aterm_appl
    {
      private:

        struct get_argument_sort : public
          std::unary_function< structured_sort_constructor_argument const&, sort_expression > {

          sort_expression operator()(structured_sort_constructor_argument const& s) const {
            return s.sort();
          }
        };

      public:

        /// \brief iterator range over list of structured sort constructors
        typedef detail::term_list_random_iterator< structured_sort_constructor_argument > arguments_iterator;
        /// \brief iterator range over list of structured sort constructors
        typedef boost::iterator_range< arguments_iterator >                               arguments_range;
        /// \brief iterator range over constant list of structured sort constructors
        typedef boost::iterator_range< arguments_iterator >                               arguments_const_range;

        /// \brief iterator range over list of structured sort constructors
        typedef boost::transform_iterator< get_argument_sort, arguments_iterator >        argument_sorts_iterator;
        /// \brief iterator range over list of structured sort constructors
        typedef boost::iterator_range< argument_sorts_iterator >                          argument_sorts_range;
        /// \brief iterator range over constant list of structured sort constructors
        typedef boost::iterator_range< argument_sorts_iterator >                          argument_sorts_const_range;

      public:

        /// \brief Constructor
        structured_sort_constructor()
          : atermpp::aterm_appl(core::detail::constructStructCons())
        {}

        /// \internal
        /// \brief Constructor
        ///
        /// \param[in] c A term
        /// \pre c has the internal format of a constructor of a structured
        ///      sort.
        structured_sort_constructor(const atermpp::aterm_appl c)
          : atermpp::aterm_appl(c)
        {
          assert(core::detail::gsIsStructCons(c));
        }

        /// \brief Constructor
        ///
        /// \param[in] c is a structured sort constructor.
        structured_sort_constructor(const structured_sort_constructor& c)
          : atermpp::aterm_appl(c)
        {}

        /// \brief Constructor
        ///
        /// \param[in] name The name of the constructor.
        /// \param[in] arguments The arguments of the constructor.
        /// \param[in] recogniser The name of the recogniser.
        /// \pre name is not empty.
        /// \pre recogniser is not empty.
        template < typename ForwardTraversalIterator >
        structured_sort_constructor(const std::string& name,
                                    const typename boost::iterator_range< ForwardTraversalIterator >& arguments,
                                    const std::string& recogniser)
          : atermpp::aterm_appl(core::detail::gsMakeStructCons(atermpp::aterm_string(name),
                                  detail::convert< atermpp::term_list<structured_sort_constructor_argument> >(arguments),
                                  atermpp::aterm_string(recogniser)))
        {
          assert(!name.empty());
          assert(!recogniser.empty());
        }

        /// \brief Constructor
        ///
        /// \param[in] name The name of the constructor.
        /// \param[in] recogniser The name of the recogniser.
        /// \pre name is not empty.
        /// \pre recogniser is not empty.
        structured_sort_constructor(const std::string& name, const std::string& recogniser = "")
          : atermpp::aterm_appl(core::detail::gsMakeStructCons(atermpp::aterm_string(name),
                                  atermpp::term_list<structured_sort_constructor_argument>(),
                                  (recogniser.empty()) ? core::detail::gsMakeNil()
                                               : static_cast< ATermAppl >(atermpp::aterm_string(recogniser))))
        {
          assert(!name.empty());
        }

        /// \brief Constructor
        ///
        /// \param[in] name The name of the constructor.
        /// \param[in] arguments The arguments of the constructor.
        /// \pre name is not empty.
        template < typename ForwardTraversalIterator >
        structured_sort_constructor(const std::string& name,
                                    const typename boost::iterator_range< ForwardTraversalIterator >& arguments)
          : atermpp::aterm_appl(core::detail::gsMakeStructCons(atermpp::aterm_string(name),
                   detail::convert< atermpp::term_list<structured_sort_constructor_argument> >(arguments),
                   core::detail::gsMakeNil()))
        {
          assert(!name.empty());
        }

        /// \brief Returns the name of the constructor.
        ///
        inline
        std::string name() const
        {
          return atermpp::aterm_string(atermpp::arg1(*this));
        }

        /// \brief Returns the arguments of the constructor.
        ///
        inline
        arguments_const_range arguments() const
        {
          return boost::make_iterator_range(add_random_access< structured_sort_constructor_argument >(atermpp::list_arg2(appl())));
        }

        /// \brief Returns the sorts of the arguments.
        ///
        inline
        argument_sorts_const_range argument_sorts() const
        {
          return new_data::make_iterator_range< argument_sorts_iterator >(arguments());
        }

        /// \brief Returns the constructor function for this constructor,
        ///        assuming it is internally represented with sort s.
        /// \param s Sort expression this sort is internally represented as.
        inline
        function_symbol constructor_function(const sort_expression& s) const
        {
          argument_sorts_const_range arguments(argument_sorts());

          return function_symbol(name(), (arguments.empty()) ? s : function_sort(arguments, s));
        }

        /// \brief Returns the projection functions for this constructor.
        /// \param s The sort as which the structured sort this constructor corresponds
        ///          to is treated internally.
        inline
        function_symbol_vector projection_functions(const sort_expression& s) const
        {
          function_symbol_vector result;
          for(arguments_const_range i(arguments()); !i.empty(); i.advance_begin(1))
          {
            if (!i.front().name().empty()) {
              result.push_back(function_symbol(i.front().name(), function_sort(s, i.front().sort())));
            }
          }
          return result;
        }

        /// \brief Returns the arguments of the constructor, without the
        ///        projection names

        /// \brief Returns the name of the recogniser of the constructor.
        inline
        std::string recogniser() const
        {
          atermpp::aterm_appl r = arg3(*this);
          if (r == core::detail::gsMakeNil())
          {
            return std::string();
          }
          else
          {
            return atermpp::aterm_string(r);
          }
        }

        /// \brief Returns the function corresponding to the recogniser of this
        /// constructor, such that it is usable in the rewriter.
        /// \param s The sort as which the structured sort this constructor
        /// corresponds to is treated internally.
        inline
        function_symbol recogniser_function(const sort_expression& s) const
        {
          return function_symbol(recogniser(), function_sort(s, sort_bool_::bool_()));
        }

    }; // class structured_sort_constructor

    /// \brief List of structured_sort_constructor.
    typedef atermpp::term_list< structured_sort_constructor > structured_sort_constructor_list;

    /// \brief List of structured_sort_constructor.
    typedef atermpp::vector< structured_sort_constructor >    structured_sort_constructor_vector;

    /// \brief structured sort.
    ///
    /// A structured sort is a sort with the following structure:
    ///  struct c1(pr1,1:S1,1, ..., pr1,k1:S1,k1)?is_c1
    ///       | ...
    ///       | cn(prn,1:Sn,1, ..., prn,kn:Sn,kn)?is_cn
    /// in this:
    /// * c1, ..., cn are called constructors.
    /// * pri,j are the projection functions (or constructor arguments).
    /// * is_ci are the recognisers.
    class structured_sort: public sort_expression
    {

      private:

        static bool has_recogniser(structured_sort_constructor const& s)
        {
          return !s.recogniser().empty();
        }

      public:

        /// \brief iterator range over list of constructor declarations
        typedef boost::iterator_range< structured_sort_constructor_list::iterator >       constructor_range;
        /// \brief iterator range over constant list of constructor declarations
        typedef boost::iterator_range< structured_sort_constructor_list::const_iterator > constructor_const_range;

        /// \brief iterator range over list of constructor functions
        typedef boost::iterator_range< function_symbol_list::iterator >                   constructor_function_range;
        /// \brief iterator range over constant list of constructor functions
        typedef boost::iterator_range< function_symbol_list::const_iterator >             constructor_function_const_range;

        /// \brief iterator range over list of constructor functions
        typedef boost::iterator_range< function_symbol_list::iterator >                   projection_function_range;
        /// \brief iterator range over constant list of constructor functions
        typedef boost::iterator_range< function_symbol_list::const_iterator >             projection_function_const_range;

        /// \brief iterator range over list of constructor functions
        typedef boost::iterator_range< function_symbol_list::iterator >                   recogniser_function_range;
        /// \brief iterator range over constant list of constructor functions
        typedef boost::iterator_range< function_symbol_list::const_iterator >             recogniser_function_const_range;

      public:

        /// \brief Constructor
        structured_sort()
          : sort_expression(core::detail::constructSortStruct())
        {}

        /// \brief Constructor
        ///
        /// \param[in] s A sort expression.
        /// \pre s is a structured sort.
        structured_sort(const sort_expression& s)
          : sort_expression(s)
        {
          assert(s.is_structured_sort());
        }

        /// \brief Constructor
        ///
        /// \param[in] struct_constructors The list of constructors.
        /// \post struct_constructors is empty.
        structured_sort(const structured_sort_constructor_list& struct_constructors)
          : sort_expression(mcrl2::core::detail::gsMakeSortStruct(struct_constructors))
        {
          assert(!struct_constructors.empty());
        }

        /// \brief Constructor
        ///
        /// \param[in] struct_constructors The list of constructors.
        /// \post struct_constructors is empty.
        structured_sort(const structured_sort_constructor_vector& struct_constructors)
          : sort_expression(mcrl2::core::detail::gsMakeSortStruct(atermpp::term_list<structured_sort_constructor>(struct_constructors.begin(), struct_constructors.end())))
        {
          assert(!struct_constructors.empty());
        }

        /// \brief Constructor
        ///
        /// \param[in] struct_constructors The list of constructors.
        /// \post struct_constructors is empty.
        template < typename ForwardTraversalIterator >
        structured_sort(const boost::iterator_range< ForwardTraversalIterator >& struct_constructors)
          : sort_expression(mcrl2::core::detail::gsMakeSortStruct(atermpp::term_list<structured_sort_constructor>(struct_constructors.begin(), struct_constructors.end())))
        {
          assert(!struct_constructors.empty());
        }

        /// \brief Returns the struct constructors of this sort
        ///
        inline
        constructor_const_range struct_constructors() const
        {
          return boost::make_iterator_range(structured_sort_constructor_list(atermpp::arg1(*this)));
        }


        /// \brief Returns the constructor functions of this sort, such that the
        ///        result can be used by the rewriter
        /// \param s The sort expression as which this sort is treated
        //         internally
        inline
        function_symbol_vector constructor_functions(const sort_expression& s) const
        {
          function_symbol_vector result;
          for(constructor_const_range i = struct_constructors(); !i.empty(); i.advance_begin(1))
          {
            result.push_back(i.front().constructor_function(s));
          }
          return result;
        }

        /// \brief Returns the projection functions of this sort, such that the
        ///        result can be used by the rewriter
        /// \param s The sort expression as which this sort is treated
        //         internally
        inline
        function_symbol_vector projection_functions(const sort_expression& s) const
        {
          function_symbol_vector result;
          for(constructor_const_range i = struct_constructors(); !i.empty(); i.advance_begin(1))
          {
            function_symbol_vector projections(i.front().projection_functions(s));

            for(function_symbol_vector::const_iterator j = projections.begin(); j != projections.end(); ++j)
            {
              result.push_back(*j);
            }
          }
          return result;
        }

        /// \brief Returns the recogniser functions of this sort, such that the
        ///        result can be used by the rewriter
        /// \param s The sort expression as which this sort is treated
        //         internally
        inline
        function_symbol_vector recogniser_functions(const sort_expression& s) const
        {
          function_symbol_vector result;
          for(constructor_const_range i = struct_constructors(); !i.empty(); i.advance_begin(1))
          {
            if (!i.front().recogniser().empty()) {
              result.push_back(i.front().recogniser_function(s));
            }
          }
          return result;
        }

        /// \brief Returns the equations for ==, < and <= for this sort, such that the
        ///        result can be used by the rewriter
        /// \param s The sort expression as which this sort is treated
        //         internally
        inline
        data_equation_vector constructor_equations(const sort_expression& s) const
        {
          data_equation_vector result;

          structured_sort_constructor_vector cl(boost::copy_range< structured_sort_constructor_vector >(struct_constructors()));

          for (structured_sort_constructor_vector::const_iterator i = cl.begin(); i != cl.end(); ++i)
          {
            for (structured_sort_constructor_vector::const_iterator j = cl.begin(); j != cl.end(); ++j)
            {
              data_expression right_equal         = (i == j) ? sort_bool_::true_() : sort_bool_::false_();
              data_expression right_smaller       = (i < j)  ? sort_bool_::true_() : sort_bool_::false_();
              data_expression right_smaller_equal = (i <= j) ? sort_bool_::true_() : sort_bool_::false_();

              if (i->argument_sorts().empty() && j->argument_sorts().empty())
              {
                data_expression operand_left  = i->constructor_function(s);
                data_expression operand_right = j->constructor_function(s);

                result.push_back(data_equation(equal_to(operand_left, operand_right), right_equal));
                result.push_back(data_equation(less(operand_left, operand_right), right_smaller));
                result.push_back(data_equation(less_equal(operand_left, operand_right), right_smaller_equal));
              }
              else { // at least one constructor takes arguments
                data_expression operand_left;
                data_expression operand_right;

                number_postfix_generator generator("v");

                // Create variables for equation
                variable_vector variables;

                if (i->argument_sorts().empty())
                {
                  operand_left  = i->constructor_function(s);
                }
                else
                {
                  for (structured_sort_constructor::arguments_const_range k(i->arguments()); k.begin() != k.end(); k.advance_begin(1))
                  {
                    variables.push_back(variable(generator(), k.front().sort()));
                  }

                  // create first operand of ==, < or <=
                  operand_left = application(i->constructor_function(s), boost::make_iterator_range(variables));
                }

                if (j->argument_sorts().empty())
                {
                  operand_right = j->constructor_function(s);
                }
                else
                {
                  for (structured_sort_constructor::arguments_const_range k(j->arguments()); k.begin() != k.end(); k.advance_begin(1))
                  {
                    variables.push_back(variable(generator(), k.front().sort()));
                  }

                  // create second operand of ==, < or <=
                  operand_right = application(j->constructor_function(s),
                             boost::make_iterator_range(variables.begin() + i->arguments().size(), variables.end()));
                }

                if (i == j)
                { // constructors are the same
                  variable_vector::const_reverse_iterator k(variables.rbegin() + i->arguments().size());
                  variable_vector::const_reverse_iterator l(variables.rbegin());

                  right_equal         = equal_to(*k, *l);
                  right_smaller       = less(*k, *l);
                  right_smaller_equal = less_equal(*k, *l);

                  for (++l, ++k; k != variables.rend(); ++l, ++k)
                  {
                    // Constructors have one or more arguments:
                    // - rhs for c(x0,...,xn) == c(y0,..,yn):
                    //     x0 == y0 && ... && xn == yn
                    right_equal         = sort_bool_::and_(equal_to(*k, *l), right_equal);
                    // - rhs for c(x0,...,xn) < c(y0,..,yn):
                    //     x0 < y0                                                     , when n = 0
                    //     x0 < y0 || (x0 == y0 && x1 < y1)                            , when n = 1
                    //     x0 < y0 || (x0 == y0 && (x1 < y1 || (x1 == y1 && x2 < y2))) , when n = 2
                    //     etcetera
                    right_smaller       = sort_bool_::or_(less(*k, *l),
                        sort_bool_::and_(equal_to(*k, *l), right_smaller));
                    // - rhs for c(x0,...,xn) <= c(y0,..,yn):
                    //     x0 <= y0                                                    , when n = 0
                    //     x0 < y0 || (x0 == y0 && x1 <= y1)                           , when n = 1
                    //     x0 < y0 || (x0 == y0 && (x1 < y1 || (x1 == y1 && x2 <= y2))), when n = 2
                    //     etcetera
                    right_smaller_equal = sort_bool_::or_(less(*k, *l),
                        sort_bool_::and_(equal_to(*k, *l), right_smaller_equal));
                  }
                }

                result.push_back(data_equation(boost::make_iterator_range(variables),
                  equal_to(operand_left, operand_right), right_equal));
                result.push_back(data_equation(boost::make_iterator_range(variables),
                  less(operand_left, operand_right), right_smaller));
                result.push_back(data_equation(boost::make_iterator_range(variables),
                  less_equal(operand_left, operand_right), right_smaller_equal));
              }
            }
          }

          return result;
        }

        inline
        data_equation_vector projection_equations(const sort_expression& s) const
        {
          data_equation_vector result;

          for (constructor_const_range i(struct_constructors());
                                         i.begin() != i.end(); i.advance_begin(1))
          {
            if (!i.front().argument_sorts().empty())
            {
              typedef structured_sort_constructor::arguments_const_range argument_range;

              structured_sort_constructor::arguments_const_range arguments(i.front().arguments());

              number_postfix_generator generator("v");

              variable_vector variables;

              // Create variables for equation
              for (argument_range::const_iterator j(arguments.begin()); j != arguments.end(); ++j)
              {
                variables.push_back(variable(generator(), j->sort()));
              }

              for (argument_range::const_iterator j(arguments.begin()); j != arguments.end(); ++j)
              {
                if (!j->name().empty()) {
                  application lhs(function_symbol(j->name(), function_sort(s, j->sort())),
                        application(i.front().constructor_function(s), boost::make_iterator_range(variables)));

                  result.push_back(data_equation(boost::make_iterator_range(variables), lhs, variables[j - arguments.begin()]));
                }
              }
            }
          }

          return result;
        }

        inline
        data_equation_vector recogniser_equations(const sort_expression& s) const
        {
          data_equation_vector result;

          constructor_const_range cl(struct_constructors());

          for (constructor_const_range::iterator i = cl.begin(); i != cl.end(); ++i)
          {
            for (constructor_const_range::iterator j = cl.begin(); j != cl.end(); ++j)
            {
              if(!j->recogniser().empty())
              {
                data_expression right = (*i == *j) ? sort_bool_::true_() : sort_bool_::false_();

                if (i->argument_sorts().empty())
                {
                  result.push_back(data_equation(application(j->recogniser_function(s),
                    i->constructor_function(s)), right));
                }
                else
                {
                  typedef structured_sort_constructor::arguments_const_range argument_range;

                  number_postfix_generator generator("v");

                  argument_range arguments(i->arguments());

                  // Create variables for equation
                  variable_vector variables;

                  for (argument_range::const_iterator k(arguments.begin()); k != arguments.end(); ++k)
                  {
                    variables.push_back(variable(generator(), k->sort()));
                  }

                  result.push_back(data_equation(boost::make_iterator_range(variables), application(j->recogniser_function(s),
                    application(i->constructor_function(s), boost::make_iterator_range(variables))), right));
                }
              }
            }
          }

          return result;
        }

    }; // class structured_sort

    /// \brief list of structured sorts
    typedef atermpp::term_list< structured_sort > structured_sort_list;
    /// \brief vector of structured sorts
    typedef atermpp::vector< structured_sort >    structured_sort_vector;

  } // namespace new_data

} // namespace mcrl2

#endif // MCRL2_NEW_DATA_SORT_EXPRESSION_H

