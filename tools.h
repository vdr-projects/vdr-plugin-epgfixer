/*
 * tools.h: Tools for handling configure files and strings
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __EPGFIXER_TOOLS_H_
#define __EPGFIXER_TOOLS_H_

#include <vdr/epg.h>
#include <vdr/tools.h>
#include <unistd.h>
#include <stdio.h>

#define error(x...) esyslog("EPGFixer: " x);

#define FREE(x) { free(x); x = NULL; }

void AddEvent(cEvent *event, tChannelID ChannelID);

char *striphtml(char *str);

class cListItem : public cListObject
{
protected:
  bool enabled;
  char *string;
  int *channels_num;
  char **channels_str;
  int numchannels;
  void Free();
  const char *GetChannelID(int index);
  int GetChannelNum(int index);
  int LoadChannelsFromString(const char *string);
  bool IsActive(tChannelID ChannelID);
public:
  cListItem();
  virtual ~cListItem();
  virtual bool Apply(cChannel *Channel) { return 0; }
  virtual bool Apply(cEvent *Event) { return 0; }
  void SetFromString(char *string, bool Enabled);
  const char *GetString() { return string; }
  bool Enabled(void) { return enabled; }
  void ToggleEnabled(void);
  void PrintConfigLineToFile(FILE *f);
};

template<class LISTITEM, class PARAMETER> class cEpgfixerList : public cList<LISTITEM>
{
protected:
  char *fileName;
  bool LoadConfigFile(bool AllowComments = true)
  {
    bool result = false;
    if (fileName && access(fileName, F_OK) == 0) {
       FILE *f = fopen(fileName, "r");
       if (f) {
          char *s;
          int line = 0;
          int count = 0;
          cReadLine ReadLine;
          cString logmsg("");
          logmsg = cString::sprintf("%s%s loaded. Active lines:", *logmsg, fileName);
          while ((s = ReadLine.Read(f)) != NULL) {
                ++line;
                if (!isempty(s)) {
                   this->Add(new LISTITEM());
                   cList<LISTITEM>::Last()->SetFromString(s, true);
                   if (cList<LISTITEM>::Last()->Enabled()) {
                      ++count;
                      logmsg = cString::sprintf("%s%s%i", *logmsg, count == 1 ? " " : ",", line);
                      }
                   }
                }
          fclose(f);
          if (count == 0)
            logmsg = cString::sprintf("%s none", *logmsg);
          isyslog("%s", *logmsg);
          }
       else {
          LOG_ERROR_STR(fileName);
          result = false;
          }
       }
    ;
    return result;
  }

public:
  cEpgfixerList() { fileName = NULL; }
  ~cEpgfixerList() { free(fileName); }
  void Clear(void) { cList<LISTITEM>::Clear(); }
  bool ReloadConfigFile(bool AllowComments = true)
  {
    Clear();
    return LoadConfigFile(AllowComments);
  }
  bool Apply(PARAMETER *Parameter)
  {
    int res = false;
    LISTITEM *item = (LISTITEM *)(cList<LISTITEM>::First());
    while (item) {
          if (item->Enabled()) {
             int ret = item->LISTITEM::Apply(Parameter);
             if (ret && !res)
                res = true;
             }
          item = (LISTITEM *)(item->Next());
          }
    return res;
  }
  void SetConfigFile(const char *FileName) { fileName = strdup(FileName); }
  const char *GetConfigFile() { return fileName; }
};

#endif //__EPGFIXER_TOOLS_H_
