/*
 * STARTREK.C - SUPER STARTREK - MAY 16, 1978 (C17 Port - ANSI Color Edition)
 * 
 * Converted to standard C17.
 * Fully cross-platform: Windows 11 and Linux compatible.
 * All input parsing is strictly case-insensitive.
 * Includes Dynamic Scaling Grids with auto-truncating viewports & coordinate axes.
 * Command Line Arguments: --easy, --medium, --hard, --impossible, --kobayashi-maru=256x256
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

// Macro to mimic GW-BASIC's INT() which truncates toward negative infinity
#define B_INT(x) ((int)floor(x))
#define MAX_GRID 257       
#define MAX_K_PER_QUAD 128 

// --- ANSI COLOR MACROS ---
#define ANSI_COLOR_RED     "\x1b[91m" 
#define ANSI_COLOR_GREEN   "\x1b[92m" 
#define ANSI_COLOR_YELLOW  "\x1b[93m" 
#define ANSI_COLOR_BLUE    "\x1b[94m" 
#define ANSI_COLOR_CYAN    "\x1b[96m" 
#define ANSI_COLOR_WHITE   "\x1b[97m" 
#define ANSI_COLOR_RESET   "\x1b[0m"

// ==============================================================================
// --- GLOBAL GAME STATE ---
// ==============================================================================

int ARG_GRID_SIZE = 8;
int IS_KOBAYASHI = 0;

int S_GRID;          
double S_SCALE;      

// --- GRIDS, VECTORS & MAPS ---
int G[MAX_GRID][MAX_GRID];         
int Z[MAX_GRID][MAX_GRID];         
char Q[MAX_GRID][MAX_GRID][4];     
double C[10][3];                   
double K[MAX_K_PER_QUAD][5];       
double D[9];                       

// --- U.S.S. ENTERPRISE (STATUS & RESOURCES) ---
int Q1, Q2;          
double S1, S2;       
double E, E0;        
double S, S9;        
double P, P0;        
int D0;              

// --- MISSION & TIME PARAMETERS ---
double T, T0, T9;    
int K9, K7;          
int B9;              

// --- LOCAL QUADRANT ENVIRONMENT ---
int K3, B3, S3;      
int B4, B5;          
double D4;           

// --- END-GAME FLAGS ---
int destroyed = 0;   
int stranded = 0;    
int relieved = 0;    
// ==============================================================================

// --- UTILITY FUNCTIONS ---

void enable_ansi() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

double last_rnd = 0.0;
double RND(int type) {
    if (type == 0) return last_rnd;
    last_rnd = (double)rand() / RAND_MAX;
    if (last_rnd >= 1.0) last_rnd = 0.999999;
    return last_rnd;
}

int FNR() {
    return B_INT(RND(1) * S_GRID) + 1;
}

double FND(int i) {
    double scale = S_GRID / 8.0;
    if (scale < 1.0) scale = 1.0;
    double dist = sqrt(pow(K[i][1] - S1, 2) + pow(K[i][2] - S2, 2)) / scale;
    return dist < 1.0 ? 1.0 : dist;
}

void get_window_bounds(int pos, int max_size, int *start, int *end) {
    if (max_size <= 8) {
        *start = 1;
        *end = max_size;
    } else {
        *start = pos - 3;
        if (*start < 1) *start = 1;
        if (*start > max_size - 7) *start = max_size - 7;
        *end = *start + 7;
    }
}

int get_input_str(char* buf, int size) {
    if (!fgets(buf, size, stdin)) return 0;
    buf[strcspn(buf, "\r\n")] = '\0'; 
    char *start = buf;
    while(*start && isspace((unsigned char)*start)) start++;
    if (start != buf) memmove(buf, start, strlen(start) + 1);
    
    int len = strlen(buf);
    while(len > 0 && isspace((unsigned char)buf[len - 1])) {
        buf[len - 1] = '\0'; len--;
    }
    
    for (int i = 0; buf[i]; i++) buf[i] = toupper((unsigned char)buf[i]);
    return 1;
}

int get_input_double(double* val) {
    char buf[256];
    if (!get_input_str(buf, sizeof(buf))) return 0;
    if (sscanf(buf, "%lf", val) != 1) return 0;
    return 1;
}

const char* device_name(int r) {
    switch(r) {
        case 1: return "WARP ENGINES"; case 2: return "SHORT RANGE SENSORS";
        case 3: return "LONG RANGE SENSORS"; case 4: return "PHASER CONTROL";
        case 5: return "PHOTON TUBES"; case 6: return "DAMAGE CONTROL";
        case 7: return "SHIELD CONTROL"; case 8: return "LIBRARY-COMPUTER";
    }
    return "UNKNOWN";
}

void print_quadrant_name(int q1, int q2, int region_only) {
    int mapped_q1 = 1 + ((q1 - 1) * 8 / S_GRID);
    int mapped_q2 = 1 + ((q2 - 1) * 8 / S_GRID);
    if (mapped_q1 > 8) mapped_q1 = 8;
    if (mapped_q2 > 8) mapped_q2 = 8;
    if (mapped_q1 < 1) mapped_q1 = 1;
    if (mapped_q2 < 1) mapped_q2 = 1;
    
    const char* names1[] = { "", "ANTARES", "RIGEL", "PROCYON", "VEGA", "CANOPUS", "ALTAIR", "SAGITTARIUS", "POLLUX" };
    const char* names2[] = { "", "SIRIUS", "DENEB", "CAPELLA", "BETELGEUSE", "ALDEBARAN", "REGULUS", "ARCTURUS", "SPICA" };
    const char* region = (mapped_q2 <= 4) ? names1[mapped_q1] : names2[mapped_q1];
    
    printf("%s", region);
    if (!region_only) {
        const char* sub[] = { "", " I", " II", " III", " IV", " I", " II", " III", " IV" };
        printf("%s", sub[mapped_q2]);
    }
}

// Translates the physical strings to their dynamic color representations
void print_cell(const char* cell) {
    if (strcmp(cell, "<*>") == 0) {
        printf("%s<*>%s", ANSI_COLOR_WHITE, ANSI_COLOR_CYAN);
    } else if (strcmp(cell, "+K+") == 0) {
        printf("%s+K+%s", ANSI_COLOR_RED, ANSI_COLOR_CYAN);
    } else if (strcmp(cell, ">!<") == 0) {
        printf("%s>!<%s", ANSI_COLOR_BLUE, ANSI_COLOR_CYAN);
    } else if (strcmp(cell, " * ") == 0) {
        printf("%s * %s", ANSI_COLOR_YELLOW, ANSI_COLOR_CYAN);
    } else if (strcmp(cell, " @ ") == 0) {
        printf("%s @ %s", ANSI_COLOR_GREEN, ANSI_COLOR_CYAN);
    } else {
        printf("%s", cell);
    }
}

// --- FORWARD DECLARATIONS ---
void short_range_scan();
void klingons_shoot();
void cmd_dam();
void help();

// --- CORE MECHANICS ---

void enter_quadrant() {
    Z[Q1][Q2] = G[Q1][Q2];
    
    if (T == T0) {
        printf("\nYOUR MISSION BEGINS WITH YOUR STARSHIP LOCATED\n");
        printf("IN THE GALACTIC QUADRANT, '"); print_quadrant_name(Q1, Q2, 0); printf("'.\n");
    } else {
        printf("\nNOW ENTERING "); print_quadrant_name(Q1, Q2, 0); printf(" QUADRANT . . .\n");
    }
    
    K3 = G[Q1][Q2] / 100;
    B3 = (G[Q1][Q2] / 10) % 10;
    S3 = G[Q1][Q2] % 10;
    D4 = 0.5 * RND(1);
    
    if (K3 > 0) {
        printf("\n%sCOMBAT AREA      CONDITION RED%s\n", ANSI_COLOR_RED, ANSI_COLOR_CYAN);
        if (S <= 200.0) printf("   %sSHIELDS DANGEROUSLY LOW%s\n", ANSI_COLOR_YELLOW, ANSI_COLOR_CYAN);
    }
    
    for (int i = 1; i < MAX_K_PER_QUAD; i++) { K[i][1] = 0; K[i][2] = 0; K[i][3] = 0; K[i][4] = 0; }
    for (int i = 1; i <= S_GRID; i++) {
        for (int j = 1; j <= S_GRID; j++) strcpy(Q[i][j], "   ");
    }
    
    strcpy(Q[(int)S1][(int)S2], "<*>");
    
    // Position Klingons & Assign AI (50/50 Guard vs Chase)
    for (int i = 1; i <= K3; i++) {
        if (i >= MAX_K_PER_QUAD) break;
        int r1, r2;
        do { r1 = FNR(); r2 = FNR(); } while (strcmp(Q[r1][r2], "   ") != 0);
        strcpy(Q[r1][r2], "+K+");
        K[i][1] = r1; K[i][2] = r2;
        K[i][3] = S9 * (0.5 + RND(1));
        K[i][4] = rand() % 2; 
    }
    
    B4 = 0; B5 = 0;
    if (B3 > 0) {
        int r1, r2;
        do { r1 = FNR(); r2 = FNR(); } while (strcmp(Q[r1][r2], "   ") != 0);
        strcpy(Q[r1][r2], ">!<");
        B4 = r1; B5 = r2;
    }
    
    for (int i = 1; i <= S3; i++) {
        int r1, r2;
        do { r1 = FNR(); r2 = FNR(); } while (strcmp(Q[r1][r2], "   ") != 0);
        
        // 25% chance of planetary generation
        if (rand() % 4 == 0) {
            strcpy(Q[r1][r2], " @ ");
        } else {
            strcpy(Q[r1][r2], " * ");
        }
    }
    
    printf("\n");
    short_range_scan();
}

void setup_game() {
    S_GRID = ARG_GRID_SIZE;
    S_SCALE = S_GRID / 8.0;

    memset(G, 0, sizeof(G)); memset(Z, 0, sizeof(Z)); memset(D, 0, sizeof(D));
    destroyed = 0; stranded = 0; relieved = 0;
    
    C[1][1]=0;  C[1][2]=1; C[2][1]=-1; C[2][2]=1; C[3][1]=-1; C[3][2]=0;
    C[4][1]=-1; C[4][2]=-1; C[5][1]=0; C[5][2]=-1; C[6][1]=1; C[6][2]=-1;
    C[7][1]=1;  C[7][2]=0; C[8][1]=1;  C[8][2]=1; C[9][1]=0;  C[9][2]=1;

    T = B_INT(RND(1) * 20.0 + 20.0) * 100.0; T0 = T;

    double scale_t9 = S_SCALE;
    double scale_k9 = S_SCALE;
    double scale_active = (S_GRID == 8) ? 0.5 : (S_GRID / 16.0);
    if (S_GRID <= 4) scale_active = 0.25;

    double random_orig_time = 25.0 + B_INT(RND(1) * 10.0);
    T9 = random_orig_time * scale_t9;

    int orig_K9 = 0;
    for (int i = 1; i <= 8; i++) {
        for (int j = 1; j <= 8; j++) {
            double r1 = RND(1);
            if (r1 > 0.98) orig_K9 += 3;
            else if (r1 > 0.95) orig_K9 += 2;
            else if (r1 > 0.80) orig_K9 += 1;
        }
    }
    K9 = (int)(orig_K9 * scale_k9);
    if (K9 < 1) K9 = 1;
    if (K9 > T9) T9 = K9 + 1; 
    K7 = K9;
    
    int max_active = (int)ceil(3.0 * scale_active);
    if (max_active < 1) max_active = 1;
    if (max_active > MAX_K_PER_QUAD - 1) max_active = MAX_K_PER_QUAD - 1;
    
    int k_placed = 0;
    while (k_placed < K9) {
        int qx = FNR(), qy = FNR();
        int clump = (rand() % max_active) + 1;
        if (k_placed + clump > K9) clump = K9 - k_placed;
        
        if ((G[qx][qy] / 100) + clump <= max_active) {
            G[qx][qy] += clump * 100;
            k_placed += clump;
        }
    }
    
    int r_param = S_GRID / 4;
    if (r_param < 1) r_param = 1; 
    
    B9 = ((rand() % r_param) + 1) * r_param; 
    if (B9 > K9 / 5) B9 = K9 / 5; 
    if (B9 < 1) B9 = 1;           

    int b_placed = 0;
    while (b_placed < B9) {
        int qx = FNR(), qy = FNR();
        if (((G[qx][qy] / 10) % 10) == 0) { 
            G[qx][qy] += 10; 
            b_placed++; 
        }
    }

    if (IS_KOBAYASHI) {
        E0 = 500.0 + (rand() % 8001); E = E0;
        P0 = (rand() % 30); P = P0;
        S9 = 50.0 + (rand() % 600); 
        for (int i=1; i<=8; i++) {
            if (rand() % 3 == 0) D[i] = -((rand() % 10) + 1.0); 
        }
    } else {
        E0 = 2000.0 + (rand() % 3001); E = E0;
        P0 = 5.0 + (rand() % 16); P = P0;
        S9 = 100.0 + (rand() % 301); 
    }
    D0 = 0; S = 0;

    for (int i = 1; i <= S_GRID; i++) {
        for (int j = 1; j <= S_GRID; j++) {
            G[i][j] += (rand() % 8) + 1;
        }
    }
    
    Q1 = FNR(); Q2 = FNR(); S1 = FNR(); S2 = FNR();
    
    printf("\n*** STARFLEET INTELLIGENCE REPORT ***\n");
    if (IS_KOBAYASHI) printf(">> %sWARNING: KOBAYASHI MARU PROTOCOL ACTIVE%s <<\n", ANSI_COLOR_RED, ANSI_COLOR_CYAN);
    printf("GALAXY SIZE: %dx%d QUADRANTS\n", S_GRID, S_GRID);
    printf("QUADRANT SIZE: %dx%d SECTORS\n\n", S_GRID, S_GRID);

    printf("YOUR ORDERS ARE AS FOLLOWS:\n");
    printf("     DESTROY THE %s%d%s KLINGON WARSHIPS WHICH HAVE INVADED\n", ANSI_COLOR_RED, K9, ANSI_COLOR_CYAN);
    printf("   THE GALAXY BEFORE THEY CAN ATTACK FEDERATION HEADQUARTERS\n");
    printf("   ON STARDATE %s%.1f%s  THIS GIVES YOU %s%d%s DAYS.  THERE %s\n", ANSI_COLOR_WHITE, T0 + T9, ANSI_COLOR_CYAN, ANSI_COLOR_RED, (int)T9, ANSI_COLOR_CYAN, (B9 != 1 ? " ARE " : " IS "));
    printf("  %s%d%s STARBASE%s IN THE GALAXY FOR RESUPPLYING YOUR SHIP\n\n", ANSI_COLOR_BLUE, B9, ANSI_COLOR_CYAN, (B9 != 1 ? "S" : ""));
}

// --- SHIP COMMANDS ---

void short_range_scan() {
    const char* C_str = "GREEN";
    const char* cond_color = ANSI_COLOR_GREEN;

    if (E < E0 * 0.1) {
        C_str = "YELLOW";
        cond_color = ANSI_COLOR_YELLOW;
    }
    if (K3 > 0) {
        C_str = "*RED*";
        cond_color = ANSI_COLOR_RED;
    }
    
    D0 = 0;
    for (int i = B_INT(S1) - 1; i <= B_INT(S1) + 1; i++) {
        for (int j = B_INT(S2) - 1; j <= B_INT(S2) + 1; j++) {
            if (i >= 1 && i <= S_GRID && j >= 1 && j <= S_GRID) {
                if (strcmp(Q[i][j], ">!<") == 0) D0 = 1;
            }
        }
    }
    
    if (D0 == 1) {
        C_str = "DOCKED"; 
        cond_color = ANSI_COLOR_BLUE;
        E = E0; P = P0;
        printf("%sSHIELDS DROPPED FOR DOCKING PURPOSES%s\n", ANSI_COLOR_YELLOW, ANSI_COLOR_CYAN);
        S = 0;
    }
    if (D[2] < 0) { printf("\n%s*** SHORT RANGE SENSORS ARE OUT ***%s\n\n", ANSI_COLOR_RED, ANSI_COLOR_CYAN); return; }
    
    int start_x, end_x, start_y, end_y;
    get_window_bounds(B_INT(S1), S_GRID, &start_x, &end_x);
    get_window_bounds(B_INT(S2), S_GRID, &start_y, &end_y);

    int display_rows = (end_x - start_x + 1);
    if (display_rows < 9) display_rows = 9;

    printf("\n    ");
    for (int j = start_y; j <= end_y; j++) {
        printf(" %2d ", j);
    }
    printf("\n");
    
    for (int row = 1; row <= display_rows; row++) {
        int i = start_x + row - 1;
        int chars_printed = 0;
        
        if (i <= end_x) {
            printf(" %2d ", i);
            chars_printed += 4;
            
            for (int j = start_y; j <= end_y; j++) {
                printf(" "); 
                print_cell(Q[i][j]);
                chars_printed += 4;
            }
        } else {
            printf("    ");
            chars_printed += 4;
            for (int j = start_y; j <= end_y; j++) {
                printf("    ");
                chars_printed += 4;
            }
        }
        
        while (chars_printed < 36) {
            printf(" ");
            chars_printed++;
        }
        
        printf(" ");
        switch(row) {
            case 1: printf("%sSTARDATE           %s%.1f%s", ANSI_COLOR_WHITE, ANSI_COLOR_WHITE, T, ANSI_COLOR_CYAN); break;
            case 2: printf("%sCONDITION          %s%s%s", ANSI_COLOR_WHITE, cond_color, C_str, ANSI_COLOR_CYAN); break;
            case 3: printf("%sQUADRANT           %s%d,%d%s", ANSI_COLOR_WHITE, ANSI_COLOR_WHITE, Q1, Q2, ANSI_COLOR_CYAN); break;
            case 4: printf("%sSECTOR             %s%d,%d%s", ANSI_COLOR_WHITE, ANSI_COLOR_WHITE, (int)S1, (int)S2, ANSI_COLOR_CYAN); break;
            case 5: {
                const char* p_col = (P < (P0 * 0.25)) ? ANSI_COLOR_RED : ANSI_COLOR_WHITE;
                printf("%sPHOTON TORPEDOES   %s%d%s", ANSI_COLOR_WHITE, p_col, (int)P, ANSI_COLOR_CYAN); 
                break;
            }
            case 6: {
                const char* e_col = ((E+S) < (E0 * 0.25)) ? ANSI_COLOR_RED : ANSI_COLOR_WHITE;
                printf("%sTOTAL ENERGY       %s%d%s", ANSI_COLOR_WHITE, e_col, (int)(E+S), ANSI_COLOR_CYAN); 
                break;
            }
            case 7: {
                const char* s_col; const char* s_txt;
                if (D[7] < 0) { s_col = ANSI_COLOR_RED; s_txt = "DISABLED"; }
                else if (S == 0) { s_col = ANSI_COLOR_WHITE; s_txt = "OFF"; }
                else if (S < 400) { s_col = ANSI_COLOR_YELLOW; s_txt = "DAMAGED"; }
                else { s_col = ANSI_COLOR_GREEN; s_txt = "ON"; }
                printf("%sSHIELDS            %s%s (%.0f)%s", ANSI_COLOR_WHITE, s_col, s_txt, S, ANSI_COLOR_CYAN); 
                break;
            }
            case 8: printf("%sKLINGONS REMAINING %s%d%s", ANSI_COLOR_WHITE, ANSI_COLOR_RED, K9, ANSI_COLOR_CYAN); break;
            case 9: printf("%sDAYS REMAINING     %s%.1f%s", ANSI_COLOR_WHITE, ANSI_COLOR_RED, (T0 + T9 - T), ANSI_COLOR_CYAN); break;
        }
        printf("\n");
    }
    printf("\n");
}

void cmd_nav() {
    double C1_in;
    printf("COURSE (0-9)? "); 
    if (!get_input_double(&C1_in)) return;
    if (C1_in == 9.0) C1_in = 1.0;
    if (C1_in < 1.0 || C1_in >= 9.0) {
        printf("   LT. SULU REPORTS, 'INCORRECT COURSE DATA, SIR!'\n"); return;
    }
    
    char X_str[8] = "8";
    if (D[1] < 0) strcpy(X_str, "0.2");
    
    double W1;
    printf("WARP FACTOR (0-%s)? ", X_str); 
    if (!get_input_double(&W1)) return;
    
    if (D[1] < 0 && W1 > 0.2) { printf("WARP ENGINES ARE DAMAGED.  MAXIMUM SPEED = WARP 0.2\n"); return; }
    if (W1 == 0.0) return;
    if (W1 < 0.0 || W1 > 8.0) {
        printf("   CHIEF ENGINEER SCOTT REPORTS 'THE ENGINES WON'T TAKE WARP %.1f!'\n", W1); return;
    }
    
    int N_steps = B_INT(W1 * S_GRID + 0.5);
    double normalized_energy_cost = (N_steps * 8.0) / S_GRID + 10.0;
    
    if (E - normalized_energy_cost < 0) {
        printf("ENGINEERING REPORTS   'INSUFFICIENT ENERGY AVAILABLE\n");
        printf("                       FOR MANEUVERING AT WARP %.1f!'\n", W1);
        if (S < normalized_energy_cost - E || D[7] < 0) return;
        printf("DEFLECTOR CONTROL ROOM ACKNOWLEDGES %d UNITS OF ENERGY\n", (int)S);
        printf("                         PRESENTLY DEPLOYED TO SHIELDS.\n");
        return;
    }
    
    for (int i = 1; i < MAX_K_PER_QUAD; i++) {
        if (K[i][3] <= 0) continue;
        
        int r1 = K[i][1];
        int r2 = K[i][2];
        strcpy(Q[r1][r2], "   ");
        
        if (K[i][4] == 1) { 
            int dx = (S1 > r1) ? 1 : ((S1 < r1) ? -1 : 0);
            int dy = (S2 > r2) ? 1 : ((S2 < r2) ? -1 : 0);
            
            if (dx != 0 || dy != 0) {
                if (r1+dx >= 1 && r1+dx <= S_GRID && r2+dy >= 1 && r2+dy <= S_GRID && strcmp(Q[r1+dx][r2+dy], "   ") == 0) {
                    r1 += dx; r2 += dy;
                } else if (dx != 0 && r1+dx >= 1 && r1+dx <= S_GRID && strcmp(Q[r1+dx][r2], "   ") == 0) {
                    r1 += dx;
                } else if (dy != 0 && r2+dy >= 1 && r2+dy <= S_GRID && strcmp(Q[r1][r2+dy], "   ") == 0) {
                    r2 += dy;
                }
            }
        }
        
        K[i][1] = r1; 
        K[i][2] = r2;
        strcpy(Q[r1][r2], "+K+");
    }
    
    klingons_shoot();
    if (S < 0) return; 
    
    int D1 = 0;
    double D6 = (W1 >= 1.0) ? 1.0 : W1;
    for (int i = 1; i <= 8; i++) {
        if (D[i] < 0) {
            D[i] += D6;
            if (D[i] > -0.1 && D[i] < 0) D[i] = -0.1;
            else if (D[i] >= 0) {
                if (D1 != 1) { D1 = 1; printf("DAMAGE CONTROL REPORT:  \n"); }
                printf("        %s REPAIR COMPLETED.\n", device_name(i));
            }
        }
    }
    
    if (RND(1) <= 0.2) {
        int r1 = (rand() % 8) + 1;
        if (RND(1) < 0.6) {
            D[r1] -= (RND(1) * 5.0 + 1.0);
            printf("DAMAGE CONTROL REPORT:  \n        %s%s DAMAGED%s\n\n", ANSI_COLOR_RED, device_name(r1), ANSI_COLOR_CYAN);
        } else {
            D[r1] += (RND(1) * 3.0 + 1.0);
            printf("DAMAGE CONTROL REPORT:  \n        %s%s STATE OF REPAIR IMPROVED%s\n\n", ANSI_COLOR_GREEN, device_name(r1), ANSI_COLOR_CYAN);
        }
    }
    
    strcpy(Q[(int)S1][(int)S2], "   ");
    int C1_int = B_INT(C1_in);
    double X1 = C[C1_int][1] + (C[C1_int+1][1] - C[C1_int][1]) * (C1_in - C1_int);
    double X2 = C[C1_int][2] + (C[C1_int+1][2] - C[C1_int][2]) * (C1_in - C1_int);
    double x = S1, y = S2;
    int q4 = Q1, q5 = Q2;
    double S1_orig = S1, S2_orig = S2;
    
    for (int i = 1; i <= N_steps; i++) {
        x += X1; y += X2;
        if (x < 1.0 || x >= S_GRID + 1.0 || y < 1.0 || y >= S_GRID + 1.0) {
            double final_x = S_GRID * (q4 - 1) + S1_orig + N_steps * X1;
            double final_y = S_GRID * (q5 - 1) + S2_orig + N_steps * X2;
            int next_q1 = B_INT(final_x / (double)S_GRID) + 1; 
            int next_q2 = B_INT(final_y / (double)S_GRID) + 1;
            
            S1 = B_INT(final_x - (next_q1 - 1) * S_GRID); 
            S2 = B_INT(final_y - (next_q2 - 1) * S_GRID);
            if (S1 <= 0) { next_q1--; S1 += S_GRID; }
            if (S2 <= 0) { next_q2--; S2 += S_GRID; }
            
            int x5 = 0;
            if (next_q1 < 1) { x5 = 1; next_q1 = 1; S1 = 1; }
            if (next_q1 > S_GRID) { x5 = 1; next_q1 = S_GRID; S1 = S_GRID; }
            if (next_q2 < 1) { x5 = 1; next_q2 = 1; S2 = 1; }
            if (next_q2 > S_GRID) { x5 = 1; next_q2 = S_GRID; S2 = S_GRID; }
            
            Q1 = next_q1; Q2 = next_q2;
            if (x5 == 1) {
                printf("LT. UHURA REPORTS MESSAGE FROM STARFLEET COMMAND:\n");
                printf("  'PERMISSION TO ATTEMPT CROSSING OF GALACTIC PERIMETER\n");
                printf("  IS HEREBY *DENIED*.  SHUT DOWN YOUR ENGINES.'\n");
                printf("CHIEF ENGINEER SCOTT REPORTS  'WARP ENGINES SHUT DOWN\n");
                printf("  AT SECTOR %d , %d OF QUADRANT %d , %d .'\n", (int)S1, (int)S2, Q1, Q2);
            }
            if (T > T0 + T9) return;
            if (Q1 == q4 && Q2 == q5) goto place_enterprise;
            
            T++; E -= normalized_energy_cost;
            if (E < 0) {
                printf("SHIELD CONTROL SUPPLIES ENERGY TO COMPLETE THE MANEUVER.\n");
                S = S + E; E = 0; if (S <= 0) S = 0;
            }
            enter_quadrant();
            return;
        }
        
        if (strcmp(Q[B_INT(x)][B_INT(y)], "   ") != 0) {
            x -= X1; y -= X2;
            printf("WARP ENGINES SHUT DOWN AT SECTOR %d , %d DUE TO BAD NAVIGATION\n", B_INT(x), B_INT(y));
            break;
        }
    }
    
place_enterprise:
    S1 = B_INT(x); S2 = B_INT(y);
    strcpy(Q[(int)S1][(int)S2], "<*>");
    double T8 = 1.0;
    if (W1 < 1.0) T8 = 0.1 * B_INT(10.0 * W1);
    T += T8;
    
    E -= normalized_energy_cost;
    if (E < 0) {
        printf("SHIELD CONTROL SUPPLIES ENERGY TO COMPLETE THE MANEUVER.\n");
        S = S + E; E = 0; if (S <= 0) S = 0;
    }
    
    if (T > T0 + T9) return;
    short_range_scan();
}

void klingons_shoot() {
    if (K3 <= 0) return;
    if (D0 != 0) { printf("%sSTARBASE SHIELDS PROTECT THE ENTERPRISE%s\n", ANSI_COLOR_BLUE, ANSI_COLOR_CYAN); return; }
    for (int i = 1; i < MAX_K_PER_QUAD; i++) {
        if (K[i][3] <= 0) continue;
        double H = B_INT((K[i][3] / FND(i)) * (2.0 + RND(1)));
        
        if (IS_KOBAYASHI) H *= (RND(1) * 3.0 + 1.0); 
        
        S -= H; K[i][3] = K[i][3] / (3.0 + RND(0));
        printf("%s%d UNIT HIT ON ENTERPRISE FROM SECTOR %d , %d%s\n", ANSI_COLOR_RED, (int)H, (int)K[i][1], (int)K[i][2], ANSI_COLOR_CYAN);
        if (S <= 0) { destroyed = 1; return; }
        printf("      <SHIELDS DOWN TO %d UNITS>\n", (int)S);
        
        if (H < 20 || RND(1) > 0.6 || H / S <= 0.02) continue;
        int r1 = (rand() % 8) + 1;
        D[r1] -= (H / S + 0.5 * RND(1));
        printf("DAMAGE CONTROL REPORTS %s DAMAGED BY THE HIT'\n", device_name(r1));
    }
}

void cmd_pha() {
    if (D[4] < 0) { printf("PHASERS INOPERATIVE\n"); return; }
    if (K3 <= 0) {
        printf("SCIENCE OFFICER SPOCK REPORTS  'SENSORS SHOW NO ENEMY SHIPS\n                                IN THIS QUADRANT'\n"); return;
    }
    if (D[8] < 0) printf("COMPUTER FAILURE HAMPERS ACCURACY\n");
    printf("PHASERS LOCKED ON TARGET;  ENERGY AVAILABLE = %d UNITS\n", (int)E);
    double X;
    while(1) {
        printf("NUMBER OF UNITS TO FIRE? "); 
        if(!get_input_double(&X)) return;
        if (X <= 0) return;
        if (E - X >= 0) break;
    }
    E -= X;
    if (D[7] < 0) X = X * RND(1);
    
    double H1 = B_INT(X / K3);
    for (int i = 1; i < MAX_K_PER_QUAD; i++) {
        if (K[i][3] <= 0) continue;
        double H = B_INT((H1 / FND(i)) * (RND(1) + 2.0));
        if (H <= 0.15 * K[i][3]) {
            printf("SENSORS SHOW NO DAMAGE TO ENEMY AT %d , %d\n", (int)K[i][1], (int)K[i][2]); continue;
        }
        K[i][3] -= H;
        printf("%d UNIT HIT ON KLINGON AT SECTOR %d , %d\n", (int)H, (int)K[i][1], (int)K[i][2]);
        if (K[i][3] <= 0) {
            printf("%s*** KLINGON DESTROYED ***%s\n", ANSI_COLOR_RED, ANSI_COLOR_CYAN);
            K3--; K9--; strcpy(Q[(int)K[i][1]][(int)K[i][2]], "   "); K[i][3] = 0;
            G[Q1][Q2] -= 100; Z[Q1][Q2] = G[Q1][Q2];
            if (K9 <= 0) return;
        } else {
            printf("   (SENSORS SHOW %d UNITS REMAINING)\n", (int)K[i][3]);
        }
    }
    klingons_shoot();
}

void cmd_tor() {
    if (P <= 0) { printf("ALL PHOTON TORPEDOES EXPENDED\n"); return; }
    if (D[5] < 0) { printf("PHOTON TUBES ARE NOT OPERATIONAL\n"); return; }
    
    double C1_in; printf("PHOTON TORPEDO COURSE (1-9)? ");
    if(!get_input_double(&C1_in)) return;
    if (C1_in == 9.0) C1_in = 1.0;
    if (C1_in < 1.0 || C1_in >= 9.0) { printf("ENSIGN CHEKOV REPORTS,  'INCORRECT COURSE DATA, SIR!'\n"); return; }
    
    int C1_int = B_INT(C1_in);
    double X1 = C[C1_int][1] + (C[C1_int+1][1] - C[C1_int][1]) * (C1_in - C1_int);
    double X2 = C[C1_int][2] + (C[C1_int+1][2] - C[C1_int][2]) * (C1_in - C1_int);
    
    E -= 2; P -= 1;
    double x = S1, y = S2;
    printf("TORPEDO TRACK:\n");
    while(1) {
        x += X1; y += X2;
        int x3 = B_INT(x + 0.5); int y3 = B_INT(y + 0.5);
        if (x3 < 1 || x3 > S_GRID || y3 < 1 || y3 > S_GRID) { printf("TORPEDO MISSED\n"); break; }
        printf("               %d , %d\n", x3, y3);
        if (strcmp(Q[x3][y3], "   ") != 0) {
            if (strcmp(Q[x3][y3], "+K+") == 0) {
                printf("%s*** KLINGON DESTROYED ***%s\n", ANSI_COLOR_RED, ANSI_COLOR_CYAN);
                K3--; K9--;
                for (int i=1; i < MAX_K_PER_QUAD; i++) { 
                    if (x3 == (int)K[i][1] && y3 == (int)K[i][2]) { 
                        K[i][3] = 0; break; 
                    } 
                }
                if (K9 <= 0) return;
            } else if (strcmp(Q[x3][y3], " * ") == 0) {
                printf("%sSTAR AT %d , %d ABSORBED TORPEDO ENERGY.%s\n", ANSI_COLOR_YELLOW, x3, y3, ANSI_COLOR_CYAN); break;
            } else if (strcmp(Q[x3][y3], " @ ") == 0) {
                printf("%sPLANET AT %d , %d ABSORBED TORPEDO ENERGY.%s\n", ANSI_COLOR_GREEN, x3, y3, ANSI_COLOR_CYAN); break;
            } else if (strcmp(Q[x3][y3], ">!<") == 0) {
                printf("%s*** STARBASE DESTROYED ***%s\n", ANSI_COLOR_BLUE, ANSI_COLOR_CYAN); B3--; B9--;
                if (B9 > 0 || K9 > T - T0 - T9) { 
                    printf("STARFLEET COMMAND REVIEWING YOUR RECORD TO CONSIDER\nCOURT MARTIAL!\n"); D0 = 0;
                } else {
                    printf("THAT DOES IT, CAPTAIN!!  YOU ARE HEREBY RELIEVED OF COMMAND\n");
                    printf("AND SENTENCED TO 99 STARDATES AT HARD LABOR ON CYGNUS 12!!\n");
                    relieved = 1; return;
                }
            }
            strcpy(Q[x3][y3], "   ");
            G[Q1][Q2] = K3*100 + B3*10 + S3; Z[Q1][Q2] = G[Q1][Q2];
            break;
        }
    }
    klingons_shoot();
}

void cmd_she() {
    if (D[7] < 0) { printf("SHIELD CONTROL INOPERABLE\n"); return; }
    printf("ENERGY AVAILABLE = %d\n", (int)(E + S));
    double X; printf("NUMBER OF UNITS TO SHIELDS? "); 
    if(!get_input_double(&X)) return;
    if (X < 0 || S == X) { printf("<SHIELDS UNCHANGED>\n"); return; }
    if (X > E + S) {
        printf("SHIELD CONTROL REPORTS  'THIS IS NOT THE FEDERATION TREASURY.'\n<SHIELDS UNCHANGED>\n"); return;
    }
    E = E + S - X; S = X;
    printf("DEFLECTOR CONTROL ROOM REPORT:\n  'SHIELDS NOW AT %d UNITS PER YOUR COMMAND.'\n", (int)S);
}

void cmd_dam() {
    if (D[6] < 0) {
        printf("DAMAGE CONTROL REPORT NOT AVAILABLE\n");
        if (D0 == 0) return;
    } else {
        printf("\nDEVICE             STATE OF REPAIR\n");
        for (int r1 = 1; r1 <= 8; r1++) {
            const char* name = device_name(r1);
            int pad = 25 - strlen(name);
            printf("%s", name);
            for(int s=0; s<pad; s++) printf(" ");
            printf("%.2f\n", (double)B_INT(D[r1] * 100.0) * 0.01);
        }
        printf("\n");
        if (D0 == 0) return;
    }
    
    double D3 = 0;
    for (int i = 1; i <= 8; i++) {
        if (D[i] < 0) D3 += 0.1;
    }
    if (D3 == 0) return;
    
    D3 += D4; if (D3 >= 1.0) D3 = 0.9;
    printf("TECHNICIANS STANDING BY TO EFFECT REPAIRS TO YOUR SHIP;\n");
    printf("ESTIMATED TIME TO REPAIR: %.2f STARDATES\n", 0.01 * B_INT(100.0 * D3));
    printf("WILL YOU AUTHORIZE THE REPAIR ORDER (Y/N)? ");
    
    char A[16]; 
    if (!get_input_str(A, sizeof(A))) return;
    if (A[0] == 'Y') {
        for (int i = 1; i <= 8; i++) {
            if (D[i] < 0) D[i] = 0;
        }
        T += D3 + 0.1;
    }
}

void calc_dir_dist(double c1, double a, double w1, double x) {
    x = x - a; 
    a = c1 - w1;
    
    if (x < 0) { goto L8350; }
    if (a < 0) { goto L8410; }
    if (x > 0) { goto L8280; }
    if (a == 0) { c1 = 5.0; goto L8290; }

L8280: 
    c1 = 1.0;
L8290: 
    if (fabs(a) <= fabs(x)) { goto L8330; }
    printf("DIRECTION = %.2f\n", c1 + (((fabs(a) - fabs(x)) + fabs(a)) / fabs(a))); 
    goto L8460;
    
L8330: 
    if (fabs(x) == 0.0) { printf("DIRECTION = %.2f\n", c1); } 
    else { printf("DIRECTION = %.2f\n", c1 + (fabs(a) / fabs(x))); }
    goto L8460;
    
L8350: 
    if (a > 0) { c1 = 3.0; goto L8420; }
    if (x != 0.0) { c1 = 5.0; goto L8290; }
    
L8410: 
    c1 = 7.0;
L8420: 
    if (fabs(a) >= fabs(x)) { goto L8450; }
    printf("DIRECTION = %.2f\n", c1 + (((fabs(x) - fabs(a)) + fabs(x)) / fabs(x))); 
    goto L8460;
    
L8450: 
    if (fabs(a) == 0.0) { printf("DIRECTION = %.2f\n", c1); } 
    else { printf("DIRECTION = %.2f\n", c1 + (fabs(x) / fabs(a))); }

L8460: 
    printf("DISTANCE = %.2f\n", sqrt(x*x + a*a));
}

void cmd_com() {
    if (D[8] < 0) { 
        printf("COMPUTER DISABLED\n"); 
        return; 
    }
    
    while (1) {
        double cmd; 
        printf("COMPUTER ACTIVE AND AWAITING COMMAND? "); 
        if(!get_input_double(&cmd)) return;
        
        if (cmd < 0) { return; }
        printf("\n");
        
        switch ((int)cmd) {
            case 0: {
                int start_x, end_x, start_y, end_y;
                get_window_bounds(Q1, S_GRID, &start_x, &end_x);
                get_window_bounds(Q2, S_GRID, &start_y, &end_y);

                printf("        COMPUTER RECORD OF GALAXY FOR QUADRANT %d,%d\n\n", Q1, Q2);
                printf("       ");
                for (int j = start_y; j <= end_y; j++) {
                    printf(" %3d", j);
                }
                printf("\n     ");
                
                int cols = end_y - start_y + 1;
                if (cols < 1) cols = 1;
                
                for (int d = 0; d < cols * 4 + 2; d++) putchar('-');
                printf("\n");
                
                for (int i = start_x; i <= end_x; i++) {
                    printf(" %2d |", i);
                    for (int j = start_y; j <= end_y; j++) {
                        if (Z[i][j] == 0) {
                            printf(" ***"); 
                        } else {
                            printf(" %03d", Z[i][j]);
                        }
                    }
                    printf("\n     ");
                    for (int d = 0; d < cols * 4 + 2; d++) putchar('-');
                    printf("\n");
                }
                return;
            }
            case 1:
                printf("   STATUS REPORT:\n");
                printf("KLINGON%s LEFT: %s%d%s\n", (K9 > 1 ? "S" : ""), ANSI_COLOR_RED, K9, ANSI_COLOR_CYAN);
                printf("MISSION MUST BE COMPLETED IN %s%.1f%s STARDATES\n", ANSI_COLOR_RED, 0.1 * B_INT((T0+T9-T)*10), ANSI_COLOR_CYAN);
                if (B9 < 1) {
                    printf("YOUR STUPIDITY HAS LEFT YOU ON YOUR OWN IN\n  THE GALAXY -- YOU HAVE NO STARBASES LEFT!\n");
                } else {
                    printf("THE FEDERATION IS MAINTAINING %s%d%s STARBASE%s IN THE GALAXY\n", ANSI_COLOR_BLUE, B9, ANSI_COLOR_CYAN, (B9 > 1 ? "S" : ""));
                }
                cmd_dam(); 
                return;
            case 2:
                if (K3 <= 0) { 
                    printf("SCIENCE OFFICER SPOCK REPORTS  'SENSORS SHOW NO ENEMY SHIPS\n                                IN THIS QUADRANT'\n"); 
                    return; 
                }
                printf("FROM ENTERPRISE TO KLINGON BATTLE CRUISER%s\n", (K3 > 1 ? "S" : ""));
                for (int i = 1; i < MAX_K_PER_QUAD; i++) { 
                    if (K[i][3] > 0) calc_dir_dist(S1, S2, K[i][1], K[i][2]); 
                }
                return;
            case 3:
                if (B3 != 0) { 
                    printf("FROM ENTERPRISE TO STARBASE:\n"); 
                    calc_dir_dist(S1, S2, B4, B5); 
                } else {
                    printf("MR. SPOCK REPORTS,  'SENSORS SHOW NO STARBASES IN THIS QUADRANT.'\n");
                }
                return;
            case 4: {
                printf("DIRECTION/DISTANCE CALCULATOR:\nYOU ARE AT QUADRANT %d,%d SECTOR %d,%d\nPLEASE ENTER\n", Q1, Q2, (int)S1, (int)S2);
                char buf[256];
                
                printf("  INITIAL COORDINATES (X,Y)? "); 
                if(!get_input_str(buf, sizeof(buf))) return;
                double c1=0, a=0; 
                char* comma = strchr(buf, ',');
                if (comma) { 
                    *comma = ' '; 
                    sscanf(buf, "%lf %lf", &c1, &a); 
                } else { 
                    sscanf(buf, "%lf", &c1); 
                }
                
                printf("  FINAL COORDINATES (X,Y)? "); 
                if(!get_input_str(buf, sizeof(buf))) return;
                double w1=0, x=0; 
                comma = strchr(buf, ',');
                if (comma) { 
                    *comma = ' '; 
                    sscanf(buf, "%lf %lf", &w1, &x); 
                } else { 
                    sscanf(buf, "%lf", &w1); 
                }
                
                calc_dir_dist(c1, a, w1, x); 
                return;
            }
            case 5: {
                int start_x, end_x;
                get_window_bounds(Q1, S_GRID, &start_x, &end_x);

                printf("               GALAXY MAP PROJECTIONS (VIEWPORT)\n");
                for (int i = start_x; i <= end_x; i++) {
                    printf(" %2d ", i);
                    
                    int mapped_qx = 1 + ((i - 1) * 8 / S_GRID);
                    if (mapped_qx > 8) mapped_qx = 8;
                    if (mapped_qx < 1) mapped_qx = 1;
                    
                    char g2_1[32] = {0}, g2_2[32] = {0};
                    const char* names1[] = { "", "ANTARES", "RIGEL", "PROCYON", "VEGA", "CANOPUS", "ALTAIR", "SAGITTARIUS", "POLLUX" };
                    const char* names2[] = { "", "SIRIUS", "DENEB", "CAPELLA", "BETELGEUSE", "ALDEBARAN", "REGULUS", "ARCTURUS", "SPICA" };
                    strcpy(g2_1, names1[mapped_qx]); 
                    strcpy(g2_2, names2[mapped_qx]);

                    int j0_1 = B_INT(15 - 0.5 * strlen(g2_1)); 
                    int j0_2 = B_INT(39 - 0.5 * strlen(g2_2));
                    int col = 3;
                    while(col < j0_1) { 
                        printf(" "); 
                        col++; 
                    } 
                    printf("%s", g2_1); 
                    col += strlen(g2_1);
                    while(col < j0_2) { 
                        printf(" "); 
                        col++; 
                    } 
                    printf("%s\n", g2_2);
                }
                return;
            }
            default:
                printf("FUNCTIONS AVAILABLE FROM LIBRARY-COMPUTER:\n");
                printf("   0 = CUMULATIVE GALACTIC RECORD\n   1 = STATUS REPORT\n   2 = PHOTON TORPEDO DATA\n");
                printf("   3 = STARBASE NAV DATA\n   4 = DIRECTION/DISTANCE CALCULATOR\n   5 = GALAXY 'REGION NAME' MAP\n\n");
        }
    }
}

void cmd_lrs() {
    if (D[3] < 0) { 
        printf("%sLONG RANGE SENSORS ARE INOPERABLE%s\n", ANSI_COLOR_RED, ANSI_COLOR_CYAN); 
        return; 
    }
    
    // Updates internal Z-Map directly adjacent to Enterprise
    for (int i = Q1 - 1; i <= Q1 + 1; i++) {
        for (int j = Q2 - 1; j <= Q2 + 1; j++) {
            if (i >= 1 && i <= S_GRID && j >= 1 && j <= S_GRID) { 
                Z[i][j] = G[i][j]; 
            }
        }
    }

    int start_x, end_x, start_y, end_y;
    get_window_bounds(Q1, S_GRID, &start_x, &end_x);
    get_window_bounds(Q2, S_GRID, &start_y, &end_y);

    int cols = end_y - start_y + 1;
    if (cols < 1) cols = 1;

    printf("\nLONG RANGE SCAN FOR QUADRANT %d , %d (VIEWPORT)\n", Q1, Q2);
    
    printf("     ");
    for (int j = start_y; j <= end_y; j++) {
        printf(" %3d", j);
    }
    printf("\n    ");
    for (int d = 0; d < cols * 4 + 3; d++) putchar('-');
    printf("\n");

    for (int i = start_x; i <= end_x; i++) {
        printf(" %2d |", i);
        for (int j = start_y; j <= end_y; j++) {
            if (i >= 1 && i <= S_GRID && j >= 1 && j <= S_GRID) {
                if (Z[i][j] == 0) {
                    printf(" ***");
                } else {
                    printf(" %03d", Z[i][j]);
                }
            } else {
                printf("    "); 
            }
        }
        printf("\n    ");
        for (int d = 0; d < cols * 4 + 3; d++) putchar('-');
        printf("\n");
    }
    printf("\n");
}

void print_page(const char** lines, int count) {
    for (int i = 0; i < count; i++) {
        printf("%s\n", lines[i]);
    }
    for (int i = count; i < 22; i++) {
        printf("\n");
    }
    printf("%s[PRESS ENTER TO CONTINUE]%s", ANSI_COLOR_WHITE, ANSI_COLOR_CYAN);
    fflush(stdout);
    char buf[256];
    get_input_str(buf, sizeof(buf));
}

void help() {
    char pg1_str1[128];
    char pg1_str2[128];
    sprintf(pg1_str1, "     THE GALAXY IS DIVIDED INTO A DYNAMIC %d X %d QUADRANT GRID,", S_GRID, S_GRID);
    sprintf(pg1_str2, "AND EACH QUADRANT IS FURTHER DIVIDED INTO A %d X %d SECTOR GRID.", S_GRID, S_GRID);
    
    char pg4_str1[128];
    sprintf(pg4_str1, "     SHOWS YOU A %dX%d VIEWPORT OF YOUR PRESENT QUADRANT.", (S_GRID < 8 ? S_GRID : 8), (S_GRID < 8 ? S_GRID : 8));
    
    char pg5_str1[128];
    sprintf(pg5_str1, "     SHOWS CONDITIONS IN SPACE FOR A %dx%d MACRO-GRID AROUND", (S_GRID < 8 ? S_GRID : 8), (S_GRID < 8 ? S_GRID : 8));

    const char* page1[] = {
        "          *************************************",
        "          *                                   *",
        "          *      * * SUPER STAR TREK * *      *",
        "          *                                   *",
        "          *************************************",
        "",
        "      INSTRUCTIONS FOR 'SUPER STAR TREK'",
        "",
        "1. WHEN YOU SEE \\COMMAND ?\\ PRINTED, ENTER ONE OF THE LEGAL",
        "     COMMANDS (NAV,SRS,LRS,PHA,TOR,SHE,DAM,COM,HELP, OR XXX).",
        "2. IF YOU SHOULD TYPE IN AN ILLEGAL COMMAND, YOU'LL GET A SHORT",
        "     LIST OF THE LEGAL COMMANDS PRINTED OUT.",
        "3. SOME COMMANDS REQUIRE YOU TO ENTER DATA (FOR EXAMPLE, THE",
        "     'NAV' COMMAND COMES BACK WITH 'COURSE (1-9) ?'.)  IF YOU",
        "     TYPE IN ILLEGAL DATA (LIKE NEGATIVE NUMBERS), THAT COMMAND",
        "     WILL BE ABORTED",
        "",
        pg1_str1,
        pg1_str2
    };

    const char* page2[] = {
        "     YOU WILL BE ASSIGNED A STARTING POINT SOMEWHERE IN THE",
        "GALAXY TO BEGIN A TOUR OF DUTY AS COMMANDER OF THE STARSHIP",
        "\\ENTERPRISE\\; YOUR MISSION: TO SEEK AND DESTROY THE FLEET OF",
        "KLINGON WARSHIPS WHICH ARE MENACING THE UNITED FEDERATION OF",
        "PLANETS.",
        "",
        "     YOU HAVE THE FOLLOWING COMMANDS AVAILABLE TO YOU AS CAPTAIN",
        "OF THE STARSHIP ENTERPRISE:"
    };

    const char* page3[] = {
        "\\NAV\\ = WARP ENGINE CONTROL --",
        "     COURSE IS IN A CIRCULAR NUMERICAL      4  3  2",
        "     VECTOR ARRANGEMENT AS SHOWN             . . .",
        "     INTEGER AND REAL VALUES MAY BE           ...",
        "     USED.  (THUS COURSE 1.5 IS HALF-     5 ---*--- 1",
        "     WAY BETWEEN 1 AND 2                      ...",
        "                                             . . .",
        "     VALUES MAY APPROACH 9.0, WHICH         6  7  8",
        "     ITSELF IS EQUIVALENT TO 1.0         ",
        "                                            COURSE",
        "     ONE WARP FACTOR IS THE SIZE OF ",
        "     ONE QUADRANT. ENERGY CONSUMPTION",
        "     SCALES HEAVILY WITH GRID SIZE."
    };

    const char* page4[] = {
        "\\SRS\\ = SHORT RANGE SENSOR SCAN",
        pg4_str1,
        "",
        "     SYMBOLOGY ON YOUR SENSOR SCREEN IS AS FOLLOWS:",
        "        <*> = YOUR STARSHIP'S POSITION",
        "        +K+ = KLINGON BATTLE CRUISER",
        "        >!< = FEDERATION STARBASE (REFUEL/REPAIR/RE-ARM HERE!)",
        "         @  = PLANET",
        "         *  = STAR",
        "",
        "     A CONDENSED 'STATUS REPORT' WILL ALSO BE PRESENTED."
    };

    const char* page5[] = {
        "\\LRS\\ = LONG RANGE SENSOR SCAN",
        pg5_str1,
        "     THE ENTERPRISE (WHICH IS IN THE MIDDLE OF THE SCAN)",
        "     THE SCAN IS CODED IN THE FORM \\###\\, WHERE THE UNITS DIGIT",
        "     IS THE NUMBER OF STARS, THE TENS DIGIT IS THE NUMBER OF",
        "     STARBASES, AND THE HUNDREDS DIGIT IS THE NUMBER OF",
        "     KLINGONS.",
        "",
        "     EXAMPLE - 207 = 2 KLINGONS, NO STARBASES, & 7 STARS/PLANETS."
    };

    const char* page6[] = {
        "\\PHA\\ = PHASER CONTROL.",
        "     ALLOWS YOU TO DESTROY THE KLINGON BATTLE CRUISERS BY ",
        "     ZAPPING THEM WITH SUITABLY LARGE UNITS OF ENERGY TO",
        "     DEPLETE THEIR SHIELD POWER.  (REMEMBER, KLINGONS HAVE",
        "     PHASERS TOO!)"
    };

    const char* page7[] = {
        "\\TOR\\ = PHOTON TORPEDO CONTROL",
        "     TORPEDO COURSE IS THE SAME AS USED IN WARP ENGINE CONTROL",
        "     IF YOU HIT THE KLINGON VESSEL, HE IS DESTROYED AND",
        "     CANNOT FIRE BACK AT YOU.  IF YOU MISS, YOU ARE SUBJECT TO",
        "     HIS PHASER FIRE.  IN EITHER CASE, YOU ARE ALSO SUBJECT TO ",
        "     THE PHASER FIRE OF ALL OTHER KLINGONS IN THE QUADRANT.",
        "",
        "     THE LIBRARY-COMPUTER (\\COM\\) HAS AN OPTION TO ",
        "     COMPUTE TORPEDO TRAJECTORY FOR YOU (OPTION 2)"
    };

    const char* page8[] = {
        "\\SHE\\ = SHIELD CONTROL",
        "     DEFINES THE NUMBER OF ENERGY UNITS TO BE ASSIGNED TO THE",
        "     SHIELDS.  ENERGY IS TAKEN FROM TOTAL SHIP'S ENERGY.  NOTE",
        "     THAT THE STATUS DISPLAY TOTAL ENERGY INCLUDES SHIELD ENERGY"
    };

    const char* page9[] = {
        "\\DAM\\ = DAMAGE CONTROL REPORT",
        "     GIVES THE STATE OF REPAIR OF ALL DEVICES.  WHERE A NEGATIVE",
        "     'STATE OF REPAIR' SHOWS THAT THE DEVICE IS TEMPORARILY",
        "     DAMAGED."
    };

    const char* page10[] = {
        "\\COM\\ = LIBRARY-COMPUTER",
        "     THE LIBRARY-COMPUTER CONTAINS SIX OPTIONS:",
        "     OPTION 0 = CUMULATIVE GALACTIC RECORD",
        "        THIS OPTION SHOWS COMPUTER MEMORY OF THE RESULTS OF ALL",
        "        PREVIOUS SHORT AND LONG RANGE SENSOR SCANS",
        "     OPTION 1 = STATUS REPORT",
        "        THIS OPTION SHOWS THE NUMBER OF KLINGONS, STARDATES,",
        "        AND STARBASES REMAINING IN THE GAME.",
        "     OPTION 2 = PHOTON TORPEDO DATA",
        "        WHICH GIVES DIRECTIONS AND DISTANCE FROM THE ENTERPRISE",
        "        TO ALL KLINGONS IN YOUR QUADRANT",
        "     OPTION 3 = STARBASE NAV DATA",
        "        THIS OPTION GIVES DIRECTION AND DISTANCE TO ANY ",
        "        STARBASE WITHIN YOUR QUADRANT",
        "     OPTION 4 = DIRECTION/DISTANCE CALCULATOR",
        "        THIS OPTION ALLOWS YOU TO ENTER COORDINATES FOR",
        "        DIRECTION/DISTANCE CALCULATIONS",
        "     OPTION 5 = GALACTIC /REGION NAME/ MAP",
        "        THIS OPTION PRINTS THE NAMES OF THE SIXTEEN MAJOR ",
        "        GALACTIC REGIONS REFERRED TO IN THE GAME."
    };
    
    const char* page11[] = {
        "\\HELP\\ = INSTRUCTION MANUAL",
        "     PRINTS THIS INSTRUCTION MANUAL AND EXPLAINS HOW TO PLAY",
        "     SUPER STAR TREK."
    };
    
    const char* page12[] = {
        "\\XXX\\ = RESIGN COMMAND",
        "     ALLOWS YOU TO ABORT YOUR MISSION AND RESIGN YOUR",
        "     COMMISSION FROM STARFLEET.",
        "     STARFLEET WILL RECORD THE REMAINING KLINGON THREAT",
        "     AND ASSIGN A NEW COMMANDER."
    };

    print_page(page1, 21); print_page(page2, 8); print_page(page3, 13);
    print_page(page4, 11); print_page(page5, 9); print_page(page6, 5);
    print_page(page7, 9); print_page(page8, 4); print_page(page9, 4);
    print_page(page10, 20); print_page(page11, 3); print_page(page12, 5);
}

int command_loop() {
    char cmd[256];
    
    // Command Prompt prints in pure WHITE, resets back to Cyan
    printf("\n%sCOMMAND? %s", ANSI_COLOR_WHITE, ANSI_COLOR_RESET);
    if (!get_input_str(cmd, sizeof(cmd))) return 1;
    
    printf("%s", ANSI_COLOR_CYAN);
    
    if (!strncmp(cmd, "NAV", 3)) cmd_nav();
    else if (!strncmp(cmd, "SRS", 3)) short_range_scan();
    else if (!strncmp(cmd, "LRS", 3)) cmd_lrs();
    else if (!strncmp(cmd, "PHA", 3)) cmd_pha();
    else if (!strncmp(cmd, "TOR", 3)) cmd_tor();
    else if (!strncmp(cmd, "SHE", 3)) cmd_she();
    else if (!strncmp(cmd, "DAM", 3)) cmd_dam();
    else if (!strncmp(cmd, "COM", 3)) cmd_com();
    else if (!strncmp(cmd, "HELP", 4)) help();
    else if (!strncmp(cmd, "XXX", 3)) return 1; 
    else {
        printf("ENTER ONE OF THE FOLLOWING:\n  NAV  (TO SET COURSE)\n  SRS  (FOR SHORT RANGE SENSOR SCAN)\n");
        printf("  LRS  (FOR LONG RANGE SENSOR SCAN)\n  PHA  (TO FIRE PHASERS)\n  TOR  (TO FIRE PHOTON TORPEDOES)\n");
        printf("  SHE  (TO RAISE OR LOWER SHIELDS)\n  DAM  (FOR DAMAGE CONTROL REPORTS)\n  COM  (TO CALL ON LIBRARY-COMPUTER)\n  HELP (FOR INSTRUCTIONS)\n  XXX  (TO RESIGN YOUR COMMAND)\n\n");
    }
    return 0;
}

void print_game_over(int won, int resigned) {
    if (resigned || relieved) {
        printf("\n%sTHERE WERE %d KLINGON BATTLE CRUISERS LEFT AT\nTHE END OF YOUR MISSION.%s\n", ANSI_COLOR_WHITE, K9, ANSI_COLOR_CYAN);
    } else if (stranded) {
        printf("\n%sIT IS STARDATE %.1f\nTHERE WERE %d KLINGON BATTLE CRUISERS LEFT AT\nTHE END OF YOUR MISSION.%s\n", ANSI_COLOR_WHITE, T, K9, ANSI_COLOR_CYAN);
    } else if (destroyed) {
        printf("\n%sTHE ENTERPRISE HAS BEEN DESTROYED.  THE FEDERATION \nWILL BE CONQUERED%s\n", ANSI_COLOR_RED, ANSI_COLOR_CYAN);
        printf("%sIT IS STARDATE %.1f\nTHERE WERE %d KLINGON BATTLE CRUISERS LEFT AT\nTHE END OF YOUR MISSION.%s\n", ANSI_COLOR_WHITE, T, K9, ANSI_COLOR_CYAN);
    } else if (won) {
        if (IS_KOBAYASHI) {
            printf("\n%sYOU HAVE BEATEN THE KOBAYASHI MARU!%s\n", ANSI_COLOR_YELLOW, ANSI_COLOR_CYAN);
        }
        printf("\n%sCONGRATULATIONS, CAPTAIN!  THE LAST KLINGON BATTLE CRUISER\nMENACING THE FEDERATION HAS BEEN DESTROYED.\n\n", ANSI_COLOR_GREEN);
        
        double diff = T - T0; 
        if (diff <= 0.0) diff = 0.1;
        
        printf("%sYOUR EFFICIENCY RATING IS %.1f\n", ANSI_COLOR_WHITE, 1000.0 * pow(K7 / diff, 2.0));
        printf("THERE WERE 0 KLINGON BATTLE CRUISERS LEFT AT\nTHE END OF YOUR MISSION.%s\n\n\n", ANSI_COLOR_CYAN);
    } else {
        printf("\n%sIT IS STARDATE %.1f\nTHERE WERE %d KLINGON BATTLE CRUISERS LEFT AT\nTHE END OF YOUR MISSION.%s\n", ANSI_COLOR_WHITE, T, K9, ANSI_COLOR_CYAN);
    }
}

int main(int argc, char* argv[]) {
    srand((unsigned int)time(NULL));
    enable_ansi();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--easy") == 0) {
            ARG_GRID_SIZE = 4;
        } else if (strcmp(argv[i], "--medium") == 0) {
            ARG_GRID_SIZE = 16;
        } else if (strcmp(argv[i], "--hard") == 0) {
            ARG_GRID_SIZE = 32;
        } else if (strcmp(argv[i], "--impossible") == 0) {
            ARG_GRID_SIZE = 64;
        } else if (strncmp(argv[i], "--kobayashi-maru", 16) == 0) {
            ARG_GRID_SIZE = 256;
            IS_KOBAYASHI = 1;
        }
    }
    
    printf("%s\n\n\n\n\n\n\n\n\n\n\n", ANSI_COLOR_CYAN);
    printf("%s                                    ,------*------,\n", ANSI_COLOR_WHITE);
    printf("                    ,-------------   '---  ------'\n");
    printf("                     '-------- --'      / /\n");
    printf("                         ,---' '-------/ /--,\n");
    printf("                          '----------------'\n\n");
    printf("                    THE USS ENTERPRISE --- NCC-1701%s\n\n\n\n\n\n", ANSI_COLOR_CYAN);
    
    while (1) {
        setup_game();
        enter_quadrant();
        int quit = 0;
        
        while (T <= T0 + T9 && K9 > 0 && !quit && !destroyed && !relieved) {
            if (S + E <= 10.0 && (E <= 10.0 || D[7] < 0.0)) {
                stranded = 1;
                printf("\n%s** FATAL ERROR **   YOU'VE JUST STRANDED YOUR SHIP IN SPACE\n", ANSI_COLOR_RED);
                printf("YOU HAVE INSUFFICIENT MANEUVERING ENERGY,\n AND SHIELD CONTROL\n");
                printf("IS PRESENTLY INCAPABLE OF CROSS\n-CIRCUITING TO ENGINE ROOM!!%s\n", ANSI_COLOR_CYAN);
                break;
            }
            quit = command_loop();
        }
        
        print_game_over(K9 <= 0, quit == 1);
        
        char aye[16];
        printf("\n%sTHE FEDERATION IS IN NEED OF A NEW STARSHIP COMMANDER\n", ANSI_COLOR_WHITE);
        printf("FOR A SIMILAR MISSION -- IF THERE IS A VOLUNTEER,\nLET HIM STEP FORWARD AND ENTER 'AYE': %s", ANSI_COLOR_RESET);
        
        if (!get_input_str(aye, sizeof(aye))) break;
        printf("%s", ANSI_COLOR_CYAN);
        
        if (strcmp(aye, "AYE") != 0) break;
    }
    
    printf("%s\n", ANSI_COLOR_RESET);
    return 0;
}