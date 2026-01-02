/*
 * regexp.c: Regular expression list item
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "regexp.h"

typedef enum { NONE,FIRST,GLOBAL } replace;

/* Global instance */
cEpgfixerList<cRegexp, cEvent> EpgfixerRegexps;

const char *strSources[] = { "title","shorttext","description","undefined" };

typedef enum { ATITLE, PTITLE, TITLE, ASHORTTEXT, PSHORTTEXT, SHORTTEXT, ADESCRIPTION, PDESCRIPTION, DESCRIPTION, RATING } backrefs;
const char *strBackrefs[] = { "atitle", "ptitle", "title", "ashorttext", "pshorttext", "shorttext", "adescription", "pdescription", "description", "rating" };

cRegexp::cRegexp()
{
  cmodifiers = 0;
  modifiers = 0;
  cregexp = NULL;
  regexp = NULL;
  replace = NONE;
  replacement = NULL;
  csource = REGEXP_UNDEFINED;
  source = REGEXP_UNDEFINED;
  cre = NULL;
  re = NULL;
}

cRegexp::~cRegexp(void)
{
  Free();
  free(cregexp);
  free(regexp);
  free(replacement);
  FreeCompiled();
}

void cRegexp::Compile()
{
  FreeCompiled();
  PCRE2_UCHAR error[256];
  PCRE2_SIZE erroffset;
  int errnumber;
  re = pcre2_compile((PCRE2_SPTR8)regexp, PCRE2_ZERO_TERMINATED, modifiers, &errnumber, &erroffset, NULL);
  if (!re) {
     pcre2_get_error_message(errnumber, error, sizeof(error));
     error("PCRE compile error: %s at offset %li", error, erroffset);
     enabled = false;
     }
  else {
     if (cregexp) {
        cre = pcre2_compile((PCRE2_SPTR8)cregexp, PCRE2_ZERO_TERMINATED, cmodifiers, &errnumber, &erroffset, NULL);
        if (!cre) {
           pcre2_get_error_message(errnumber, error, sizeof(error));
           error("PCRE compile error: %s at offset %li", error, erroffset);
           enabled = false;
           }
        }
     }
}

void cRegexp::FreeCompiled()
{
  if (re) {
     pcre2_code_free(re);
     re = NULL;
     }
  if (cre) {
     pcre2_code_free(cre);
     cre = NULL;
     }
}

int cRegexp::ParseModifiers(char *modstring, int substitution)
{
  int i = 0;
  int mods = 0;
  // handle all modifiers
  while (*(modstring + i) != 0) {
       switch (*(modstring + i)) {
         case 'g':
           if (substitution)
              replace = GLOBAL;
           break;
         case 'i':
           mods |= PCRE2_CASELESS;
           break;
         case 'm':
           mods |= PCRE2_MULTILINE;
           break;
         case 's':
           mods |= PCRE2_DOTALL;
           break;
         case 'u':
           mods |= PCRE2_UTF;
           break;
         case 'x':
           mods |= PCRE2_EXTENDED;
           break;
         default:
           break;
         }
       i++;
       }
  return mods;
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
           modifiers = ParseModifiers(l + 1, restring[0] == 's');
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
                 p++; // advance past escaped slash to avoid infinite loop
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
  cmodifiers = 0;
  modifiers = 0;
  FREE(cregexp);
  FREE(regexp);
  replace = NONE;
  FREE(replacement);
  Free();
  FreeCompiled();
  csource = REGEXP_UNDEFINED;
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
        // parse field conditional
        char *cond = strchr(field, '?');
        if (cond) {
           *cond = 0;
           cond += 1;
           char *m = strrchr(cond, '/');
           if (m) {
              *m = 0;
              cmodifiers = ParseModifiers(m + 1);
              }
           char *cs = strchr(cond, '~');
           if (cs) {
              *cs = 0;
              cregexp = strdup(cs + 3);
              }
           if (strcmp(cond, "title") == 0)
              csource = REGEXP_TITLE;
           if (strcmp(cond, "shorttext") == 0)
              csource = REGEXP_SHORTTEXT;
           if (strcmp(cond, "description") == 0)
              csource = REGEXP_DESCRIPTION;
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
     int tmpstringlen = strlen(*tmpstring);
     PCRE2_SIZE *ovector;
     int rc = 0;
     cString ctmpstring;
     switch (csource) {
       case REGEXP_TITLE:
         ctmpstring = Event->Title();
         break;
       case REGEXP_SHORTTEXT:
         ctmpstring = Event->ShortText();
         break;
       case REGEXP_DESCRIPTION:
         ctmpstring = Event->Description();
         break;
       default:
         ctmpstring = "";
         break;
       }
     pcre2_match_data *match_data;
     // is cre set and are there any matches?
     if (cre) {
        match_data = pcre2_match_data_create_from_pattern(cre, NULL);
        rc = pcre2_match(cre, (PCRE2_SPTR8)*ctmpstring, strlen(*ctmpstring), 0, 0, match_data, NULL);
        pcre2_match_data_free(match_data);
        // pcre2_match returns (number of matches + 1) if there are matches
        if (rc < 2)
           return false;
       }

     if (replace != NONE) {// find and replace
        int last_match_end = -1;
        int options = 0;
        int start_offset = 0;
        cString resultstring = "";
        match_data = pcre2_match_data_create_from_pattern(re, NULL);
        // loop through matches
        while ((rc = pcre2_match(re, (PCRE2_SPTR8)*tmpstring, tmpstringlen, start_offset, options, match_data, NULL)) > 0) {
              ovector = pcre2_get_ovector_pointer(match_data);
              last_match_end = ovector[1];
              resultstring = cString::sprintf("%s%.*s%s", *resultstring, (int)(ovector[0] - start_offset), &tmpstring[start_offset], replacement);
              options = 0;
              if (ovector[0] == ovector[1]) {
                 if (ovector[0] == (PCRE2_SIZE)tmpstringlen)
                    break;
                 options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
                 }
              if (replace == FIRST) // only first match wanted
                 break;
              start_offset = ovector[1];
              }
        pcre2_match_data_free(match_data);
        // replace EPG field if regexp matched
        if (last_match_end > 0 && (last_match_end <= tmpstringlen)) {
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
        PCRE2_UCHAR8 *capturestring;
        PCRE2_SIZE capturelen;
        match_data = pcre2_match_data_create_from_pattern(re, NULL);
        rc = pcre2_match(re, (PCRE2_SPTR8)*tmpstring, tmpstringlen, 0, 0, match_data, NULL);
        pcre2_match_data_free(match_data);
        if (rc == 0) {
           error("maximum number of captured substrings has been exceeded\n");
           }
        else if (rc > 1) {
           int i = 0;
           // loop through all possible backreferences
           // TODO allow duplicate backreference names?
           match_data = pcre2_match_data_create_from_pattern(re, NULL);
           pcre2_match(re, (PCRE2_SPTR8)*tmpstring, tmpstringlen, 0, 0, match_data, NULL);
           while (i < 10) {
             if (pcre2_substring_get_byname(match_data, (PCRE2_SPTR8)strBackrefs[i], &capturestring, &capturelen) != PCRE2_ERROR_NOSUBSTRING) {
                switch (i) {
                  case ATITLE:
                  case PTITLE:
                    if (Event->Title()) {
                       if (i == ATITLE)
                          Event->SetTitle(*cString::sprintf("%s %s", Event->Title(), capturestring));
                       else
                          Event->SetTitle(*cString::sprintf("%s %s", capturestring, Event->Title()));
                       break;
                       }
                  case TITLE:
                    Event->SetTitle((const char *)capturestring);
                    break;
                  case ASHORTTEXT:
                  case PSHORTTEXT:
                    if (Event->ShortText()) {
                       if (i == ASHORTTEXT)
                          Event->SetShortText(*cString::sprintf("%s %s", Event->ShortText(), capturestring));
                       else
                          Event->SetShortText(*cString::sprintf("%s %s", capturestring, Event->ShortText()));
                       break;
                       }
                  case SHORTTEXT:
                    Event->SetShortText((const char *)capturestring);
                    break;
                  case ADESCRIPTION:
                  case PDESCRIPTION:
                    if (Event->Description()) {
                       if (i == ADESCRIPTION)
                          Event->SetDescription(*cString::sprintf("%s %s", Event->Description(), capturestring));
                       else
                          Event->SetDescription(*cString::sprintf("%s %s", capturestring, Event->Description()));
                       break;
                       }
                  case DESCRIPTION:
                    Event->SetDescription((const char *)capturestring);
                    break;
                  case RATING:
                    Event->SetParentalRating(atoi((const char *)capturestring));
                    break;
                  default:
                    break;
                  }
                pcre2_substring_free(capturestring);
                }
              ++i;
              }
           pcre2_match_data_free(match_data);
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
