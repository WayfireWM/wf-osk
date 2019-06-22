#include "osk.hpp"
#include <getopt.h>
#include <iostream>
#include <linux/input-event-codes.h>

#define ABC_TOGGLE 0x12345678
#define NUM_TOGGLE 0x87654321

#define IS_COMMAND(x) ((x) == ABC_TOGGLE || (x) == NUM_TOGGLE)

#define USE_SHIFT  0x10000000

namespace wf
{
    namespace osk
    {
        int spacing = 8;
        int default_x = 100;
        int default_y = 100;
        int default_width = 800;
        int default_height = 400;

        KeyButton::KeyButton(Key key, int width, int height)
        {
            this->code = key.code;

            this->button.set_size_request(width, height);
            this->button.set_label(key.text);

            this->button.signal_pressed().connect_notify(
                sigc::mem_fun(this, &KeyButton::on_pressed));
            this->button.signal_released().connect_notify(
                sigc::mem_fun(this, &KeyButton::on_released));
        }

        void KeyButton::on_pressed()
        {
            auto& keyboard = Keyboard::get();
            if (IS_COMMAND(this->code))
                return;

            if (this->code & USE_SHIFT)
                keyboard.get_device().set_shift(true);

            keyboard.get_device().send_key(this->code & ~(USE_SHIFT),
                WL_KEYBOARD_KEY_STATE_PRESSED);
        }

        void KeyButton::on_released()
        {
            auto& keyboard = Keyboard::get();
            if (IS_COMMAND(this->code))
                return keyboard.handle_action(this->code);

            if (this->code & USE_SHIFT)
                keyboard.get_device().set_shift(false);

            keyboard.get_device().send_key(this->code & ~(USE_SHIFT),
                WL_KEYBOARD_KEY_STATE_RELEASED);
        }

        KeyboardRow::KeyboardRow(std::vector<Key> keys,
            int width, int height)
        {
            double sum = 0;
            for (auto& key : keys)
                sum += key.width;

            box.set_spacing(spacing);
            int total_spacing = std::min((int)keys.size() - 1, 0) * spacing;
            int total_buttons = width - total_spacing;

            for (auto& key : keys)
            {
                this->keys.emplace_back(std::make_unique<KeyButton>
                    (key, int(key.width / sum * total_buttons), height));
                this->box.pack_start(this->keys.back()->button);
            }
        }

        KeyboardLayout::KeyboardLayout(std::vector<std::vector<Key>> keys,
            int32_t width, int32_t height)
        {
            box.set_spacing(spacing);
            int total_spacing = std::min((int)keys.size() - 1, 0) * spacing;

            int row_height = (height - total_spacing) / keys.size();
            for (auto& row : keys)
            {
                this->rows.emplace_back(
                    std::make_unique<KeyboardRow> (row, width, row_height));
                this->box.pack_start(this->rows.back()->box);
            }
        }

        void Keyboard::init_layouts()
        {
            /* Key layouts are defined in layouts.tpp,
             * it defines default_keys, shift_keys, numeric_keys */
            #include "layouts.tpp"

            this->default_layout = std::make_unique<KeyboardLayout>
                (default_keys, default_width, default_height);

            this->shift_layout = std::make_unique<KeyboardLayout>
                (shift_keys, default_width, default_height);

            this->numeric_layout = std::make_unique<KeyboardLayout>
                (numeric_keys, default_width, default_height);
        }

        void Keyboard::set_layout(KeyboardLayout *new_layout)
        {
            if (this->current_layout)
                this->window->remove();

            this->current_layout = new_layout;
            this->window->add(new_layout->box);
            this->window->show_all();
        }

        Keyboard::Keyboard()
        {
            window = std::make_unique<WaylandWindow>
                (default_x, default_y, default_width, default_height);
            vk = std::make_unique<VirtualKeyboardDevice> ();

            init_layouts();
            set_layout(default_layout.get());
        }

        std::unique_ptr<Keyboard> Keyboard::instance;
        void Keyboard::create()
        {
            if (instance)
                throw std::logic_error("Creating keyboard twice!");

            instance = std::unique_ptr<Keyboard>(new Keyboard());
        }

        Keyboard& Keyboard::get()
        {
            if (!instance)
                throw std::logic_error("Getting keyboard before creating it!");

            return *instance;
        }

        VirtualKeyboardDevice& Keyboard::get_device()
        {
            return *vk;
        }

        Gtk::Window& Keyboard::get_window()
        {
            return *window;
        }

        void Keyboard::handle_action(uint32_t action)
        {
            if (action == ABC_TOGGLE)
            {
                if (current_layout == default_layout.get()) {
                    set_layout(shift_layout.get());
                } else {
                    set_layout(default_layout.get());
                }
            }

            if (action == NUM_TOGGLE)
                set_layout(numeric_layout.get());
        }
    }
}

int main(int argc, char **argv)
{
    struct option opts[] = {
        { "geometry",          required_argument, NULL, 'g' },
        { 0,                   0,                 NULL,  0  }
    };

    int c, i;
    while((c = getopt_long(argc, argv, "g:", opts, &i)) != -1)
    {
        using namespace wf::osk;
        switch(c)
        {
            case 'g':
                if (sscanf(optarg, "%d,%d %dx%d", &default_x, &default_y,
                        &default_width, &default_height) != 4)
                {
                    std::cerr << "Invalid geometry: " << optarg << std::endl;
                    std::exit(-1);
                } else
                {
                    std::cout << "Geometry " << default_x << "," << default_y << " "
                        << default_width << "x" << default_height;
                }

                break;
            default:
                std::cerr << "Unrecognized argument " << char(c) << std::endl;
        }
    }

    auto app = Gtk::Application::create();
    wf::osk::Keyboard::create();
    return app->run(wf::osk::Keyboard::get().get_window());
}
