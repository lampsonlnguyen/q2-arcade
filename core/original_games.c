/* SPDX-License-Identifier: GPL-3.0-or-later
 * Portable game rules from DiamondBond/q2-pod patch/games.c, fb23157cafc4e80b066a2c026ff9c220c7b29b4b.
 * Firmware-specific drawing, saves and input are replaced by Arcade adapters.
 */
#include "original_games.h"
static unsigned rng;
static unsigned rand_below(unsigned n) { rng=rng*1664525u+1013904223u; return (rng>>8)%n; }
static void copy_bytes(void *dst,const void *src,unsigned n) {
    unsigned char *d=dst;const unsigned char *s=src;while(n--)*d++=*s++;
}
static void set_bytes(void *dst,int value,unsigned n) {
    unsigned char *d=dst;while(n--)*d++=(unsigned char)value;
}
/* 0..n-1 in a random order. */
static void deal(unsigned char *p, int n) {
    for (int i = 0; i < n; ++i) p[i] = (unsigned char)i;
    for (int i = n - 1; i > 0; --i) {
        int j = (int)rand_below((unsigned)i + 1);
        unsigned char t = p[i];
        p[i] = p[j];
        p[j] = t;
    }
}

/* Sudoku: a cell is its digit (0 blank) and CLUE. The bundled puzzles, 4 bits a cell, each with
 * exactly one solution (the host test proves it); a new game is one of them relabelled, its rows
 * and columns shuffled within bands and stacks, the bands and stacks shuffled and maybe transposed,
 * which keeps the solution unique.
 * ponytail: 24 fixed puzzles, no generator, no pencil marks or hints. */
#define SUD_MAGIC 0x31445553u /* "SUD1" */


static const char puzzles[][42] = {
    "\x20\x00\x50\x60\x40\x00\x80\x21\x00\x03\x00\x00\x00\x81\x00\x30\x00\x07\x00\x09\x68\x05\x10"
    "\x00\x00\x10\x63\x00\x02\x40\x70\x04\x00\x69\x00\x10\x35\x00\x80\x00\x00",
    "\x09\x00\x06\x00\x30\x02\x50\x00\x09\x60\x00\x03\x50\x08\x00\x00\x06\x30\x20\x40\x71\x80\x00"
    "\x59\x00\x01\x04\x40\x00\x02\x90\x00\x81\x03\x02\x00\x50\x00\x00\x00\x80",
    "\x00\x00\x30\x00\x06\x00\x47\x90\x05\x30\x02\x86\x00\x00\x00\x00\x05\x23\x00\x00\x51\x00\x60"
    "\x00\x00\x30\x10\x20\x10\x98\x05\x05\x00\x00\x02\x70\x70\x00\x10\x86\x00",
    "\x05\x40\x00\x00\x99\x10\x00\x00\x00\x82\x00\x07\x54\x14\x00\x12\x57\x80\x60\x00\x70\x00\x01"
    "\x02\x00\x60\x00\x00\x03\x00\x02\x80\x00\x04\x00\x05\x20\x08\x00\x60\x00",
    "\x89\x04\x01\x60\x00\x00\x06\x00\x00\x20\x05\x00\x00\x30\x21\x95\x00\x30\x65\x00\x00\x00\x04"
    "\x00\x00\x21\x00\x50\x30\x00\x00\x99\x80\x00\x00\x00\x17\x40\x05\x82\x00",
    "\x49\x00\x00\x20\x02\x00\x08\x01\x00\x71\x03\x29\x80\x60\x60\x00\x10\x00\x87\x30\x02\x50\x00"
    "\x00\x04\x00\x00\x00\x70\x08\x05\x10\x01\x00\x00\x20\x08\x00\x06\x03\x00",
    "\x00\x06\x00\x00\x30\x57\x19\x86\x40\x00\x05\x07\x89\x10\x20\x05\x00\x00\x10\x04\x00\x50\x07"
    "\x80\x00\x00\x14\x03\x00\x10\x90\x05\x00\x80\x00\x30\x06\x00\x00\x00\x00",
    "\x60\x50\x00\x30\x00\x07\x04\x69\x00\x09\x00\x20\x00\x00\x01\x87\x00\x60\x00\x80\x00\x07\x00"
    "\x62\x90\x10\x40\x30\x06\x00\x08\x20\x00\x23\x07\x04\x00\x00\x09\x00\x10",
    "\x40\x07\x02\x00\x80\x07\x05\x80\x42\x60\x00\x00\x00\x09\x06\x00\x08\x00\x00\x00\x80\x20\x00"
    "\x21\x00\x40\x53\x76\x05\x00\x02\x00\x00\x00\x00\x00\x10\x50\x46\x78\x00",
    "\x00\x09\x00\x00\x00\x00\x00\x07\x00\x00\x00\x20\x49\x80\x94\x60\x80\x00\x80\x02\x40\x05\x00"
    "\x00\x37\x96\x04\x03\x50\x60\x10\x20\x00\x00\x58\x00\x00\x64\x90\x03\x00",
    "\x08\x00\x00\x00\x00\x70\x00\x00\x00\x00\x27\x40\x91\x00\x14\x00\x07\x00\x03\x50\x06\x00\x28"
    "\x09\x00\x00\x05\x10\x80\x53\x60\x93\x97\x00\x14\x00\x00\x69\x00\x00\x00",
    "\x09\x00\x06\x80\x06\x70\x00\x40\x00\x40\x80\x19\x70\x30\x00\x10\x05\x00\x13\x50\x60\x04\x00"
    "\x00\x90\x30\x00\x00\x00\x00\x40\x00\x07\x48\x06\x00\x80\x00\x71\x05\x00",
    "\x07\x80\x02\x00\x50\x00\x03\x00\x01\x30\x04\x06\x00\x80\x40\x70\x81\x52\x09\x00\x00\x00\x00"
    "\x07\x00\x00\x00\x05\x00\x07\x00\x00\x21\x00\x03\x07\x00\x02\x00\x08\x00",
    "\x70\x00\x13\x00\x00\x08\x70\x00\x60\x20\x40\x00\x08\x00\x00\x00\x05\x00\x59\x00\x00\x37\x14"
    "\x00\x06\x00\x00\x00\x00\x00\x09\x09\x00\x07\x00\x35\x38\x20\x90\x00\x00",
    "\x00\x45\x00\x00\x87\x00\x00\x09\x65\x00\x00\x00\x00\x09\x00\x43\x00\x02\x40\x80\x09\x00\x31"
    "\x00\x70\x06\x40\x00\x30\x80\x10\x00\x00\x04\x08\x90\x00\x09\x00\x00\x00",
    "\x09\x80\x02\x10\x00\x15\x40\x00\x09\x00\x00\x00\x00\x29\x20\x00\x07\x10\x00\x00\x30\x69\x00"
    "\x00\x00\x60\x08\x00\x06\x20\x00\x00\x00\x30\x00\x80\x08\x40\x00\x07\x60",
    "\x00\x20\x60\x00\x08\x00\x10\x72\x00\x07\x10\x35\x04\x00\x30\x02\x00\x00\x51\x40\x00\x00\x06"
    "\x00\x07\x05\x10\x08\x50\x00\x00\x10\x60\x00\x08\x07\x00\x00\x00\x00\x00",
    "\x14\x70\x90\x00\x60\x90\x00\x30\x01\x60\x00\x51\x04\x00\x28\x00\x00\x05\x00\x00\x00\x40\x20"
    "\x00\x00\x00\x60\x03\x01\x02\x08\x00\x00\x00\x00\x00\x50\x10\x09\x00\x30",
    "\x00\x00\x50\x20\x06\x70\x00\x00\x03\x00\x21\x00\x40\x00\x07\x03\x80\x64\x00\x04\x72\x00\x50"
    "\x30\x00\x07\x00\x80\x60\x00\x05\x00\x00\x20\x00\x80\x00\x00\x05\x00\x10",
    "\x03\x69\x01\x80\x05\x08\x00\x00\x36\x00\x00\x30\x00\x40\x00\x00\x02\x07\x01\x32\x00\x00\x80"
    "\x00\x00\x00\x00\x00\x10\x07\x00\x00\x02\x18\x50\x40\x00\x40\x00\x02\x00",
    "\x05\x02\x00\x08\x00\x00\x01\x00\x00\x90\x30\x06\x02\x00\x05\x00\x00\x30\x00\x90\x01\x50\x06"
    "\x00\x00\x00\x00\x08\x65\x07\x04\x00\x00\x08\x40\x07\x50\x00\x09\x26\x00",
    "\x90\x00\x00\x00\x00\x01\x00\x50\x63\x30\x00\x80\x71\x00\x00\x00\x04\x05\x64\x00\x10\x83\x00"
    "\x30\x40\x00\x00\x01\x00\x02\x05\x00\x02\x00\x79\x01\x00\x40\x00\x00\x00",
    "\x04\x20\x50\x00\x85\x10\x00\x00\x64\x70\x00\x40\x00\x00\x04\x00\x29\x00\x00\x00\x00\x00\x19"
    "\x60\x01\x57\x00\x00\x00\x80\x00\x72\x00\x00\x10\x06\x00\x82\x00\x00\x00",
    "\x70\x80\x00\x01\x00\x41\x06\x07\x00\x00\x00\x00\x00\x09\x06\x00\x03\x00\x00\x00\x03\x10\x70"
    "\x10\x09\x00\x52\x30\x40\x00\x02\x00\x00\x20\x40\x00\x08\x00\x09\x56\x00",
};
#define PUZZLES (int)(sizeof puzzles / sizeof *puzzles)

void sud_puzzle(int k, unsigned char *out) {
    for (int i = 0; i < 81; ++i) out[i] = (unsigned char)puzzles[k][i / 2] >> (i & 1 ? 0 : 4) & 15;
}

void sud_new(struct sudoku *s) {
    unsigned char clue[81], digit[9], band[3], stack[3], row[3][3], col[3][3];
    sud_puzzle((int)rand_below(PUZZLES), clue);
    deal(digit, 9);
    deal(band, 3);
    deal(stack, 3);
    for (int k = 0; k < 3; ++k) deal(row[k], 3), deal(col[k], 3);
    int flip = (int)rand_below(2);
    set_bytes(s, 0, sizeof *s);
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c) {
            int sr = 3 * band[r / 3] + row[r / 3][r % 3], sc = 3 * stack[c / 3] + col[c / 3][c % 3];
            int v = clue[flip ? sc * 9 + sr : sr * 9 + sc];
            s->cell[r * 9 + c] = v ? (unsigned char)(digit[v - 1] + 1) | CLUE : 0;
        }
}

/* A clue stays. */
int sud_set(struct sudoku *s, int i, int v) {
    if (i < 0 || i > 80 || v < 0 || v > 9 || s->cell[i] & CLUE) return 0;
    s->cell[i] = (unsigned char)v;
    return 1;
}

/* Cell i's digit is also in its row, column or box. */
int sud_conflict(const struct sudoku *s, int i) {
    int v = s->cell[i] & 15, r = i / 9, c = i % 9, b = r / 3 * 27 + c / 3 * 3;
    for (int k = 0; v && k < 9; ++k) {
        int peer[3] = { r * 9 + k, k * 9 + c, b + k / 3 * 9 + k % 3 };
        for (int j = 0; j < 3; ++j)
            if (peer[j] != i && (s->cell[peer[j]] & 15) == v) return 1;
    }
    return 0;
}

int sud_done(const struct sudoku *s) {
    for (int i = 0; i < 81; ++i)
        if (!(s->cell[i] & 15) || sud_conflict(s, i)) return 0;
    return 1;
}

/* Bricks: rings of bricks round the centre, the paddle an arc on the outer ring, the ball bouncing
 * between. Angles are 1024 a turn, rising clockwise on screen; positions and velocities 16.16 px
 * from the centre, per 1/60 s step. All integer, so the same inputs give the same game.
 * ponytail: three fixed layouts repeating with the level, bricks reflect radially only. */






#define SPIN 128 /* the turn off the radial at the paddle's ends */
#define MISS (R_PAD + 10)
#define V0 (2 << 16) /* 120 px/s */
#define VSTEP (1 << 14)
#define VMAX (9 << 15) /* 270 px/s */
_Static_assert(VMAX < BRICK_T << 16, "a step never crosses a whole ring");


static const unsigned short layouts[][RINGS] = {
    { 0xffff, 0xffff, 0xffff },
    { 0x5555, 0xffff, 0xaaaa },
    { 0x0f0f, 0xf0f0, 0xffff },
};
static const unsigned short quarter[257] = {
    /* sin, 16.16, a quarter turn */
    0,     402,   804,   1206,  1608,  2010,  2412,  2814,  3216,  3617,  4019,  4420,  4821,
    5222,  5623,  6023,  6424,  6824,  7224,  7623,  8022,  8421,  8820,  9218,  9616,  10014,
    10411, 10808, 11204, 11600, 11996, 12391, 12785, 13180, 13573, 13966, 14359, 14751, 15143,
    15534, 15924, 16314, 16703, 17091, 17479, 17867, 18253, 18639, 19024, 19409, 19792, 20175,
    20557, 20939, 21320, 21699, 22078, 22457, 22834, 23210, 23586, 23961, 24335, 24708, 25080,
    25451, 25821, 26190, 26558, 26925, 27291, 27656, 28020, 28383, 28745, 29106, 29466, 29824,
    30182, 30538, 30893, 31248, 31600, 31952, 32303, 32652, 33000, 33347, 33692, 34037, 34380,
    34721, 35062, 35401, 35738, 36075, 36410, 36744, 37076, 37407, 37736, 38064, 38391, 38716,
    39040, 39362, 39683, 40002, 40320, 40636, 40951, 41264, 41576, 41886, 42194, 42501, 42806,
    43110, 43412, 43713, 44011, 44308, 44604, 44898, 45190, 45480, 45769, 46056, 46341, 46624,
    46906, 47186, 47464, 47741, 48015, 48288, 48559, 48828, 49095, 49361, 49624, 49886, 50146,
    50404, 50660, 50914, 51166, 51417, 51665, 51911, 52156, 52398, 52639, 52878, 53114, 53349,
    53581, 53812, 54040, 54267, 54491, 54714, 54934, 55152, 55368, 55582, 55794, 56004, 56212,
    56418, 56621, 56823, 57022, 57219, 57414, 57607, 57798, 57986, 58172, 58356, 58538, 58718,
    58896, 59071, 59244, 59415, 59583, 59750, 59914, 60075, 60235, 60392, 60547, 60700, 60851,
    60999, 61145, 61288, 61429, 61568, 61705, 61839, 61971, 62101, 62228, 62353, 62476, 62596,
    62714, 62830, 62943, 63054, 63162, 63268, 63372, 63473, 63572, 63668, 63763, 63854, 63944,
    64031, 64115, 64197, 64277, 64354, 64429, 64501, 64571, 64639, 64704, 64766, 64827, 64884,
    64940, 64993, 65043, 65091, 65137, 65180, 65220, 65259, 65294, 65328, 65358, 65387, 65413,
    65436, 65457, 65476, 65492, 65505, 65516, 65525, 65531, 65535, 65535,
};
static const unsigned char atans[257] = {
    /* atan(i / 256), 1024 a turn */
    0,   1,   1,   2,   3,   3,   4,   4,   5,   6,   6,   7,   8,   8,   9,   10,  10,  11,  11,
    12,  13,  13,  14,  15,  15,  16,  16,  17,  18,  18,  19,  20,  20,  21,  22,  22,  23,  23,
    24,  25,  25,  26,  27,  27,  28,  28,  29,  30,  30,  31,  31,  32,  33,  33,  34,  34,  35,
    36,  36,  37,  38,  38,  39,  39,  40,  41,  41,  42,  42,  43,  44,  44,  45,  45,  46,  46,
    47,  48,  48,  49,  49,  50,  51,  51,  52,  52,  53,  53,  54,  55,  55,  56,  56,  57,  57,
    58,  58,  59,  60,  60,  61,  61,  62,  62,  63,  63,  64,  65,  65,  66,  66,  67,  67,  68,
    68,  69,  69,  70,  70,  71,  71,  72,  72,  73,  74,  74,  75,  75,  76,  76,  77,  77,  78,
    78,  79,  79,  80,  80,  81,  81,  82,  82,  83,  83,  84,  84,  84,  85,  85,  86,  86,  87,
    87,  88,  88,  89,  89,  90,  90,  91,  91,  91,  92,  92,  93,  93,  94,  94,  95,  95,  96,
    96,  96,  97,  97,  98,  98,  99,  99,  99,  100, 100, 101, 101, 102, 102, 102, 103, 103, 104,
    104, 104, 105, 105, 106, 106, 106, 107, 107, 108, 108, 108, 109, 109, 110, 110, 110, 111, 111,
    112, 112, 112, 113, 113, 113, 114, 114, 115, 115, 115, 116, 116, 116, 117, 117, 118, 118, 118,
    119, 119, 119, 120, 120, 120, 121, 121, 121, 122, 122, 122, 123, 123, 123, 124, 124, 124, 125,
    125, 125, 126, 126, 126, 127, 127, 127, 128, 128,
};

int original_sin(int a) {
    a &= 1023;
    int q = a & 255, v = a & 256 ? quarter[256 - q] : quarter[q];
    return a & 512 ? -v : v;
}
int original_cos(int a) { return original_sin(a + 256); }

/* The angle of x, y (any scale, below 2^23). */
int original_angle(int y, int x) {
    int ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
    if (!ax && !ay) return 0;
    /* NOLINTNEXTLINE(clang-analyzer-core.DivideZero): ax > 0 when ax >= ay, as both 0 returned */
    int a = ax >= ay ? atans[ay * 256 / ax] : 256 - atans[ax * 256 / ay];
    if (x < 0) a = 512 - a;
    if (y < 0) a = -a;
    return a & 1023;
}

static unsigned vsqrt(unsigned long long n) {
    unsigned long long r = 0, b = 1ull << 62;
    while (b > n) b >>= 2;
    for (; b; b >>= 2)
        if (n >= r + b)
            n -= r + b, r = (r >> 1) + b;
        else
            r >>= 1;
    return (unsigned)r;
}

/* x, y's distance from the centre, 1/256 px. */
static int radius(int x, int y) {
    long long a = x >> 8, b = y >> 8;
    return (int)vsqrt((unsigned long long)(a * a + b * b));
}

static void place(struct bricks *v) {
    v->x = (R_PAD - 6) * original_cos(v->pa);
    v->y = (R_PAD - 6) * original_sin(v->pa);
}

static void head(struct bricks *v, int a) {
    int speed = V0 + VSTEP * v->level;
    if (speed > VMAX) speed = VMAX;
    v->vx = (int)((long long)speed * original_cos(a) >> 16);
    v->vy = (int)((long long)speed * original_sin(a) >> 16);
}

static void level(struct bricks *v) {
    copy_bytes(v->brick, layouts[v->level % 3], sizeof v->brick);
    v->state = AIM;
    place(v);
}

void bricks_new(struct bricks *v) {
    set_bytes(v, 0, sizeof *v);
    v->lives = 3;
    v->pa = 768; /* the top */
    level(v);
}

void bricks_launch(struct bricks *v) {
    if (v->state != AIM) return;
    head(v, v->pa + 512);
    v->state = PLAY;
}

/* One step, the paddle turned by dpa first. */
void bricks_step(struct bricks *v, int dpa) {
    v->pa = (v->pa + dpa) & 1023;
    if (v->state == AIM) place(v);
    if (v->state != PLAY) return;
    int ox = v->x, oy = v->y, before = radius(ox, oy);
    v->x += v->vx;
    v->y += v->vy;
    int r = radius(v->x, v->y), X = v->x >> 8, Y = v->y >> 8, ring = (r / 256 - R_B) / BRICK_T;
    if (r >= R_B * 256 && ring < RINGS) {
        int seg = original_angle(Y, X) * SEGS / 1024;
        if (v->brick[ring] >> seg & 1) {
            v->brick[ring] &= (unsigned short)~(1u << seg);
            v->score += 10 * (RINGS - ring);
            int nx = X * 4096 / r, ny = Y * 4096 / r; /* the normal, 1/4096 */
            long long dot = ((long long)v->vx * nx + (long long)v->vy * ny) >> 12;
            v->vx -= (int)(2 * dot * nx >> 12);
            v->vy -= (int)(2 * dot * ny >> 12);
            v->x = ox, v->y = oy;
            if (!(v->brick[0] | v->brick[1] | v->brick[2])) {
                v->score += 100;
                ++v->level;
                level(v);
            }
            return;
        }
    }
    int edge = (R_PAD - 4) * 256;
    if (before < edge && r >= edge && (long long)v->vx * X + (long long)v->vy * Y > 0) {
        int a = original_angle(Y, X), d = ((a - v->pa + 512) & 1023) - 512;
        if (d >= -PAD_HALF && d <= PAD_HALF) head(v, a + 512 + d * SPIN / PAD_HALF);
    } else if (r >= MISS * 256) {
        v->state = --v->lives ? AIM : OVER;
        place(v);
    }
}

/* Solitaire: Klondike, draw one, unlimited redeals. A card is suit * 13 + rank (Ace 0, King 12;
 * spades, hearts, diamonds, clubs) and UP when face up. The piles in the wheel's order: the stock,
 * the waste, four foundations (any takes an Ace), seven tableau columns.
 * ponytail: no undo or score; a held run drops whatever part of it fits. */
#define SOL_MAGIC 0x314c4f53u /* "SOL1" */





void sol_new(struct solitaire *s) {
    unsigned char deck[52];
    int n = 0;
    deal(deck, 52);
    set_bytes(s, 0, sizeof *s);
    for (int k = 0; k < 7; ++k) {
        for (int i = 0; i <= k; ++i) s->card[TABLEAU + k][i] = deck[n++];
        s->card[TABLEAU + k][k] |= UP;
        s->len[TABLEAU + k] = (unsigned char)(k + 1);
    }
    copy_bytes(s->card[STOCK], deck + n, 52 - n);
    s->len[STOCK] = (unsigned char)(52 - n);
}

/* The stock's top card face up on the waste; an empty stock takes the waste back, face down. */
int sol_draw(struct solitaire *s) {
    if (s->len[STOCK]) {
        s->card[WASTE][s->len[WASTE]++] = s->card[STOCK][--s->len[STOCK]] | UP;
        return 1;
    }
    for (; s->len[WASTE]; ++s->len[STOCK])
        s->card[STOCK][s->len[STOCK]] = s->card[WASTE][--s->len[WASTE]] & ~UP;
    return s->len[STOCK] != 0;
}

static int sol_fits(const struct solitaire *s, int c, int to) {
    int n = s->len[to], top = n ? s->card[to][n - 1] & ~UP : 0;
    if (to < TABLEAU) return n ? c == top + 1 && c / 13 == top / 13 : c % 13 == 0;
    return n ? c % 13 == top % 13 - 1 && (c / 13 % 3 == 0) != (top / 13 % 3 == 0) : c % 13 == 12;
}

/* From's cards that fit onto to, moved: a foundation takes the top card, a column the part of
 * the face-up run that fits (at most one does). A column left with a face-down top turns it. */
int sol_move(struct solitaire *s, int from, int to) {
    int n = s->len[from], run = 0;
    if (from == STOCK || from == to || to < FOUND) return 0;
    while (run < n && s->card[from][n - 1 - run] & UP && (run < 1 || from >= TABLEAU)) ++run;
    /* a pile at SOL_MAX (a forged save's: no deal fills one) takes nothing */
    for (int k = 1; k <= run && (k == 1 || to >= TABLEAU); ++k)
        if (s->len[to] + k <= SOL_MAX && sol_fits(s, s->card[from][n - k] & ~UP, to)) {
            copy_bytes(&s->card[to][s->len[to]], &s->card[from][n - k], (unsigned)k);
            s->len[to] = (unsigned char)(s->len[to] + k);
            s->len[from] = (unsigned char)(n - k);
            if (n > k) s->card[from][n - k - 1] |= UP;
            return 1;
        }
    return 0;
}

/* From's top card onto the foundation that takes it. */
int sol_home(struct solitaire *s, int from) {
    for (int f = FOUND; f < TABLEAU; ++f)
        if (sol_move(s, from, f)) return 1;
    return 0;
}

int sol_won(const struct solitaire *s) {
    return s->len[FOUND] + s->len[FOUND + 1] + s->len[FOUND + 2] + s->len[FOUND + 3] == 52;
}

/* Every card dealt and face up: all of them home, the lowest left always being a column's top. */
void sol_finish(struct solitaire *s) {
    if (s->len[STOCK] || s->len[WASTE]) return;
    for (int p = TABLEAU; p < PILES; ++p)
        if (s->len[p] && !(s->card[p][0] & UP)) return;
    for (int moved = 1; moved;) {
        moved = 0;
        for (int p = TABLEAU; p < PILES; ++p) moved |= sol_home(s, p);
    }
}

/* Snake: SN_N x SN_N cells, a cell y * SN_N + x; the body a ring of cells from the tail, the head
 * last. Walls and the body kill. A turn is relative (+1 right, -1 left), one a step and a second
 * waiting for the next, so quick turns can't fold the snake back into itself. Each apple adds one
 * and a little speed.
 * ponytail: no wrap mode, levels or obstacles. */


#define SN_MS 160 /* the first step's period; SN_FAST at most, SN_FASTER less an apple */
#define SN_FAST 70
#define SN_FASTER 4


static void sn_food(struct snake *s) {
    unsigned char taken[SN_CELLS] = { 0 };
    for (int i = 0; i < s->len; ++i) taken[s->body[(s->tail + i) % SN_CELLS]] = 1;
    int k = SN_CELLS - s->len ? (int)rand_below((unsigned)(SN_CELLS - s->len)) : -1;
    for (int c = 0; c < SN_CELLS; ++c)
        if (!taken[c] && !k--) {
            s->food = (unsigned short)c;
            return;
        }
    s->state = OVER; /* the board is full */
}

void snake_new(struct snake *s) {
    set_bytes(s, 0, sizeof *s);
    s->len = 3;
    s->dir = 1;
    for (int i = 0; i < 3; ++i) s->body[i] = (unsigned short)(SN_N / 2 * SN_N + SN_N / 2 - 3 + i);
    s->state = PLAY;
    sn_food(s);
}

void snake_turn(struct snake *s, int d) {
    if (s->state != PLAY) return;
    if (s->turned) {
        if (!s->next) s->next = (signed char)d; /* one waits; any more are dropped */
        return;
    }
    s->dir = (unsigned char)((s->dir + d + 4) & 3);
    s->turned = 1;
}

/* The step's period, ms. */
int snake_ms(const struct snake *s) {
    int ms = SN_MS - SN_FASTER * s->score;
    return ms < SN_FAST ? SN_FAST : ms;
}

/* 0 moved, 1 ate, -1 died (or nothing, after). */
int snake_step(struct snake *s) {
    if (s->state != PLAY) return -1;
    int head = s->body[(s->tail + s->len - 1) % SN_CELLS], x = head % SN_N, y = head / SN_N;
    s->turned = 0;
    x += s->dir == 1 ? 1 : s->dir == 3 ? -1 : 0;
    y += s->dir == 2 ? 1 : s->dir == 0 ? -1 : 0;
    int next = y * SN_N + x, ate = next == s->food;
    if (x < 0 || x >= SN_N || y < 0 || y >= SN_N) {
        s->state = OVER;
        return -1;
    }
    for (int i = !ate; i < s->len; ++i) /* the tail moves off its cell unless the snake grows */
        if (s->body[(s->tail + i) % SN_CELLS] == next) {
            s->state = OVER;
            return -1;
        }
    s->body[(s->tail + s->len) % SN_CELLS] = (unsigned short)next;
    if (s->next) snake_turn(s, s->next), s->next = 0; /* the waiting turn, for the next step */
    if (!ate) {
        s->tail = (unsigned short)((s->tail + 1) % SN_CELLS);
        return 0;
    }
    ++s->len;
    ++s->score;
    sn_food(s);
    return 1;
}


static int wrap(int v,int n) { v%=n;return v<0?v+n:v; }
static void reset(void *ctx) {
    OriginalGame *g=ctx;
    g->elapsed=0;g->picking=0;g->pick=0;g->won=0;g->ended=0;g->wheel_remainder=0;
    rng=g->seed;
    switch(g->kind) {
    case ORIGINAL_SUDOKU:sud_new(&g->state.sud);break;
    case ORIGINAL_BRICKS:bricks_new(&g->state.bricks);break;
    case ORIGINAL_SOLITAIRE:sol_new(&g->state.sol);break;
    case ORIGINAL_SNAKE:snake_new(&g->state.snake);break;
    }
    g->seed=rng*1664525u+1013904223u;
}
static int input(void *ctx,Q2GameInput e) {
    OriginalGame *g=ctx;
    int d=e.action==Q2_GAME_NEXT?1:e.action==Q2_GAME_PREV?-1:0;
    int wheel=e.action==Q2_GAME_WHEEL;
    if(wheel) {
        if(g->kind==ORIGINAL_SNAKE)return 0;
        if(g->kind==ORIGINAL_BRICKS){
            g->wheel_remainder-=e.value*1024*g->wheel_percent;
            int turn=g->wheel_remainder/10000;g->wheel_remainder%=10000;
            struct bricks *b=&g->state.bricks;
            b->pa=wrap(b->pa+turn,1024);if(b->state==AIM)place(b);
            return turn!=0;
        }
        g->wheel_remainder-=e.value*16;
        int units=(28*16*50+g->wheel_percent/2)/g->wheel_percent;
        d=g->wheel_remainder/units;g->wheel_remainder%=units;
        if(!d)return 0;
    }
    if(e.action==Q2_GAME_RETURN&&g->picking){g->picking=0;return 1;}
    if(g->kind==ORIGINAL_SUDOKU){
        struct sudoku *s=&g->state.sud;
        if(d){if(g->picking)g->pick=wrap(g->pick+d,10);else s->cur=(unsigned char)wrap(s->cur+(wheel?d:9*d),81);return 1;}
        if(e.action==Q2_GAME_TOUCH_UP&&e.x>=79&&e.x<295&&e.y>=58&&e.y<274){
            s->cur=(unsigned char)(((e.y-58)/24)*9+(e.x-79)/24);g->picking=0;return 1;
        }
        if(e.action==Q2_GAME_CENTER){
            if(g->picking){sud_set(s,s->cur,g->pick);g->picking=0;g->ended=g->won=sud_done(s);}
            else if(!(s->cell[s->cur]&CLUE)){g->picking=1;g->pick=s->cell[s->cur]&15;}
            return 1;
        }
    }else if(g->kind==ORIGINAL_BRICKS){
        struct bricks *b=&g->state.bricks;
        if(d){b->pa=wrap(b->pa+32*d,1024);if(b->state==AIM)place(b);return 1;}
        if(e.action==Q2_GAME_CENTER){bricks_launch(b);return 1;}
        if(e.action==Q2_GAME_TOUCH_UP){b->pa=original_angle(e.y-174,e.x-187);if(b->state==AIM)place(b);return 1;}
    }else if(g->kind==ORIGINAL_SOLITAIRE){
        struct solitaire *s=&g->state.sol;
        static const unsigned char col[PILES]={0,1,3,4,5,6,0,1,2,3,4,5,6};
        if(d){int cur=s->cur;s->cur=(unsigned char)(wheel?wrap(cur+d,PILES):cur<TABLEAU?TABLEAU+col[cur]:cur<TABLEAU+2?cur-TABLEAU:cur-TABLEAU-1);return 1;}
        if(e.action==Q2_GAME_TOUCH_UP&&e.x>=18&&e.x<361&&e.y>=58&&e.y<280){
            int c=(e.x-18)/49;
            if(e.y>=116)s->cur=(unsigned char)(TABLEAU+c);
            else if(c!=2)s->cur=(unsigned char)(c<2?c:c-1);
            else return 0;
            e.action=Q2_GAME_CENTER;
        }
        if(e.action==Q2_GAME_CENTER){
            int cur=s->cur,moved=0;
            if(g->picking){
                moved=cur==g->pick?((cur<FOUND||cur>=TABLEAU)&&sol_home(s,cur)):sol_move(s,g->pick,cur);
                if(moved||cur==g->pick)g->picking=0;
                else if(cur==STOCK){g->picking=0;moved=sol_draw(s);}
                else if(s->len[cur])g->pick=cur;
            }else if(cur==STOCK)moved=sol_draw(s);
            else if(s->len[cur]){g->picking=1;g->pick=cur;}
            if(moved)sol_finish(s);
            g->ended=g->won=sol_won(s);return 1;
        }
    }else if(g->kind==ORIGINAL_SNAKE){
        if(e.action==Q2_GAME_TOUCH_UP)d=e.x<187?-1:1;
        if(d){snake_turn(&g->state.snake,d>0?1:-1);return 1;}
    }
    return 0;
}
static void step(void *ctx,unsigned ms) {
    OriginalGame *g=ctx;
    if(g->ended)return;
    if(g->kind==ORIGINAL_BRICKS){
        g->elapsed+=ms*60;
        while(g->elapsed>=1000){g->elapsed-=1000;bricks_step(&g->state.bricks,0);}
        g->ended=g->state.bricks.state==OVER;
    }else if(g->kind==ORIGINAL_SNAKE){
        g->elapsed+=ms;
        if(g->elapsed>=(unsigned)snake_ms(&g->state.snake)){
            g->elapsed-=(unsigned)snake_ms(&g->state.snake);
            rng=g->seed;snake_step(&g->state.snake);g->seed=rng;
            g->ended=g->state.snake.state==OVER;g->won=g->state.snake.len==SN_CELLS;
        }
    }
}
int original_animated(const OriginalGame *g){return g->kind==ORIGINAL_SNAKE||(g->kind==ORIGINAL_BRICKS&&g->state.bricks.state==PLAY);}
const Q2GameHooks original_hooks={input,step,0,reset};
