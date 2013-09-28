/*
 * config.c: Global configuration and user settings
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "config.h"

/* Global instance */
cEpgfixerSetup EpgfixerSetup;

cEpgfixerSetup::cEpgfixerSetup()
{
  quotedshorttext = 0;
  blankbeforedescription = 0;
  repeatedtitle = 0;
  doublequotedshorttext = 0;
  removeformatting = 0;
  longshorttext = 0;
  equalshorttextanddescription = 0;
  nobackticks = 0;
  components = 0;
  striphtml = 0;
}

cString cEpgfixerSetup::m_ProcessedArgs;

bool cEpgfixerSetup::ProcessArg(const char *Name, const char *Value)
{
  if (SetupParse(Name, Value)) {
     m_ProcessedArgs = cString::sprintf("%s%s ", *m_ProcessedArgs ? *m_ProcessedArgs : " ", Name);
     return true;
     }
  return false;
}

bool cEpgfixerSetup::ProcessArgs(int argc, char *argv[])
{
  return true;
}

bool cEpgfixerSetup::SetupParse(const char *Name, const char *Value)
{
  const char *pt;
  if (*m_ProcessedArgs && NULL != (pt = strstr(m_ProcessedArgs + 1, Name)) &&
      *(pt - 1) == ' ' && *(pt + strlen(Name)) == ' ') {
     dsyslog("Skipping configuration entry %s=%s (overridden in command line)", Name, Value);
     return true;
     }

  if (!strcasecmp(Name, "RemoveQuotesFromShortText"))                quotedshorttext = atoi(Value);
  else if (!strcasecmp(Name, "MoveDescriptionFromShortText"))        blankbeforedescription = atoi(Value);
  else if (!strcasecmp(Name, "RemoveRepeatedTitleFromShortText"))    repeatedtitle = atoi(Value);
  else if (!strcasecmp(Name, "RemoveDoubleQuotesFromShortText"))     doublequotedshorttext = atoi(Value);
  else if (!strcasecmp(Name, "RemoveUselessFormatting"))             removeformatting = atoi(Value);
  else if (!strcasecmp(Name, "MoveLongShortTextToDescription"))      longshorttext = atoi(Value);
  else if (!strcasecmp(Name, "PreventEqualShortTextAndDescription")) equalshorttextanddescription = atoi(Value);
  else if (!strcasecmp(Name, "ReplaceBackticksWithSingleQuotes"))    nobackticks = atoi(Value);
  else if (!strcasecmp(Name, "FixStreamComponentDescriptions"))      components = atoi(Value);
  else if (!strcasecmp(Name, "StripHTMLEntities"))                   striphtml = atoi(Value);
  else
     return false;

  return true;
}
