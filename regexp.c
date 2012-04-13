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
  regexp = NULL;
  string = NULL;
  source = REGEXP_UNDEFINED;
  re = NULL;
  sd = NULL;
  numchannels = 0;
  channels_num = NULL;
  channels_str = NULL;
}

cRegexp::~cRegexp(void)
{
  Free();
}

void cRegexp::Free(void)
{
  free(channels_num);
  if (channels_str) {
     int i = 0;
     while (i < numchannels) {
           free(channels_str[i]);
           i++;
           }
     }
  free(channels_str);
  free(regexp);
  free(string);
  channels_num = NULL;
  channels_str = NULL;
  regexp = NULL;
  string = NULL;
  FreeCompiled();
}

int cRegexp::GetChannelNum(int index)
{
  if (channels_num && index >= 0 && index < numchannels)
     return channels_num[index];
  else
     return -1;
}

const char *cRegexp::GetChannelID(int index)
{
  if (channels_str && index >= 0 && index < numchannels)
     return channels_str[index];
  else
     return NULL;
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
              // Use channel numbers
              if (atoi(chanfield))
                 channels_num = (int *)malloc(sizeof(int)*numchannels);
              else// use channel IDs
                 channels_str = (char **)malloc(sizeof(char *)*numchannels);
              int i = 0;
              char *pc = strtok(c, ",");
              while (i < numchannels) {
                    // Use channel numbers
                    if (atoi(chanfield))
                       channels_num[i] = atoi(pc);
                    else// use channel IDs
                       channels_str[i] = strdup(pc);
                    pc = strtok(NULL, ",");
                    i++;
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
  const char *error;
  int erroffset;
  re = pcre_compile(regexp, 0, &error, &erroffset, NULL);
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
        cReadLine ReadLine;
        while ((s = ReadLine.Read(f)) != NULL) {
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
