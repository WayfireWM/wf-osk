#include "wayland-window.hpp"
#include <iostream>
#include <gdk/gdkwayland.h>
#include <wayland-client.h>

namespace wf
{
    // listeners
    static void registry_add_object(void *data, struct wl_registry *registry, uint32_t name,
        const char *interface, uint32_t version)
    {
        auto display = static_cast<WaylandDisplay*> (data);

        if (strcmp(interface, wl_seat_interface.name) == 0 && !display->seat)
        {
            display->seat = (wl_seat*) wl_registry_bind(registry, name,
                &wl_seat_interface, std::min(version, 1u));
        }

        if (strcmp(interface, zwf_shell_manager_v1_interface.name) == 0)
        {
            display->wf_manager =
                (zwf_shell_manager_v1*) wl_registry_bind(registry, name,
                    &zwf_shell_manager_v1_interface,
                    std::min(version, 1u));
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
        wl_display_roundtrip(display);

        if (!vk_manager || !seat || !wf_manager)
        {
            std::cerr << "Compositor doesn't support the virtual-keyboard-v1 "
                << "and/or the wayfire-shell protocols, exiting" << std::endl;
            std::exit(-1);
        }
    }

    WaylandDisplay& WaylandDisplay::get()
    {
        static WaylandDisplay instance;
        return instance;
    }

    WaylandWindow::WaylandWindow(int x, int y, int width, int height)
        : Gtk::Window()
    {
        auto display = WaylandDisplay::get();

        /* Trick: first show the window, get frame size, then subtract it again */
        this->set_size_request(width, height);
        this->set_default_size(width, height);
        this->set_type_hint(Gdk::WINDOW_TYPE_HINT_DOCK);
        this->show_all();

        auto gdk_window = this->get_window()->gobj();
        auto surface = gdk_wayland_window_get_wl_surface(gdk_window);

        if (!surface)
        {
            std::cerr << "Error: created window was not a wayland surface" << std::endl;
            std::exit(-1);
        }

        wm_surface = zwf_shell_manager_v1_get_wm_surface(display.wf_manager,
            surface, ZWF_WM_SURFACE_V1_ROLE_DESKTOP_WIDGET, NULL);
        zwf_wm_surface_v1_set_keyboard_mode(wm_surface,
            ZWF_WM_SURFACE_V1_KEYBOARD_FOCUS_MODE_NO_FOCUS);
        zwf_wm_surface_v1_configure(wm_surface, x, y);
    }
}
