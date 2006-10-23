#include <map>
#include <set>
#include <stack>
#include <utility>

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/ref.hpp>

#include "gui_main.h"
#include "gui_project.h"
#include "gui_resources.h"
#include "gui_miscellaneous.h"
#include "project_manager.h"
#include "tool_manager.h"
#include "gui_dialog_base.h"
#include "gui_tool_display.h"
#include "gui_dialog_project.h"
#include "gui_dialog_processor.h"

#include <wx/choicdlg.h>
#include <wx/filedlg.h>

#define cmID_EDIT      (wxID_HIGHEST + 1)
#define cmID_REMOVE    (wxID_HIGHEST + 2)
#define cmID_REFRESH   (wxID_HIGHEST + 3)
#define cmID_RENAME    (wxID_HIGHEST + 4)
#define cmID_CLEAN     (wxID_HIGHEST + 5)
#define cmID_CONFIGURE (wxID_HIGHEST + 6)
#define cmID_DETAILS   (wxID_HIGHEST + 7)
#define cmID_TOOLS     (wxID_HIGHEST + 8)

namespace squadt {
  namespace GUI {

    project::builder::builder() : timer(this, wxID_ANY) {
      Connect(wxEVT_TIMER, wxTimerEventHandler(builder::process));

      timer.Start(50);
    }
    
    void project::builder::process(wxTimerEvent&) {
      static volatile bool r = false;
      
      if (!r) {
        r = true;

        while (0 < tasks.size()) {
          boost::function < void () > task = tasks.front();
       
          tasks.pop_front();
       
          /* Execute task */
          task();
        }

        r = false;
      }
    }
    
    void project::builder::schedule_update(boost::function < void () > l) {
      tasks.push_back(l);
    }
        
    project::~project() {
      manager.reset();
    }

    void project::store() {
      manager->write();
    }

    /**
     * \param p the parent window
     * \param l is the path
     * \param d is a description for this project
     * \param b whether or not the project should be created anew
     *
     * Creates a project_manager object that is either:
     *  - read from l, if l is a project store
     *  - the default project_manager, and l is the new project store
     **/
    project::project(main* p, const boost::filesystem::path& l, const std::string& d, bool b) :
                                wxSplitterWindow(p, wxID_ANY), manager(project_manager::create(l, b)), registry(*p->registry) {

      if (!d.empty()) {
        manager->set_description(d);
        manager->write();
      }

      build();
    }

    /* Updates the status of files in the object view */
    void project::set_object_status(processor::wptr const& w, const wxTreeItemId s) {
      processor::sptr g = w.lock();

      if (g.get() != 0) {
        wxTreeItemIdValue cookie; // For wxTreeCtrl traversal

        for (wxTreeItemId j = object_view->GetFirstChild(s, cookie); j.IsOk(); j = object_view->GetNextChild(s, cookie)) {
          project::tool_data* instance_data = static_cast < project::tool_data* > (object_view->GetItemData(j));

          if (instance_data->get_processor().get() == g.get()) {
            object_view->SetItemImage(j, instance_data->get_object()->status);
          }
        }

        gui_builder.schedule_update(boost::bind(&wxTreeCtrl::Update, object_view));
      }
    }

    void project::load_views() {
      wxTreeItemId root_item_id = object_view->GetRootItem();

      object_view->DeleteChildren(root_item_id);

      project_manager::processor_iterator i = manager->get_processor_iterator();

      /* Update view: First add objects that are not generated from other objects in the project */
      while (i.valid() && (*i)->number_of_inputs() == 0) {
        for (processor::output_object_iterator j = (*i)->get_output_iterator(); j.valid(); ++j) {
          wxTreeItemId item = object_view->AppendItem(root_item_id,
                      wxString(boost::filesystem::path((*j)->location).leaf().c_str(), wxConvLocal), (*j)->status);

          object_view->SetItemData(item, new tool_data(*this, j.pointer()));

          object_view->EnsureVisible(item);
        }

        ++i;
      }

      std::stack < wxTreeItemId > id_stack;

      id_stack.push(root_item_id);

      while (!id_stack.empty()) {
        wxTreeItemIdValue cookie;             // For wxTreeCtrl traversal
        wxTreeItemId      c = id_stack.top(); // The current node

        id_stack.pop();

        for (wxTreeItemId j = object_view->GetFirstChild(c, cookie); j.IsOk(); j = object_view->GetNextChild(c, cookie)) {

          processor::sptr t = static_cast < tool_data* > (object_view->GetItemData(j))->get_processor();

          /* Look for inputs, *i is generated by *t (assuming t.get() != 0) */
          for (project_manager::processor_iterator k = i; k.valid(); ++k) {
            for (processor::input_object_iterator l = (*k)->get_input_iterator(); l.valid(); ++l) {
              if (*l != 0 && (*l)->generator.lock() == t) {
                for (processor::output_object_iterator m = (*k)->get_output_iterator(); m.valid(); ++m) {
              
                  wxTreeItemId item = object_view->AppendItem(j,
                              wxString(boost::filesystem::path((*m)->location).leaf().c_str(), wxConvLocal), (*m)->status);

                  object_view->SetItemData(item, new tool_data(*this, m.pointer()));
              
                  object_view->EnsureVisible(item);
                }
              }
            }

          }

          t->check_status(false);

          processor::monitor& m = *(t->get_monitor());

          /* Register handler for status changes */
          m.set_status_handler(boost::bind(&project::set_object_status, this, t, c));

          /* Register handler to on update the object view after process termination */
          m.on_completion(boost::bind(&project::process_configuration, this, c, t));

          if (object_view->ItemHasChildren(j)) {
            id_stack.push(j);
          }
        }
      }
    }

    void project::build() {
      process_display_view = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL|wxTAB_TRAVERSAL);
      object_view          = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        (wxTR_LINES_AT_ROOT|wxTR_HIDE_ROOT|wxTR_HAS_BUTTONS|wxTR_SINGLE|wxSUNKEN_BORDER)&(~wxTR_EDIT_LABELS));

      SetMinimumPaneSize(1);
      SplitVertically(object_view, process_display_view);
      SetSashPosition(250);

      process_display_view->SetSizer(new wxBoxSizer(wxVERTICAL));
      process_display_view->SetScrollRate(10, 10);

      object_view->SetImageList(format_icon_list);
      object_view->AddRoot(wxT("Store"));

      /* Connect event handlers */
      object_view->Connect(wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler(project::on_tree_item_activate), 0, this);
      object_view->Connect(wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler(project::on_tree_item_activate), 0, this);
      object_view->Connect(wxEVT_COMMAND_TREE_END_LABEL_EDIT, wxTreeEventHandler(project::on_object_name_edited), 0, this);

      Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(project::on_context_menu_select));
    }

    /**
     * \param[in] p the processor::monitor that is connected to the associated tool process
     * \param[in] t the title for the tool display
     **/
    GUI::tool_display* project::add_tool_display(boost::shared_ptr < processor::monitor > p, std::string const& t) {
      wxSizer* s = process_display_view->GetSizer();

      GUI::tool_display* display = new GUI::tool_display(process_display_view, this, p);

      s->Insert(0, display, 0, wxEXPAND|wxALL, 2);
      s->Layout();

      display->set_title(wxString(t.c_str(), wxConvLocal));

      return (display);
    }
            
    /**
     * \param e a reference to a tree event object
     **/
    void project::on_tree_item_activate(wxTreeEvent& e) {
      if (object_view->GetRootItem() != e.GetItem()) {
        spawn_context_menu(*(static_cast < tool_data* > (object_view->GetItemData(e.GetItem()))));
      }
      else {
        project::add();
      }
    }

    /**
     * \param e a reference to a tree event object
     **/
    void project::on_object_name_edited(wxTreeEvent& e) {
      wxTreeItemId                       s = e.GetItem();
      processor::sptr                    p = reinterpret_cast < tool_data* > (object_view->GetItemData(s))->get_processor();
      processor::object_descriptor::sptr t = reinterpret_cast < tool_data* > (object_view->GetItemData(s))->get_object();

      if (!e.GetLabel().IsEmpty()) {
        p->rename_output(t->location, std::string(e.GetLabel().fn_str()));
      }
      else {
        e.Veto();
      }
    }

    void project::add() {
      dialog::add_to_project dialog(this, wxString(manager->get_project_store().c_str(), wxConvLocal));

      if (dialog.ShowModal()) {
        /* File does not exist in project directory */
        processor* p = manager->import_file(
                              boost::filesystem::path(dialog.get_source()), 
                              boost::filesystem::path(dialog.get_destination()).leaf()).get();

        /* Add to the new project */
        wxTreeItemId i = object_view->AppendItem(object_view->GetRootItem(), wxString(dialog.get_name().c_str(), wxConvLocal), processor::object_descriptor::original);

        object_view->SetItemData(i, new tool_data(*this, p->get_output_iterator().pointer()));
        object_view->EnsureVisible(i);

        manager->write();
      }
    }

    /* Helper function */
    void project::prepare_tool_display(processor* p) {
      processor::output_object_iterator t = p->get_output_iterator();

      add_tool_display(p->get_monitor(), p->get_tool()->get_name() + " : " + boost::filesystem::path((*t)->location).leaf());
    }

    void project::update() {
      manager->update(boost::bind(&project::prepare_tool_display, this, _1));
    }

    /* Helper class for associating a tool input combination with a menu item */
    class cmMenuItem : public wxMenuItem {

      public:

        const tool::sptr               the_tool;

        const tool::input_combination* input_combination;

      public:

        cmMenuItem(wxMenu* m, int id, const wxString& t, const tool::sptr& tp, const tool::input_combination* ic) :
                                        wxMenuItem(m, id, t), the_tool(tp), input_combination(ic) {
        }
    };

    /**
     * \param n an tool_data object used to establish which tools to add to the menu
     **/
    void project::spawn_context_menu(tool_data& n) {
      using namespace boost;
      using namespace squadt::miscellaneous;

      size_t separator_position     = 3;
      bool   editable               = registry.has_registered_command(n.get_object()->format);
      bool   generated              = (0 < n.get_processor()->number_of_inputs());
      bool   show_update_operations = !n.get_processor()->is_active();

      wxMenu  context_menu;

      context_menu.Append(cmID_EDIT, wxT("Edit"))->Enable(show_update_operations && editable);
      context_menu.Append(cmID_RENAME, wxT("Rename"))->Enable(show_update_operations);
      context_menu.Append(cmID_REMOVE, wxT("Remove"))->Enable(show_update_operations);

      if (generated) {
        context_menu.Append(cmID_REFRESH, wxT("Refresh"))->Enable(show_update_operations);
        context_menu.Append(cmID_CONFIGURE, wxT("Configure"))->Enable(show_update_operations);
        context_menu.Append(cmID_CLEAN, wxT("Clean"))->Enable(show_update_operations);

        separator_position += 3;
      }

      /* wxWidgets identifier for menu items */
      int identifier = cmID_TOOLS;

      std::string format = n.get_object()->format;

      type_registry::tool_sequence range = static_cast < main* > (GetParent())->registry->tools_by_mime_type(format);

      std::string last_seen_category;
      wxMenu*     target_menu = 0;

      BOOST_FOREACH(type_registry::tool_sequence::value_type i, range) {

        if (last_seen_category != i.first) {
          target_menu = new wxMenu();

          last_seen_category = i.first;

          context_menu.Append(identifier++, wxString(i.first.c_str(), wxConvLocal), target_menu);
        }

        cmMenuItem* new_menu_item = new cmMenuItem(target_menu, identifier++, 
                                  wxString(i.second->get_name().c_str(), wxConvLocal),
                                  i.second, i.second->find_input_combination(i.first, format));

        target_menu->Append(new_menu_item);
      }

      context_menu.AppendSeparator();

      if (!context_menu.FindItemByPosition(separator_position)->IsSeparator()) {
        context_menu.InsertSeparator(separator_position);
      }

      context_menu.Append(cmID_DETAILS, wxT("Details"));

      PopupMenu(&context_menu);
    }

    /**
     * \param e a reference to a menu event object
     **/
    void project::on_context_menu_select(wxCommandEvent& e) {
      wxTreeItemId                       s = object_view->GetSelection();
      processor::sptr                    p = reinterpret_cast < tool_data* > (object_view->GetItemData(s))->get_processor();
      processor::object_descriptor::sptr t = reinterpret_cast < tool_data* > (object_view->GetItemData(s))->get_object();

      switch (e.GetId()) {
        case cmID_EDIT:
          p->edit(registry.get_registered_command(t->format, t->location).get());
          break;
        case cmID_REMOVE:
          manager->remove(p.get());

          object_view->Delete(s);
          break;
        case cmID_RENAME:
          object_view->EditLabel(s);
          break;
        case cmID_REFRESH:
          p->flush_outputs();

          /* Attach tool display */
          add_tool_display(p->get_monitor(), p->get_tool()->get_name() + " : " + boost::filesystem::path(t->location).leaf());

          p->update();
          break;
        case cmID_CLEAN:
          p->flush_outputs();
          break;
        case cmID_DETAILS: {
            dialog::processor_details dialog(this, wxString(manager->get_project_store().c_str(), wxConvLocal), p);

            dialog.set_name(object_view->GetItemText(s));

            if (object_view->GetItemParent(s) == object_view->GetRootItem()) {
              dialog.show_tool_selector(false);
              dialog.show_input_objects(false);
            }
            else {
              /* Add the main input (must exist) */
              dialog.populate_tool_list(static_cast < main* > (GetParent())->registry->
                                        tools_by_mime_type(p->get_input_combination()->format));

              if (p->get_tool().get() != 0) {
                dialog.select_tool(p->get_input_combination(), p->get_tool()->get_name());
              }

              dialog.allow_tool_selection(false);
            }

            if (dialog.ShowModal()) {
            }
          }
          break;
        case cmID_CONFIGURE:
            /* Attach tool display */
            add_tool_display(p->get_monitor(), p->get_tool()->get_name() + " : " + boost::filesystem::path(t->location).leaf());

            /* Start tool configuration phase */
            p->reconfigure();
          break;
        default: {
            /* Assume that a tool was selected */
            wxMenu*     menu      = reinterpret_cast < wxMenu* > (e.GetEventObject());
            cmMenuItem* menu_item = reinterpret_cast < cmMenuItem* > (menu->FindItem(e.GetId()));

            /* Create a temporary processor */
            processor::sptr tp = processor::create(manager, menu_item->the_tool);

            /* Attach the new processor by relating it to t */
            tp->append_input(t);

            /* Attach tool display */
            add_tool_display(tp->get_monitor(), menu_item->the_tool->get_name() + " : " + boost::filesystem::path(t->location).leaf());

            /* Register handler to on update the object view after process termination */
            tp->get_monitor()->on_completion(boost::bind(&project::process_configuration, this, s, tp));

            /* Start tool configuration phase */
            tp->configure(menu_item->input_combination, boost::filesystem::path(t->location));
          }
          break;
      }
    }

    /**
     * \param[in] s the id of the tree item to which the objects should be attached
     * \param[in] tp the processor of which to read objects
     **/
    void project::process_configuration(wxTreeItemId s, processor::sptr tp) {
      if (0 < tp->number_of_outputs()) {
        /* Add the processor to the project */
        if (add_outputs_as_objects(s, tp)) {
          processor::monitor& m = *(tp->get_monitor());

          m.set_status_handler(boost::bind(&project::set_object_status, this, tp, s));
        }
      }
    }

    /**
     * \param[in] s the id of the tree item to which the objects should be attached
     * \param[in] tp the processor of which to read objects
     *
     * \return whether or not there were no conflicts
     **/
    bool project::add_outputs_as_objects(wxTreeItemId s, processor::sptr tp) {
      std::set < wxString >                                      existing;
      std::map < wxString, processor::object_descriptor::sptr >  string_to_object;

      /* Gather existing objects */
      wxTreeItemIdValue cookie;   // For wxTreeCtrl traversal

      for (wxTreeItemId j = object_view->GetFirstChild(s, cookie); j.IsOk(); j = object_view->GetNextChild(s, cookie)) {
        processor::object_descriptor::sptr object = static_cast < tool_data* > (object_view->GetItemData(j))->get_object();

        if (object.get() != 0) {
          string_to_object[object_view->GetItemText(j)] = object;

          existing.insert(object_view->GetItemText(j));
        }
      }

      std::auto_ptr < project_manager::conflict_list > conflicts(manager->get_conflict_list(tp));

      if (tp->number_of_outputs() == 0 || 0 < conflicts->size()) {
        manager->remove(tp.get());
      }

      if (0 < conflicts->size()) {
        for (project_manager::conflict_list::iterator j = conflicts->begin(); j != conflicts->end(); ++j) {
          gui_builder.schedule_update(boost::bind(&project::report_conflict, this,
              wxString(boost::str(boost::format("The file %s was already part of the project but has now also been produced by %s."
                "The original file will be restored.") % (*j)->location % tp->get_tool()->get_name()).c_str(), wxConvLocal)));

          (*j)->self_check(*manager);

          boost::shared_ptr < processor > g((*j)->generator.lock());

          if (g.get() != 0) {
            manager->update_status(g.get());
          }
        }
      }
      else {
        for (processor::output_object_iterator j = tp->get_output_iterator(); j.valid(); ++j) {
          wxString new_object = wxString(boost::filesystem::path((*j)->location).leaf().c_str(), wxConvLocal);
       
          if (existing.find(new_object) == existing.end()) {
            add_to_object_view(s, j.pointer());
          }
        }
      }

      return (conflicts->size() == 0);
    }

    void project::report_conflict(wxString const& s) {
      wxMessageDialog dialog(this, s, wxT("Warning: file overwritten"), wxOK);
      
      dialog.ShowModal();
    }

    /**
     * \param[in] s the tree item to connect to
     * \param[in] t the object to associate the new item with
     **/
    void project::add_to_object_view(wxTreeItemId& s, processor::object_descriptor::sptr t) {
      wxTreeItemId item = object_view->AppendItem(s, wxString(boost::filesystem::path(t->location).leaf().c_str(), wxConvLocal), t->status);

      object_view->SetItemData(item, new tool_data(*this, t));
      object_view->EnsureVisible(item);
    }

    wxString project::get_name() const {
      return (wxString(manager->get_name().c_str(), wxConvLocal));
    }
  }
}
