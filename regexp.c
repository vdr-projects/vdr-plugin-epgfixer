/*
 * regexp.c: Regular expression list
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "regexp.h"
#include <unistd.h>

/* Global instance */
cEpgfixerRegexps EpgfixerRegexps;

const char *strSources[] = { "title","shorttext","description","undefined" };

cRegexp::cRegexp()
{
  enabled = false;
  error = NULL;
  erroffset = -1;
  regexp = NULL;
  string = NULL;
  source = REGEXP_UNDEFINED;
  re = NULL;
  sd = NULL;
  numchannels = 0;
  channels = NULL;
}

cRegexp::~cRegexp(void)
{
  Free();
}

void cRegexp::Free(void)
{
  free(channels);
  free(regexp);
  free(string);
  channels = NULL;
  regexp = NULL;
  string = NULL;
  FreeCompiled();
}

int cRegexp::GetChannel(int index)
{
  if (index >= 0 && index < numchannels)
     return channels[index];
  else
     return -1;
}

void cRegexp::ToggleEnabled(void)
{
  enabled = enabled ? 0 : 1;
}

void cRegexp::SetFromString(char *s, bool Enabled, bool Precompile)
{
  Free();
  string = strdup(s);
  enabled = Enabled;
  bool precompile = Precompile;
  if (s[0] == '!' || s[0] == '#') {
     enabled = false;
     precompile = false;
     }
  char *p = (s[0] == '#') ? s : strchr(s, '=');
  if (p) {
     if (p[0] == '#')
        source = REGEXP_UNDEFINED;
     else {
        *p = 0;
        regexp = strdup(p + 1);
        char *chanfield = (s[0] == '!') ? s+1 : s;
        char *field = chanfield;
        if (atoi(chanfield)) {
           char *f = strchr(chanfield, ':');
           if (f) {
              *f = 0;
              field = f+1;
              char *c = chanfield;
              numchannels = 0;
              while (c) {
                    numchannels++;
                    c = strchr(c+1, ',');
                    }
              if (numchannels > 0) {
                 c = chanfield;
                 channels = (int *)malloc(sizeof(int)*numchannels);
                 int i = 1;
                 channels[i] = atoi(c);
                 while (i < numchannels) {
                       c = strchr(c+1, ',');
                       channels[i] = atoi(c+1);
                       i++;
                       }
                 }
              }
           }
        if (strcmp(field, "title") == 0)
           source = REGEXP_TITLE;
        if (strcmp(field, "shorttext") == 0)
           source = REGEXP_SHORTTEXT;
        if (strcmp(field, "description") == 0)
           source = REGEXP_DESCRIPTION;
        if (precompile)
            Compile();
        }
     }
}

void cRegexp::FreeCompiled()
{
  if (re) {
     pcre_free(re);
     re = NULL;
     }
  if (sd) {
#ifdef PCRE_CONFIG_JIT
     pcre_free_study(sd);
#else
     pcre_free(sd);
#endif
     sd = NULL;
     }
}

void cRegexp::Compile()
{
  FreeCompiled();
  re = pcre_compile(regexp, 0, (const char **)&error, &erroffset, NULL);
  if (error) {
     esyslog("PCRE compile error: %s at offset %i", error, erroffset);
     enabled = false;
     }
  else {
     sd = pcre_study(re, 0, (const char **)&error);
     if (error)
        esyslog("PCRE study error: %s", error);
     }
}

cEpgfixerRegexps::cEpgfixerRegexps()
{
  fileName = NULL;
  regexps = new cRegexpList();
}

cEpgfixerRegexps::~cEpgfixerRegexps()
{
  free(fileName);
  delete regexps;
}

void cEpgfixerRegexps::SetRegexpConfigFile(const char *FileName)
{
  fileName = strdup(FileName);
}

const char *cEpgfixerRegexps::RegexpConfigFile()
{
  return fileName;
}

bool cEpgfixerRegexps::ReloadRegexps(bool AllowComments, bool Precompile)
{
  regexps->Clear();
  return LoadRegexps(fileName, AllowComments, Precompile);
}

bool cEpgfixerRegexps::LoadRegexps(const char *FileName, bool AllowComments, bool Precompile)
{
  bool result = false;
  if (FileName && access(FileName, F_OK) == 0) {
     FILE *f = fopen(FileName, "r");
     if (f) {
        char *s;
        int line = 0;
        cReadLine ReadLine;
        while ((s = ReadLine.Read(f)) != NULL) {
              line++;
              if (!isempty(s)) {
                 regexps->Add(new cRegexp());
                 regexps->Last()->SetFromString(s, true, Precompile);
                 }
              }
        fclose(f);
        }
     else {
        LOG_ERROR_STR(FileName);
        result = false;
        }
     }
  return result;
}
