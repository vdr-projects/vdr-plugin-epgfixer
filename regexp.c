/*
 * regexp.c: Regular expression list item
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "regexp.h"
#include <unistd.h>

/* Global instance */
cEpgfixerList<cRegexp> EpgfixerRegexps;

const char *strSources[] = { "title","shorttext","description","undefined" };

typedef enum { ATITLE,PTITLE,TITLE,ASHORTTEXT,PSHORTTEXT,SHORTTEXT,ADESCRIPTION,PDESCRIPTION,DESCRIPTION,RATING } backrefs;
const char *strBackrefs[] = { "atitle","ptitle","title","ashorttext","pshorttext","shorttext","adescription","pdescription","description","rating" };

cRegexp::cRegexp()
{
  regexp = NULL;
  source = REGEXP_UNDEFINED;
  re = NULL;
  sd = NULL;
}

cRegexp::~cRegexp(void)
{
  Free();
  free(regexp);
  FreeCompiled();
}

void cRegexp::Compile()
{
  FreeCompiled();
  const char *error;
  int erroffset;
  re = pcre_compile(regexp, 0, &error, &erroffset, NULL);
  if (error) {
     error("PCRE compile error: %s at offset %i", error, erroffset);
     enabled = false;
     }
  else {
     sd = pcre_study(re, 0, (const char **)&error);
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

void cRegexp::SetFromString(char *s, bool Enabled)
{
  FREE(regexp);
  Free();
  FreeCompiled();
  enabled = Enabled;
  bool compile = true;
  if (s[0] == '#') {
     enabled = false;
     source = REGEXP_UNDEFINED;
     string = strdup(s);
     return;
     }
  if (s[0] == '!') {
     enabled = compile = false;
     string = strdup(s+1);
     }
  else
     string = strdup(s);
  char *p = strchr(s, '=');
  if (p) {
     *p = 0;
     regexp = strdup(p + 1);
     char *chanfield = (s[0] == '!') ? s+1 : s;
     char *field = chanfield;
     char *f = strchr(chanfield, ':');
     if (f) {
        *f = 0;
        field = f+1;
        numchannels = LoadChannelsFromString(chanfield);
        }
     if (strcmp(field, "title") == 0)
        source = REGEXP_TITLE;
     if (strcmp(field, "shorttext") == 0)
        source = REGEXP_SHORTTEXT;
     if (strcmp(field, "description") == 0)
        source = REGEXP_DESCRIPTION;
     if (compile)
        Compile();
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
     const char *string;
     int ovector[20];
     int rc;
     rc = pcre_exec(re, sd, *tmpstring, strlen(*tmpstring), 0, 0, ovector, 20);
     if (rc > 0) {
        int i = 0;
        while (i < 10) {
          if (pcre_get_named_substring(re, tmpstring, ovector, rc, strBackrefs[i], &string) != PCRE_ERROR_NOSUBSTRING) {
             switch (i) {
               case TITLE:
                 Event->SetTitle(string);
                 break;
               case ATITLE:
                 Event->SetTitle(*cString::sprintf("%s %s", Event->Title(), string));
                 break;
               case PTITLE:
                 Event->SetTitle(*cString::sprintf("%s %s", string, Event->Title()));
                 break;
               case SHORTTEXT:
                 Event->SetShortText(string);
                 break;
               case ASHORTTEXT:
                 Event->SetShortText(*cString::sprintf("%s %s", Event->ShortText(), string));
                 break;
               case PSHORTTEXT:
                 Event->SetShortText(*cString::sprintf("%s %s", string, Event->ShortText()));
                 break;
               case DESCRIPTION:
                 Event->SetDescription(string);
                 break;
               case ADESCRIPTION:
                 Event->SetDescription(*cString::sprintf("%s %s", Event->Description(), string));
                 break;
               case PDESCRIPTION:
                 Event->SetDescription(*cString::sprintf("%s %s", string, Event->Description()));
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
  return false;
}

void cRegexp::PrintConfigLineToFile(FILE *f)
{
  if (f) {
     if (source == REGEXP_UNDEFINED)
        fprintf(f, "%s\n", string);
     else
        fprintf(f, "%s%s\n", enabled ? "" : "!", string);
     }
}

void cRegexp::ToggleEnabled(void)
{
  if (source != REGEXP_UNDEFINED)
     enabled = !enabled;
}
