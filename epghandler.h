/*
 * epghandler.h: EpgHandler
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __EPGFIXER_EPGHANDLER_H
#define __EPGFIXER_EPGHANDLER_H

#include <vdr/epg.h>

class cEpgfixerEpgHandler : public cEpgHandler
{
private:
  void FixOriginalEpgBugs(cEvent *event);
  bool FixCharSets(cEvent *Event);
  bool FixBugs(cEvent *Event);
  void StripHTML(cEvent *Event);
public:
  cEpgfixerEpgHandler(void) {};
  virtual bool HandleEvent(cEvent *Event);
  virtual bool IgnoreChannel(const cChannel *Channel);
  virtual bool FixEpgBugs(cEvent *Event);
};

#endif //__EPGFIXER_EPGHANDLER_H
