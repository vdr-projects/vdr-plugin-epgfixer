/*
 * setup_menu.c: Setup Menu
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "setup_menu.h"
#include <vdr/config.h>
#include <vdr/i18n.h>
#include "regexp.h"

//--- cMenuSetupRegexp ------------------------------------------------------

#define MAXREGEXPLENGTH 512

const char *RegexpChars = 
  " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-=.,*%$^<>~:;\\/?!()[]{}#";

class cMenuSetupRegexp : public cMenuSetupPage
{
private:
  const char *fileName;
  char **lines;
  char **numlines;
  void LoadRegexpArray();
  void FreeRegexpArray();
  void Save();
protected:
  virtual void Store(void);
  void Set(void);
public:
  cMenuSetupRegexp(void);
  ~cMenuSetupRegexp(void);
  virtual eOSState ProcessKey(eKeys Key);
};

void cMenuSetupRegexp::LoadRegexpArray()
{
  lines = (char **)malloc(sizeof(char *)*(EpgfixerRegexps.regexps->Count()));
  int i = 0;
  cRegexp *regex = (cRegexp *)EpgfixerRegexps.regexps->First();
  while (regex) {
        lines[i] = (char *)malloc(sizeof(char)*MAXREGEXPLENGTH);
        snprintf(lines[i], MAXREGEXPLENGTH, "%s", regex->GetString());
        regex = (cRegexp *)regex->Next();
        i++;
        }
}

cMenuSetupRegexp::cMenuSetupRegexp(void)
{
  cEitFilter::SetDisableUntil(time(NULL) + 1000);
  SetCols(2);
  fileName = EpgfixerRegexps.RegexpConfigFile();
  LoadRegexpArray();
  Set();
}

void cMenuSetupRegexp::FreeRegexpArray(void)
{
  int i = 0;
  while (i < EpgfixerRegexps.regexps->Count()) {
        free(lines[i]);
        i++;
        }
  free(lines);
}

cMenuSetupRegexp::~cMenuSetupRegexp(void)
{
  FreeRegexpArray();
  cEitFilter::SetDisableUntil(time(NULL) + 5);
}

void cMenuSetupRegexp::Set(void)
{
  Clear();
  int i = 0;
  cRegexp *regex = (cRegexp *)EpgfixerRegexps.regexps->First();
  while (i < EpgfixerRegexps.regexps->Count()) {
        Add(new cMenuEditStrItem(regex->Enabled() ? "+" : "-", lines[i], MAXREGEXPLENGTH, RegexpChars));
        regex = (cRegexp *)regex->Next();
        i++;
        }
  SetHelp(tr("Toggle state"), tr("Add"), tr("Delete"), tr("Cancel"));
  Display();
}

void cMenuSetupRegexp::Store(void)
{
  // Store regular expressions back to list
  int i = 0;
  cRegexp *regex = (cRegexp *)EpgfixerRegexps.regexps->First();
  while (i < EpgfixerRegexps.regexps->Count()) {
        regex->SetFromString(lines[i], regex->Enabled(), true);
        regex = (cRegexp *)regex->Next();
        i++;
        }
}

void cMenuSetupRegexp::Save(void)
{
  // Store regular expressions to regxep.conf
  if (fileName && access(fileName, F_OK) == 0) {
     FILE *f = fopen(fileName, "w");
     if (f) {
        cRegexp *regex = (cRegexp *)EpgfixerRegexps.regexps->First();
        while (regex) {
              if (regex->GetSource() == REGEXP_UNDEFINED)
                 fprintf(f, "%s\n", regex->GetString());
              else
                 fprintf(f, "%s%s\n", regex->Enabled() ? "" : "!", regex->GetString());
              regex = (cRegexp *)regex->Next();
              }
        fclose(f);
        }
     }
}

eOSState cMenuSetupRegexp::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed:
         if (EpgfixerRegexps.regexps->Get(Current())->GetSource() != REGEXP_UNDEFINED) {
            EpgfixerRegexps.regexps->Get(Current())->ToggleEnabled();
            Set();
            Display();
            }
         state = osContinue;
         break;
       case kGreen:
         Store();
         FreeRegexpArray();
         EpgfixerRegexps.regexps->Add(new cRegexp());
         LoadRegexpArray();
         Set();
         Display();
         state = osContinue;
         break;
       case kYellow:
         Store();
         FreeRegexpArray();
         EpgfixerRegexps.regexps->Del(EpgfixerRegexps.regexps->Get(Current()),true);
         LoadRegexpArray();
         Set();
         Display();
         state = osContinue;
         break;
       case kBlue:
         EpgfixerRegexps.ReloadRegexps();
         state = osBack;
         break;
       case kOk:
         Store();
         Save();
         EpgfixerRegexps.ReloadRegexps();
         state = osBack;
         break;
       default:
         break;
       }
     }
  return state;
}

//--- cMenuSetupEpgfixer ------------------------------------------------------

cMenuSetupEpgfixer::cMenuSetupEpgfixer(void)
{
  memcpy(&newconfig, &EpgfixerSetup, sizeof(cEpgfixerSetup));
  Set();
}

void cMenuSetupEpgfixer::Set(void)
{
  Clear();

  Add(new cMenuEditBoolItem(tr("Remove quotes from ShortText"),
                            &newconfig.quotedshorttext));
  Add(new cMenuEditBoolItem(tr("Move Description from ShortText"),
                            &newconfig.blankbeforedescription));
  Add(new cMenuEditBoolItem(tr("Remove repeated title from ShortText"),
                            &newconfig.repeatedtitle));
  Add(new cMenuEditBoolItem(tr("Remove double quotes from ShortText"),
                            &newconfig.doublequotedshorttext));
  Add(new cMenuEditBoolItem(tr("Remove useless formatting"),
                            &newconfig.removeformatting));
  Add(new cMenuEditBoolItem(tr("Move long ShortText to Description"),
                            &newconfig.longshorttext));
  Add(new cMenuEditBoolItem(tr("Prevent equal ShortText and Description"),
                            &newconfig.equalshorttextanddescription));
  Add(new cMenuEditBoolItem(tr("Replace backticks with single quotes"),
                            &newconfig.nobackticks));
  Add(new cMenuEditBoolItem(tr("Fix stream component descriptions"),
                            &newconfig.components));
  Add(new cOsdItem(tr("Regular expressions"), osUser1));
  SetHelp(tr("Reload regexp.conf"),NULL,NULL, tr("Clear EPG data"));
  Display();
}

void cMenuSetupEpgfixer::Store(void)
{
  memcpy(&EpgfixerSetup, &newconfig, sizeof(cEpgfixerSetup));

  SetupStore("RemoveQuotesFromShortText",           EpgfixerSetup.quotedshorttext);
  SetupStore("MoveDescriptionFromShortText",        EpgfixerSetup.blankbeforedescription);
  SetupStore("RemoveRepeatedTitleFromShortText",    EpgfixerSetup.repeatedtitle);
  SetupStore("RemoveDoubleQuotesFromShortText",     EpgfixerSetup.doublequotedshorttext);
  SetupStore("RemoveUselessFormatting",             EpgfixerSetup.removeformatting);
  SetupStore("MoveLongShortTextToDescription",      EpgfixerSetup.longshorttext);
  SetupStore("PreventEqualShortTextAndDescription", EpgfixerSetup.equalshorttextanddescription);
  SetupStore("ReplaceBackticksWithSingleQuotes",    EpgfixerSetup.nobackticks);
  SetupStore("FixStreamComponentDescriptions",      EpgfixerSetup.components);

  Setup.Save();
}

eOSState cMenuSetupEpgfixer::ProcessKey(eKeys Key)
{
  eOSState state = cMenuSetupPage::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed:
         EpgfixerRegexps.ReloadRegexps();
         state = osContinue;
         break;
       case kBlue:
         cEitFilter::SetDisableUntil(time(NULL) + 10);
         if (cSchedules::ClearAll()) {
            cEitFilter::SetDisableUntil(time(NULL) + 10);
            }
         state = osContinue;
         break;
       default:
         break;
       }
     }
  else if (state == osUser1)
     return AddSubMenu(new cMenuSetupRegexp());
  return state;
}
