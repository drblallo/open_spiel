import action
import bounded_arg
import string
import collections.vector


fun dictionary(Int value) -> String:
  if value == 0:
    return "porta"s
  if value == 1:
    return "chiav"s
  if value == 2:
    return "letto"s
  if value == 3:
    return "erika"s

fun erase_from_string(String str, Byte to_erase):
  let new_string : String
  let index = 0
  while str.size() != index:
    if str.get(index) != to_erase:
      new_string.append(str.get(index))
    index = index + 1
  str = new_string

act play() -> Game:
  frm current_player = byte(0)
  frm winner = byte(-1)

  act pick_word(BoundedArg<0, 4> arg) {
     arg.value >= 0
  }
  frm picked = dictionary(arg.value)

  current_player = byte(1)

  frm attempts = byte(8)
  while attempts > byte(0):
      act guess(BoundedArg<0, 24> arg)
      let guessed_numer = 'a' + byte(arg.value)
      attempts = attempts - byte(1)
      erase_from_string(picked, guessed_numer)
      if picked.size() == 0:
        winner = byte(1)
        return 

  winner = byte(0)

fun get_current_player(Game g) -> Int:
  return int(g.current_player)

fun score(Game g) -> Float:
  if g.winner == byte(1):
    return -1.0
  else if g.winner == byte(0):
    return 1.0 / (6.0 - float(g.picked.size()))
  else:
    return 0.0

fun gen_methods():
  let x : AnyGameAction
  gen_python_methods(play(), x)

fun fuzz(Vector<Byte> input):
    if input.size() == 0:
        return
    let state = play()
    let action : AnyGameAction 
    parse_and_execute(state, action, input) 

fun main() -> Int:
  let x : AnyGameAction
  let state = enumerate(x)
  print(state)
  return 0
