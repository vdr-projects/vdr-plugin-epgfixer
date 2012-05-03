/*
 * regexp.h: Regular expression list
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef _CHARSET_H_
#define _CHARSET_H_

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
  bool ConvertCharSet(cEvent *Event);
  void SetFromString(char *string, bool Enabled);
  virtual void PrintConfigLineToFile(FILE *f);
};

// Global instance
extern cEpgfixerList<cCharSet> EpgfixerCharSets;

#endif //_REGEXP_H_
