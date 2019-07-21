#include "wayland-window.hpp"
#include <iostream>
#include <algorithm>
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
                    &zwf_shell_manager_v1_interface, std::min(version, 1u));
        }

        if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
        {
            display->layer_shell =
                (zwlr_layer_shell_v1*) wl_registry_bind(registry, name,
                    &zwlr_layer_shell_v1_interface, std::min(version, 1u));
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

    static void layer_shell_handle_configure(void *data,
            struct zwlr_layer_surface_v1 *zwlr_layer_surface, uint32_t serial,
            uint32_t width, uint32_t height)
    {
        zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface, serial);
    }

    static void layer_shell_handle_close(void *data,
            struct zwlr_layer_surface_v1 *surface)
    {
        zwlr_layer_surface_v1_destroy(surface);

        Gtk::Application *app = (Gtk::Application *) data;
        app->quit();
    }

    static struct zwlr_layer_surface_v1_listener layer_surface_listener =
    {
        &layer_shell_handle_configure,
        &layer_shell_handle_close
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

        if (!vk_manager || !seat || (!wf_manager && !layer_shell))
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

    uint32_t WaylandWindow::check_anchor_for_wayfire_shell(int width,
            int height, std::string anchor)
    {
        if (anchor.empty())
        {
            return 0;
        }

        std::transform(anchor.begin(), anchor.end(), anchor.begin(), ::tolower);

        uint32_t parsed_anchor = 0;
        if (anchor.compare("top") == 0)
        {
            parsed_anchor = ZWF_WM_SURFACE_V1_ANCHOR_EDGE_TOP;
        } else if (anchor.compare("bottom") == 0)
        {
            parsed_anchor = ZWF_WM_SURFACE_V1_ANCHOR_EDGE_BOTTOM;
        } else if (anchor.compare("left") == 0)
        {
            parsed_anchor = ZWF_WM_SURFACE_V1_ANCHOR_EDGE_LEFT;
        } else if (anchor.compare("right") == 0)
        {
            parsed_anchor = ZWF_WM_SURFACE_V1_ANCHOR_EDGE_RIGHT;
        }

        return parsed_anchor;
    }

    uint32_t WaylandWindow::check_anchor_for_layer_shell(int width, int height,
            std::string anchor)
    {
        if (anchor.empty())
        {
            return 0;
        }

        std::transform(anchor.begin(), anchor.end(), anchor.begin(), ::tolower);

        uint32_t parsed_anchor = 0;
        if (anchor.compare("top") == 0)
        {
            parsed_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
        } else if (anchor.compare("bottom") == 0)
        {
            parsed_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
        } else if (anchor.compare("left") == 0)
        {
            parsed_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
        } else if (anchor.compare("right") == 0)
        {
            parsed_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
        }

        return parsed_anchor;
    }

    void WaylandWindow::init_wayfire_shell(WaylandDisplay display, int x, int y,
            int width, int height, std::string anchor)
    {
        this->show_all();
        auto gdk_window = this->get_window()->gobj();
        auto surface = gdk_wayland_window_get_wl_surface(gdk_window);

        if (!surface)
        {
            std::cerr << "Error: created window was not a wayland surface" << std::endl;
            std::exit(-1);
        }

        wf_surface = zwf_shell_manager_v1_get_wm_surface(display.wf_manager,
                surface, ZWF_WM_SURFACE_V1_ROLE_DESKTOP_WIDGET, nullptr);
        zwf_wm_surface_v1_set_keyboard_mode(wf_surface,
                ZWF_WM_SURFACE_V1_KEYBOARD_FOCUS_MODE_NO_FOCUS);

        uint32_t parsed_anchor = check_anchor_for_wayfire_shell(width,
                height, anchor);
        zwf_wm_surface_v1_set_anchor(wf_surface, parsed_anchor);
        zwf_wm_surface_v1_configure(wf_surface, x, y);

    }

    void WaylandWindow::init_layer_shell(WaylandDisplay display, int width,
            int height, std::string anchor)
    {
        auto gtk_window = this->gobj();
        auto gtk_widget = GTK_WIDGET(gtk_window);
        gtk_widget_realize(gtk_widget);

        auto gdk_window = this->get_window()->gobj();
        gdk_wayland_window_set_use_custom_surface(gdk_window);
        auto surface = gdk_wayland_window_get_wl_surface(gdk_window);

        if (!surface)
        {
            std::cerr << "Error: created window was not a wayland surface" << std::endl;
            std::exit(-1);
        }

        uint32_t layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
        layer_surface = zwlr_layer_shell_v1_get_layer_surface(
                display.layer_shell, surface, NULL, layer, "wf-osk");
        if (!layer_surface)
        {
            std::cerr << "Error: could not create layer surface" << std::endl;
            std::exit(-1);
        }

        Gtk::Application *data = this->get_application().get();
        zwlr_layer_surface_v1_add_listener(layer_surface,
                &layer_surface_listener, data);
        zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface, 0);
        zwlr_layer_surface_v1_set_size(layer_surface, width, height);

        uint32_t parsed_anchor = check_anchor_for_layer_shell(width,
                height, anchor);
        zwlr_layer_surface_v1_set_anchor(layer_surface, parsed_anchor);

        wl_surface_commit(surface);
        auto gdk_display = gdk_display_get_default();
        auto wl_display = gdk_wayland_display_get_wl_display(gdk_display);
        wl_display_roundtrip(wl_display);

        this->show_all();
    }

    WaylandWindow::WaylandWindow(int x, int y, int width, int height,
            std::string anchor)
        : Gtk::Window()
    {
        auto display = WaylandDisplay::get();

        /* Trick: first show the window, get frame size, then subtract it again */
        this->set_size_request(width, height);
        this->set_default_size(width, height);
        this->set_type_hint(Gdk::WINDOW_TYPE_HINT_DOCK);

        if (display.wf_manager)
        {
            init_wayfire_shell(display, x, y, width, height, anchor);
        } else if (display.layer_shell)
        {
            init_layer_shell(display, width, height, anchor);
        } else {
            std::cerr << "Error: cannot find any supported shell protocol" << std::endl;
            std::exit(-1);
        }
    }
}
