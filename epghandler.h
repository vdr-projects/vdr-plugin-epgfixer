/*
 * epghandler.h: EpgHandler
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __EPGFIXER_EPGHANDLER_H
#define __EPGFIXER_EPGHANDLER_H

#include <vdr/epg.h>
#include "regexp.h"

class cEpgfixerEpgHandler : public cEpgHandler
{
private:
  bool ApplyRegexp(cRegexp *regexp, cEvent *Event, const char *input);
  void FixOriginalEpgBugs(cEvent *event);
  bool FixBugs(cEvent *Event);
public:
  cEpgfixerEpgHandler(void) {};
  virtual bool FixEpgBugs(cEvent *Event);
};

#endif //__EPGFIXER_EPGHANDLER_H
