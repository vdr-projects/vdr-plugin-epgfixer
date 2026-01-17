/*
 * charset.c: Character set list item
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <string.h>
#include "charset.h"
#include "config.h"

/* Global instance */
cEpgfixerList<cCharSet, cEvent> EpgfixerCharSets;

cCharSet::cCharSet()
{
  origcharset = NULL;
  realcharset = NULL;
}

cCharSet::~cCharSet(void)
{
  free(origcharset);
  free(realcharset);
}

bool cCharSet::Apply(cEvent *Event, tChannelID ChannelID)
{
  // Use provided ChannelID if Event->ChannelID() is invalid
  tChannelID eventChannelID = Event ? Event->ChannelID() : tChannelID();
  if (!eventChannelID.Valid() && ChannelID.Valid()) {
     eventChannelID = ChannelID;
     }

  if (enabled && IsActive(eventChannelID)) {
     // Save original values for comparison
     const char *orig_title = Event->Title() ? Event->Title() : "";
     const char *orig_shorttext = Event->ShortText() ? Event->ShortText() : "";
     const char *orig_description = Event->Description() ? Event->Description() : "";

     DEBUG_CHARSET("Apply() - Event='%s', Channel='%s', Conversion: %s -> %s",
                   Event->Title(), *eventChannelID.ToString(),
                   origcharset ? origcharset : "iso6937",
                   realcharset ? realcharset : "system");

     cCharSetConv backconv(cCharSetConv::SystemCharacterTable(), origcharset ? origcharset : "iso6937");
     cString title(backconv.Convert(Event->Title()));
     cString shortText(backconv.Convert(Event->ShortText()));
     cString description(backconv.Convert(Event->Description()));
     cCharSetConv conv(realcharset, cCharSetConv::SystemCharacterTable());
     Event->SetTitle(conv.Convert(title));
     Event->SetShortText(conv.Convert(shortText));
     Event->SetDescription(conv.Convert(description));

     // Log only if something changed
     bool changed = false;
     if (strcmp(orig_title, Event->Title() ? Event->Title() : "") != 0) {
        DEBUG_CHARSET("Apply() - Title: '%s' => '%s'", orig_title, Event->Title());
        changed = true;
        }
     if (strcmp(orig_shorttext, Event->ShortText() ? Event->ShortText() : "") != 0) {
        DEBUG_CHARSET("Apply() - ShortText: '%s' => '%s'", orig_shorttext, Event->ShortText());
        changed = true;
        }
     if (strcmp(orig_description, Event->Description() ? Event->Description() : "") != 0) {
        DEBUG_CHARSET("Apply() - Description: '%s' => '%s'", orig_description, Event->Description());
        changed = true;
        }
     if (!changed) {
        DEBUG_CHARSET("Apply() - No changes needed");
        }
     }
  return false;
}

void cCharSet::SetFromString(char *s, bool Enabled, int LineNumber)
{
  FREE(origcharset);
  FREE(realcharset);
  Free();
  cListItem::SetFromString(s, Enabled, LineNumber);
  if (enabled) {
     char *p = (s[0] == '!') ? s + 1 : s;
     char *r = strchr(p, ':');
     if (r) {
        *r = 0;
        numchannels = LoadChannelsFromString(p);
        p = r + 1;
        }
     r = strchr(p, '=');
     if (r) {
        *r = 0;
        origcharset = strdup(p);
        realcharset = strdup(r + 1);
        }
     else
        realcharset = strdup(p);
     }
}
