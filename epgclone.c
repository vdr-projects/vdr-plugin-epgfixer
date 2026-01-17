/*
 * epgclone.c: EpgClone list item
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "epgclone.h"

/* Global instance */
cEpgfixerList<cEpgClone, cEvent> EpgfixerEpgClones;

cEpgClone::cEpgClone()
{
  dest_num = 0;
  dest_str = NULL;
}

cEpgClone::~cEpgClone()
{
  free(dest_str);
}

void cEpgClone::CloneEvent(cEvent *Source, cEvent *Dest) {
  DEBUG_EPGCLONE("CloneEvent() - Source Event='%s', EventID=%u",
                 Source->Title(), Source->EventID());
  Dest->SetEventID(Source->EventID());
  Dest->SetTableID(Source->TableID());
  Dest->SetVersion(Source->Version());
  Dest->SetRunningStatus(Source->RunningStatus());
  Dest->SetTitle(Source->Title());
  Dest->SetShortText(Source->ShortText());
  Dest->SetDescription(Source->Description());
  cComponents *components = new cComponents();
  if (Source->Components()) {
     for (int i = 0; i < Source->Components()->NumComponents(); ++i)
         components->SetComponent(i, Source->Components()->Component(i)->ToString());
     }
  Dest->SetComponents(components);
  uchar contents[MaxEventContents];
  for (int i = 0; i < MaxEventContents; ++i)
      contents[i] = Source->Contents(i);
  Dest->SetContents(contents);
  Dest->SetParentalRating(Source->ParentalRating());
  Dest->SetStartTime(Source->StartTime());
  Dest->SetDuration(Source->Duration());
  Dest->SetVps(Source->Vps());
  if (Source->Seen())
     Dest->SetSeen();
  tChannelID channelID;
  if (dest_num) {
#if VDRVERSNUM >= 20301
     LOCK_CHANNELS_READ;
     const cChannel *dest_chan = Channels->GetByNumber(dest_num);
     if (dest_chan)
        channelID = Channels->GetByNumber(dest_num)->GetChannelID();
#else
     cChannel *dest_chan = Channels.GetByNumber(dest_num);
     if (dest_chan)
        channelID = Channels.GetByNumber(dest_num)->GetChannelID();
#endif
     else
        channelID = tChannelID::InvalidID;
     DEBUG_EPGCLONE("CloneEvent() - Destination channel number=%d, ChannelID='%s'",
                    dest_num, *channelID.ToString());
     }
  else {
     if (dest_str)
        channelID = tChannelID::FromString(dest_str);
     else
        channelID = tChannelID::InvalidID;
     DEBUG_EPGCLONE("CloneEvent() - Destination channel string='%s', ChannelID='%s'",
                    dest_str ? dest_str : "NULL", *channelID.ToString());
     }
  if (channelID == tChannelID::InvalidID) {
     enabled = false;
     delete Dest;
     error("Destination channel %s not found for cloning, disabling cloning!", (dest_num ? *itoa(dest_num) : (dest_str ? dest_str : "NULL")));
     }
  else {
     DEBUG_EPGCLONE("CloneEvent() - Adding cloned event to channel '%s'", *channelID.ToString());
     AddEvent(Dest, channelID);
     }
}

bool cEpgClone::Apply(cEvent *Event, tChannelID ChannelID)
{
  // Use provided ChannelID if Event->ChannelID() is invalid
  tChannelID eventChannelID = Event ? Event->ChannelID() : tChannelID();
  if (!eventChannelID.Valid() && ChannelID.Valid()) {
     eventChannelID = ChannelID;
     }

  if (Event && enabled && IsActive(eventChannelID)) {
     DEBUG_EPGCLONE("Apply() - Cloning Event='%s' from Channel='%s'",
                    Event->Title(), *eventChannelID.ToString());
     cEvent *event = new cEvent(Event->EventID());
     CloneEvent(Event, event);
     return true;
     }
  return false;
}

void cEpgClone::SetFromString(char *s, bool Enabled, int LineNumber)
{
  dest_num = 0;
  FREE(dest_str);
  Free();
  cListItem::SetFromString(s, Enabled, LineNumber);
  if (enabled) {
     char *p = (s[0] == '!') ? s + 1 : s;
     char *f = strchr(p, '=');
     if (f) {
        *f = 0;
        if (atoi(f + 1))
           dest_num = atoi(f + 1);
        else
           dest_str = strdup(f + 1);
        numchannels = LoadChannelsFromString(p);
        }
     }
}
