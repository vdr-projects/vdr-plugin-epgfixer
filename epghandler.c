/*
 * epghandler.c: EpgHandler
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "blacklist.h"
#include "epgclone.h"
#include "tools.h"
#include "epghandler.h"

#if VDRVERSNUM >= 20304
bool cEpgfixerEpgHandler::BeginSegmentTransfer(const cChannel *Channel, bool Dummy)
{
  currentChannel = Channel;
  return true;
}
#endif

bool cEpgfixerEpgHandler::FixEpgBugs(cEvent *Event)
{
  // Get ChannelID from Event, or use currentChannel as fallback for events without schedule
  tChannelID channelID = Event->ChannelID();
  if (!channelID.Valid() && currentChannel)
     channelID = currentChannel->GetChannelID();

  FixOriginalEpgBugs(Event);
  FixCharSets(Event, channelID);
  StripHTML(Event);
  FixBugs(Event, channelID);
  return false;
}

bool cEpgfixerEpgHandler::HandleEvent(cEvent *Event)
{
  // Get ChannelID from Event, or use currentChannel as fallback for events without schedule
  tChannelID channelID = Event->ChannelID();
  if (!channelID.Valid() && currentChannel)
     channelID = currentChannel->GetChannelID();

  return EpgfixerEpgClones.Apply(Event, channelID);
}

bool cEpgfixerEpgHandler::IgnoreChannel(const cChannel *Channel)
{
  return EpgfixerBlacklists.Apply((cChannel *)Channel);
}
