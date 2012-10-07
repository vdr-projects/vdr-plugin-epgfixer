/*
 * regexp.c: Regular expression list item
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "regexp.h"

// for PCRE without JIT support
#ifndef PCRE_STUDY_JIT_COMPILE
#define PCRE_STUDY_JIT_COMPILE 0
#endif

#define OVECCOUNT 33    /* should be a multiple of 3 */

typedef enum { NONE,FIRST,GLOBAL } replace;

/* Global instance */
cEpgfixerList<cRegexp, cEvent> EpgfixerRegexps;

const char *strSources[] = { "title","shorttext","description","undefined" };

typedef enum { ATITLE, PTITLE, TITLE, ASHORTTEXT, PSHORTTEXT, SHORTTEXT, ADESCRIPTION, PDESCRIPTION, DESCRIPTION, RATING } backrefs;
const char *strBackrefs[] = { "atitle", "ptitle", "title", "ashorttext", "pshorttext", "shorttext", "adescription", "pdescription", "description", "rating" };

cRegexp::cRegexp()
{
  modifiers = 0;
  regexp = NULL;
  replace = NONE;
  replacement = NULL;
  source = REGEXP_UNDEFINED;
  re = NULL;
  sd = NULL;
}

cRegexp::~cRegexp(void)
{
  Free();
  free(regexp);
  free(replacement);
  FreeCompiled();
}

void cRegexp::Compile()
{
  FreeCompiled();
  const char *error;
  int erroffset;
  re = pcre_compile(regexp, modifiers, &error, &erroffset, NULL);
  if (error) {
     error("PCRE compile error: %s at offset %i", error, erroffset);
     enabled = false;
     }
  else {
     sd = pcre_study(re, PCRE_STUDY_JIT_COMPILE, (const char **)&error);
     if (error)
        error("PCRE study error: %s", error);
     }
}

void cRegexp::FreeCompiled()
{
  if (re) {
     pcre_free(re);
     re = NULL;
     }
  if (sd) {
#ifdef PCRE_CONFIG_JIT
     pcre_free_study(sd);
#else
     pcre_free(sd);
#endif
     sd = NULL;
     }
}

void cRegexp::ParseRegexp(char *restring)
{
  if (restring) {
     int len = strlen(restring);
     if (len > 2 && restring[1] == '/' && (restring[0] == 'm' || restring[0] == 's')) {
        // separate modifiers from end of regexp
        char *l = strrchr(restring, '/');
        if (l) {
           *l = 0;
           int i = 1;
           // handle all modifiers
           while (*(l + i) != 0) {
                 switch (*(l + i)) {
                   case 'g':
                     if (restring[0] == 's')
                        replace = GLOBAL;
                     break;
                   case 'i':
                     modifiers = modifiers | PCRE_CASELESS;
                     break;
                   case 'm':
                     modifiers = modifiers | PCRE_MULTILINE;
                     break;
                   case 's':
                     modifiers = modifiers | PCRE_DOTALL;
                     break;
                   case 'u':
                     modifiers = modifiers | PCRE_UTF8;
                     break;
                   case 'x':
                     modifiers = modifiers | PCRE_EXTENDED;
                     break;
                   case 'X':
                     modifiers = modifiers | PCRE_EXTRA;
                     break;
                   default:
                     break;
                   }
                 i++;
                 }
           }
        // parse regexp format 's///'
        if (restring[0] == 's') {
           if (replace == NONE)
              replace = FIRST;
           char *p = &restring[2];
           while (p = strchr(p, '/')) {
                 // check for escaped slashes
                 if (*(p - 1) != '\\') {
                    *p = 0;
                    regexp = strdup(&restring[2]);
                    if (*(p + 1) != '/') // 
                       replacement = strdup(p + 1);
                    else
                       replacement = strdup("");
                    break;
                    }
                 }
           }
        else if (restring[0] == 'm') // parse regexp format 'm//'
           regexp = strdup(&restring[2]);
        }
     else // use backreferences
        regexp = strdup(restring);
     }
}

void cRegexp::SetFromString(char *s, bool Enabled)
{
  modifiers = 0;
  FREE(regexp);
  replace = NONE;
  FREE(replacement);
  Free();
  FreeCompiled();
  source = REGEXP_UNDEFINED;
  cListItem::SetFromString(s, Enabled);
  if (enabled) {
     char *p = strchr(s, '=');
     if (p) {
        *p = 0;
        ParseRegexp(p + 1);
        char *chanfield = (s[0] == '!') ? s + 1 : s;
        char *field = chanfield;
        // find active channels list
        char *f = strchr(chanfield, ':');
        if (f) {
           *f = 0;
           field = f + 1;
           numchannels = LoadChannelsFromString(chanfield);
           }
        if (strcmp(field, "title") == 0)
           source = REGEXP_TITLE;
        if (strcmp(field, "shorttext") == 0)
           source = REGEXP_SHORTTEXT;
        if (strcmp(field, "description") == 0)
           source = REGEXP_DESCRIPTION;
        Compile();
        }
     }
}

bool cRegexp::Apply(cEvent *Event)
{
  if (enabled && re && IsActive(Event->ChannelID())) {
     cString tmpstring;
     switch (source) {
       case REGEXP_TITLE:
         tmpstring = Event->Title();
         break;
       case REGEXP_SHORTTEXT:
         tmpstring = Event->ShortText();
         break;
       case REGEXP_DESCRIPTION:
         tmpstring = Event->Description();
         break;
       default:
         tmpstring = "";
         break;
       }
     if (!*tmpstring)
        tmpstring = "";
     int ovector[OVECCOUNT];
     int rc = 0;
     if (replace != NONE) {// find and replace
        int last_match_end = -1;
        int options = 0;
        int start_offset = 0;
        int tmpstringlen = strlen(*tmpstring);
        cString resultstring = "";
        // loop through matches
        while ((rc = pcre_exec(re, sd, *tmpstring, tmpstringlen, start_offset, options, ovector, OVECCOUNT)) > 0) {
              last_match_end = ovector[1];
              resultstring = cString::sprintf("%s%.*s%s", *resultstring, ovector[0] - start_offset, &tmpstring[start_offset], replacement);
              options = 0;
              if (ovector[0] == ovector[1]) {
                 if (ovector[0] == tmpstringlen)
                    break;
                 options = PCRE_NOTEMPTY_ATSTART | PCRE_ANCHORED;
                 }
              if (replace == FIRST) // only first match wanted
                 break;
              start_offset = ovector[1];
              }
        // replace EPG field if regexp matched
        if (last_match_end > 0 && (last_match_end < tmpstringlen - 1)) {
           resultstring = cString::sprintf("%s%s", *resultstring, tmpstring + last_match_end);
           switch (source) {
             case REGEXP_TITLE:
               Event->SetTitle(resultstring);
               break;
             case REGEXP_SHORTTEXT:
               Event->SetShortText(resultstring);
               break;
             case REGEXP_DESCRIPTION:
               Event->SetDescription(resultstring);
               break;
             default:
               break;
             }
           return true;
           }
        }
     else {// use backreferences
        const char *string;
        rc = pcre_exec(re, sd, *tmpstring, strlen(*tmpstring), 0, 0, ovector, OVECCOUNT);
        if (rc == 0) {
           error("maximum number of captured substrings is %d\n", OVECCOUNT / 3 - 1);
           }
        else if (rc > 0) {
           int i = 0;
           // loop through all possible backreferences
           // TODO allow duplicate backreference names?
           while (i < 10) {
             if (pcre_get_named_substring(re, tmpstring, ovector, rc, strBackrefs[i], &string) != PCRE_ERROR_NOSUBSTRING) {
                switch (i) {
                  case ATITLE:
                  case PTITLE:
                    if (Event->Title()) {
                       if (i == ATITLE)
                          Event->SetTitle(*cString::sprintf("%s %s", Event->Title(), string));
                       else
                          Event->SetTitle(*cString::sprintf("%s %s", string, Event->Title()));
                       break;
                       }
                  case TITLE:
                    Event->SetTitle(string);
                    break;
                  case ASHORTTEXT:
                  case PSHORTTEXT:
                    if (Event->ShortText()) {
                       if (i == ASHORTTEXT)
                          Event->SetShortText(*cString::sprintf("%s %s", Event->ShortText(), string));
                       else
                          Event->SetShortText(*cString::sprintf("%s %s", string, Event->ShortText()));
                       break;
                       }
                  case SHORTTEXT:
                    Event->SetShortText(string);
                    break;
                  case ADESCRIPTION:
                  case PDESCRIPTION:
                    if (Event->Description()) {
                       if (i == ADESCRIPTION)
                          Event->SetDescription(*cString::sprintf("%s %s", Event->Description(), string));
                       else
                          Event->SetDescription(*cString::sprintf("%s %s", string, Event->Description()));
                       break;
                       }
                  case DESCRIPTION:
                    Event->SetDescription(string);
                    break;
                  case RATING:
                    Event->SetParentalRating(atoi(string));
                    break;
                  default:
                    break;
                  }
                pcre_free_substring(string);
                }
              ++i;
              }
           return true;
           }
        }
     }
  return false;
}

void cRegexp::ToggleEnabled(void)
{
  if (source != REGEXP_UNDEFINED)
     enabled = !enabled;
}
