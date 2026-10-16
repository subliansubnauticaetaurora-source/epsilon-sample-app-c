// Jeu de survie - NumWorks (app externe .nwa)
// A placer dans src/main.c du depot epsilon-sample-app-c (avec eadk.h)

#include "eadk.h"
#include <stdint.h>
#include <stdbool.h>

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Survie";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

#define RGB(r,g,b) ((eadk_color_t)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3)))
#define SKY     RGB(110,180,255)
#define GREEN   RGB(40,140,60)
#define WHITE   RGB(255,255,255)
#define BLACK   RGB(0,0,0)
#define GRAY    RGB(110,110,110)
#define DARK    RGB(12,16,30)
#define RED     RGB(220,40,40)
#define ORANGE  RGB(255,150,20)
#define YELLOW  RGB(255,230,60)
#define LIME    RGB(120,230,120)

/* ---------- Outils d'affichage ---------- */

static void fill(int x, int y, int w, int h, eadk_color_t c) {
  eadk_rect_t r = {x, y, w, h};
  eadk_display_push_rect_uniform(r, c);
}

static void text(const char *s, int x, int y, bool big, eadk_color_t c, eadk_color_t bg) {
  eadk_point_t p = {x, y};
  eadk_display_draw_string(s, p, big, c, bg);
}

// Ecrit un paragraphe avec retour a la ligne automatique
static int para(const char *s, int x, int y, int max, eadk_color_t c, eadk_color_t bg) {
  char line[48];
  int n = 0;
  while (*s) {
    int w = 0;
    while (s[w] && s[w] != ' ') w++;
    if (n > 0 && n + 1 + w > max) {
      line[n] = 0;
      text(line, x, y, false, c, bg);
      y += 16;
      n = 0;
    }
    if (n > 0) line[n++] = ' ';
    for (int i = 0; i < w && n < 46; i++) line[n++] = s[i];
    s += w;
    while (*s == ' ') s++;
  }
  if (n) {
    line[n] = 0;
    text(line, x, y, false, c, bg);
    y += 16;
  }
  return y;
}

static void hearts(int hp) {
  for (int i = 0; i < 3; i++) fill(256 + i * 18, 8, 14, 14, i < hp ? RED : GRAY);
}

/* ---------- Entrees clavier ---------- */

// ok_only = true : attend OK/EXE (retourne 0)
// ok_only = false : attend 1, 2 ou 3 (retourne 1..3)
static int wait_choice(bool ok_only) {
  while (eadk_keyboard_scan()) eadk_timing_msleep(20);
  while (1) {
    eadk_keyboard_state_t k = eadk_keyboard_scan();
    if (ok_only) {
      if (eadk_keyboard_key_down(k, eadk_key_ok) || eadk_keyboard_key_down(k, eadk_key_exe)) return 0;
    } else {
      if (eadk_keyboard_key_down(k, eadk_key_one)) return 1;
      if (eadk_keyboard_key_down(k, eadk_key_two)) return 2;
      if (eadk_keyboard_key_down(k, eadk_key_three)) return 3;
    }
    eadk_timing_msleep(20);
  }
}

static void page(const char *s) {
  fill(0, 0, 320, 240, BLACK);
  para(s, 14, 40, 40, WHITE, BLACK);
  text("[OK] continuer", 100, 214, false, GRAY, BLACK);
  wait_choice(true);
}

/* ---------- Cinematique 2D ---------- */

static void intro(void) {
  page("Vol 714, 11h42. Tu es assis dans l'avion quand une violente turbulence secoue l'appareil. Les moteurs s'eteignent un par un.");

  int x = 0, y = 0;
  for (int f = 0; f < 59; f++) {
    x = 10 + f * 4;
    y = 10 + f * 3;
    fill(0, 0, 320, 190, SKY);
    fill(0, 190, 320, 50, GREEN);
    // montagnes
    fill(180, 150, 60, 40, GRAY);
    fill(195, 135, 30, 15, GRAY);
    fill(260, 160, 50, 30, GRAY);
    // traine de fumee
    for (int t = 1; t <= 7; t++) fill(x - t * 8, y - t * 6, 9, 9, (t % 2) ? GRAY : DARK);
    // avion
    fill(x, y, 38, 8, WHITE);
    fill(x + 10, y - 7, 12, 7, RGB(200,200,200));
    fill(x + 10, y + 8, 12, 5, RGB(200,200,200));
    fill(x - 4, y - 9, 6, 9, RED);
    fill(x + 30, y + 1, 6, 4, SKY);
    // flammes
    fill(x - 8, y + 2, 7, 5, ORANGE);
    fill(x - 12, y + 3, 5, 3, YELLOW);
    eadk_display_wait_for_vblank();
    eadk_timing_msleep(25);
  }

  // impact + explosion
  for (int r = 1; r <= 12; r++) {
    fill(x + 18 - r * 8, 190 - r * 6, r * 16, r * 6, r % 2 ? ORANGE : RED);
    fill(x + 18 - r * 4, 190 - r * 3, r * 8, r * 3, YELLOW);
    eadk_timing_msleep(40);
  }
  fill(0, 0, 320, 240, WHITE);
  eadk_timing_msleep(150);
  fill(0, 0, 320, 240, BLACK);
  eadk_timing_msleep(500);

  page("Un grand silence. Puis des flammes, de la fumee, des debris partout... Tu es peut-etre le seul survivant. Ta survie ne depend plus que de tes choix.");
}

/* ---------- Histoire ---------- */

typedef struct {
  const char *txt;
  const char *opt[3];
  const char *res[3];
  int8_t dmg[3];
} Node;

#define N 8

static const Node story[N] = {
  { "Tu reprends connaissance dans l'epave. Du feu, de la fumee, des cris au loin. Que fais-tu ?",
    {"Fuir loin de l'epave", "Fouiller la cabine en flammes", "Rester assis et attendre"},
    {"Tu t'eloignes juste avant une explosion. Ouf !",
     "Tu trouves un sac mais tu te brules les bras.",
     "La fumee t'intoxique. Tu t'evanouis un moment."},
    {0, 1, 1} },

  { "La nuit tombe. Il fait de plus en plus froid et tu entends des bruits dans la foret.",
    {"Dormir au milieu de l'epave", "Allumer un feu avec des debris", "Marcher dans le noir"},
    {"Tu grelottes toute la nuit. Tu es epuise.",
     "Le feu te rechauffe et tient les betes a distance.",
     "Tu tombes dans un ravin. Aie !"},
    {1, 0, 1} },

  { "Le soleil se leve. Tu as tres soif, ta gorge est seche.",
    {"Boire l'eau d'une flaque", "Manger des baies rouges inconnues", "Chercher une source d'eau"},
    {"L'eau est sale. Tu as mal au ventre.",
     "Elles sont toxiques ! Tu vomis violemment.",
     "Tu entends un ruisseau et tu le trouves."},
    {1, 2, 0} },

  { "Tu trouves une riviere. L'eau semble claire mais tu n'as aucun recipient.",
    {"Faire bouillir l'eau dans une boite", "Boire directement a la riviere", "Suivre la riviere vers l'aval"},
    {"Eau propre. Tu te sens beaucoup mieux.",
     "Un parasite... Tu as de la fievre.",
     "Les rivieres menent souvent aux villages. Bon reflexe."},
    {0, 1, 0} },

  { "Au loin, un helicoptere survole la foret. Il ne t'a pas vu.",
    {"Courir en criant", "Monter sur une colline degagee", "Faire un feu a fumee epaisse"},
    {"Tu t'epuises et tu te tords la cheville.",
     "De la, tu es beaucoup plus visible.",
     "La fumee monte haut dans le ciel. Bon signal."},
    {1, 0, 0} },

  { "Un violent orage eclate. La pluie et la foudre s'abattent sur la foret.",
    {"Te mettre sous le plus grand arbre", "Continuer a marcher sous la pluie", "T'abriter sous un rocher"},
    {"La foudre frappe pres de toi ! Tu es sonne.",
     "Tu es trempe et glace.",
     "Tu restes au sec et en securite."},
    {2, 1, 0} },

  { "Un grognement. Un loup rode a quelques metres, les yeux fixes sur toi.",
    {"Brandir une torche enflammee", "Courir de toutes tes forces", "Grimper dans un arbre"},
    {"Le loup recule et s'enfuit.",
     "Le loup te rattrape et te mord le bras.",
     "Hors de portee, tu attends. Le loup part."},
    {0, 2, 0} },

  { "Un helicoptere de secours approche enfin. Il faut absolument te faire reperer !",
    {"Rester cache sous les arbres", "Ecrire SOS au sol avec des pierres", "Signaler avec un morceau de metal"},
    {"Ils ne te voient pas... il repasse de justesse.",
     "Le pilote lit ton message. Il se pose !",
     "Le reflet attire les secours !"},
    {1, 0, 0} },
};

/* ---------- Ecrans de jeu ---------- */

static void draw_node(const Node *n, int i, int hp) {
  fill(0, 0, 320, 240, DARK);
  char d[] = "Jour X";
  d[5] = '1' + i / 2;
  text(d, 10, 8, false, YELLOW, DARK);
  hearts(hp);
  para(n->txt, 10, 36, 42, WHITE, DARK);
  for (int k = 0; k < 3; k++) {
    char b[48];
    int p = 0;
    b[p++] = '1' + k;
    b[p++] = '.';
    b[p++] = ' ';
    for (const char *s = n->opt[k]; *s && p < 44; s++) b[p++] = *s;
    b[p] = 0;
    text(b, 10, 140 + k * 28, false, LIME, DARK);
  }
}

static void show_result(const char *res, int dmg, int hp) {
  fill(0, 0, 320, 240, dmg ? RGB(40,10,10) : RGB(10,35,15));
  eadk_color_t bg = dmg ? RGB(40,10,10) : RGB(10,35,15);
  hearts(hp > 0 ? hp : 0);
  para(res, 10, 50, 42, WHITE, bg);
  if (dmg) {
    char b[] = "Tu perds 1 vie !";
    b[9] = '0' + dmg;
    text(b, 10, 130, true, RED, bg);
  } else {
    text("Bon choix !", 10, 130, true, LIME, bg);
  }
  text("[OK] continuer", 100, 214, false, GRAY, bg);
  wait_choice(true);
}

static void death(int i) {
  fill(0, 0, 320, 240, RGB(60,0,0));
  text("TU ES MORT", 90, 60, true, RED, RGB(60,0,0));
  char b[] = "Tu as tenu jusqu'au jour X";
  b[25] = '1' + i / 2;
  text(b, 40, 110, false, WHITE, RGB(60,0,0));
  text("Trop de mauvais choix...", 60, 135, false, WHITE, RGB(60,0,0));
  text("[OK] rejouer", 110, 214, false, GRAY, RGB(60,0,0));
  wait_choice(true);
}

static void win(int hp) {
  eadk_color_t bg = RGB(10,60,30);
  fill(0, 0, 320, 240, bg);
  text("SAUVE !", 110, 50, true, YELLOW, bg);
  para("Les secours t'ont retrouve. Tu es le seul survivant du vol 714.", 14, 100, 40, WHITE, bg);
  char b[] = "Vies restantes : X";
  b[17] = '0' + hp;
  text(b, 14, 160, false, LIME, bg);
  text("[OK] rejouer", 110, 214, false, GRAY, bg);
  wait_choice(true);
}

static void play(void) {
  int hp = 3;
  for (int i = 0; i < N; i++) {
    const Node *n = &story[i];
    draw_node(n, i, hp);
    int c = wait_choice(false) - 1;
    hp -= n->dmg[c];
    show_result(n->res[c], n->dmg[c], hp);
    if (hp <= 0) {
      death(i);
      return;
    }
  }
  win(hp);
}

int main(int argc, char *argv[]) {
  bool first = true;
  while (1) {
    if (first) {
      intro();
      first = false;
    }
    play();
  }
  return 0;
}
