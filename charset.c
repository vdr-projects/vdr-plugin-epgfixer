/*
 * charset.c: Character set list item
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "charset.h"
#include <unistd.h>

/* Global instance */
cEpgfixerList<cCharSet> EpgfixerCharSets;

cCharSet::cCharSet()
{
  charset = NULL;
}

cCharSet::~cCharSet(void)
{
  free(charset);
}

bool cCharSet::Apply(cEvent *Event)
{
  if (enabled && IsActive(Event->ChannelID())) {
     cCharSetConv conv(charset, cCharSetConv::SystemCharacterTable());
     Event->SetTitle(conv.Convert(Event->Title()));
     Event->SetShortText(conv.Convert(Event->ShortText()));
     Event->SetDescription(conv.Convert(Event->Description()));
     }
  return false;
}

void cCharSet::SetFromString(char *s, bool Enabled)
{
  FREE(charset);
  Free();
  enabled = Enabled;
  if (s[0] == '!')
     string = strdup(s+1);
  else
     string = strdup(s);
  if (s[0] == '!' || s[0] == '#')
     enabled = false;
  char *p = (s[0] == '#') ? NULL : s;
  if (p) {
     char *p = (s[0] == '!') ? s+1 : s;
     char *f = strchr(p, ':');
     if (f) {
        *f = 0;
        charset = strdup(f + 1);
        numchannels = LoadChannelsFromString(p);
        }
     else
        charset = strdup(p);
     }
}

void cCharSet::PrintConfigLineToFile(FILE *f)
{
  if (f)
     fprintf(f, "%s%s\n", enabled ? "" : "!", string);
}
