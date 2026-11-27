// SPDX-License-Identifier: GPL-3.0-or-later
// Original 32px white glove with coverage levels for its antialiased outline.
SDL_Cursor* sonic_create_glove_cursor() {
    constexpr int size = 32;
    constexpr char palette[] = "_.:;ox%#123456789@";
    constexpr char rows[size][size + 1] = {
        "____________:o:_________________",
        "___________%111x________________",
        "__________;18@71._______________",
        "__________%4@@@1:_______________",
        "__________%5@@@1:_______________",
        "__________%5@@@1:_______________",
        "__________%5@@91:_______________",
        "__________%4@@81:_______________",
        "__________%3@@81oo:_____________",
        "__________o3@@81111%.___________",
        "__________o2@@@3@@82%::_________",
        "__________;1@@@39@@8111%:_______",
        "__________:1@@@57@@@4783%_______",
        "__________:1@@@84@@@94@81;______",
        "____::.___:1@@@@@@@@@@@@21#;____",
        "__:#111%:_:1@@@@@@@@@@@@5541:___",
        "_:1488731%o2@@@@@@@@@@@@98@3%___",
        "_#4@@@@@7214@@@@@@@@@@@@@@@61___",
        "_#5@@@@@@948@@@@@@@@@@@@@@@81.__",
        "_o148@@@@@@@@@@@@@@@@@@@@@@81:__",
        "__:#1259@@@@@@@@@@@@@@@@@@@81:__",
        "____:o#159@@@@@@@@@@9@@@@@@71___",
        "_______o#39@@@@@87@93@98@@@4%___",
        "________:%28@@@@85@85@75@@92o___",
        "_________.%29@@@67@57@48@@51.___",
        "__________.%39@@78@39@3@@82o____",
        "___________:1259@@@@@@@@93%_____",
        "___________:1411357888862%._____",
        "___________.1596421111111:______",
        "____________:#2577986541x_______",
        "_____________.;%#11111%:________",
        "__________________::____________",
    };
    std::array<uint32_t, size * size> pixels{};
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const char symbol = rows[y][x];
            if (symbol == '_') continue;
            const auto level = static_cast<uint32_t>(std::strchr(palette, symbol) - palette);
            const uint32_t alpha = level < 8 ? (level * 255u + 4) / 8 : 255u;
            const uint32_t gray = level < 8 ? 26u : 26u + ((level - 8) * 224u + 4) / 9;
            pixels[y * size + x] = (alpha << 24) | (gray * 0x00010101u);
        }
    }
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        size, size, SDL_PIXELFORMAT_ARGB8888, pixels.data(), size * sizeof(uint32_t));
    if (!surface) return nullptr;
    SDL_Cursor* cursor = SDL_CreateColorCursor(surface, 13, 1);
    SDL_DestroySurface(surface);
    return cursor;
}
