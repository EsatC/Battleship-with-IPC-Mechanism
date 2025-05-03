#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <ncurses.h>
#include <sys/wait.h>
#include <string.h>

#define EMPTY '.'  //For empty slots it is shown with '.'
#define SHIP 'S'   //For the slots with ships it is shown with 'S'
#define HIT 'O'    //Hit shots are represented with 'O' 
#define MISS 'o'   //Missed shots are represented with 'o'

#define READ_END 0      //Read end of the pipe 
#define WRITE_END 1     //Write end of the pipe

int grid_size = 8;      
const int small_ship_count = 4;
const int large_ship_sizes[] = {4, 3, 3, 2, 2};     //For 8x8 game ship sizes are represented in this array
const int num_large_ships = sizeof(large_ship_sizes) / sizeof(large_ship_sizes[0]); //To calculate the size of the array 

typedef struct {
    char grid[8][8];
    char opponent_grid[8][8];
    int hits;
    int total_ship_cells;
    int score;
    int last_hit_row;   //To improve AI last hit row is stored 
    int last_hit_col;   //To improve AI last hit col is stored 
} Player;

void initialize_grid(Player* player, int size) { //A funciton to initialize grid 
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {        //Firstly both player grid and opponent grid are initialized with empty slots
            player->grid[i][j] = EMPTY;     
            player->opponent_grid[i][j] = EMPTY;
        }
    }
    player->hits = 0;                           //No hit at the beginning
    player->total_ship_cells = 0;               //Grid initialized without placing ships at the beginning. They will be placed after calling place ship functions
    player->score = 0;                          //Initial score is 0
    player->last_hit_row = -1;                  //At the beginning last_hit_row initialized to -1 because not hit occur
    player->last_hit_col = -1;                  //At the beginning last_hit_col initialized to -1 because not hit occur
}

int is_within_bounds(int row, int col) {    //Check whether specified position is valid or not
    return row >= 0 && row < grid_size && col >= 0 && col < grid_size;
}

void place_ship_4x4(Player* player) {           //Place ships randomly for 4x4 game 
    int row, col;
    do {                                        //Find row and column until find a valid one
        row = rand() % 4;                       //Random row 
        col = rand() % 4;                       //Random col
    } while (player->grid[row][col] != EMPTY);
    player->grid[row][col] = SHIP;              //Place ship
    player->total_ship_cells++;                 //Increment total ship cells each time a ship is placed
}

int is_position_empty(Player* player, int row, int col, int length, int horizontal) {   //Checks whether the position is valid or not
    for (int i = 0; i < length; i++) {
        int nrow = row + (horizontal ? 0 : i);
        int ncol = col + (horizontal ? i : 0);

        if (!is_within_bounds(nrow, ncol) || player->grid[nrow][ncol] != EMPTY)
            return 0;

        for (int dr = -1; dr <= 1; dr++) {          //Checks whether there are another ships or not around
            for (int dc = -1; dc <= 1; dc++) {
                int adj_row = nrow + dr;
                int adj_col = ncol + dc;
                if (is_within_bounds(adj_row, adj_col) && player->grid[adj_row][adj_col] == SHIP)
                    return 0;
            }
        }
    }
    return 1;
}

void place_ship_8x8(Player* player, int length) {           //Places ships for 8x8 game 
    int placed = 0;                                         //A boolean flag to determine has a ship placed
    while (!placed) {
        int row = rand() % grid_size;
        int col = rand() % grid_size;
        int horizontal = rand() % 2;

        if (is_position_empty(player, row, col, length, horizontal)) {  //Place a ship if the current position is valid
            for (int i = 0; i < length; i++) {
                int nrow = row + (horizontal ? 0 : i);
                int ncol = col + (horizontal ? i : 0);
                player->grid[nrow][ncol] = SHIP;             
                player->total_ship_cells++;
            }
            placed = 1;                 //To flag a ship has placed
        }
    }
}

int fire(Player* attacker, Player* defender, int row, int col) {    //Fire to a specified position with some checking
    if (!is_within_bounds(row, col) || attacker->opponent_grid[row][col] == HIT || attacker->opponent_grid[row][col] == MISS)       
        return 0;
    if (defender->grid[row][col] == SHIP) {
        defender->grid[row][col] = HIT;
        defender->hits++;
        attacker->opponent_grid[row][col] = HIT;
        attacker->score++;
        attacker->last_hit_row = row;
        attacker->last_hit_col = col;
        return 1;
    } else {
        defender->grid[row][col] = MISS;
        attacker->opponent_grid[row][col] = MISS;
        return 0;
    }
}

void random_fire(Player* attacker, int coords[2]) {         //Finds random coordinates then shoots 
    int row, col;
    do {
        row = rand() % grid_size;
        col = rand() % grid_size;
    } while (attacker->opponent_grid[row][col] == HIT || attacker->opponent_grid[row][col] == MISS);

    coords[0] = row;
    coords[1] = col;
}

void strategic_fire(Player* attacker, int coords[2]) {      //This is a basic AI for 8x8 game which makes some strategic fires referring its previous hit shots 
    int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    int valid_directions[4][2];
    int valid_count = 0;

    if (attacker->last_hit_row != -1 && attacker->last_hit_col != -1) {
        for (int i = 0; i < 4; i++) {
            int new_row = attacker->last_hit_row + directions[i][0];
            int new_col = attacker->last_hit_col + directions[i][1];
            if (is_within_bounds(new_row, new_col) && attacker->opponent_grid[new_row][new_col] == EMPTY) {
                valid_directions[valid_count][0] = new_row;
                valid_directions[valid_count][1] = new_col;
                valid_count++;
            }
        }
    }

    if (valid_count > 0) {
        int choice = rand() % valid_count;
        coords[0] = valid_directions[choice][0];
        coords[1] = valid_directions[choice][1];
    } else {
        random_fire(attacker, coords);
        attacker->last_hit_row = -1;
        attacker->last_hit_col = -1;
    }
}

void print_grids(Player* p1, Player* p2) {                 //Prints the grid
    clear();
    mvprintw(0, 0, "P1 Board%*sP2 Board", grid_size * 2, "");
    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            if (p1->grid[i][j] == SHIP) {
                attron(COLOR_PAIR(1));
                mvprintw(i + 1, j * 2, "%c ", p1->grid[i][j]);
                attroff(COLOR_PAIR(1));
            } else if (p1->grid[i][j] == HIT) {
                attron(COLOR_PAIR(2));
                mvprintw(i + 1, j * 2, "%c ", p1->grid[i][j]);
                attroff(COLOR_PAIR(2));
            } else if (p1->grid[i][j] == MISS) {
                attron(COLOR_PAIR(3));
                mvprintw(i + 1, j * 2, "%c ", p1->grid[i][j]);
                attroff(COLOR_PAIR(3));
            } else {
                mvprintw(i + 1, j * 2, "%c ", p1->grid[i][j]);
            }
        }
        mvprintw(i + 1, grid_size * 2, "   ");
        for (int j = 0; j < grid_size; j++) {
            if (p2->grid[i][j] == SHIP) {
                attron(COLOR_PAIR(1));
                mvprintw(i + 1, grid_size * 2 + 4 + j * 2, "%c ", p2->grid[i][j]);
                attroff(COLOR_PAIR(1));
            } else if (p2->grid[i][j] == HIT) {
                attron(COLOR_PAIR(2));
                mvprintw(i + 1, grid_size * 2 + 4 + j * 2, "%c ", p2->grid[i][j]);
                attroff(COLOR_PAIR(2));
            } else if (p2->grid[i][j] == MISS) {
                attron(COLOR_PAIR(3));
                mvprintw(i + 1, grid_size * 2 + 4 + j * 2, "%c ", p2->grid[i][j]);
                attroff(COLOR_PAIR(3));
            } else {
                mvprintw(i + 1, grid_size * 2 + 4 + j * 2, "%c ", p2->grid[i][j]);
            }
        }
    }
    mvprintw(grid_size + 2, 0, "P1 Score: %d", p1->score);
    mvprintw(grid_size + 3, 0, "P2 Score: %d", p2->score);
    refresh();
}

int display_menu() {  //Displays the menu on GUI
    clear();
    mvprintw(5, 10, "Battleship Game");
    mvprintw(7, 10, "1) 4x4 Board");
    mvprintw(8, 10, "2) 8x8 Board");
    mvprintw(9, 10, "3) Load the Game");
    mvprintw(10, 10, "4) Exit");
    mvprintw(12, 10, "Please choose an option:");
    refresh();

    int choice;
    while (1) {
        choice = getch();
        if (choice >= '1' && choice <= '4') {
            break;
        }
    }

    switch (choice) {
        case '1':
            grid_size = 4;
            return 1;
        case '2':
            grid_size = 8;
            return 1;
        case '3':
            return 2;
        case '4':
            return 0;
    }
    return 0;
}

void save_game(Player* p1, Player* p2) {        //Saves game
    FILE *file = fopen("save.dat", "wb");
    if (file == NULL) return;

    // Save game state
    fwrite(&grid_size, sizeof(int), 1, file);

    // Save player 1 data
    fwrite("P1", sizeof(char), 2, file);
    fwrite(p1, sizeof(Player), 1, file);

    // Save player 2 data
    fwrite("P2", sizeof(char), 2, file);
    fwrite(p2, sizeof(Player), 1, file);

    fclose(file);
}

int load_game(Player* p1, Player* p2) {        //Loads the game from where it is saved
    FILE *file = fopen("save.dat", "rb");
    if (file == NULL) return 0;

    // Load game state
    fread(&grid_size, sizeof(int), 1, file);

    char player_id[3];
    player_id[2] = '\0';  // Null-terminate the string

    // Load player 1 data
    fread(player_id, sizeof(char), 2, file);
    if (strcmp(player_id, "P1") == 0) {
        fread(p1, sizeof(Player), 1, file);
    } else {
        fclose(file);
        return 0;  // Invalid save file
    }

    // Load player 2 data
    fread(player_id, sizeof(char), 2, file);
    if (strcmp(player_id, "P2") == 0) {
        fread(p2, sizeof(Player), 1, file);
    } else {
        fclose(file);
        return 0;  // Invalid save file
    }

    fclose(file);

    if (p1->hits >= p1->total_ship_cells || p2->hits >= p2->total_ship_cells) {
        print_grids(p1, p2);
        mvprintw(grid_size + 5, 0, "This game has already ended. Press any key to return to menu.");
        refresh();
        getch();
        return 0;
    }

    return 1;
}

void child_process(int read_fd, int write_fd) {     //Shoots its shots and communicates with its parent
    Player p1, p2;
    int turn = 1;
    int coords[2];

    while (1) {
        char command;
        read(read_fd, &command, sizeof(char));

        if (command == 'I') {
            read(read_fd, &grid_size, sizeof(int));
            read(read_fd, &p1, sizeof(Player));
            read(read_fd, &p2, sizeof(Player));
        } else if (command == 'F') {
            Player *current_player = (turn == 1) ? &p1 : &p2;
            Player *opponent = (turn == 1) ? &p2 : &p1;

            strategic_fire(current_player, coords);
            int hit = fire(current_player, opponent, coords[0], coords[1]);

            write(write_fd, coords, sizeof(int) * 2);
            write(write_fd, &hit, sizeof(int));
            write(write_fd, current_player, sizeof(Player));
            write(write_fd, opponent, sizeof(Player));

            if (current_player->hits >= opponent->total_ship_cells) {
                char game_over = 1;
                write(write_fd, &game_over, sizeof(char));
            } else {
                char game_over = 0;
                write(write_fd, &game_over, sizeof(char));
            }

            turn = 3 - turn;
        } else if (command == 'Q') {
            break;
        }
    }
}

int main() {
    srand(time(NULL));
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_GREEN, COLOR_BLACK);

    int parent_to_child[2];
    int child_to_parent[2];
    char command;
    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1) {
        perror("Pipe failed");
        exit(1);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        close(parent_to_child[WRITE_END]);
        close(child_to_parent[READ_END]);
        child_process(parent_to_child[READ_END], child_to_parent[WRITE_END]);
        close(parent_to_child[READ_END]);
        close(child_to_parent[WRITE_END]);
        exit(0);
    } else {
        close(parent_to_child[READ_END]);
        close(child_to_parent[WRITE_END]);

        Player p1, p2;
        int game_loaded = 0;
        int turn = 1;

        while (1) {
            int menu_choice = display_menu();
            if (menu_choice == 0) {
                break;
            } else if (menu_choice == 2) {
                if (load_game(&p1, &p2)) {
                    game_loaded = 1;
                    turn = 1;
                    mvprintw(14, 10, "Game has been loaded. Press a key to continue...");
                    refresh();
                    getch();
                } else {
                    mvprintw(14, 10, "Game has been not loaded. Press a key to continue...");
                    refresh();
                    getch();
                    continue;
                }
            }

            if (!game_loaded) {
                initialize_grid(&p1, grid_size);
                initialize_grid(&p2, grid_size);

                if (grid_size == 4) {
                    for (int i = 0; i < small_ship_count; i++) {
                        place_ship_4x4(&p1);
                        place_ship_4x4(&p2);
                    }
                } else {
                    for (int i = 0; i < num_large_ships; i++) {
                        place_ship_8x8(&p1, large_ship_sizes[i]);
                        place_ship_8x8(&p2, large_ship_sizes[i]);
                    }
                }
            }

            command = 'I';
            write(parent_to_child[WRITE_END], &command, sizeof(char));
            write(parent_to_child[WRITE_END], &grid_size, sizeof(int));
            write(parent_to_child[WRITE_END], &p1, sizeof(Player));
            write(parent_to_child[WRITE_END], &p2, sizeof(Player));

            int game_over = 0;
            int coords[2];

            while (!game_over) {
                if (p1.hits >= p1.total_ship_cells || p2.hits >= p2.total_ship_cells) {
                    print_grids(&p1, &p2);
                    game_over = 1;
                    attron(COLOR_PAIR(4));
                    mvprintw(grid_size + 6, 0, "Player%d won!", (p1.hits >= p1.total_ship_cells) ? 2 : 1);
                    attroff(COLOR_PAIR(4));
                    refresh();

                    mvprintw(grid_size + 8, 0, "Choose an option:");
                    mvprintw(grid_size + 9, 0, "(M)enu  (E)xit");
                    refresh();

                    int choice;
                    do {
                        choice = getch();
                    } while (choice != 'm' && choice != 'M' && choice != 'e' && choice != 'E');

                    if (choice == 'e' || choice == 'E') {
                        endwin();
                        exit(0);
                    }
                    // If 'M' is chosen, we break out of this inner while loop and return to the outer loop (menu)
                    break;
                }

                print_grids(&p1, &p2);

                mvprintw(grid_size + 4, 0, "Player%d's Turn. Press 'p' to pause.", turn);
                refresh();

                // Check for 'p' key press without blocking
                timeout(0);
                int ch = getch();
                if (ch == 'p' || ch == 'P') {
                    timeout(-1); // Switch back to blocking mode
                    mvprintw(grid_size + 5, 0, "Game paused. (C)ontinue, (M)enu, (E)xit:");
                    refresh();
                    int choice = getch();
                    if (choice == 'm' || choice == 'M') {
                        break;
                    } else if (choice == 'e' || choice == 'E') {
                        endwin();
                        exit(0);
                    }

                    mvprintw(grid_size + 5, 0, "                                            ");
                }
                timeout(0); // Switch back to non-blocking mode

                command = 'F';
                write(parent_to_child[WRITE_END], &command, sizeof(char));

                read(child_to_parent[READ_END], coords, sizeof(int) * 2);
                int hit;
                read(child_to_parent[READ_END], &hit, sizeof(int));
                read(child_to_parent[READ_END], (turn == 1) ? &p1 : &p2, sizeof(Player));
                read(child_to_parent[READ_END], (turn == 1) ? &p2 : &p1, sizeof(Player));

                if (hit) {
                    mvprintw(grid_size + 5, 0, "Player%d shot (%d,%d). Successful Shot!", turn, coords[0], coords[1]);
                } else {
                    mvprintw(grid_size + 5, 0, "Player%d shot (%d,%d). Failed Shot.", turn, coords[0], coords[1]);
                }

                refresh();
                usleep(500000); // Sleep for 0.5 seconds to make the game progress visible

                char game_over_flag;
                read(child_to_parent[READ_END], &game_over_flag, sizeof(char));
                if (game_over_flag) {
                    attron(COLOR_PAIR(4));
                    mvprintw(grid_size + 6, 0, "Player%d won!", turn);
                    attroff(COLOR_PAIR(4));
                    game_over = 1;
                }

                turn = 3 - turn;

                if(turn == 1){
                    save_game(&p1, &p2);
                }
            }

            if (game_over) {
                save_game(&p1, &p2);
            }

            game_loaded = 0;
            
            continue;
        }

        command = 'Q';
        write(parent_to_child[WRITE_END], &command, sizeof(char));

        close(parent_to_child[WRITE_END]);
        close(child_to_parent[READ_END]);
        wait(NULL);
    }

    endwin();
    return 0;
}