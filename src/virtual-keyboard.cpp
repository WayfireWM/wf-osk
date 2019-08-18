#include "virtual-keyboard.hpp"
#include "wayland-window.hpp"
#include "shared/os-compatibility.h"

#include <sys/mman.h>
#include <cstring>
#include <time.h>
#include <iostream>

#include <gdkmm/display.h>
#include <gdkmm/seat.h>
#include <gdk/gdkwayland.h>

namespace wf
{
    VirtualKeyboardDevice::VirtualKeyboardDevice()
    {
        auto& display = WaylandDisplay::get();
        auto seat = Gdk::Display::get_default()->get_default_seat();
        vk = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(
            display.vk_manager, gdk_wayland_seat_get_wl_seat(seat->gobj()));

        this->send_keymap();
    }

    void VirtualKeyboardDevice::send_keymap()
    {
        /* The keymap string is defined in keymap.tpp, it is keymap_normal */
        #include "keymap.tpp"

        size_t keymap_size = strlen(keymap) + 1;
        int keymap_fd = os_create_anonymous_file(keymap_size);
        void *ptr = mmap(NULL, keymap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
            keymap_fd, 0);

        std::strcpy((char*)ptr, keymap);
        zwp_virtual_keyboard_v1_keymap(vk, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
            keymap_fd, keymap_size);
    }

    uint32_t get_current_time()
    {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec * 1000ll + ts.tv_nsec / 1000000ll;
    }

    void VirtualKeyboardDevice::send_key(uint32_t key, uint32_t state) const
    {
        zwp_virtual_keyboard_v1_key(vk, get_current_time(), key, state);
    }

    void VirtualKeyboardDevice::set_shift(bool shift_on)
    {
        shift_pressed_counter += (shift_on ? 1 : -1);

        const int modifier_shift_code = 1;
        zwp_virtual_keyboard_v1_modifiers(vk,
            shift_pressed_counter ? modifier_shift_code : 0, 0, 0, 0);
    }
}
