import main

fun get_current_player(Game g) -> Int:
    if g.board.is_marine_decision:
        return 0
    return 1

fun score(Game g) -> Float:
    return g.board.score()
