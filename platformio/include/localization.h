#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "config.h"

// Only include the selected language at compile time to save memory
// To add a new language, create a new file in include/lang/ directory

#if DISPLAY_LANGUAGE == 0
#include "lang/en.h"
#elif DISPLAY_LANGUAGE == 1
#include "lang/es.h"
#elif DISPLAY_LANGUAGE == 2
#include "lang/fr.h"
#elif DISPLAY_LANGUAGE == 3
#include "lang/de.h"
#elif DISPLAY_LANGUAGE == 4
#include "lang/it.h"
#elif DISPLAY_LANGUAGE == 5
#include "lang/pt.h"
#elif DISPLAY_LANGUAGE == 6
#include "lang/nl.h"
#else
#include "lang/en.h" // Default to English
#endif

#endif // LOCALIZATION_H