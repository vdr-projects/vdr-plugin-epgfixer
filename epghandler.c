/*
 * epghandler.c: EpgHandler
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "blacklist.h"
#include "config.h"
#include "epgclone.h"
#include "tools.h"
#include "epghandler.h"

#if VDRVERSNUM >= 20304
bool cEpgfixerEpgHandler::BeginSegmentTransfer(const cChannel *Channel, bool Dummy)
{
  currentChannel = Channel;
  DEBUG_EPGHANDLER("BeginSegmentTransfer() - Channel=%s (%s)",
                   Channel ? *Channel->GetChannelID().ToString() : "NULL",
                   Channel ? Channel->Name() : "NULL");
  return true;
}
#endif

bool cEpgfixerEpgHandler::FixEpgBugs(cEvent *Event)
{
  // Get ChannelID from Event, or use currentChannel as fallback for events without schedule
  tChannelID channelID = Event->ChannelID();
  if (!channelID.Valid() && currentChannel)
     channelID = currentChannel->GetChannelID();

  DEBUG_EPGHANDLER("FixEpgBugs() START - Event='%s', ChannelID='%s'",
                   Event->Title(), *channelID.ToString());

  FixOriginalEpgBugs(Event);
  FixCharSets(Event, channelID);
  StripHTML(Event);
  FixBugs(Event, channelID);

  DEBUG_EPGHANDLER("FixEpgBugs() END - Event='%s'", Event->Title());
  return false;
}

bool cEpgfixerEpgHandler::HandleEvent(cEvent *Event)
{
  DEBUG_EPGHANDLER("HandleEvent() - Event='%s', ChannelID='%s'",
                   Event->Title(), *Event->ChannelID().ToString());
  return EpgfixerEpgClones.Apply(Event);
}

bool cEpgfixerEpgHandler::IgnoreChannel(const cChannel *Channel)
{
  bool ignored = EpgfixerBlacklists.Apply((cChannel *)Channel);
  DEBUG_EPGHANDLER("IgnoreChannel() - Channel=%s (%s), Ignored=%d",
                   *Channel->GetChannelID().ToString(), Channel->Name(), ignored);
  return ignored;
}
