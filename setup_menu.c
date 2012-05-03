/*
 * setup_menu.c: Setup Menu
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "setup_menu.h"
#include <vdr/config.h>
#include <vdr/i18n.h>
#include "tools.h"
#include "charset.h"
#include "regexp.h"

//--- cMenuSetupConfigEditor ------------------------------------------------------

#define MAXREGEXPLENGTH 512

const char *RegexpChars = 
  " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789%~\\/?!()[]{}<>$^*.,:;-=#";

template<class T> class cMenuSetupConfigEditor : public cMenuSetupPage
{
private:
  cEpgfixerList<T> *list;
  const char *fileName;
  char **lines;
  char **numlines;
  virtual void LoadListToArray(void)
  {
    lines = (char **)malloc(sizeof(char *)*(list->Count()));
    int i = 0;
    T *item = (T *)list->First();
    while (item) {
          lines[i] = (char *)malloc(sizeof(char)*MAXREGEXPLENGTH);
          snprintf(lines[i], MAXREGEXPLENGTH, "%s", item->GetString());
          item = (T *)item->Next();
          i++;
          }
  }
  void FreeArray()
  {
    int i = 0;
    while (i < list->Count()) {
          free(lines[i]);
          i++;
          }
    free(lines);
  }
  void Save()
  {
    // Store regular expressions to config file
    if (fileName && access(fileName, F_OK) == 0) {
       FILE *f = fopen(fileName, "w");
       if (f) {
          T *item = (T *)list->First();
          while (item) {
                item->PrintConfigLineToFile(f);
                item = (T *)item->Next();
                }
          fclose(f);
          }
       }
  }
protected:
  virtual void Store(void)
  {
    // Store regular expressions back to list
    int i = 0;
    T *item = (T *)list->First();
    while (i < list->Count()) {
          item->SetFromString(lines[i], item->Enabled());
          item = (T *)item->Next();
          i++;
          }
  }
  void Set(void)
  {
    Clear();
    int i = 0;
    T *item = (T *)list->First();
    while (i < list->Count()) {
          Add(new cMenuEditStrItem(item->Enabled() ? "+" : "-", lines[i], MAXREGEXPLENGTH, RegexpChars));
          item = (T *)item->Next();
          i++;
          }
    SetHelp(tr("Toggle state"), tr("Add"), tr("Delete"), tr("Cancel"));
    Display();
  }
public:
  cMenuSetupConfigEditor(cEpgfixerList<T> *l)
  {
    list = l;
    cEitFilter::SetDisableUntil(time(NULL) + 1000);
    SetCols(2);
    fileName = list->GetConfigFile();
    LoadListToArray();
    Set();
  }
  ~cMenuSetupConfigEditor(void)
  {
    FreeArray();
    cEitFilter::SetDisableUntil(time(NULL) + 5);
  }
  virtual eOSState ProcessKey(eKeys Key)
  {
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (state == osUnknown) {
       switch (Key) {
         case kRed:
           list->Get(Current())->ToggleEnabled();
           Set();
           Display();
           state = osContinue;
           break;
         case kGreen:
           Store();
           FreeArray();
           list->Add(new T());
           LoadListToArray();
           Set();
           Display();
           state = osContinue;
           break;
         case kYellow:
           Store();
           FreeArray();
           list->Del(list->Get(Current()),true);
           LoadListToArray();
           Set();
           Display();
           state = osContinue;
           break;
         case kBlue:
           list->ReloadConfigFile();
           state = osBack;
           break;
         case kOk:
           Store();
           Save();
           list->ReloadConfigFile();
           state = osBack;
           break;
         default:
           break;
         }
       }
    return state;
  }
};

//--- cMenuSetupEpgfixer ------------------------------------------------------

cMenuSetupEpgfixer::cMenuSetupEpgfixer(void)
{
  memcpy(&newconfig, &EpgfixerSetup, sizeof(cEpgfixerSetup));
  Set();
}

void cMenuSetupEpgfixer::Set(void)
{
  Clear();
  Add(new cOsdItem(tr("Regular expressions"), osUser1));
  Add(new cOsdItem(tr("Character set conversions"), osUser2));

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
  Add(new cMenuEditBoolItem(tr("Strip HTML entities"),
                            &newconfig.striphtml));
  SetHelp(tr("Reload files"),NULL,NULL, tr("Clear EPG data"));
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
  SetupStore("StripHTMLentities",           		EpgfixerSetup.striphtml);

  Setup.Save();
}

eOSState cMenuSetupEpgfixer::ProcessKey(eKeys Key)
{
  eOSState state = cMenuSetupPage::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed:
         EpgfixerRegexps.ReloadConfigFile();
         EpgfixerCharSets.ReloadConfigFile();
         state = osContinue;
         break;
       case kBlue:
         cEitFilter::SetDisableUntil(time(NULL) + 10);
         if (cSchedules::ClearAll())
            cEitFilter::SetDisableUntil(time(NULL) + 10);
         state = osContinue;
         break;
       default:
         break;
       }
     }
  else if (state == osUser1)
     return AddSubMenu(new cMenuSetupConfigEditor<cRegexp>(&EpgfixerRegexps));
  else if (state == osUser2)
     return AddSubMenu(new cMenuSetupConfigEditor<cCharSet>(&EpgfixerCharSets));
  return state;
}
