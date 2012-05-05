/*
 * charset.h: Character set list item
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __EPGFIXER_CHARSET_H_
#define __EPGFIXER_CHARSET_H_

#include "tools.h"
#include <vdr/epg.h>
#include <vdr/tools.h>
#include <stdio.h>

class cCharSet : public cListItem
{
private:
  char *charset;
public:
  cCharSet();
  virtual ~cCharSet();
  virtual bool Apply(cEvent *Event);
  void SetFromString(char *string, bool Enabled);
  virtual void PrintConfigLineToFile(FILE *f);
};

// Global instance
extern cEpgfixerList<cCharSet> EpgfixerCharSets;

#endif //__EPGFIXER_CHARSET_H_
