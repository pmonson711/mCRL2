#include "mcrl2/utilities/input_tool.h"
#include "mcrl2/utilities/qt_tool.h"
#include "mcrl2/utilities/rewriter_tool.h"
#include "mainwindow.h"

using namespace mcrl2::utilities;

typedef qt::qt_tool<tools::rewriter_tool<tools::input_tool> > lpsxsim_base;

class lpsxsim_tool : public lpsxsim_base
{
   
  protected:

    bool m_do_not_use_dummies;

    void add_options(interface_description& desc)
    {
      lpsxsim_base::add_options(desc);
      desc.add_option("nodummy", "do not replace global variables in the LPS with dummy values", 'y');
    }

    void parse_options(const command_line_parser& parser)
    {
      lpsxsim_base::parse_options(parser);
      m_do_not_use_dummies = 0 < parser.options.count("nodummy");
    }


  public:
    lpsxsim_tool():
      lpsxsim_base("LpsXSim",
        "Ruud Koolen",
        "graphical simulation of an LPS",
        "Simulates linear process descriptions in a graphical environment. If INFILE is supplied it will be loaded into the simulator.",
        "Simulator for linear process specifications.",
        "http://mcrl2.org/release/user_manual/tools/lpsxsim.html")
    {}

    bool run()
    {
      qRegisterMetaType<QSemaphore *>("QSemaphore *");

      MainWindow *window = new MainWindow(rewrite_strategy(),m_do_not_use_dummies);

      if (!m_input_filename.empty())
      {
        window->openSpecification(QString::fromStdString(m_input_filename));
      }

      return show_main_window(window);
    }
};

int main(int argc, char *argv[])
{
  return lpsxsim_tool().execute(argc, argv);
}