/*
 * epghandler.c: EpgHandler
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "epghandler.h"
#include "config.h"
#include <vdr/tools.h>
#include <string.h>

#ifdef HAVE_PCREPOSIX
#include <pcre.h>
#endif

typedef enum { ATITLE,PTITLE,TITLE,ASHORTTEXT,PSHORTTEXT,SHORTTEXT,ADESCRIPTION,PDESCRIPTION,DESCRIPTION,RATING } backrefs;
const char *strBackrefs[] = { "atitle","ptitle","title","ashorttext","pshorttext","shorttext","adescription","pdescription","description","rating" };

void cEpgfixerEpgHandler::FixOriginalEpgBugs(cEvent *event)
{
  if (isempty(event->Title())) {
     // we don't want any "(null)" titles
     event->SetTitle("No title");
     }

  // Some TV stations apparently have their own idea about how to fill in the
  // EPG data. Let's fix their bugs as good as we can:

  // Some channels put the ShortText in quotes and use either the ShortText
  // or the Description field, depending on how long the string is:
  //
  // Title
  // "ShortText". Description
  //
  if (EpgfixerSetup.quotedshorttext && (event->ShortText() == NULL) != (event->Description() == NULL)) {
     char *p = event->ShortText() ? strdup(event->ShortText()) : strdup(event->Description());
     if (*p == '"') {
        const char *delim = "\".";
        char *e = strstr(p + 1, delim);
        if (e) {
           *e = 0;
           char *s = strdup(p + 1);
           char *d = strdup(e + strlen(delim));
           event->SetShortText(s);
           event->SetDescription(d);
           }
        }
        free(p);
     }

  // Some channels put the Description into the ShortText (preceded
  // by a blank) if there is no actual ShortText and the Description
  // is short enough:
  //
  // Title
  //  Description
  //
  if (EpgfixerSetup.blankbeforedescription && event->ShortText() && !event->Description()) {
     if (*event->ShortText() == ' ') {
        char *shortText = strdup(event->ShortText());
        memmove(shortText, shortText + 1, strlen(shortText));
        event->SetDescription(shortText);
        event->SetShortText(NULL);
        free(shortText);
        }
     }

  // Sometimes they repeat the Title in the ShortText:
  //
  // Title
  // Title
  //
  if (EpgfixerSetup.repeatedtitle && event->ShortText() && strcmp(event->Title(), event->ShortText()) == 0) {
     event->SetShortText(NULL);
     }

  // Some channels put the ShortText between double quotes, which is nothing
  // but annoying (some even put a '.' after the closing '"'):
  //
  // Title
  // "ShortText"[.]
  //
  if (EpgfixerSetup.doublequotedshorttext && event->ShortText() && *event->ShortText() == '"') {
     char *shortText = strdup(event->ShortText());
     int l = strlen(shortText);
     if (l > 2 && (shortText[l - 1] == '"' || (shortText[l - 1] == '.' && shortText[l - 2] == '"'))) {
        memmove(shortText, shortText + 1, l);
        char *p = strrchr(shortText, '"');
        if (p)
           *p = 0;
        }
     event->SetShortText(shortText);
     free(shortText);
     }

  // Some channels apparently try to do some formatting in the texts,
  // which is a bad idea because they have no way of knowing the width
  // of the window that will actually display the text.
  // Remove excess whitespace:
  if (EpgfixerSetup.removeformatting) {
     char *temp;
     temp = strdup(event->Title());
     event->SetTitle(compactspace(temp));
     free(temp);
     if (event->ShortText()) {
        temp = strdup(event->ShortText());
        event->SetShortText(compactspace(temp));
        free(temp);
        }
     if (event->Description()) {
        temp = strdup(event->Description());
        event->SetDescription(compactspace(temp));
        free(temp);
        }
     }

#define MAX_USEFUL_EPISODE_LENGTH 40
  // Some channels put a whole lot of information in the ShortText and leave
  // the Description totally empty. So if the ShortText length exceeds
  // MAX_USEFUL_EPISODE_LENGTH, let's put this into the Description
  // instead:
  if (EpgfixerSetup.longshorttext && !isempty(event->ShortText()) && isempty(event->Description())) {
     if (strlen(event->ShortText()) > MAX_USEFUL_EPISODE_LENGTH) {
        event->SetDescription(event->ShortText());
        event->SetShortText(NULL);
        }
     }

  // Some channels put the same information into ShortText and Description.
  // In that case we delete one of them:
  if (EpgfixerSetup.equalshorttextanddescription && event->ShortText() && event->Description() && strcmp(event->ShortText(), event->Description()) == 0) {
     if (strlen(event->ShortText()) > MAX_USEFUL_EPISODE_LENGTH) {
        event->SetShortText(NULL);
        }
     else {
        event->SetDescription(NULL);

        }
     }

  // Some channels use the ` ("backtick") character, where a ' (single quote)
  // would be normally used. Actually, "backticks" in normal text don't make
  // much sense, so let's replace them:
  if (EpgfixerSetup.nobackticks) {
     char *temp;
     temp = strdup(event->Title());
     event->SetTitle(strreplace(temp, '`', '\''));
     free(temp);
     if (event->ShortText()) {
        temp = strdup(event->ShortText());
        event->SetShortText(strreplace(temp, '`', '\''));
        free(temp);
        }
     if (event->Description()) {
        temp = strdup(event->Description());
        event->SetDescription(strreplace(temp, '`', '\''));
        free(temp);
        }
     }

  // The stream components have a "description" field which some channels
  // apparently have no idea of how to set correctly:
  const cComponents *components = event->Components();
  if (EpgfixerSetup.components && components) {
     for (int i = 0; i < components->NumComponents(); i++) {
         tComponent *p = components->Component(i);
         switch (p->stream) {
           case 0x01: { // video
                if (p->description) {
                   if (strcasecmp(p->description, "Video") == 0 ||
                        strcasecmp(p->description, "Bildformat") == 0) {
                      // Yes, we know it's video - that's what the 'stream' code
                      // is for! But _which_ video is it?
                      free(p->description);
                      p->description = NULL;
                      }
                   }
                if (!p->description) {
                   switch (p->type) {
                     case 0x01:
                     case 0x05: p->description = strdup("4:3"); break;
                     case 0x02:
                     case 0x03:
                     case 0x06:
                     case 0x07: p->description = strdup("16:9"); break;
                     case 0x04:
                     case 0x08: p->description = strdup(">16:9"); break;
                     case 0x09:
                     case 0x0D: p->description = strdup("HD 4:3"); break;
                     case 0x0A:
                     case 0x0B:
                     case 0x0E:
                     case 0x0F: p->description = strdup("HD 16:9"); break;
                     case 0x0C:
                     case 0x10: p->description = strdup("HD >16:9"); break;
                     default: ;
                     }
                   }
                }
                break;
           case 0x02: { // audio
                if (p->description) {
                   if (strcasecmp(p->description, "Audio") == 0) {
                      // Yes, we know it's audio - that's what the 'stream' code
                      // is for! But _which_ audio is it?
                      free(p->description);
                      p->description = NULL;
                      }
                   }
                if (!p->description) {
                   switch (p->type) {
                     case 0x05: p->description = strdup("Dolby Digital"); break;
                     default: ; // all others will just display the language
                     }
                   }
                }
                break;
           default: ;
           }
         }
     }

  // VDR can't usefully handle newline characters in the title, shortText or component description of EPG
  // data, so let's always convert them to blanks (independent of the setting of EPGBugfixLevel):
  char *temp = strdup(event->Title());
  event->SetTitle(strreplace(temp, '\n', ' '));
  free(temp);
  if (event->ShortText()) {
     temp = strdup(event->ShortText());
     event->SetShortText(strreplace(temp, '\n', ' '));
     free(temp);
     }
  if (components) {
     for (int i = 0; i < components->NumComponents(); i++) {
         tComponent *p = components->Component(i);
         if (p->description)
            strreplace(p->description, '\n', ' ');
         }
     }
}

bool cEpgfixerEpgHandler::ApplyRegexp(cRegexp *regexp, cEvent *Event, const char *input)
{
  bool active = true;
  if (regexp->NumChannels() > 0) {
     bool found = false;
     int i = 0;
     while (i < regexp->NumChannels()) {
           if (Channels.GetByChannelID(Event->ChannelID())->Number() == regexp->GetChannel(i))
              found = true;
           i++;
           }
     if (!found) {
        active = false;
        }
     }
  if (active && regexp->Enabled() && regexp->GetRe()) {
     const char *string;
     int ovector[20];
     int rc;
     rc = pcre_exec(regexp->GetRe(), regexp->GetSd(), input, strlen(input), 0, 0, ovector, 20);
     if (rc > 0) {
        int i = 0;
        while (i < 10) {
          if (pcre_get_named_substring(regexp->GetRe(), input, ovector, rc, strBackrefs[i], &string) != PCRE_ERROR_NOSUBSTRING) {
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
        return true;
        }
     }
  return false;
}

bool cEpgfixerEpgHandler::FixBugs(cEvent *Event)
{
  int res = false;
  char *tmpstring;
  cRegexp *regex = (cRegexp *)EpgfixerRegexps.regexps->First();
  while (regex) {
        if (regex->Enabled()) {
           switch (regex->GetSource()) {
             case REGEXP_TITLE:
               tmpstring = strdup(Event->Title());
               break;
             case REGEXP_SHORTTEXT:
               tmpstring = strdup(Event->ShortText());
               break;
             case REGEXP_DESCRIPTION:
               tmpstring = strdup(Event->Description());
               break;
             default:
               tmpstring = strdup("");
               break;
             }
           int ret = ApplyRegexp(regex, Event, tmpstring);
           if (ret && !res)
              res = true;
           free(tmpstring);
           }
        regex = (cRegexp *)regex->Next();
        }
  return res;
}

bool cEpgfixerEpgHandler::FixEpgBugs(cEvent *Event)
{
  FixOriginalEpgBugs(Event);
  FixBugs(Event);
  return false;
}
