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

bool cCharSet::ConvertCharSet(cEvent *Event)
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
  if (active && enabled) {
     cCharSetConv conv(charset, cCharSetConv::SystemCharacterTable());
     Event->SetTitle(conv.Convert(Event->Title()));
     Event->SetShortText(conv.Convert(Event->ShortText()));
     Event->SetDescription(conv.Convert(Event->Description()));
     }
  return false;
}

void cCharSet::SetFromString(char *s, bool Enabled)
{
  Free();
  enabled = Enabled;
  if (s[0] == '!')
     string = strdup(s+1);
  else
     string = strdup(s);
  if (s[0] == '!' || s[0] == '#')
     enabled = false;
  char *p = (s[0] == '#') ? s : strchr(s, '=');
  if (p) {
     if (p[0] != '#') {
        *p = 0;
        charset = strdup(p + 1);
        char *chans = (s[0] == '!') ? s+1 : s;
        numchannels = loadChannelsFromString(chans, &channels_num, &channels_str);
        }
     }
}

void cCharSet::PrintConfigLineToFile(FILE *f)
{
  if (f)
     fprintf(f, "%s%s\n", enabled ? "" : "!", string);
}
