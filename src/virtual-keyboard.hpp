#pragma once

#include <cstdint>
#include <virtual-keyboard-unstable-v1-client-protocol.h>

namespace wf
{
    class VirtualKeyboardDevice
    {
        int shift_pressed_counter = 0;

        void send_keymap();
        zwp_virtual_keyboard_v1 *vk;

        public:
        VirtualKeyboardDevice();

        void set_shift(bool shift_on);
        void send_key(uint32_t key, uint32_t state) const;
    };
}
