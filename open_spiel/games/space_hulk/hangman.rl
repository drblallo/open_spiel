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
  let index = 0
  while str.size() != index:
    if str.get(index) == to_erase:
      str.get(index) = '0'
    index = index + 1

act play() -> Game:
  frm current_player = false
  frm winner : BInt<0, 3>
  frm picked = "00000"s

  act pick_word(BInt<0, 4> arg) 
  picked = dictionary(arg.value)

  current_player = true

  frm attempts : BInt<0, 9>
  while attempts.value != 8:
      act guess(BInt<0, 24> arg)
      let guessed_numer = 'a' + byte(arg.value)
      attempts.value = attempts.value + 1
      erase_from_string(picked, guessed_numer)
      if picked == "00000":
        winner.value = 1
        return 

  winner.value = 2

fun get_current_player(Game g) -> Int:
  return int(g.current_player)

fun score(Game g) -> Float:

  if g.winner == 1:
    return -1.0
  else if g.winner == 2:
    return 1.0 / (1.0 + float(g.picked.count('0')))
  else:
    return 0.0

fun gen_methods():
  let x : AnyGameAction
  let x2 = play() 
  print(x2)
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
