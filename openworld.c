// openworld.c
// Simple turn-based open-world demo (terminal). C11, no external deps.
// Controls: w/a/s/d + Enter to move, i inspect/pickup, p show status, q quit.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Configurable world size */
#define MAP_W 40
#define MAP_H 20
#define MAX_NPCS 8
#define MAX_ITEMS 64
#define MAX_INVENTORY 16

/* Tile definitions */
typedef enum { T_PLAINS, T_FOREST, T_WATER, T_RUINS } TileType;
const char *tile_names[] = { "Plains", "Forest", "Water", "Ruins" };
const char tile_glyph[] = { '.', '♣', '~', '#' };

/* Simple RNG helpers */
static inline int rnd(int n) { return rand() % n; }

/* Item struct */
typedef struct {
    int id;
    char name[32];
    int x, y;
} Item;

/* NPC struct */
typedef struct {
    int id;
    char name[16];
    int x, y;
    int hp;
    int alive;
} NPC;

/* Player struct */
typedef struct {
    int x, y;
    int hp;
    int maxhp;
    Item inventory[MAX_INVENTORY];
    int inv_count;
} Player;

/* World data */
TileType map[MAP_H][MAP_W];
NPC npcs[MAX_NPCS];
int npc_count = 0;
Item items[MAX_ITEMS];
int item_count = 0;
Player player;
int time_of_day = 0; // 0..23 hours

/* Utility: clamp */
int clamp(int v, int a, int b) { if (v < a) return a; if (v > b) return b; return v; }

/* Generate procedural terrain using noise-like seeds (simple) */
void generate_map(void) {
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            int v = (x * 13 + y * 17 + rnd(100)) % 100;
            if (v < 55) map[y][x] = T_PLAINS;
            else if (v < 80) map[y][x] = T_FOREST;
            else if (v < 90) map[y][x] = T_RUINS;
            else map[y][x] = T_WATER;
        }
    }
    // carve a rough path/clearing
    int cx = MAP_W/2, cy = MAP_H/2;
    for (int i = 0; i < 200; ++i) {
        map[clamp(cy,0,MAP_H-1)][clamp(cx,0,MAP_W-1)] = T_PLAINS;
        cx += rnd(3)-1;
        cy += rnd(3)-1;
    }
}

/* Place player near center on a non-water tile */
void place_player(void) {
    int tries = 0;
    player.inv_count = 0;
    player.maxhp = 100;
    player.hp = 100;
    while (tries++ < 1000) {
        int x = MAP_W/2 + rnd(7)-3;
        int y = MAP_H/2 + rnd(7)-3;
        if (x >= 0 && x < MAP_W && y >= 0 && y < MAP_H && map[y][x] != T_WATER) {
            player.x = x; player.y = y; return;
        }
    }
    player.x = MAP_W/2; player.y = MAP_H/2;
}

/* Spawn items scattered on the map */
void spawn_items(void) {
    const char *names[] = { "Medkit", "Ration", "EncryptedDatapad", "Ammo", "StrangeTalisman" };
    int n_names = sizeof(names)/sizeof(names[0]);
    item_count = 0;
    for (int i = 0; i < 20; ++i) {
        int tries = 0;
        while (tries++ < 100) {
            int x = rnd(MAP_W), y = rnd(MAP_H);
            if (map[y][x] != T_WATER) {
                items[item_count].id = item_count;
                strncpy(items[item_count].name, names[rnd(n_names)], 31);
                items[item_count].x = x; items[item_count].y = y;
                item_count++; break;
            }
        }
    }
}

/* Spawn simple NPCs */
void spawn_npcs(void) {
    npc_count = 0;
    const char *mob_names[] = { "Scav", "Mutant", "Raider", "Howler" };
    int n_names = sizeof(mob_names)/sizeof(mob_names[0]);
    for (int i = 0; i < MAX_NPCS; ++i) {
        int tries = 0;
        while (tries++ < 200) {
            int x = rnd(MAP_W), y = rnd(MAP_H);
            if (map[y][x] != T_WATER && (abs(x-player.x)+abs(y-player.y)) > 6) {
                npcs[npc_count].id = npc_count;
                strncpy(npcs[npc_count].name, mob_names[rnd(n_names)], 15);
                npcs[npc_count].x = x; npcs[npc_count].y = y;
                npcs[npc_count].hp = 20 + rnd(20);
                npcs[npc_count].alive = 1;
                npc_count++; break;
            }
        }
    }
}

/* Render the visible map to terminal (simple) */
void render(void) {
    system("clear||cls");
    printf("The Shift: OpenWorld Demo | Time: %02d:00 | Player HP: %d/%d\n", time_of_day, player.hp, player.maxhp);
    printf("Controls: W/A/S/D + Enter to move, i inspect/pickup, p status, q quit\n");
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            char out = tile_glyph[ map[y][x] ];
            if (player.x == x && player.y == y) {
                printf("@"); continue;
            }
            int npc_here = 0;
            for (int n = 0; n < npc_count; ++n) {
                if (npcs[n].alive && npcs[n].x == x && npcs[n].y == y) { printf("N"); npc_here = 1; break; }
            }
            if (npc_here) continue;
            int item_here = 0;
            for (int it = 0; it < item_count; ++it) {
                if (items[it].id >= 0 && items[it].x == x && items[it].y == y) { printf("*"); item_here = 1; break; }
            }
            if (item_here) continue;
            putchar(out);
        }
        putchar('\n');
    }
}

/* Show a short description of the tile and what's on it */
void inspect_tile(void) {
    TileType t = map[player.y][player.x];
    printf("\nYou are on: %s\n", tile_names[t]);
    if (t == T_FOREST) printf("Trees and dense undergrowth. Movement is slower.\n");
    if (t == T_RUINS) printf("Broken concrete and twisted metal. Old tech may hide here.\n");
    if (t == T_WATER) printf("Water. Dangerous to cross.\n");
    int anyItem = 0;
    for (int i = 0; i < item_count; ++i) {
        if (items[i].id >= 0 && items[i].x == player.x && items[i].y == player.y) {
            printf("You see an item: %s\n", items[i].name); anyItem++;
        }
    }
    for (int n = 0; n < npc_count; ++n) {
        if (npcs[n].alive && npcs[n].x == player.x && npcs[n].y == player.y) {
            printf("A hostile %s stands before you (HP: %d)\n", npcs[n].name, npcs[n].hp);
        }
    }
    if (!anyItem) printf("No notable items here.\n");
}

/* Player picks up first item on tile (if any) */
void pickup_item(void) {
    for (int i = 0; i < item_count; ++i) {
        if (items[i].id >= 0 && items[i].x == player.x && items[i].y == player.y) {
            if (player.inv_count < MAX_INVENTORY) {
                player.inventory[player.inv_count++] = items[i];
                printf("Picked up: %s\n", items[i].name);
                items[i].id = -1; // removed
            } else {
                printf("Inventory full.\n");
            }
            return;
        }
    }
    printf("Nothing to pick up.\n");
}

/* Show player status/inventory */
void player_status(void) {
    printf("\n-- PLAYER STATUS --\nHP: %d/%d\nPosition: (%d,%d)\nInventory (%d):\n", player.hp, player.maxhp, player.x, player.y, player.inv_count);
    for (int i = 0; i < player.inv_count; ++i) {
        printf(" - %s\n", player.inventory[i].name);
    }
}

/* Simple combat: player attacks first, then npc if alive */
void attack_npc(int idx) {
    if (idx < 0 || idx >= npc_count || !npcs[idx].alive) return;
    int dmg = 8 + rnd(8);
    npcs[idx].hp -= dmg;
    printf("You strike the %s for %d damage.\n", npcs[idx].name, dmg);
    if (npcs[idx].hp <= 0) {
        npcs[idx].alive = 0;
        printf("The %s collapses.\n", npcs[idx].name);
        return;
    }
    // NPC retaliates
    int ndmg = 3 + rnd(8);
    player.hp -= ndmg;
    printf("The %s hits you for %d damage.\n", npcs[idx].name, ndmg);
    if (player.hp <= 0) {
        printf("You have been slain...\n");
    }
}

/* Find NPC at player's tile; return index or -1 */
int find_npc_at_player(void) {
    for (int n = 0; n < npc_count; ++n) {
        if (npcs[n].alive && npcs[n].x == player.x && npcs[n].y == player.y) return n;
    }
    return -1;
}

/* Simple NPC AI: random move or attack if on player */
void npc_turns(void) {
    for (int n = 0; n < npc_count; ++n) {
        if (!npcs[n].alive) continue;
        // if adjacent to player (including same tile), move towards or attack
        int dx = player.x - npcs[n].x;
        int dy = player.y - npcs[n].y;
        int dist = abs(dx) + abs(dy);
        if (dist == 0) {
            // same tile: immediate attack
            int ndmg = 2 + rnd(6);
            player.hp -= ndmg;
            printf("%s bites you for %d damage.\n", npcs[n].name, ndmg);
            if (player.hp <= 0) { printf("You were killed by %s.\n", npcs[n].name); }
            continue;
        } else if (dist <= 2 && rnd(100) < 50) {
            // move towards player
            if (abs(dx) > abs(dy)) npcs[n].x += (dx>0)?1:-1;
            else npcs[n].y += (dy>0)?1:-1;
        } else {
            // random wander
            int dir = rnd(5);
            if (dir == 0 && npcs[n].y > 0) npcs[n].y--;
            else if (dir == 1 && npcs[n].y < MAP_H-1) npcs[n].y++;
            else if (dir == 2 && npcs[n].x > 0) npcs[n].x--;
            else if (dir == 3 && npcs[n].x < MAP_W-1) npcs[n].x++;
        }
        // avoid water tiles
        if (map[npcs[n].y][npcs[n].x] == T_WATER) {
            // push back
            if (npcs[n].x > 0) npcs[n].x--;
            if (npcs[n].y > 0) npcs[n].y--;
        }
    }
}

/* Advance time and apply day/night effects */
void advance_time(void) {
    time_of_day = (time_of_day + 1) % 24;
    // Regenerate small HP at dawn
    if (time_of_day == 6) {
        player.hp = clamp(player.hp + 5, 0, player.maxhp);
        printf("Dawn: you feel slightly rested (+5 HP).\n");
    }
}

/* Read a single-line command from stdin */
void read_command(char *buf, size_t n) {
    if (!fgets(buf, n, stdin)) { buf[0] = 'q'; buf[1] = '\0'; return; }
    // trim newline
    size_t L = strlen(buf);
    if (L && buf[L-1] == '\n') buf[L-1] = '\0';
}

/* Execute player movement / actions */
void player_action(char cmd) {
    int nx = player.x, ny = player.y;
    if (cmd == 'w') ny--;
    else if (cmd == 's') ny++;
    else if (cmd == 'a') nx--;
    else if (cmd == 'd') nx++;
    else if (cmd == 'i') {
        inspect_tile();
        // if item present, attempt pickup
        pickup_item();
        return;
    } else if (cmd == 'p') {
        player_status(); return;
    } else if (cmd == 'q') {
        printf("Quitting...\n"); exit(0);
    } else {
        printf("Unknown command '%c'\n", cmd); return;
    }
    // bounds check
    if (nx < 0 || nx >= MAP_W || ny < 0 || ny >= MAP_H) {
        printf("You can't move there.\n"); return;
    }
    // water check
    if (map[ny][nx] == T_WATER) {
        printf("Deep water. You can't move into it right now.\n"); return;
    }
    // move
    player.x = nx; player.y = ny;
    // if NPC at new tile, auto-attack
    int nid = find_npc_at_player();
    if (nid >= 0) {
        printf("You encounter a %s!\n", npcs[nid].name);
        attack_npc(nid);
    }
}

/* Main loop */
int main(void) {
    srand((unsigned)time(NULL));
    generate_map();
    place_player();
    spawn_items();
    spawn_npcs();
    time_of_day = 8; // morning

    char buf[128];
    while (1) {
        render();
        if (player.hp <= 0) {
            printf("You have died. Game over.\n"); break;
        }
        printf("\nEnter command: ");
        read_command(buf, sizeof(buf));
        if (buf[0] == '\0') continue;
        char cmd = buf[0];
        player_action(cmd);
        // NPCs act after player (simple)
        npc_turns();
        // advance time slightly each turn
        advance_time();
        // small pause for readability
        printf("\n(Press Enter to continue...)\n");
        fgets(buf, sizeof(buf), stdin);
    }
    printf("Exiting. Thanks for testing the demo.\n");
    return 0;
}
