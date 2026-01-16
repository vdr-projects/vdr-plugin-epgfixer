/*
 * blacklist.c: Blacklist list item
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "blacklist.h"
#include "config.h"

/* Global instance */
cEpgfixerList<cBlacklist, cChannel> EpgfixerBlacklists;

bool cBlacklist::Apply(cChannel *Channel, tChannelID ChannelID)
{
  if (enabled && IsActive(Channel->GetChannelID())) {
     DEBUG_BLACKLIST("Apply() - Channel='%s' (%s) BLACKLISTED",
                     *Channel->GetChannelID().ToString(), Channel->Name());
     return true;
     }
  DEBUG_BLACKLIST("Apply() - Channel='%s' (%s) allowed",
                  *Channel->GetChannelID().ToString(), Channel->Name());
  return false;
}

void cBlacklist::SetFromString(char *s, bool Enabled, int LineNumber)
{
  Free();
  cListItem::SetFromString(s, Enabled, LineNumber);
  if (enabled) {
     char *p = (s[0] == '!') ? s + 1 : s;
     numchannels = LoadChannelsFromString(p);
     }
}
