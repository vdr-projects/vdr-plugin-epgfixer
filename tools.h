/*
 * tools.h: Tools for handling configure files and strings
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef _STRINGTOOLS_H_
#define _STRINGTOOLS_H_

#include <vdr/tools.h>
#include <unistd.h>
#include <stdio.h>

char *striphtml(char *str);
int count(const char *string, const char separator);
int loadChannelsFromString(const char *string, int **channels_num, char ***channels_str);

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
public:
  cListItem();
  virtual ~cListItem();
  void SetFromString(char *string, bool Enabled);
  const char *GetString() { return string; }
  bool Enabled(void) { return enabled; }
  void ToggleEnabled(void);
  int NumChannels() { return numchannels; }
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
                   Add(new T());
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
  void SetConfigFile(const char *FileName) { fileName = strdup(FileName); }
  const char *GetConfigFile() { return fileName; }
};

#endif //_STRINGTOOLS_H_
