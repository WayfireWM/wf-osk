#pragma once

#include <gtkmm/window.h>
#include <wayfire-shell-client-protocol.h>
#include <virtual-keyboard-unstable-v1-client-protocol.h>

namespace wf
{
    class WaylandDisplay
    {
        WaylandDisplay();

        public:
        static WaylandDisplay& get();

        wl_seat *seat = nullptr;
        zwf_shell_manager_v1 *wf_manager = nullptr;
        zwp_virtual_keyboard_manager_v1 *vk_manager = nullptr;
    };

    class WaylandWindow : public Gtk::Window
    {
        zwf_wm_surface_v1 *wm_surface;
        public:
        WaylandWindow(int x, int y, int width, int height);
    };
}
