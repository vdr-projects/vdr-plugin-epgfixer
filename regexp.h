/*
 * regexp.h: Regular expression list item
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __EPGFIXER_REGEXP_H_
#define __EPGFIXER_REGEXP_H_

#ifdef HAVE_PCRE2POSIX
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

#include <vdr/epg.h>
#include <vdr/tools.h>
#include "tools.h"

typedef enum { REGEXP_TITLE, REGEXP_SHORTTEXT, REGEXP_DESCRIPTION, REGEXP_UNDEFINED } sources;

class cRegexp : public cListItem
{
private:
  char *cregexp;
  char *regexp;
  char *replacement;
  int replace;
  int cmodifiers;
  int modifiers;
  int csource;
  int source;
  pcre2_code *cre;
  pcre2_code *re;
  void Compile();
  void FreeCompiled();
  int ParseModifiers(char *modstring, int substitution = 0);
  void ParseRegexp(char *restring);

public:
  cRegexp();
  virtual ~cRegexp();
  using cListItem::Apply;
  virtual bool Apply(cEvent *Event, tChannelID ChannelID = tChannelID());
  void SetFromString(char *string, bool Enabled, int LineNumber = 0);
  int GetSource() { return source; };
  void ToggleEnabled(void);
};

// Global instance
extern cEpgfixerList<cRegexp, cEvent> EpgfixerRegexps;

#endif //__EPGFIXER_REGEXP_H_
