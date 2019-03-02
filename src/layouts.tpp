std::vector<std::vector<Key>> default_keys = {
    {
        {KEY_Q, "q", 1},
        {KEY_W, "w", 1},
        {KEY_E, "e", 1},
        {KEY_R, "r", 1},
        {KEY_T, "t", 1},
        {KEY_Y, "y", 1},
        {KEY_U, "u", 1},
        {KEY_I, "i", 1},
        {KEY_O, "o", 1},
        {KEY_P, "p", 1},
        {KEY_BACKSPACE, "⌫", 2}
    },
    {
        {0, " ", 0.5},
        {KEY_A, "a", 1},
        {KEY_S, "s", 1},
        {KEY_D, "d", 1},
        {KEY_F, "f", 1},
        {KEY_G, "g", 1},
        {KEY_H, "h", 1},
        {KEY_J, "j", 1},
        {KEY_K, "k", 1},
        {KEY_L, "l", 1},
        {KEY_ENTER, "↵", 2}
    },
    {
        {ABC_TOGGLE, "ABC", 1},
        {KEY_Z, "z", 1},
        {KEY_X, "x", 1},
        {KEY_C, "c", 1},
        {KEY_V, "v", 1},
        {KEY_B, "b", 1},
        {KEY_N, "n", 1},
        {KEY_M, "m", 1},
        {KEY_COMMA, ",", 1},
        {KEY_DOT, ".", 1}
    },
    {
        {NUM_TOGGLE, "123?", 1.5},
        {KEY_SPACE, "_", 9.5},
        {KEY_LEFT, "←", 0.5},
        {KEY_RIGHT, "→", 0.5},
        {KEY_UP, "↑", 0.5},
        {KEY_DOWN, "↓", 0.5}
    }
};

auto shift_keys = default_keys;
for (auto& row : shift_keys)
{
    for (auto& key : row)
    {
        key.text = key.text.uppercase();
        if (key.code < USE_SHIFT)
            key.code |= USE_SHIFT;

        if (key.text == "ABC")
            key.text = "abc";
    }
}

std::vector<std::vector<Key>> numeric_keys = {
    {
        {KEY_1, "1", 1},
        {KEY_2, "2", 1},
        {KEY_3, "3", 1},
        {KEY_4, "4", 1},
        {KEY_5, "5", 1},
        {KEY_6, "6", 1},
        {KEY_7, "7", 1},
        {KEY_8, "8", 1},
        {KEY_9, "9", 1},
        {KEY_0, "0", 1},
        {KEY_MINUS, "-", 1},
        {KEY_EQUAL, "=", 1},
        {KEY_BACKSPACE, "⌫", 2}
    },
    {
        {KEY_1 | USE_SHIFT, "!", 1},
        {KEY_2 | USE_SHIFT, "@", 1},
        {KEY_3 | USE_SHIFT, "#", 1},
        {KEY_4 | USE_SHIFT, "$", 1},
        {KEY_5 | USE_SHIFT, "%", 1},
        {KEY_6 | USE_SHIFT, "^", 1},
        {KEY_7 | USE_SHIFT, "&", 1},
        {KEY_8 | USE_SHIFT, "*", 1},
        {KEY_9 | USE_SHIFT, "(", 1},
        {KEY_0 | USE_SHIFT, ")", 1},
        {KEY_SEMICOLON, ";", 1},
        {KEY_SEMICOLON | USE_SHIFT, ":", 1},
        {KEY_ENTER, "↵", 1}
    },
    {
        {KEY_LEFTBRACE, "[", 1},
        {KEY_RIGHTBRACE, "]", 1},
        {KEY_LEFTBRACE | USE_SHIFT, "{", 1},
        {KEY_RIGHTBRACE | USE_SHIFT, "}", 1},
        {KEY_COMMA | USE_SHIFT, "<", 1},
        {KEY_DOT | USE_SHIFT, ">", 1},
        {KEY_EQUAL | USE_SHIFT, "+", 1},
        {KEY_SLASH, "/", 1},
        {KEY_SLASH | USE_SHIFT, "?", 1},
        {KEY_APOSTROPHE, "\'", 1},
        {KEY_APOSTROPHE | USE_SHIFT, "\"", 1},
        {KEY_GRAVE, "`", 1},
        {KEY_GRAVE | USE_SHIFT, "~", 1}
    },
    {
        {ABC_TOGGLE, "abc", 1},
        {KEY_SPACE, "_", 10},
        {KEY_BACKSLASH, "\\", 1},
        {KEY_BACKSLASH | USE_SHIFT, "|", 1}
    }
};
