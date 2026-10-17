// Ile - survie tropicale (NumWorks, app externe .nwa)
// A placer dans src/main.c, avec ile_data.h dans le meme dossier src/

#include "eadk.h"
#include "ile_data.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Ile";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

#define RGB(r,g,b) ((eadk_color_t)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3)))
#define WHITE   RGB(255,255,255)
#define BLACK   RGB(0,0,0)
#define GRAY    RGB(110,110,110)
#define DARK    RGB(12,16,30)
#define RED     RGB(220,40,40)
#define YELLOW  RGB(255,230,60)
#define LIME    RGB(120,230,120)
#define BLUE    RGB(70,140,255)
#define PURPLE  RGB(170,80,200)
#define SAND    RGB(210,180,110)
#define JUNGLE  RGB(30,90,40)

/* ---------- Etat du jeu ---------- */

static int hy, en, sa, inf;          // hydratation, energie, sante, infection
static int jour, ph, wx, prof;       // jour, phase (0-2), meteo (0-2), profil (1-3)
static int px, py;                   // position
static int wood, deb, mtl, vian, gour, abri, fin;
static char mp[5][5];                // '.', 'E', 'C', 'P', 'M'
static uint8_t vis[5][5];
static uint32_t rs = 2463534242u;    // generateur aleatoire
static char lg[4][44];               // journal de messages

/* ---------- Outils ---------- */

static int rnd(int n) {
  rs ^= rs << 13;
  rs ^= rs >> 17;
  rs ^= rs << 5;
  return (int)(rs % (uint32_t)n);
}
static int rr(int a, int b) { return a + rnd(b - a + 1); }
static int cl(int v) { return v < 0 ? 0 : v > 100 ? 100 : v; }
static void md(int h, int e, int s, int i) {
  hy = cl(hy + h); en = cl(en + e); sa = cl(sa + s); inf = cl(inf + i);
}
static int kk(int n) {
  if (prof != 3) return n;
  int r = (n * 7 + 5) / 10;
  return r < 1 ? 1 : r;
}

static char *ap(char *p, const char *s) { while (*s) *p++ = *s++; return p; }
static char *an(char *p, int v) {
  char t[12];
  int n = 0;
  if (v < 0) { *p++ = '-'; v = -v; }
  if (v == 0) t[n++] = '0';
  while (v) { t[n++] = '0' + v % 10; v /= 10; }
  while (n) *p++ = t[--n];
  return p;
}

static void msg(const char *s) {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 44; j++) lg[i][j] = lg[i + 1][j];
  int j = 0;
  while (s[j] && j < 43) { lg[3][j] = s[j]; j++; }
  lg[3][j] = 0;
}
static void msgn(const char *a, int n, const char *b) {
  char t[44];
  char *p = ap(t, a);
  p = an(p, n);
  p = ap(p, b);
  *p = 0;
  msg(t);
}

/* ---------- Affichage ---------- */

static void fill(int x, int y, int w, int h, eadk_color_t c) {
  eadk_rect_t r = {x, y, w, h};
  eadk_display_push_rect_uniform(r, c);
}
static void text(const char *s, int x, int y, bool big, eadk_color_t c, eadk_color_t bg) {
  eadk_point_t p = {x, y};
  eadk_display_draw_string(s, p, big, c, bg);
}
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

/* ---------- Clavier ---------- */

// 0-9 chiffres, 10 OK, 11 retour, 12 haut, 13 bas, 14 gauche, 15 droite
static int get_key(void) {
  while (eadk_keyboard_scan()) eadk_timing_msleep(20);
  while (1) {
    eadk_keyboard_state_t k = eadk_keyboard_scan();
    int r = -1;
#define K(x) eadk_keyboard_key_down(k, eadk_key_##x)
    if (K(zero)) r = 0;
    else if (K(one)) r = 1;
    else if (K(two)) r = 2;
    else if (K(three)) r = 3;
    else if (K(four)) r = 4;
    else if (K(five)) r = 5;
    else if (K(six)) r = 6;
    else if (K(seven)) r = 7;
    else if (K(eight)) r = 8;
    else if (K(nine)) r = 9;
    else if (K(ok) || K(exe)) r = 10;
    else if (K(back) || K(backspace)) r = 11;
    else if (K(up)) r = 12;
    else if (K(down)) r = 13;
    else if (K(left)) r = 14;
    else if (K(right)) r = 15;
#undef K
    if (r >= 0) {
      rs ^= (uint32_t)eadk_timing_millis() * 2654435761u;
      if (!rs) rs = 1;
      return r;
    }
    eadk_timing_msleep(20);
  }
}

static void wait_ok(void) {
  while (get_key() != 10) {}
}

static void page(const char *s) {
  fill(0, 0, 320, 240, BLACK);
  para(s, 14, 40, 40, WHITE, BLACK);
  text("[OK] continuer", 100, 214, false, GRAY, BLACK);
  wait_ok();
}

// Fenetre de choix (1 a 3 options). Retourne 1, 2 ou 3.
static int ask(const char *t, const char *o1, const char *o2, const char *o3) {
  const char *o[3] = {o1, o2, o3};
  int n = 0;
  fill(0, 0, 320, 240, DARK);
  int y = para(t, 10, 20, 40, WHITE, DARK) + 14;
  for (int i = 0; i < 3; i++) {
    if (!o[i]) continue;
    char b[48];
    char *p = b;
    *p++ = '1' + i;
    *p++ = ' ';
    p = ap(p, o[i]);
    *p = 0;
    text(b, 14, y, false, LIME, DARK);
    y += 22;
    n++;
  }
  while (1) {
    int k = get_key();
    if (k >= 1 && k <= n) return k;
  }
}

static const char *lieu(void) {
  if (px == 2 && py == 2) return LIEU_CAMP;
  switch (mp[py][px]) {
    case 'E': return LIEU_EAU;
    case 'C': return LIEU_CHASSE;
    case 'P': return LIEU_PLAGE;
  }
  return LIEU_JUNGLE;
}

static void bar(int i, const char *lab, int val, eadk_color_t c) {
  int y = 26 + i * 16;
  text(lab, 4, y - 1, false, WHITE, DARK);
  fill(35, y - 1, 102, 12, GRAY);
  fill(36, y, 100, 10, BLACK);
  fill(36, y, val, 10, c);
}

static void inv(int i, const char *a, int n, const char *b) {
  char t[24];
  char *p = ap(t, a);
  p = an(p, n);
  p = ap(p, b);
  *p = 0;
  text(t, 190, 25 + i * 16, false, WHITE, DARK);
}

static void draw_main(int n, const char *const *lab) {
  fill(0, 0, 320, 240, DARK);
  char b[40];
  char *p = ap(b, "J");
  p = an(p, jour);
  p = ap(p, " ");
  p = ap(p, NOM_PHASE[ph]);
  p = ap(p, " ");
  p = ap(p, NOM_METEO[wx]);
  *p = 0;
  text(b, 4, 4, false, YELLOW, DARK);
  text(lieu(), 190, 4, false, LIME, DARK);

  bar(0, "Eau", hy, BLUE);
  bar(1, "Ene", en, YELLOW);
  bar(2, "San", sa, sa > 30 ? LIME : RED);
  bar(3, "Inf", inf, inf > 50 ? RED : PURPLE);
  inv(0, "Bois ", wood, "");
  inv(1, "Debris ", deb, "");
  inv(2, "Metal ", mtl, "/3");
  inv(3, "Viande ", vian, "");
  inv(4, "Gourdes ", gour, "");

  fill(0, 104, 320, 1, GRAY);
  for (int i = 0; i < 4; i++) text(lg[i], 4, 108 + i * 16, false, WHITE, DARK);
  fill(0, 171, 320, 1, GRAY);
  for (int i = 0; i < n; i++) {
    char t[28];
    char *q = t;
    *q++ = '1' + i;
    *q++ = ' ';
    q = ap(q, lab[i]);
    *q = 0;
    text(t, i < 4 ? 6 : 166, 176 + (i % 4) * 16, false, YELLOW, DARK);
  }
}

static void show_map(void) {
  fill(0, 0, 320, 240, DARK);
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      eadk_color_t c = GRAY;
      char s[2] = {'?', 0};
      if (vis[y][x]) {
        s[0] = '.';
        c = JUNGLE;
        if (mp[y][x] == 'E') { c = BLUE; s[0] = 'E'; }
        if (mp[y][x] == 'C') { c = RGB(60,170,60); s[0] = 'H'; }
        if (mp[y][x] == 'P') { c = SAND; s[0] = 'P'; }
      }
      if (x == 2 && y == 2) { c = YELLOW; s[0] = '#'; }
      fill(70 + x * 36, 10 + y * 36, 34, 34, c);
      text(s, 82 + x * 36, 20 + y * 36, false, BLACK, c);
      if (x == px && y == py) text("@", 82 + x * 36, 33 + y * 36, false, RED, c);
    }
  }
  text("# camp  E eau  H chasse  P plage", 30, 198, false, WHITE, DARK);
  text("@ toi  ? inconnu   [OK] retour", 30, 214, false, GRAY, DARK);
  wait_ok();
}

/* ---------- Evenements ---------- */

static void tk(void);

static void ev(void) {
  int r = rnd(NB_EV);
  int c = ask(EV[r].txt, EV[r].o1, EV[r].o2, NULL);
  switch (r) {
    case 0:
      if (c == 1) { md(0, 0, -15, 25); msg("Plaie infectee par le venin !"); }
      else { md(0, -15, -6, 8); msg("Tu limites les degats."); }
      break;
    case 1:
      if (c == 1) md(0, -10, 0, 0);
      if (c == 1 && rnd(2)) msg("Tu recuperes tout !");
      else { wood /= 2; deb /= 2; vian /= 2; msg("Il part avec la moitie !"); }
      break;
    case 2:
      if (c == 1) {
        md(0, 30, 0, 0);
        if (rnd(10) < 5) { md(0, 0, -8, 30); msg("Mauvais gout... tu es malade !"); }
        else msg("Ca passe !");
      }
      break;
    case 3:
      if (c == 1) {
        md(30, -8, 0, 0);
        if (rr(1, 5) == 1) { md(0, 0, -15, 0); msg("Tu chutes de l'arbre !"); }
        else msg("Eau de coco : +hydratation");
      }
      break;
    case 4:
      if (c == 1 && wood > 0) { wood--; msg("La fumee chasse les moustiques."); }
      else { md(0, 0, -3, 15); msg("Piqures : infection !"); }
      break;
    case 5:
      if (c == 1) { md(30, 0, 0, 35); msg("Eau douteuse... infection !"); }
      break;
    case 6:
      if (rnd(3)) msg("Esquive de justesse !");
      else { md(0, -5, -15, 0); msg("L'arbre te blesse !"); }
      break;
    default:
      deb += 3;
      msg("Epave : +3 debris");
  }
}

/* ---------- Temps qui passe ---------- */

static void tk(void) {
  static const int METEO[6] = {0, 0, 0, 1, 1, 2};
  if (ph < 2 && rnd(10) < 3) ev();
  md(-8, -3, 0, 0);
  if (wx == 1) md(0, -2, 0, 0);
  md(0, 0, 0, prof == 2 ? -4 : -2);
  if (hy < 1) { md(0, 0, -12, 0); msg("Deshydrate !"); }
  if (en < 1) { md(0, 0, -10, 0); msg("Epuise !"); }
  if (inf > 50) { md(0, 0, -6, 0); msg("Infection !"); }
  else if (hy > 40 && en > 30) md(0, 0, 3, 0);
  ph++;
  if (ph > 2) {
    ph = 0;
    jour++;
    wx = METEO[rnd(6)];
    char t[44];
    char *p = ap(t, "Jour ");
    p = an(p, jour);
    p = ap(p, " : ");
    p = ap(p, NOM_METEO[wx]);
    *p = 0;
    msg(t);
  }
}

/* ---------- Actions ---------- */

enum { A_FOUILLE, A_CARTE, A_BSRC, A_REMPLIR, A_CHASSE, A_CUEILLE, A_PLUIE,
       A_SOIN, A_RADIO, A_ABRI, A_GOURDE, A_MANGE, A_DORMIR };

static void move(int dx, int dy) {
  int nx = px + dx, ny = py + dy;
  if (nx < 0 || nx > 4 || ny < 0 || ny > 4) { msg("Mer a perte de vue."); return; }
  px = nx; py = ny;
  vis[py][px] = 1;
  md(0, -(prof == 1 ? 4 : 7) - (wx == 2 ? 4 : 0), 0, 0);
  if (wx == 2 && rnd(3) == 0) { msg("Chute dans la tempete !"); md(0, 0, -8, 0); }
  if (mp[py][px] == 'M' && rnd(10) < 7) msg("Un eclat metallique brille !");
  tk();
}

static void drink(bool source) {
  int q = 15;
  if (wood > 0 && ask("Comment boire cette eau ?", "Crue", "Bouillie (-1 bois)", NULL) == 2) {
    wood--;
    q = 0;
  }
  md(45, 0, 0, q);
  msg(q ? "Eau non purifiee : infection." : "Eau bouillie : saine.");
  if (source) tk();
}

static void act(int code) {
  switch (code) {
    case A_FOUILLE: {
      md(0, -6, 0, 0);
      char t = mp[py][px];
      if (t == 'M') {
        mtl++;
        mp[py][px] = '.';
        msgn("PIECE DE METAL ! ", mtl, "/3");
      } else if (t == 'P') {
        int n = rr(1, 3);
        deb += n;
        msgn("+", n, " debris");
      } else {
        int n = rr(1, 3);
        wood += n;
        msgn("+", n, " bois");
      }
      tk();
      break;
    }
    case A_CARTE: show_map(); break;
    case A_BSRC: drink(true); break;
    case A_REMPLIR:
      gour = gour + 2 > 4 ? 4 : gour + 2;
      md(0, -3, 0, 0);
      msg("+2 gourdes (eau non purifiee)");
      tk();
      break;
    case A_CHASSE:
      md(0, -8, 0, 0);
      if (rr(1, 100) < (prof == 1 ? 85 : 55)) {
        int n = prof == 1 ? 2 : 1;
        vian += n;
        msgn("Chasse reussie : +", n, " viande");
      } else msg("Tu n'attrapes rien.");
      tk();
      break;
    case A_CUEILLE:
      md(10, 15, 0, 0);
      msg("Fruits : +eau +energie");
      if (rr(1, 100) < (prof == 2 ? 8 : 20)) { msg("Baie douteuse !"); md(0, 0, 0, 20); }
      tk();
      break;
    case A_PLUIE:
      md(35, -2, 0, 0);
      msg("Eau de pluie : propre.");
      tk();
      break;
    case A_SOIN:
      md(0, -4, 5, prof == 2 ? -40 : -20);
      msg("Tu soignes tes plaies.");
      tk();
      break;
    case A_ABRI: {
      int a1 = kk(6), a2 = kk(4);
      if (wood < a1 || deb < a2) {
        char t[44];
        char *p = ap(t, "Il faut ");
        p = an(p, a1);
        p = ap(p, " bois, ");
        p = an(p, a2);
        p = ap(p, " debris");
        *p = 0;
        msg(t);
        break;
      }
      wood -= a1; deb -= a2; abri = 1;
      md(0, -10, 0, 0);
      msg("Abri construit !");
      tk();
      break;
    }
    case A_RADIO: {
      int a1 = kk(6);
      if (mtl < 3 || deb < a1) {
        char t[44];
        char *p = ap(t, "Il faut 3 metal, ");
        p = an(p, a1);
        p = ap(p, " debris");
        *p = 0;
        msg(t);
        break;
      }
      deb -= a1;
      fin = 1;
      break;
    }
    case A_GOURDE:
      gour--;
      drink(false);
      break;
    case A_MANGE: {
      vian--;
      int q = 1;
      if (wood > 0 && ask("Comment manger la viande ?", "Crue", "Cuite (-1 bois)", NULL) == 2) {
        wood--;
        q = 0;
      }
      md(0, q ? 25 : 40, 0, 25 * q);
      msg(q ? "Viande crue : infection." : "Viande cuite : bon repas.");
      break;
    }
    case A_DORMIR:
      if (abri && px == 2 && py == 2) {
        md(0, wx == 2 ? 30 : 35, 2, 0);
        msg("Tu dors a l'abri.");
      } else if (wx == 2) {
        md(0, -25, -30, 0);
        msg("TEMPETE SANS ABRI !!!");
      } else if (wx == 1) {
        md(0, 10, -4, 0);
        msg("Nuit sous la pluie.");
      } else {
        md(0, 28, 0, 0);
        msg("Nuit a la belle etoile.");
      }
      tk();
      break;
  }
}

/* ---------- Partie ---------- */

static void init_game(void) {
  hy = en = 100;
  inf = 0;
  sa = prof == 2 ? 100 : 75;
  jour = 1; ph = 0; wx = 0;
  px = py = 2;
  wood = deb = mtl = vian = gour = abri = fin = 0;
  for (int y = 0; y < 5; y++)
    for (int x = 0; x < 5; x++) { mp[y][x] = '.'; vis[y][x] = 0; }
  vis[2][2] = 1;
  for (int i = 0; i < 4; i++) lg[i][0] = 0;
  int cx[24], cy[24], n = 0;
  for (int y = 0; y < 5; y++)
    for (int x = 0; x < 5; x++)
      if (!(x == 2 && y == 2)) { cx[n] = x; cy[n] = y; n++; }
  const char *types = "EECCPMMM";
  for (int t = 0; t < 8; t++) {
    int i = rnd(n);
    mp[cy[i]][cx[i]] = types[t];
    cx[i] = cx[n - 1];
    cy[i] = cy[n - 1];
    n--;
  }
  msg("Fleches : se deplacer.");
}

static void ending(int type) {
  eadk_color_t bg = type == 1 ? RGB(10, 60, 30) : RGB(60, 0, 0);
  fill(0, 0, 320, 240, bg);
  if (type == 1) {
    text("SAUVE !", 110, 40, true, YELLOW, bg);
    para("La radio crepite : les secours arrivent.", 14, 90, 40, WHITE, bg);
  } else if (type == 0) {
    text("TU ES MORT", 90, 40, true, RED, bg);
    para("Les dangers de l'ile ont eu raison de toi.", 14, 90, 40, WHITE, bg);
  } else {
    text("FIN DE SAISON", 70, 40, true, RED, bg);
    para("Les cyclones arrivent. Plus aucun espoir de secours.", 14, 90, 40, WHITE, bg);
  }
  char t[40];
  char *p = ap(t, "Jours sur l'ile : ");
  p = an(p, jour);
  *p = 0;
  text(t, 14, 150, false, LIME, bg);
  text("[OK] rejouer", 110, 214, false, GRAY, bg);
  wait_ok();
}

static void run(void) {
  page(INTRO_1);
  page(INTRO_2);
  page(INTRO_3);
  prof = ask(PROFILS_TXT, "Chasseur", "Medecin", "Ingenieur");
  init_game();

  while (1) {
    if (sa < 1) { ending(0); return; }
    if (fin) { ending(1); return; }
    if (jour > 40) { ending(2); return; }

    int code[10], n = 0;
    const char *lab[10];
#define ADD(c, l) do { code[n] = c; lab[n] = l; n++; } while (0)
    char t = mp[py][px];
    if (ph == 2) {
      ADD(A_DORMIR, "Dormir");
    } else {
      ADD(A_FOUILLE, "Fouiller");
      ADD(A_CARTE, "Carte");
      if (t == 'E') { ADD(A_BSRC, "Boire source"); ADD(A_REMPLIR, "Remplir gourdes"); }
      if (t == 'C') { ADD(A_CHASSE, "Chasser"); ADD(A_CUEILLE, "Cueillir"); }
      if (wx == 1) ADD(A_PLUIE, "Boire pluie");
      if (px == 2 && py == 2) {
        ADD(A_SOIN, "Soigner");
        ADD(A_RADIO, "Reparer radio");
        if (!abri) ADD(A_ABRI, "Construire abri");
      }
    }
    if (gour > 0) ADD(A_GOURDE, "Boire gourde");
    if (vian > 0) ADD(A_MANGE, "Manger viande");
#undef ADD

    draw_main(n, lab);
    int k = get_key();
    if (k >= 12 && k <= 15) {
      if (ph == 2) msg("Il fait nuit : dors.");
      else if (k == 12) move(0, -1);
      else if (k == 13) move(0, 1);
      else if (k == 14) move(-1, 0);
      else move(1, 0);
    } else if (k >= 1 && k <= n) {
      act(code[k - 1]);
    }
  }
}

int main(int argc, char *argv[]) {
  while (1) run();
  return 0;
}
