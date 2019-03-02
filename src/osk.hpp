#pragma once

#include <string>
#include <vector>
#include <memory>

#include <gtkmm.h>

#include "virtual-keyboard.hpp"
#include "wayland-window.hpp"

namespace wf
{
    namespace osk
    {
        extern int spacing;

        struct Key
        {
            uint32_t code;
            std::string text;
            double width;
        };

        struct KeyButton
        {
            Gtk::Button button;

            /* keycode as in linux/input-event-codes.h */
            uint32_t code;
            KeyButton(Key key, int width, int height);

            private:
            void on_pressed();
            void on_released();
        };

        struct KeyboardRow
        {
            Gtk::HBox box;
            std::vector<std::unique_ptr<KeyButton>> keys;

            KeyboardRow(std::vector<Key> keys,
                int width, int height);
        };

        struct KeyboardLayout
        {
            Gtk::VBox box;
            std::vector<std::unique_ptr<KeyboardRow>> rows;

            KeyboardLayout(std::vector<std::vector<Key>> keys,
                int32_t width, int32_t height);
        };

        class Keyboard
        {
            std::unique_ptr<KeyboardLayout> default_layout, shift_layout,
                numeric_layout;
            KeyboardLayout *current_layout = nullptr;
            void init_layouts();
            void set_layout(KeyboardLayout *new_layout);

            std::unique_ptr<WaylandWindow> window;
            std::unique_ptr<VirtualKeyboardDevice> vk;
            Keyboard();

            static std::unique_ptr<Keyboard> instance;

            public:
            static void create();
            static Keyboard& get();

            void handle_action(uint32_t action);
            VirtualKeyboardDevice& get_device();
            Gtk::Window& get_window();
        };
    }
}
