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
  virtual bool Apply(cEvent *Event) { return 0; }
  void SetFromString(char *string, bool Enabled);
  const char *GetString() { return string; }
  bool Enabled(void) { return enabled; }
  void ToggleEnabled(void);
  virtual void PrintConfigLineToFIle(FILE *f) {}
};

template<class T> class cEpgfixerList : public cList<T>
{
protected:
  char *fileName;
  bool LoadConfigFile(const char *FileName = NULL, bool AllowComments = true)
  {
    bool result = false;
    if (FileName && access(FileName, F_OK) == 0) {
       FILE *f = fopen(FileName, "r");
       if (f) {
          char *s;
          cReadLine ReadLine;
          while ((s = ReadLine.Read(f)) != NULL) {
                if (!isempty(s)) {
                   this->Add(new T());
                   cList<T>::Last()->SetFromString(s, true);
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

public:
  cEpgfixerList() { fileName = NULL; }
  ~cEpgfixerList() { free(fileName); }
  void Clear(void) { cList<T>::Clear(); }
  bool ReloadConfigFile(bool AllowComments = true)
  {
    Clear();
    return LoadConfigFile(fileName, AllowComments);
  }
  bool Apply(cEvent *Event)
  {
    int res = false;
    T *item = (T *)(cList<T>::First());
    while (item) {
          if (item->Enabled()) {
             int ret = item->Apply(Event);
             if (ret && !res)
                res = true;
             }
          item = (T *)(item->Next());
          }
    return res;
  }
  void SetConfigFile(const char *FileName) { fileName = strdup(FileName); }
  const char *GetConfigFile() { return fileName; }
};

#endif //__EPGFIXER_TOOLS_H_
