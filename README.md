# Battleship with Inter‑Process Communication (IPC)

A terminal‑based Battleship game written in C that showcases:

* **Dual‑process architecture** (parent + child) communicating over UNIX pipes
* **Real‑time ncurses UI** with coloured grids
* **Save / Load** support to resume matches
* **Two board sizes** – classic *4 × 4* and *8 × 8* with multiple‑cell ships
* **Simple AI** that alternates between random and tactical shots
* **Portable, single‑file project** (only `main.c`) – ideal for coursework or demos

---

## 1. Gameplay

| Key       | Action                                            |
| --------- | ------------------------------------------------- |
| **1 / 2** | Start a new game on 4×4 or 8×8 board              |
| **3**     | Load the most recent save                         |
| **4**     | Quit                                              |
| **p**     | Pause during a match                              |
| **m / e** | In pause screen: return to menu / exit            |
| *(auto)*  | Game is auto‑saved at the end of every full round |

Both processes take turns:

1. **Parent**: draws UI, decides whose turn it is and flashes the shot result.
2. **Child**: receives a *Fire* command, computes coordinates, sends the result back.

---

## 2. Build & Run

```bash
sudo apt install build-essential libncurses5-dev   # Debian / Ubuntu
gcc main.c -o battleship -lncurses
./battleship
```

> The game depends only on the POSIX C library and **ncurses**.
> Tested on Linux 5.10+ and macOS Ventura (clang).

---

## 3. File layout

```
main.c         ← complete source (≈900 loc)
save.dat       ← auto‑generated binary save file
README.md      ← you‑are‑here
```

---

## 4. Code architecture

### 4.1 Data model

```c
typedef struct {
    char grid[8][8];        // own fleet
    char opponent_grid[8][8];
    int  hits;
    int  total_ship_cells;
    int  score;
    int  last_hit_row, last_hit_col; // AI memory
} Player;                                 
```

* `EMPTY`, `SHIP`, `HIT`, `MISS` glyphs keep the UI simple.
* Total ship cells let us detect victory in O(1).

### 4.2 Process & IPC flow

```
┌────────────┐     pipe[0] (parent→child)       ┌────────────┐
│  Parent    │ ───────────────────────────────► │   Child    │
│ UI + menu  │ ◄─────────────────────────────── │ AI logic   │
└────────────┘  pipe[1] (child→parent)          └────────────┘
```

Commands:

| Byte      | Direction | Payload                                                |
| --------- | --------- | ------------------------------------------------------ |
| `'I'`     | P → C     | initial state (board, players)                         |
| `'F'`     | P → C     | “fire for current player”                              |
| `'Q'`     | P → C     | quit child                                             |
| *structs* | C → P     | shot coords, hit flag, updated players, game‑over flag |

The parent forks exactly once at launch; subsequent menu returns do **not** create new children, keeping resources tidy.

### 4.3 AI strategy

* If the previous shot was a hit, try the four orthogonal neighbours (`strategic_fire`).
* Otherwise fall back to uniform random selection.
  *For 4×4 boards, a pure random shooter is used.*

### 4.4 Persistence

```c
void save_game(Player *p1, Player *p2) {
    FILE *f = fopen("save.dat","wb");
    fwrite(&grid_size, sizeof(int), 1, f);
    fwrite("P1",2,1,f); fwrite(p1,sizeof(Player),1,f);
    fwrite("P2",2,1,f); fwrite(p2,sizeof(Player),1,f);
}
```

*Binary blob saves are tiny (< 3 kB) and loaded via `load_game` with sanity
checks (player tags, end‑of‑game detection).*

---

## 5. Extending the project

* **Human vs Human / Network play** – swap pipes for sockets.
* **Better AI** – add probability heat‑maps or hunt/target modes.
* **Resizable boards** – make `grid_size` dynamic, allocate grids.
* **Cross‑platform TUI** – switch to *ncursesw* for Unicode ships 🔱.

---

## 6. Troubleshooting

| Symptom                         | Fix                                                                      |
| ------------------------------- | ------------------------------------------------------------------------ |
| `error: curses.h: No such file` | Install `libncurses5-dev` (Deb/Ubuntu) or `brew install ncurses` (macOS) |
| Window flickers / no colours    | Ensure `$TERM` supports colour (e.g. `xterm-256color`)                   |
| Save won’t load                 | Delete `save.dat` if format changed after code edits                     |

---

## 7. License

This sample README leaves the choice to you.
Typical options:

```text
SPDX-License-Identifier: MIT
```

---

## 8. Acknowledgements

Created as a university IPC assignment; thanks to the ncurses community for the timeless TUI library.

Enjoy sinking ships! 🎯

## 9. Some Images From the Game
![5](https://github.com/user-attachments/assets/01783777-e139-463c-85d6-d9742f6d7f21)
![2](https://github.com/user-attachments/assets/e1b84d76-8eb1-4894-b225-8ed052de38e3)
![3](https://github.com/user-attachments/assets/455b11ec-3013-4590-a41d-88389fd1bf3b)
![1](https://github.com/user-attachments/assets/f7c44885-1b62-4bb7-8542-4f13dd5a3f3c)


