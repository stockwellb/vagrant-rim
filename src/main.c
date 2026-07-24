#include "game/game.h"

int main(void)
{
    Game game;
    game_init(&game);

    while (!game_should_close(&game)) {
        game_update(&game);
        game_draw(&game);
    }

    game_shutdown(&game);
    return 0;
}
