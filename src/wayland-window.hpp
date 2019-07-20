#pragma once

#include <gtkmm/window.h>
#include <wayfire-shell-client-protocol.h>
#include <wlr-layer-shell-unstable-v1-client-protocol.h>
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
        zwlr_layer_shell_v1 *layer_shell = nullptr;
        zwp_virtual_keyboard_manager_v1 *vk_manager = nullptr;
    };

    class WaylandWindow : public Gtk::Window
    {
    	zwf_wm_surface_v1 *wf_surface;
        zwlr_layer_surface_v1 *layer_surface;

        void initWayfireShell(WaylandDisplay display, int x, int y, int width, int height);
        void initLayerShell(WaylandDisplay display, int width, int height);

        public:
        WaylandWindow(int x, int y, int width, int height);
    };
}
