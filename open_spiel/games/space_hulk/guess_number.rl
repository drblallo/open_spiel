import action
import bounded_arg

act play() -> Game:
  frm winner = byte(-1)
  frm player_1_decision = byte(0)

  frm current_player = byte(0)
  act asd(BoundedArg<0, 10> decision)
  player_1_decision = byte(decision.value)

  current_player = byte(1)
  act asd(BoundedArg<0, 10> decision)

  if player_1_decision != byte(decision.value):
    winner = byte(1)
  else:
    winner = byte(0)

fun get_current_player(Game g) -> Int:
  return int(g.current_player)

fun score(Game g) -> Float:
  if g.winner == byte(-1):
    return 0.0
  else if g.winner == byte(1):
    return 1.0
  else:
    return -1.0

fun gen_methods():
  let x : AnyGameAction
  gen_python_methods(play(), x)

fun main() -> Int:
  return 0
