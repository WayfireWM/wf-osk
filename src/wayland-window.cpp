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
        } else if (anchor.compare("pinned") == 0)
        {
            parsed_anchor = ANCHOR_PINNED_BOTTOM;
        }

        return parsed_anchor;
    }

    void WaylandWindow::init(int width, int height, std::string anchor)
    {
        gtk_layer_init_for_window(this->gobj());
        gtk_layer_set_layer(this->gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);
        gtk_layer_set_namespace(this->gobj(), "keyboard");
        gtk_layer_set_exclusive_zone(this->gobj(), -1);
        auto layer_anchor = check_anchor(anchor);
        if (layer_anchor > -1)
        {
            gtk_layer_set_anchor(this->gobj(),
                (GtkLayerShellEdge)layer_anchor, true);
        } else if (layer_anchor == ANCHOR_PINNED_BOTTOM)
        {
            gtk_layer_set_anchor(this->gobj(),
                GTK_LAYER_SHELL_EDGE_BOTTOM, true);
            gtk_layer_set_anchor(this->gobj(),
                GTK_LAYER_SHELL_EDGE_LEFT, true);
            gtk_layer_set_anchor(this->gobj(),
                GTK_LAYER_SHELL_EDGE_RIGHT, true);
            gtk_layer_auto_exclusive_zone_enable(this->gobj());
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

    void WaylandWindow::init_headerbar(int headerbar_size)
    {
        std::vector<Gtk::Button*> buttons = {
            &top_button, &bottom_button, &close_button
        };

        const int button_size = 0.8 * headerbar_size;
        for (auto& button : buttons)
        {
            button->get_style_context()->add_class("image-button");
            button->set_size_request(button_size, button_size);
            button->set_margin_bottom(OSK_SPACING);
            button->set_margin_top(OSK_SPACING);
            button->set_margin_left(OSK_SPACING);
            button->set_margin_right(OSK_SPACING);
        }

        static const std::map<Gtk::BuiltinIconSize, int> gtk_size_map = {
            {Gtk::ICON_SIZE_MENU, 16},
            {Gtk::ICON_SIZE_SMALL_TOOLBAR, 16},
            {Gtk::ICON_SIZE_LARGE_TOOLBAR, 24},
            {Gtk::ICON_SIZE_BUTTON, 16},
            {Gtk::ICON_SIZE_DND, 32},
            {Gtk::ICON_SIZE_DIALOG, 48}
        };

        Gtk::BuiltinIconSize desired_gtk_icon_size = Gtk::ICON_SIZE_MENU;
        for (auto [gtk_size, pixel_size] : gtk_size_map)
        {
            if (pixel_size <= button_size)
            {
                desired_gtk_icon_size = gtk_size;
            }
        }

        close_button.set_image_from_icon_name("window-close-symbolic", desired_gtk_icon_size);
        close_button.signal_clicked().connect_notify([=] () {
            this->get_application()->quit();
        });

        top_button.set_image_from_icon_name("pan-up-symbolic", desired_gtk_icon_size);
        top_button.signal_clicked().connect_notify([=] () {
            gtk_layer_set_anchor(this->gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
            gtk_layer_set_anchor(this->gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, false);
        });

        bottom_button.set_image_from_icon_name("pan-down-symbolic", desired_gtk_icon_size);
        bottom_button.signal_clicked().connect_notify([=] () {
            gtk_layer_set_anchor(this->gobj(), GTK_LAYER_SHELL_EDGE_TOP, false);
            gtk_layer_set_anchor(this->gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, true);
        });

        // setup move gesture
        Gtk::HeaderBar bar;
        headerbar_box.override_background_color(bar.get_style_context()->get_background_color());

        // setup headerbar layout
        headerbar_box.set_size_request(-1, headerbar_size);
        headerbar_box.pack_end(close_button, false, false);
        headerbar_box.pack_start(top_button, false, false);
        headerbar_box.pack_start(bottom_button, false, false);

        layout_box.pack_start(headerbar_box);
        layout_box.set_spacing(OSK_SPACING);
        this->add(layout_box);
    }

    WaylandWindow::WaylandWindow(int width, int height, std::string anchor, int headerbar_size)
        : Gtk::Window()
    {
        init_headerbar(headerbar_size);
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
