#include "ParamViewUI.hpp"

/**
 * \file ParamViewUI.cpp
 * \ingroup lcc_ui
 */

ParamViewUI::ParamViewUI(std::shared_ptr<ParameterStorage> _parameter_storage, int _float_precision) :
    parameter_storage(_parameter_storage),
    float_precision(_float_precision)
{
    try
    {
        params_builder = Gtk::Builder::create_from_file("ui/params/params.glade");
    }
    catch(const Gtk::BuilderError &e)
    {
        std::stringstream str;
        str << "Gtk::BuilderError: " << e.what() << std::endl;
        // exit(1); Exit statements cause problems as destructors are not being called
        throw std::runtime_error(str.str()); //This should ideally be catched
    }

    params_builder->get_widget("parameters_box", parent);
    // params_builder->get_widget("parameters_flow_top", parameters_flow_top);
    // params_builder->get_widget("parameters_search", parameters_search);
    // params_builder->get_widget("parameters_box_filter", parameters_box_filter);
    // params_builder->get_widget("parameters_filter_description", parameters_filter_description);
    // params_builder->get_widget("parameters_filter", parameters_filter);
    params_builder->get_widget("parameters_list_scroll_window", parameters_list_scroll_window);
    params_builder->get_widget("parameters_list_tree", parameters_list_tree);
    params_builder->get_widget("parameters_box_buttons", parameters_box_buttons);
    params_builder->get_widget("parameters_box_delete", parameters_box_delete);
    params_builder->get_widget("parameters_button_delete", parameters_button_delete);
    params_builder->get_widget("parameters_box_edit", parameters_box_edit);
    params_builder->get_widget("parameters_button_edit", parameters_button_edit);
    params_builder->get_widget("parameters_box_create", parameters_box_create);
    params_builder->get_widget("parameters_button_create", parameters_button_create);

    assert(parent);
    // assert(parameters_flow_top);
    // assert(parameters_search);
    // assert(parameters_box_filter);
    // assert(parameters_filter_description);
    // assert(parameters_filter);
    assert(parameters_list_scroll_window);
    assert(parameters_list_tree);
    assert(parameters_box_buttons);
    assert(parameters_box_delete);
    assert(parameters_button_delete);
    assert(parameters_box_edit);
    assert(parameters_button_edit);
    assert(parameters_box_create);
    assert(parameters_button_create);

    //Create data model_record for parameters
    parameter_list_storage = Gtk::ListStore::create(model_record);
    parameters_list_tree->set_model(parameter_list_storage);

    //Use model_record, add it to the view
    parameters_list_tree->append_column("Name", model_record.column_name);
    parameters_list_tree->append_column("Type", model_record.column_type);
    parameters_list_tree->append_column("Value", model_record.column_value);
    parameters_list_tree->append_column("Info", model_record.column_info);

    //Set equal width for all columns
    for (int i = 0; i < 4; ++i) {
        parameters_list_tree->get_column(i)->set_resizable(true);
        parameters_list_tree->get_column(i)->set_min_width(20);
        parameters_list_tree->get_column(i)->set_fixed_width(50);
        parameters_list_tree->get_column(i)->set_expand(true);
    }

    //Read all param data
    read_storage_data();

    //Delete button listener (also keyboard: del)
    parameters_button_delete->signal_clicked().connect(sigc::mem_fun(this, &ParamViewUI::delete_selected_row));
    parameters_list_tree->signal_key_release_event().connect(sigc::mem_fun(this, &ParamViewUI::handle_button_released));
    parameters_list_tree->signal_button_press_event().connect(sigc::mem_fun(this, &ParamViewUI::handle_mouse_event));
    parameters_list_tree->add_events(Gdk::KEY_RELEASE_MASK);

    //Create and edit button listener
    parameter_view_unchangeable.store(false); //Window for creation should only exist once
    parameters_button_create->signal_clicked().connect(sigc::mem_fun(this, &ParamViewUI::open_param_create_window));
    parameters_button_edit->signal_clicked().connect(sigc::mem_fun(this, &ParamViewUI::open_param_edit_window));

    //Save all button listener (save the currently visible configuration to the currently open yaml file)
    //parameters_button_show->signal_clicked().connect(sigc::mem_fun(this, &ParamViewUI::save_configuration));

    //For commas in doubles, also for conversion in ParamsCreateView
    std::setlocale(LC_NUMERIC, "C");
}

void ParamViewUI::read_storage_data() {
    //Clear previously set data
    parameter_list_storage->clear();

    //Read all rows
    for (ParameterWithDescription param : parameter_storage->get_all_parameters()) {
        Gtk::TreeModel::Row row = *(parameter_list_storage->append());

        std::string name;
        std::string type;
        std::string value;
        std::string info;

        ParameterWithDescription::parameter_to_string(param, name, type, value, info, float_precision);

        Glib::ustring name_ustring(name);
        Glib::ustring type_ustring(type);
        Glib::ustring value_ustring(value);
        Glib::ustring info_ustring(info);

        row[model_record.column_name] = name_ustring;
        row[model_record.column_type] = type_ustring;
        row[model_record.column_value] = value_ustring;
        row[model_record.column_info] = info_ustring;
    }
}

Gtk::Widget* ParamViewUI::get_parent() {
    return parent;
}

bool ParamViewUI::get_selected_row(std::string &name) {
    Gtk::TreeModel::iterator iter = parameters_list_tree->get_selection()->get_selected();
    if(iter) //If anything is selected
    {
        Gtk::TreeModel::Row row = *iter;
        Glib::ustring name_ustring(row[model_record.column_name]);
        name = name_ustring;

        return true;
    }
    else return false;
}

bool ParamViewUI::handle_button_released(GdkEventKey* event) {
    if (event->type == GDK_KEY_RELEASE && event->keyval == GDK_KEY_Delete)
    {
        delete_selected_row();
        return true;
    }
    else if (event->type == GDK_KEY_RELEASE && event->keyval == GDK_KEY_Return)
    {
        open_param_edit_window();
        return true;
    }
    return false;
}

bool ParamViewUI::handle_mouse_event(GdkEventButton* button_event) {
    if (button_event->type == GDK_2BUTTON_PRESS)
    {
        open_param_edit_window();
        return true;
    }
    return false;
}

void ParamViewUI::delete_selected_row() {
    //Parameters cannot be deleted when the edit/create window is opened (or when another parameter has not yet been deleted)
    if (! parameter_view_unchangeable.exchange(true)) {
        Glib::RefPtr<Gtk::TreeSelection> selection = parameters_list_tree->get_selection();
        //Also get the name of the deleted parameter to delete it in the storage object
        std::string name;
        if (selection->get_selected()) {
            Gtk::TreeModel::Row row = *(selection->get_selected());
            Glib::ustring name_ustring(row[model_record.column_name]); //Access name of row
            name = name_ustring;
        }
        std::vector<Gtk::TreeModel::Path> paths = selection->get_selected_rows();

        // convert all of the paths to RowReferences
        std::vector<Gtk::TreeModel::RowReference> rows;
        for (Gtk::TreeModel::Path path : paths)
        {
            rows.push_back(Gtk::TreeModel::RowReference(parameter_list_storage, path));
        }

        // remove the rows from the treemodel
        for (std::vector<Gtk::TreeModel::RowReference>::iterator i = rows.begin(); i != rows.end(); i++)
        {
            Gtk::TreeModel::iterator treeiter = parameter_list_storage->get_iter(i->get_path());
            parameter_list_storage->erase(treeiter);
        }

        //Remove data from parameter storage
        parameter_storage->delete_parameter(name);

        parameter_view_unchangeable.store(false);
    }
}

using namespace std::placeholders;
void ParamViewUI::open_param_create_window() {
    //Get a "lock" for the window if it does not already exist, else ignore the user request
    //Also, a reference to the main window must already exist
    if(! parameter_view_unchangeable.exchange(true) && get_main_window) {
        make_insensitive();
        //Make the main window insensitive as well (because we do not want the user to be able to reload params etc. during edit)
        get_main_window().set_sensitive(false);
        
        create_window_open = true;
        create_window = std::unique_ptr<ParamsCreateView>(new ParamsCreateView(get_main_window(), std::bind(&ParamViewUI::window_on_close_callback, this, _1, _2), std::bind(&ParamViewUI::check_param_exists_callback, this, _1), float_precision));
    } 
    else if (!get_main_window)
    {
        std::cerr << "ERROR: Main window reference is missing, cannot create param create window";
        LCCErrorLogger::Instance().log_error("ERROR: Main window reference is missing, cannot create param create window");
    }
}

void ParamViewUI::open_param_edit_window() {
    //Get a "lock" for the window if it does not already exist, else ignore the user request
    if(! parameter_view_unchangeable.exchange(true)) {
        make_insensitive();

        //Get currently selected data
        std::string name;
        std::string type;
        std::string value;
        std::string info;

        //Only create an edit window if a row was selected
        if (get_selected_row(name)) {
            ParameterWithDescription param;
            //Get the parameter
            if (parameter_storage->get_parameter(name, param) && get_main_window) {
                //Make the main window insensitive as well (because we do not want the user to be able to reload params etc. during edit)
                get_main_window().set_sensitive(false);

                create_window = std::unique_ptr<ParamsCreateView>(new ParamsCreateView(get_main_window(), std::bind(&ParamViewUI::window_on_close_callback, this, _1, _2), std::bind(&ParamViewUI::check_param_exists_callback, this, _1), param, float_precision));
            }
            else if (!get_main_window)
            {
                make_sensitive();

                std::cerr << "ERROR: Main window reference is missing, cannot create param edit window";
                LCCErrorLogger::Instance().log_error("ERROR: Main window reference is missing, cannot create param edit window");
            }
        }
        else {
            make_sensitive();
            parameter_view_unchangeable.store(false);
        }
    } 
}

void ParamViewUI::window_on_close_callback(ParameterWithDescription param, bool valid_parameter) {
    if (valid_parameter) {
        std::string name;
        std::string type;
        std::string value;
        std::string info;

        ParameterWithDescription::parameter_to_string(param, name, type, value, info, float_precision);

        Glib::ustring name_ustring = name;
        Glib::ustring type_ustring = type;
        Glib::ustring value_ustring = value;
        Glib::ustring info_ustring = info;

        Gtk::TreeModel::Row row;

        //If a parameter was modified, get its current row or else create a new row
        Gtk::TreeModel::iterator iter = parameters_list_tree->get_selection()->get_selected();
        if(iter && !create_window_open) //If anything is selected and if param modification window was opened
        {
            row = *iter;
        }
        else if (create_window_open) { //Create a new parameter only if it does not already exist
            row = *(parameter_list_storage->append());
        }

        if (create_window_open || iter) {
            row[model_record.column_name] = name_ustring;
            row[model_record.column_type] = type_ustring;
            row[model_record.column_value] = value_ustring;
            row[model_record.column_info] = info_ustring;

            parameter_storage->set_parameter(name, param);
        }
    }

    //Reset variables so that new windows can be opened etc
    parameter_view_unchangeable.store(false);
    create_window_open = false;
    make_sensitive();

    //Make the main window sensitive again
    if (get_main_window) {
        get_main_window().set_sensitive(true);
    }
    else if (!get_main_window)
    {
        std::cerr << "ERROR: Main window reference is missing in ParamView";
        LCCErrorLogger::Instance().log_error("ERROR: Main window reference is missing in ParamView");
    }
}

bool ParamViewUI::check_param_exists_callback(std::string name) {
    ParameterWithDescription param;
    return parameter_storage->get_parameter(name, param);
}

//Menu bar item handlers

void ParamViewUI::params_reload_handler() {
    params_load_file_handler("");
}

void ParamViewUI::params_save_handler() {
    parameter_storage->storeFile();
}

void ParamViewUI::params_save_as_handler(std::string filename) {
    parameter_storage->storeFile(filename);
}

void ParamViewUI::params_load_file_handler(std::string filename) {
    //Try to load the file; it might not be conformant to a param YAML file, in that case a domain error is thrown
    std::string error_string = "";

    try {
        if (filename == "")
        {
            parameter_storage->loadFile();
        }
        else
        {
            parameter_storage->loadFile(filename);
        }
    }
    catch (const std::domain_error& err)
    {
        error_string = err.what();
    }
    catch (const std::exception& err)
    {
        error_string = err.what();
    }

    if (error_string != "")
    {
        //Create new window
        error_dialog = std::make_shared<Gtk::MessageDialog>(
            get_main_window(),
            error_string,
            false,
            Gtk::MessageType::MESSAGE_INFO,
            Gtk::ButtonsType::BUTTONS_CLOSE,
            false
        );
    
        //Connect new window with parent, show window
        error_dialog->set_transient_for(get_main_window());
        error_dialog->property_destroy_with_parent().set_value(true);
        error_dialog->show();

        //Callback for closing
        error_dialog->signal_response().connect(
            [this] (auto response)
            {
                if (response == Gtk::ResponseType::RESPONSE_CLOSE)
                {
                    error_dialog->close();
                }
            }
        );
    }

    //Load everything that could be loaded before an error was thrown or the whole file if no error was thrown
    read_storage_data();
}

// void ParamViewUI::params_load_multiple_files_handler() {

// }

// void ParamViewUI::params_load_params_handler() {

// }

void ParamViewUI::make_sensitive() {
    parent->set_sensitive(true);
}

void ParamViewUI::make_insensitive() {
    parent->set_sensitive(false);
}

void ParamViewUI::set_main_window_callback(std::function<Gtk::Window&()> _get_main_window)
{
    get_main_window = _get_main_window;
}
