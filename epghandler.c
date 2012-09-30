/*
 * epghandler.c: EpgHandler
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "epghandler.h"
#include "blacklist.h"
#include "charset.h"
#include "config.h"
#include "epgclone.h"
#include "regexp.h"
#include <vdr/tools.h>
#include <string.h>

//
// Original VDR bug fixes adapted from epg.c of VDR
// by Klaus Schmidinger
//

static void StripControlCharacters(char *s)
{
  if (s) {
     int len = strlen(s);
     while (len > 0) {
           int l = Utf8CharLen(s);
           uchar *p = (uchar *)s;
           if (l == 2 && *p == 0xC2) // UTF-8 sequence
              p++;
           if (*p == 0x86 || *p == 0x87) {
              memmove(s, p + 1, len - l + 1); // we also copy the terminating 0!
              len -= l;
              l = 0;
              }
           s += l;
           len -= l;
           }
     }
}

void cEpgfixerEpgHandler::FixOriginalEpgBugs(cEvent *event)
{
  // Copy event title, shorttext and description to temporary variables
  // we don't want any "(null)" titles
  char *title = event->Title() ? strdup(event->Title()) : strdup("No title");
  char *shortText = event->ShortText() ? strdup(event->ShortText()) : NULL;
  char *description = event->Description() ? strdup(event->Description()) : NULL;

  // Some TV stations apparently have their own idea about how to fill in the
  // EPG data. Let's fix their bugs as good as we can:

  // Some channels put the ShortText in quotes and use either the ShortText
  // or the Description field, depending on how long the string is:
  //
  // Title
  // "ShortText". Description
  //
  if (EpgfixerSetup.quotedshorttext && (shortText == NULL) != (description == NULL)) {
     char *p = shortText ? shortText : description;
     if (*p == '"') {
        const char *delim = "\".";
        char *e = strstr(p + 1, delim);
        if (e) {
           *e = 0;
           char *s = strdup(p + 1);
           char *d = strdup(e + strlen(delim));
           free(shortText);
           free(description);
           shortText = s;
           description = d;
           }
        }
     }

  // Some channels put the Description into the ShortText (preceded
  // by a blank) if there is no actual ShortText and the Description
  // is short enough:
  //
  // Title
  //  Description
  //
  if (EpgfixerSetup.blankbeforedescription && shortText && !description) {
     if (*shortText == ' ') {
        memmove(shortText, shortText + 1, strlen(shortText));
        description = shortText;
        shortText = NULL;
        }
     }

  // Sometimes they repeat the Title in the ShortText:
  //
  // Title
  // Title
  //
  if (EpgfixerSetup.repeatedtitle && shortText && strcmp(title, shortText) == 0) {
     free(shortText);
     shortText = NULL;
     }

  // Some channels put the ShortText between double quotes, which is nothing
  // but annoying (some even put a '.' after the closing '"'):
  //
  // Title
  // "ShortText"[.]
  //
  if (EpgfixerSetup.doublequotedshorttext && shortText && *shortText == '"') {
     int l = strlen(shortText);
     if (l > 2 && (shortText[l - 1] == '"' || (shortText[l - 1] == '.' && shortText[l - 2] == '"'))) {
        memmove(shortText, shortText + 1, l);
        char *p = strrchr(shortText, '"');
        if (p)
           *p = 0;
        }
     }

  // Some channels apparently try to do some formatting in the texts,
  // which is a bad idea because they have no way of knowing the width
  // of the window that will actually display the text.
  // Remove excess whitespace:
  if (EpgfixerSetup.removeformatting) {
     title = compactspace(title);
     shortText = compactspace(shortText);
     description = compactspace(description);
     }

#define MAX_USEFUL_EPISODE_LENGTH 40
  // Some channels put a whole lot of information in the ShortText and leave
  // the Description totally empty. So if the ShortText length exceeds
  // MAX_USEFUL_EPISODE_LENGTH, let's put this into the Description
  // instead:
  if (EpgfixerSetup.longshorttext && !isempty(shortText) && isempty(description)) {
     if (strlen(shortText) > MAX_USEFUL_EPISODE_LENGTH) {
        free(description);
        description = shortText;
        shortText = NULL;
        }
     }

  // Some channels put the same information into ShortText and Description.
  // In that case we delete one of them:
  if (EpgfixerSetup.equalshorttextanddescription && shortText && description && strcmp(shortText, description) == 0) {
     if (strlen(shortText) > MAX_USEFUL_EPISODE_LENGTH) {
        free(shortText);
        shortText = NULL;
        }
     else {
        free(description);
        description = NULL;
        }
     }

  // Some channels use the ` ("backtick") character, where a ' (single quote)
  // would be normally used. Actually, "backticks" in normal text don't make
  // much sense, so let's replace them:
  if (EpgfixerSetup.nobackticks) {
     strreplace(title, '`', '\'');
     strreplace(shortText, '`', '\'');
     strreplace(description, '`', '\'');
     }

  // The stream components have a "description" field which some channels
  // apparently have no idea of how to set correctly:
  const cComponents *components = event->Components();
  if (EpgfixerSetup.components && components) {
     for (int i = 0; i < components->NumComponents(); ++i) {
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
  strreplace(title, '\n', ' ');
  strreplace(shortText, '\n', ' ');
  if (components) {
     for (int i = 0; i < components->NumComponents(); ++i) {
         tComponent *p = components->Component(i);
         if (p->description)
            strreplace(p->description, '\n', ' ');
         }
     }
  // Same for control characters:
  StripControlCharacters(title);
  StripControlCharacters(shortText);
  StripControlCharacters(description);
  // Set modified data back to event
  event->SetTitle(title);
  event->SetShortText(shortText);
  event->SetDescription(description);
}

bool cEpgfixerEpgHandler::FixBugs(cEvent *Event)
{
  return EpgfixerRegexps.Apply(Event);
}

bool cEpgfixerEpgHandler::FixCharSets(cEvent *Event)
{
  return EpgfixerCharSets.Apply(Event);
}

void cEpgfixerEpgHandler::StripHTML(cEvent *Event)
{
  if (EpgfixerSetup.striphtml) {
     char *tmpstring = NULL;
     tmpstring = Event->Title() ? strdup(Event->Title()) : NULL;
     Event->SetTitle(striphtml(tmpstring));
     FREE(tmpstring);
     tmpstring = Event->ShortText() ? strdup(Event->ShortText()) : NULL;
     Event->SetShortText(striphtml(tmpstring));
     FREE(tmpstring);
     tmpstring = Event->Description() ? strdup(Event->Description()) : NULL;
     Event->SetDescription(striphtml(tmpstring));
     FREE(tmpstring);
     }
}

bool cEpgfixerEpgHandler::FixEpgBugs(cEvent *Event)
{
  FixOriginalEpgBugs(Event);
  FixCharSets(Event);
  StripHTML(Event);
  FixBugs(Event);
  return false;
}

bool cEpgfixerEpgHandler::HandleEvent(cEvent *Event)
{
  return EpgfixerEpgClones.Apply(Event);
}

bool cEpgfixerEpgHandler::IgnoreChannel(const cChannel *Channel)
{
  return EpgfixerBlacklists.Apply((cChannel *)Channel);
}
