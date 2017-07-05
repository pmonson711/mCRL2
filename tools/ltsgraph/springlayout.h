// Author(s): Rimco Boudewijns and Sjoerd Cranen
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

/**

  @file springlayout.h
  @author S. Cranen, R. Boudewijns

  This file contains an implementation and user interface which enables automatic positioning for a graph.

*/

#ifndef SPRINGLAYOUT_H
#define SPRINGLAYOUT_H

#include <QDockWidget>
#include "ui_springlayout.h"

#include "graph.h"

namespace Graph
{
  class SpringLayoutUi;

  class SpringLayout
  {
    public:

      /**
       * @brief An enumeration that identifies the types of spring types which can be selected.
       */
      enum ForceCalculation
      {
        ltsgraph,                   ///< LTSGraph implementation.
        linearsprings               ///< Linear spring implementation.
      };
    private:
      float m_speed;                ///< The rate of change each step.
      float m_attraction;           ///< The attraction of the edges.
      float m_repulsion;            ///< The repulsion of other nodes.
      float m_natLength;            ///< The natural length of springs.
      float m_controlPointWeight;   ///< The handle repulsion wight factor.
      std::vector<Coord3D> m_nforces, m_hforces, m_lforces, m_sforces;  ///< Vector of the calculated forces..
      Coord3D m_clipMin;            ///< The minimum coordinates for any node.
      Coord3D m_clipMax;            ///< The maximum coordinates for any node.

      Graph& m_graph;               ///< The graph on which the algorithm is applied.
      SpringLayoutUi* m_ui;         ///< The user interface generated by Qt.

      Coord3D (SpringLayout::*m_forceCalculation)(const Coord3D&, const Coord3D&, float);

      /**
       * @brief Calculate the force of a linear spring between @e a and @e b.
       * @param a The first coordinate.
       * @param b The second coordinate.
       * @param ideal The ideal distance between @e a and @e b.
       */
      Coord3D forceLinearSprings(const Coord3D& a, const Coord3D& b, float ideal);

      /**
       * @brief Calculate the force of a LTSGraph "spring" between @e a and @e b.
       * @param a The first coordinate.
       * @param b The second coordinate.
       * @param ideal The ideal distance between @e a and @e b.
       */
      Coord3D forceLTSGraph(const Coord3D& a, const Coord3D& b, float ideal);
    public:

      /**
       * @brief Constructor of the algorithm for the given @e graph.
       * @param graph The graph on which the algorithm should be applied.
       */
      SpringLayout(Graph& graph);

      /**
       * @brief Destructor.
       */
      virtual ~SpringLayout();

      /**
       * @brief Calculate the forces and update the positions.
       */
      void apply();

      /**
       * @brief Set the type of the force calculation.
       * @param c The desired calculaten (LTSGraph or linear springs)
       */
      void setForceCalculation(ForceCalculation c);

      /**
       * @brief Returns the current force calculation used.
       */
      ForceCalculation forceCalculation();

      /**
       * @brief Restrains all nodes of the graph between @e min and @e max.
       * @param min The minimum coordinates for any node.
       * @param max The maximum coordinates for any node.
       */
      void setClipRegion(const Coord3D& min, const Coord3D& max);

      /**
       * @brief Returns the user interface object. If no user interface is available,
       *        one is created using the provided @e parent.
       * @param The parent of the user inferface in the case none exists yet.
       */
      SpringLayoutUi* ui(QWidget* parent = 0);

      //Getters and setters
      int speed() const { return m_speed * 10000.0; }
      int attraction() const { return m_attraction * 500.0; }
      int repulsion() const { return m_repulsion; }
      int controlPointWeight() const { return m_controlPointWeight * 1000.0; }
      int naturalTransitionLength() const { return m_natLength; }
      void setSpeed(int v) { m_speed = (float)v / 10000.0; }
      void setAttraction(int v) { m_attraction = (float)v / 500.0; }
      void setRepulsion(int v) { m_repulsion = v * m_natLength * m_natLength * m_natLength; }
      void setControlPointWeight(int v) { m_controlPointWeight = (float)v / 1000.0; }
      void setNaturalTransitionLength(int v) {
        m_repulsion /= m_natLength * m_natLength * m_natLength;
        m_natLength = v;
        m_repulsion *= m_natLength * m_natLength * m_natLength;
      }
  };

  class SpringLayoutUi : public QDockWidget
  {
      Q_OBJECT
    private:
      SpringLayout& m_layout;     ///< The layout algorithm that corresponds to this user interface.
      Ui::DockWidgetLayout m_ui;  ///< The user interface generated by Qt.
      QThread* m_thread;          ///< The thread that is used to calculate the new positions.
    public:

      /**
       * @brief Constructor.
       * @param layout The layout object this user interface corresponds to.
       * @param parent The parent widget for this user interface.
       */
      SpringLayoutUi(SpringLayout& layout, QWidget* parent=0);

      /**
       * @brief Destructor.
       */
      virtual ~SpringLayoutUi();

      /**
       * @brief Get the current state of the settings.
       */
      QByteArray settings();

      /**
       * @brief Restore the settings of the given state.
       * @param state The original state
       */
      void setSettings(QByteArray state);

    signals:

      /**
       * @brief Indicates that the thread is started or stopped.
       */
      void runningChanged(bool);

    public slots:

      /**
       * @brief Updates the attraction value.
       * @param value The new value.
       */
      void onAttractionChanged(int value);

      /**
       * @brief Updates the repulsion value.
       * @param value The new value.
       */
      void onRepulsionChanged(int value);

      /**
       * @brief Updates the speed value.
       * @param value The new value.
       */
      void onSpeedChanged(int value);

      /**
       * @brief Updates the handle weight.
       * @param value The new weight.
       */
      void onHandleWeightChanged(int value);

      /**
       * @brief Updates the natural length value.
       * @param value The new value.
       */
      void onNatLengthChanged(int value);

      /**
       * @brief Updates the force calculation.
       * @param value The new index selected.
       */
      void onForceCalculationChanged(int value);

      /**
       * @brief Starts or stops the force calculation depending on the current state.
       */
      void onStartStop();

      /**
       * @brief Starts the force calculation.
       */
      void onStarted();

      /**
       * @brief Stops the force calculation.
       */
      void onStopped();

      /**
       * @brief Starts or stops the force calculation.
       * @param active The calculation is started if this argument is true.
       */
      void setActive(bool active);
  };
}

#endif // SPRINGLAYOUT_H