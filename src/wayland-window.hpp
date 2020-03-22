#pragma once

#include <gtkmm/hvbox.h>
#include <gtkmm/button.h>
#include <gtkmm/window.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/gesturedrag.h>
#include <gtkmm/headerbar.h>
#include <wayfire-shell-unstable-v2-client-protocol.h>
#include <virtual-keyboard-unstable-v1-client-protocol.h>

#define OSK_SPACING 8
static constexpr int32_t ANCHOR_PINNED_BOTTOM = -2;

namespace wf
{
    class WaylandDisplay
    {
        WaylandDisplay();

        public:
        static WaylandDisplay& get();

        zwf_shell_manager_v2 *zwf_manager = nullptr;
        zwp_virtual_keyboard_manager_v1 *vk_manager = nullptr;
    };

    class WaylandWindow : public Gtk::Window
    {
        zwf_surface_v2 *wf_surface = nullptr;

        Gtk::Widget* current_widget = nullptr;
        Glib::RefPtr<Gtk::GestureDrag> headerbar_drag;
        Gtk::EventBox drag_box;
        Gtk::Button close_button;
        Gtk::HBox headerbar_box;
        Gtk::VBox layout_box;

        int32_t check_anchor(std::string anchor);
        void init(int width, int height, std::string anchor);

      public:
        WaylandWindow(int width, int height, std::string anchor);
        void set_widget(Gtk::Widget& w);
    };
}
