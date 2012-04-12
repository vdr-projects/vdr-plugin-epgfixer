/*
 * config.h: Global configuration and user settings
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef _EPGFIXER_CONFIG_H_
#define _EPGFIXER_CONFIG_H_

#include "regexp.h"

class cEpgfixerSetup
{
public:
  int quotedshorttext;
  int blankbeforedescription;
  int repeatedtitle;
  int doublequotedshorttext;
  int removeformatting;
  int longshorttext;
  int equalshorttextanddescription;
  int nobackticks;
  int components;

  cEpgfixerSetup();
  bool SetupParse(const char *Name, const char *Value);
  bool ProcessArgs(int argc, char *argv[]);

protected:
  bool ProcessArg(const char *Name, const char *Value);
  static cString m_ProcessedArgs;
};

// Global instance
extern cEpgfixerSetup EpgfixerSetup;

#endif //_EPGFIXER_CONFIG_H_
