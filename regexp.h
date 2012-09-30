/*
 * regexp.h: Regular expression list item
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __EPGFIXER_REGEXP_H_
#define __EPGFIXER_REGEXP_H_

#include "tools.h"
#include <vdr/epg.h>

#ifdef HAVE_PCREPOSIX
#include <pcre.h>
#endif

typedef enum { REGEXP_TITLE, REGEXP_SHORTTEXT, REGEXP_DESCRIPTION, REGEXP_UNDEFINED } sources;

class cRegexp : public cListItem
{
private:
  char *regexp;
  char *replacement;
  int replace;
  int modifiers;
  int source;
  pcre *re;
  pcre_extra *sd;
  void Compile();
  void FreeCompiled();
  void ParseRegexp(char *restring);

public:
  cRegexp();
  virtual ~cRegexp();
  using cListItem::Apply;
  virtual bool Apply(cEvent *Event);
  void SetFromString(char *string, bool Enabled);
  int GetSource() { return source; };
  void ToggleEnabled(void);
};

// Global instance
extern cEpgfixerList<cRegexp, cEvent> EpgfixerRegexps;

#endif //__EPGFIXER_REGEXP_H_
