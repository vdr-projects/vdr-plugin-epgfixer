/*
 * regexp.h: Regular expression list item
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef _REGEXP_H_
#define _REGEXP_H_

#include "tools.h"
#include <vdr/epg.h>
#include <vdr/tools.h>
#include <stdio.h>

#ifdef HAVE_PCREPOSIX
#include <pcre.h>
#endif

typedef enum { REGEXP_TITLE,REGEXP_SHORTTEXT,REGEXP_DESCRIPTION,REGEXP_UNDEFINED } sources;

class cRegexp : public cListItem
{
private:
  char *regexp;
  int source;
  pcre *re;
  pcre_extra *sd;
  void Compile();
  void FreeCompiled();
public:
  cRegexp();
  virtual ~cRegexp();
  bool Apply(cEvent *Event);
  void SetFromString(char *string, bool Enabled);
  int GetSource() { return source; };
  void ToggleEnabled(void);
  virtual void PrintConfigLineToFile(FILE *f);
};

// Global instance
extern cEpgfixerList<cRegexp> EpgfixerRegexps;

#endif //_REGEXP_H_
