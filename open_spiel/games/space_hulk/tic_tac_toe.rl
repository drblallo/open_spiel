import action
import bounded_arg


ent Board:
    BInt<0, 3>[9] slots
    Bool playerTurn


    fun get(Int x, Int y) -> Int:
        return self.slots[x + (y*3)].value

    fun set(Int x, Int y, Int val): 
        self.slots[x + (y * 3)] = val

    fun full() -> Bool:
        let x = 0

        while x < 3:
            let y = 0
            while y < 3:
                if self.get(x, y) == 0:
                    return false
                y = y + 1
            x = x + 1

        return true

    fun three_in_a_line_player_row(Int player_id, Int row) -> Bool:
        return self.get(0, row) == self.get(1, row) and self.get(0, row) == self.get(2, row) and self.get(0, row) == player_id

    fun three_in_a_line_player(Int player_id) -> Bool:
        let x = 0
        while x < 3:
            if self.get(x, 0) == self.get(x, 1) and self.get(x, 0) == self.get(x, 2) and self.get(x, 0) == player_id:
                return true

            if self.three_in_a_line_player_row(player_id, x):
                return true
            x = x + 1

        if self.get(0, 0) == self.get(1, 1) and self.get(0, 0) == self.get(2, 2) and self.get(0, 0) == player_id:
            return true

        if self.get(0, 2) == self.get(1, 1) and self.get(0, 2) == self.get(2, 0) and self.get(0, 2) == player_id:
            return true

        return false

    fun current_player() -> Int:
        return int(self.playerTurn) + 1

    fun next_turn():
        self.playerTurn = !self.playerTurn

act play() -> Game:
    frm board : Board
    board.playerTurn = false
    while !board.full():
        act mark(BInt<0, 3> x, BInt<0, 3> y) {
            board.get(x.value, y.value) == 0
        }

        board.set(x.value, y.value, board.current_player())

        if board.three_in_a_line_player(board.current_player()):
            return

        board.next_turn()

fun get_current_player(Game g) -> Int:
  return int(g.board.current_player() - 1)

fun score(Game g) -> Float:
  if g.board.three_in_a_line_player(1):
    return 1.0
  else if g.board.three_in_a_line_player(2):
    return -1.0
  else:
    return 0.0

fun gen_methods():
  let x : AnyGameAction
  gen_python_methods(play(), x)
  let v = enumerate(x)
  v.size()

fun main() -> Int:
  return 0
