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
  regexp = NULL;
  FreeCompiled();
}

void cRegexp::Compile()
{
  FreeCompiled();
  const char *error;
  int erroffset;
  re = pcre_compile(regexp, 0, &error, &erroffset, NULL);
  if (error) {
     esyslog("PCRE compile error: %s at offset %i", error, erroffset);
     enabled = false;
     }
  else {
     sd = pcre_study(re, 0, (const char **)&error);
     if (error)
        esyslog("PCRE study error: %s", error);
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
  Free();
  enabled = Enabled;
  if (s[0] == '!')
     string = strdup(s+1);
  else
     string = strdup(s);
  bool compile = true;
  if (s[0] == '!' || s[0] == '#') {
     enabled = false;
     compile = false;
     }
  char *p = (s[0] == '#') ? s : strchr(s, '=');
  if (p) {
     if (p[0] == '#')
        source = REGEXP_UNDEFINED;
     else {
        *p = 0;
        regexp = strdup(p + 1);
        char *chanfield = (s[0] == '!') ? s+1 : s;
        char *field = chanfield;
        char *f = strchr(chanfield, ':');
        if (f) {
           *f = 0;
           field = f+1;
           numchannels = loadChannelsFromString(chanfield, &channels_num, &channels_str);
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
}

bool cRegexp::Apply(cEvent *Event)
{
  bool active = true;
  if (numchannels > 0) {
     bool found = false;
     int i = 0;
     while (i < numchannels && !found) {
           if (Channels.GetByChannelID(Event->ChannelID())->Number() == GetChannelNum(i))
              found = true;
           if (GetChannelID(i) && strcmp(*(Event->ChannelID().ToString()), GetChannelID(i)) == 0)
              found = true;
           i++;
           }
     if (!found)
        active = false;
     }
  if (active && enabled && re) {
     char *tmpstring;
     switch (source) {
            case REGEXP_TITLE:
              tmpstring = strdup(Event->Title());
              break;
            case REGEXP_SHORTTEXT:
              if (Event->ShortText())
                 tmpstring = strdup(Event->ShortText());
              else
                 tmpstring = strdup("");
              break;
            case REGEXP_DESCRIPTION:
              if (Event->Description())
                 tmpstring = strdup(Event->Description());
              else
                 tmpstring = strdup("");
              break;
            default:
              tmpstring = strdup("");
              break;
            }
     const char *string;
     int ovector[20];
     int rc;
     rc = pcre_exec(re, sd, tmpstring, strlen(tmpstring), 0, 0, ovector, 20);
     if (rc > 0) {
        int i = 0;
        while (i < 10) {
          if (pcre_get_named_substring(re, tmpstring, ovector, rc, strBackrefs[i], &string) != PCRE_ERROR_NOSUBSTRING) {
             char *tempstring = 0;
             switch (i) {
               case TITLE:
                 Event->SetTitle(string);
                 break;
               case ATITLE:
                 tempstring = (char *)malloc(strlen(Event->Title())+strlen(string)+1);
                 strcpy(tempstring, Event->Title());
                 strcat(tempstring, string);
                 Event->SetTitle(tempstring);
                 break;
               case PTITLE:
                 tempstring = (char *)malloc(strlen(Event->Title())+strlen(string)+1);
                 strcpy(tempstring, string);
                 strcat(tempstring, Event->Title());
                 Event->SetTitle(tempstring);
                 break;
               case SHORTTEXT:
                 Event->SetShortText(string);
                 break;
               case ASHORTTEXT:
                 tempstring = (char *)malloc(strlen(Event->ShortText())+strlen(string)+1);
                 strcpy(tempstring, Event->ShortText());
                 strcat(tempstring, string);
                 Event->SetShortText(tempstring);
                 break;
               case PSHORTTEXT:
                 tempstring = (char *)malloc(strlen(Event->ShortText())+strlen(string)+1);
                 strcpy(tempstring, string);
                 strcat(tempstring, Event->ShortText());
                 Event->SetShortText(tempstring);
                 break;
               case DESCRIPTION:
                 Event->SetDescription(string);
                 break;
               case ADESCRIPTION:
                 tempstring = (char *)malloc(strlen(Event->Description())+strlen(string)+1);
                 strcpy(tempstring, Event->Description());
                 strcat(tempstring, string);
                 Event->SetDescription(tempstring);
                 break;
               case PDESCRIPTION:
                 tempstring = (char *)malloc(strlen(Event->Description())+strlen(string)+1);
                 strcpy(tempstring, string);
                 strcat(tempstring, Event->Description());
                 Event->SetDescription(tempstring);
                 break;
               case RATING:
                 Event->SetParentalRating(atoi(string));
                 break;
               default:
                 break;
               }
             pcre_free_substring(string);
             free(tempstring);
             }
           i++;
           }
        free(tmpstring);
        return true;
        }
     free(tmpstring);
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
     enabled = enabled ? 0 : 1;
}
