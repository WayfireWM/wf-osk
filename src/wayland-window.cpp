#include "wayland-window.hpp"
#include <iostream>
#include <algorithm>
#include <gtkmm/icontheme.h>
#include <gtkmm/main.h>
#include <gdk/gdkwayland.h>
#include <wayland-client.h>
#include <gtk-layer-shell.h>
#include <gtkmm/headerbar.h>

#include <gdkmm/display.h>
#include <gdkmm/seat.h>

static constexpr int HEADERBAR_SIZE = 60;

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
        gtk_layer_set_layer(this->gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);
        gtk_layer_set_namespace(this->gobj(), "keyboard");
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
        // setup close button
        close_button.get_style_context()->add_class("image-button");
        close_button.set_image_from_icon_name("window-close-symbolic",
            Gtk::ICON_SIZE_LARGE_TOOLBAR);
        close_button.signal_clicked().connect_notify([=] () {
            this->get_application()->quit();
        });

        // setup move gesture
        headerbar_drag = Gtk::GestureDrag::create(drag_box);
        headerbar_drag->signal_drag_begin().connect_notify([=] (double, double) {
            if (this->wf_surface)
            {
                zwf_surface_v2_interactive_move(this->wf_surface);
                /* Taken from GDK's Wayland impl of begin_move_drag() */
                Gdk::Display::get_default()->get_default_seat()->ungrab();
                headerbar_drag->reset();
            }
        });
        Gtk::HeaderBar bar;
        headerbar_box.override_background_color(bar.get_style_context()->get_background_color());


        // setup headerbar layout
        headerbar_box.set_size_request(-1, HEADERBAR_SIZE);

        close_button.set_size_request(HEADERBAR_SIZE * 0.8, HEADERBAR_SIZE * 0.8);
        close_button.set_margin_bottom(OSK_SPACING);
        close_button.set_margin_top(OSK_SPACING);
        close_button.set_margin_left(OSK_SPACING);
        close_button.set_margin_right(OSK_SPACING);

        headerbar_box.pack_end(close_button, false, false);
        headerbar_box.pack_start(drag_box, true, true);
        layout_box.pack_start(headerbar_box);
        layout_box.set_spacing(OSK_SPACING);
        this->add(layout_box);

        // setup gtk layer shell
        init(width, height, anchor);
    }

    void WaylandWindow::set_widget(Gtk::Widget& w)
    {
        if (current_widget)
            this->layout_box.remove(*current_widget);

        this->layout_box.pack_end(w);
        current_widget = &w;

        w.set_margin_bottom(OSK_SPACING);
        w.set_margin_left(OSK_SPACING);
        w.set_margin_right(OSK_SPACING);
        this->show_all();
    }
}
