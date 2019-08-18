#include "wayland-window.hpp"
#include <iostream>
#include <algorithm>
#include <gdk/gdkwayland.h>
#include <wayland-client.h>
#include <gtk-layer-shell.h>

namespace wf
{
    // listeners
    static void registry_add_object(void *data, struct wl_registry *registry,
        uint32_t name, const char *interface, uint32_t version)
    {
        auto display = static_cast<WaylandDisplay*> (data);

        if (strcmp(interface, zwf_shell_manager_v2_interface.name) == 0)
        {
            display->zwf_manager =
                (zwf_shell_manager_v2*) wl_registry_bind(registry, name,
                    &zwf_shell_manager_v2_interface, std::min(version, 1u));
        }

        if (strcmp(interface, zwp_virtual_keyboard_manager_v1_interface.name) == 0)
        {
            display->vk_manager = (zwp_virtual_keyboard_manager_v1*)
                wl_registry_bind(registry, name,
                    &zwp_virtual_keyboard_manager_v1_interface, 1u);
        }
    }

    static void registry_remove_object(void *data, struct wl_registry *registry, uint32_t name)
    {
        /* no-op */
    }

    static struct wl_registry_listener registry_listener =
    {
        &registry_add_object,
        &registry_remove_object
    };

    WaylandDisplay::WaylandDisplay()
    {
        auto gdk_display = gdk_display_get_default();
        auto display = gdk_wayland_display_get_wl_display(gdk_display);

        if (!display)
        {
            std::cerr << "Failed to connect to wayland display!"
                << " Are you sure you are running a wayland compositor?" << std::endl;
            std::exit(-1);
        }

        wl_registry *registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, this);
        wl_display_dispatch(display);
        wl_display_roundtrip(display);

        if (!vk_manager)
        {
            std::cerr << "Compositor doesn't support the virtual-keyboard-v1 "
                << "protocol, exiting" << std::endl;
            std::exit(-1);
        }
    }

    WaylandDisplay& WaylandDisplay::get()
    {
        static WaylandDisplay instance;
        return instance;
    }

    int32_t WaylandWindow::check_anchor(std::string anchor)
    {
        std::transform(anchor.begin(), anchor.end(), anchor.begin(), ::tolower);

        int32_t parsed_anchor = -1;
        if (anchor.compare("top") == 0)
        {
            parsed_anchor = GTK_LAYER_SHELL_EDGE_TOP;
        } else if (anchor.compare("bottom") == 0)
        {
            parsed_anchor = GTK_LAYER_SHELL_EDGE_BOTTOM;
        } else if (anchor.compare("left") == 0)
        {
            parsed_anchor = GTK_LAYER_SHELL_EDGE_LEFT;
        } else if (anchor.compare("right") == 0)
        {
            parsed_anchor = GTK_LAYER_SHELL_EDGE_RIGHT;
        }

        return parsed_anchor;
    }

    void WaylandWindow::init(int width, int height, std::string anchor)
    {
        gtk_layer_init_for_window(this->gobj());
        gtk_layer_set_layer(this->gobj(), GTK_LAYER_SHELL_LAYER_TOP);
        auto layer_anchor = check_anchor(anchor);
        if (layer_anchor > -1)
        {
            gtk_layer_set_anchor(this->gobj(),
                (GtkLayerShellEdge)layer_anchor, true);
        }

        this->set_size_request(width, height);
        this->show_all();
        auto gdk_window = this->get_window()->gobj();
        auto surface = gdk_wayland_window_get_wl_surface(gdk_window);

        if (surface && WaylandDisplay::get().zwf_manager)
        {
            this->wf_surface = zwf_shell_manager_v2_get_wf_surface(
                WaylandDisplay::get().zwf_manager, surface);
        }
    }

    WaylandWindow::WaylandWindow(int width, int height, std::string anchor)
        : Gtk::Window()
    {
        /* Trick: first show the window, get frame size, then subtract it again */
        this->set_type_hint(Gdk::WINDOW_TYPE_HINT_DOCK);
        init(width, height, anchor);
    }
}
