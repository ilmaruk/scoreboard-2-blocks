#ifndef WELCOME_H
#define WELCOME_H

#include "fonts.h"

typedef struct {
  char *message;
  MD_MAX72XX::fontType_t *font;
} WelcomeMessage;

typedef struct {
  textEffect_t left;
  textEffect_t right;
} WelcomeTransition;

#define ARR_SIZE(arr) ( sizeof((arr)) / sizeof((arr[0])) )
// char *welcome_messages[] = {"BENVENUTI", "BEM-VINDOS", "BIENVENIDOS", "BIENVENUE", // "BONVENON",
//                             "BUN VENItI", "DOBRODOsLI",
//                             "dmbre dmhln", // "LASKAVO PROSIMO",
//                             "TERVETULOA", "uDVoZoLJuK", "VaLKOMNA",
//                             "VELKOMMEN", "ViTEJTE", "WELCOME", "WELKOM", "WILLKOMMEN", "WITAMY"};
WelcomeMessage welcome_messages[] = {
  {"BENVENUTI", apoteke},   // IT
  {"BEM-VINDOS", apoteke},  // PT
  {"BIENVENIDOS", apoteke}, // ES
  {"BIENVENUE", apoteke},   // FR
  {"BUN VENItI", apoteke},  // RO
  {"DOBRODOsLI", apoteke},  // HR
  {"\x0c4\x0ce\x0c1\x0d0\x0c5 \x0c4\x0ce\x0d8\x0cb\x0c8", cyrillic_bg},    // BG: ДОБРЕ ДОШЛИ
  {"\x089\x080\x08a\x097\x091\x08e\x090\x088\x091\x08b\x080", greek}, // GR: ΚΑΛΩΣΟΡΙΣΜΑ or maybe ΚΑΛΩΣ ΟΡΙΣΑΤΕ
  // {"LASKAVO PROSIMO", apoteke}, // UA - too long!
  {"TERVETULOA", apoteke}, // FI
  {"uDVoZoLJuK", apoteke}, // HU
  {"VaLKOMNA", apoteke},   // SE
  {"VELKOMMEN", apoteke},  // DK/NO
  {"ViTEJTE", apoteke},    // CZ
  {"WELCOME", apoteke},    // EN
  {"WELKOM", apoteke},     // NL
  {"WILLKOMMEN", apoteke}, // DE
  {"WITAMY", apoteke},     // PL
};

WelcomeTransition welcome_transitions[] = {
  {PA_SCROLL_DOWN, PA_SCROLL_UP},
  {PA_DISSOLVE, PA_DISSOLVE},
  {PA_GROW_DOWN, PA_GROW_UP},
  {PA_BLINDS, PA_BLINDS},
};

#endif // WELCOME_H