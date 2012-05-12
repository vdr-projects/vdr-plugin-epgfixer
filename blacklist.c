/*
 * blacklist.c: Blacklist list item
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "blacklist.h"

/* Global instance */
cEpgfixerList<cBlacklist, cChannel> EpgfixerBlacklists;


bool cBlacklist::Apply(cChannel *Channel)
{
  if (enabled && IsActive(Channel->GetChannelID()))
     return true;
  return false;
}

void cBlacklist::SetFromString(char *s, bool Enabled)
{
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
     numchannels = LoadChannelsFromString(p);
     }
}
