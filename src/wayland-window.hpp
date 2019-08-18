#pragma once

#include <gtkmm/window.h>
#include <wayfire-shell-unstable-v2-client-protocol.h>
#include <virtual-keyboard-unstable-v1-client-protocol.h>

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
        zwf_surface_v2 *wf_surface;

        int32_t check_anchor(std::string anchor);
        void init(int width, int height, std::string anchor);

        public:
        WaylandWindow(int width, int height, std::string anchor);
    };
}
