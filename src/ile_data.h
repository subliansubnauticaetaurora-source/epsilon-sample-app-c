// Textes du jeu "Ile" (survie tropicale) - ASCII sans accents
#ifndef ILE_DATA_H
#define ILE_DATA_H

static const char *const NOM_METEO[3] = {"Soleil", "Pluie", "Tempete"};
static const char *const NOM_PHASE[3] = {"Matin", "Aprem", "Nuit"};

// Nom du lieu selon le type de case : camp, vide, eau, chasse, plage
static const char *const LIEU_CAMP = "Camp";
static const char *const LIEU_JUNGLE = "Jungle";
static const char *const LIEU_EAU = "Source d'eau";
static const char *const LIEU_CHASSE = "Gibier et fruits";
static const char *const LIEU_PLAGE = "Plage de debris";

static const char *const INTRO_1 =
  "Ton avion s'est ecrase sur une ile tropicale deserte. Tu es peut-etre le seul survivant.";
static const char *const INTRO_2 =
  "Trouve 3 pieces de metal cachees sur l'ile, reviens au camp et repare la radio pour appeler les secours. Tu as 40 jours avant la saison des cyclones.";
static const char *const INTRO_3 =
  "Fleches : se deplacer. Chiffres : choisir une action. Chaque deplacement ou action prend une phase de la journee.";
static const char *const PROFILS_TXT =
  "Choisis ton profil. 1 Chasseur : chasse efficace, marche peu fatigante. 2 Medecin : sante max, guerit vite des infections. 3 Ingenieur : abri et radio moins chers (-30%).";

// Banque d'evenements aleatoires (la logique est dans main.c)
typedef struct {
  const char *txt;
  const char *o1;
  const char *o2;
} Ev;

#define NB_EV 8
static const Ev EV[NB_EV] = {
  {"Un serpent te mord la cheville !", "Inciser et sucer le venin", "Immobiliser et rester calme"},
  {"Un singe voleur attrape ton sac !", "Le poursuivre", "Le laisser filer"},
  {"Tu trouves une conserve perimee. Tu la manges ?", "Oui, j'ai faim", "Non, trop risque"},
  {"Un cocotier charge de noix de coco.", "Grimper (-8 energie)", "Passer ton chemin"},
  {"Une nuee de moustiques t'attaque !", "Faire de la fumee (-1 bois)", "Subir les piqures"},
  {"Une mare verdatre : l'eau est trouble.", "Boire quand meme", "Ne pas boire"},
  {"Un arbre mort s'effondre pres de toi !", "Se jeter de cote", NULL},
  {"Une epave de canot est echouee !", "Fouiller l'epave", NULL}
};

#endif
