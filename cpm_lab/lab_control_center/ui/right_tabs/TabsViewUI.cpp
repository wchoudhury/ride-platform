#include "TabsViewUI.hpp"

/**
 * \file TabsViewUI.cpp
 * \ingroup lcc_ui
 */

TabsViewUI::TabsViewUI
(
    std::shared_ptr<SetupViewUI> setupViewUi, 
    std::shared_ptr<VehicleManualControlUi> vehicleManualControlUi, 
    std::shared_ptr<ParamViewUI> paramViewUI, 
    std::shared_ptr<TimerViewUI> timerViewUi, 
    std::shared_ptr<LCCErrorViewUI> lccErrorViewUi,
    std::shared_ptr<LoggerViewUI> loggerViewUi,
    std::shared_ptr<CommonroadViewUI> commonroadViewUi
) 
 {
    tabs_builder = Gtk::Builder::create_from_file("ui/right_tabs/right_tabs.glade");

    tabs_builder->get_widget("right_notebook", right_notebook);

    assert(right_notebook);

    Glib::ustring setup_label("Setup");
    Glib::ustring commonroad_label("Commonroad");
    Glib::ustring manual_control_label("Manual Control");
    Glib::ustring parameters_label("Parameters");
    Glib::ustring timer_label("Timer");
    Glib::ustring logger_label("Logs");
    Glib::ustring lcc_error_label("LCC Errors");

    right_notebook->insert_page(*(setupViewUi->get_parent()), setup_label, -1);
    right_notebook->insert_page(*(commonroadViewUi->get_parent()), commonroad_label, -1);
    right_notebook->insert_page(*(vehicleManualControlUi->get_parent()), manual_control_label, -1);
    right_notebook->insert_page(*(paramViewUI->get_parent()), parameters_label, -1);
    right_notebook->insert_page(*(timerViewUi->get_parent()), timer_label, -1);
    right_notebook->insert_page(*(loggerViewUi->get_parent()), logger_label, -1);
    right_notebook->insert_page(*(lccErrorViewUi->get_parent()), lcc_error_label, -1);
}

Gtk::Widget* TabsViewUI::get_parent() {
    return right_notebook;
}