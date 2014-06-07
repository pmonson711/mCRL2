// Author(s): Rimco Boudewijns
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <QCoreApplication>

#include "solver.h"
#include "parsing.h"

#include "mcrl2/utilities/atermthread.h"

const std::string Solver::className = "Solver";

Solver::Solver()
{
  moveToThread(mcrl2::utilities::qt::get_aterm_thread());
  thread()->setPriority(QThread::IdlePriority);
  m_parsed = false;
}

void Solver::setRewriter(QString solver)
{
  m_rewrite_strategy = mcrl2::data::parse_rewrite_strategy(solver.toStdString())  ;
}

void Solver::solve(QString specification, QString dataExpression)
{
  m_abort = false;
  if (m_specification != specification || !m_parsed)
  {
    m_parsed = false;
    m_specification = specification;
    try
    {
      mcrl2xi_qt::parseMcrl2Specification(m_specification.toStdString(), m_data_spec, m_global_vars);
      m_parsed = true;
    }
    catch (mcrl2::runtime_error e)
    {
      m_parseError = QString::fromStdString(e.what());
    }
  }

  if (m_parsed)
  {
    try
    {
      std::string stdDataExpression = dataExpression.toStdString();

      mCRL2log(info) << "Solving: \"" << stdDataExpression << "\"" << std::endl;

      int dotpos =  stdDataExpression.find('.');
      if (dotpos  == -1)
      {
        throw mcrl2::runtime_error("Expected input of the shape 'x1:Type1,...,xn:Typen.b' where b is a boolean expression.");
      }

      std::set <mcrl2::data::variable > m_vars=m_global_vars;
      parse_variables(std::string(stdDataExpression.substr(0, dotpos)
                                  ) + ";",std::inserter(m_vars,m_vars.begin()),m_data_spec);

      mcrl2::data::data_expression term =
          mcrl2::data::parse_data_expression(
            stdDataExpression.substr(dotpos+1, stdDataExpression.length()-1),
            m_vars.begin(), m_vars.end(),
            m_data_spec
            );

      if (term.sort()!=mcrl2::data::sort_bool::bool_())
      {
        throw mcrl2::runtime_error("Expression is not of sort Bool.");
      }

      std::set<mcrl2::data::sort_expression> all_sorts=find_sort_expressions(term);
      m_data_spec.add_context_sorts(all_sorts);

      mcrl2::data::rewriter rewr(m_data_spec,m_rewrite_strategy);

      term=rewr(term);

      typedef mcrl2::data::classic_enumerator< mcrl2::data::rewriter > enumerator_type;

      enumerator_type enumerator(rewr,m_data_spec);

      mcrl2::data::mutable_indexed_substitution<> sigma;
      // Stop when more than 10000 internal variables are required.
      for (enumerator_type::iterator i = enumerator.begin(sigma,mcrl2::data::variable_list(m_vars.begin(),m_vars.end()),term,10000); 
           i != enumerator.end() && !m_abort; ++i)
      {
        mCRL2log(info) << "Solution found" << std::endl;

        QString s('[');

        // mcrl2::data::data_expression_list::const_iterator j=i->begin();
        mcrl2::data::mutable_indexed_substitution<> sigma_i;
        i->add_assignments(m_vars,sigma_i,rewr);
        for (std::set< mcrl2::data::variable >::const_iterator v=m_vars.begin(); v!=m_vars.end() ; ++v)
        {
          if( v != m_vars.begin() )
          {
            s.append(", ");
          }
          s.append(mcrl2::data::pp(*v).c_str());
          s.append(" := ");
          s.append(mcrl2::data::pp(sigma_i(*v)).c_str());
        }
        s.append("] evaluates to ");
        s.append(mcrl2::data::pp(rewr(term,sigma_i)).c_str());

        emit solvedPart(s);

        QCoreApplication::processEvents(); // To process the signals
        if (m_abort)
          break;
      }
      if (m_abort)
        mCRL2log(info) << "Abort by user." << std::endl;
      else
        mCRL2log(info) << "Done solving." << std::endl;

    }
    catch (mcrl2::runtime_error e)
    {
      QString err = QString::fromStdString(e.what());
      emit(exprError(err));
    }
  }
  else
  {
    emit(parseError(m_parseError));
  }
  emit(finished());
}

void Solver::abort()
{
  m_abort = true;
}

