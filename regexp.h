/*
 * regexp.h: Regular expression list
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef _REGEXP_H_
#define _REGEXP_H_

#include <vdr/tools.h>

#ifdef HAVE_PCREPOSIX
#include <pcre.h>
#endif

typedef enum { REGEXP_TITLE,REGEXP_SHORTTEXT,REGEXP_DESCRIPTION,REGEXP_UNDEFINED } sources;
extern const char *strSources[];

class cRegexp : public cListObject
{
private:
  bool enabled;
  char *error;
  int erroffset;
  char *regexp;
  char *string;
  int source;
  pcre *re;
  pcre_extra *sd;
  int numchannels;
  int *channels;
  void Free();
public:
  cRegexp();
  cRegexp(int Source, const char *Regex, bool Enabled, bool Precompile);
  virtual ~cRegexp();
  void SetFromString(char *string, bool Enabled, bool Precompile);
  const char *GetString() { return string; }
  void Compile();
  void FreeCompiled();
  bool Enabled(void) { return enabled; }
  void ToggleEnabled(void);
  int NumChannels() { return numchannels; }
  int GetChannel(int index);
  int GetSource(void) { return source; }
  pcre *GetRe(void) { return re; }
  pcre_extra *GetSd(void) { return sd; }
};

class cRegexpList : public cList<cRegexp>
{
public:
  void Clear(void)
  {
    cList<cRegexp>::Clear();
  }
  cRegexpList(void) {}
};

class cEpgfixerRegexps
{
private:
  bool LoadRegexps(const char *FileName = NULL, bool AllowComments = true, bool Precompile = true);
  char *fileName;
public:
  cRegexpList *regexps;

  cEpgfixerRegexps();
  ~cEpgfixerRegexps();
  void SetRegexpConfigFile(const char *FileName);
  const char *RegexpConfigFile();
  bool ReloadRegexps(bool AllowComments = true, bool Precompile = true);
};

// Global instance
extern cEpgfixerRegexps EpgfixerRegexps;

#endif //_REGEXP_H_
