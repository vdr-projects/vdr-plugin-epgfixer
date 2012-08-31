/*
 * tools.c: Tools for handling configuration files and strings
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "tools.h"
#include <vdr/thread.h>

//
// HTML conversion code taken from RSS Reader plugin for VDR
// http://www.saunalahti.fi/~rahrenbe/vdr/rssreader/
// by Rolf Ahrenberg
//

// --- Static -----------------------------------------------------------

#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))

struct conv_table {
  const char *from;
  const char *to;
};

static struct conv_table pre_conv_table[] =
{
  // 'to' field must be smaller than 'from'
  {"<br />",   "\n"}
};

// Conversion page: http://www.ltg.ed.ac.uk/~richard/utf-8.cgi

static struct conv_table post_conv_table[] =
{
  // 'to' field must be smaller than 'from'
  {"&quot;",   "\x22"},
  {"&#34;",    "\x22"},
  {"&amp;",    "\x26"},
  {"&#38;",    "\x26"},
  {"&#038;",   "\x26"},
  {"&#039;",   "\x27"},
  {"&#40;",    "\x28"},
  {"&#41;",    "\x29"},
  {"&#58;",    "\x3a"},
  {"&lt;",     "\x3c"},
  {"&#60;",    "\x3c"},
  {"&gt;",     "\x3e"},
  {"&#62;",    "\x3e"},
  {"&#91;",    "\x5b"},
  {"&#93;",    "\x5d"},
  {"&nbsp;",   "\xc2\xa0"},
  {"&#160;",   "\xc2\xa0"},
  {"&deg;",    "\xc2\xb0"},
  {"&#176;",   "\xc2\xb0"},
  {"&acute;",  "\xc2\xb4"},
  {"&#180;",   "\xc2\xb4"},
  {"&Auml;",   "\xc3\x84"},
  {"&#196;",   "\xc3\x84"},
  {"&Aring;",  "\xc3\x85"},
  {"&#197;",   "\xc3\x85"},
  {"&Ouml;",   "\xc3\x96"},
  {"&#214;",   "\xc3\x96"},
  {"&Uuml;",   "\xc3\x9c"},
  {"&#220;",   "\xc3\x9c"},
  {"&szlig;",  "\xc3\x9f"},
  {"&#223;",   "\xc3\x9f"},
  {"&acirc;",  "\xc3\xa2"},
  {"&#226;",   "\xc3\xa2"},
  {"&auml;",   "\xc3\xa4"},
  {"&#228;",   "\xc3\xa4"},
  {"&aring;",  "\xc3\xa5"},
  {"&#229;",   "\xc3\xa5"},
  {"&ccedil;", "\xc3\xa7"},
  {"&#231;",   "\xc3\xa7"},
  {"&eacute;", "\xc3\xa9"},
  {"&#233;",   "\xc3\xa9"},
  {"&ecirc;",  "\xc3\xaa"},
  {"&#234;",   "\xc3\xaa"},
  {"&ouml;",   "\xc3\xb6"},
  {"&#246;",   "\xc3\xb6"},
  {"&uuml;",   "\xc3\xbc"},
  {"&#252;",   "\xc3\xbc"},
  {"&ndash;",  "\xe2\x80\x93"},
  {"&#8211;",  "\xe2\x80\x93"},
  {"&mdash;",  "\xe2\x80\x94"},
  {"&#8212;",  "\xe2\x80\x94"},
  {"&lsquo;",  "\xe2\x80\x98"},
  {"&#8216;",  "\xe2\x80\x98"},
  {"&rsquo;",  "\xe2\x80\x99"},
  {"&#8217;",  "\xe2\x80\x99"},
  {"&sbquo;",  "\xe2\x80\x9a"},
  {"&#8218;",  "\xe2\x80\x9a"},
  {"&ldquo;",  "\xe2\x80\x9c"},
  {"&#8220;",  "\xe2\x80\x9c"},
  {"&rdquo;",  "\xe2\x80\x9d"},
  {"&#8221;",  "\xe2\x80\x9d"},
  {"&bdquo;",  "\xe2\x80\x9e"},
  {"&#8222;",  "\xe2\x80\x9e"},
  {"&prime;",  "\xe2\x80\xb3"},
  {"&#8243;",  "\xe2\x80\xb3"},
  {"&euro;",   "\xe2\x82\xac"},
  {"&#8364;",  "\xe2\x82\xac"},
  {"\n\n",     "\n"}, // let's also strip multiple linefeeds
};

static char *htmlcharconv(char *str, struct conv_table *conv, unsigned int elem)
{
  if (str && conv) {
     for (unsigned int i = 0; i < elem; ++i) {
        char *ptr = strstr(str, conv[i].from);
        while (ptr) {
           long of = ptr - str;
           size_t l  = strlen(str);
           size_t l1 = strlen(conv[i].from);
           size_t l2 = strlen(conv[i].to);
           if (l2 > l1) {
              error("htmlcharconv(): cannot reallocate string");
              return str;
              }
           if (l2 != l1)
              memmove(str + of + l2, str + of + l1, l - of - l1 + 1);
           strncpy(str + of, conv[i].to, l2);
           ptr = strstr(str, conv[i].from);
           }
        }
     return str;
     }
  return NULL;
}

// --- General functions ------------------------------------------------

char *striphtml(char *str)
{
  if (str) {
     char *c, t = 0, *r;
     str = htmlcharconv(str, pre_conv_table, ELEMENTS(pre_conv_table));
     c = str;
     r = str;
     while (*str != '\0') {
       if (*str == '<')
          t++;
       else if (*str == '>')
          t--;
       else if (t < 1)
          *(c++) = *str;
       str++;
       }
     *c = '\0';
     return htmlcharconv(r, post_conv_table, ELEMENTS(post_conv_table));
     }
  return NULL;
}

// --- cAddEventThread ----------------------------------------

class cAddEventListItem : public cListObject
{
protected:
  cEvent *event;
  tChannelID channelID;
public:
  cAddEventListItem(cEvent *Event, tChannelID ChannelID) { event = Event; channelID = ChannelID; }
  tChannelID GetChannelID() { return channelID; }
  cEvent *GetEvent() { return event; }
  ~cAddEventListItem() { }
};

class cAddEventThread : public cThread
{
private:
  cTimeMs LastHandleEvent;
  cList<cAddEventListItem> *list;
  enum { INSERT_TIMEOUT_IN_MS = 10000 };
protected:
  virtual void Action(void);
public:
  cAddEventThread(void);
  ~cAddEventThread(void);
  void AddEvent(cEvent *Event, tChannelID ChannelID);
};

cAddEventThread::cAddEventThread(void)
:cThread("cAddEventThread"), LastHandleEvent()
{
  list = new cList<cAddEventListItem>;
}

cAddEventThread::~cAddEventThread(void)
{
  LOCK_THREAD;
  list->cList::Clear();
  Cancel(3);
}

void cAddEventThread::Action(void)
{
  SetPriority(19);
  while (Running() && !LastHandleEvent.TimedOut()) {
     cAddEventListItem *e = NULL;
     cSchedulesLock SchedulesLock(true, 10);
     cSchedules *schedules = (cSchedules *)cSchedules::Schedules(SchedulesLock);
     Lock();
     while (schedules && (e = list->First()) != NULL) {
           cSchedule *schedule = (cSchedule *)schedules->GetSchedule(Channels.GetByChannelID(e->GetChannelID()), true);
           schedule->AddEvent(e->GetEvent());
           EpgHandlers.SortSchedule(schedule);
           EpgHandlers.DropOutdated(schedule, e->GetEvent()->StartTime(), e->GetEvent()->EndTime(), e->GetEvent()->TableID(), e->GetEvent()->Version());
           list->Del(e);
           }
     Unlock();
     cCondWait::SleepMs(10);
     }
}

void cAddEventThread::AddEvent(cEvent *Event, tChannelID ChannelID)
{
  LOCK_THREAD;
  list->Add(new cAddEventListItem(Event, ChannelID));
  LastHandleEvent.Set(INSERT_TIMEOUT_IN_MS);
}

static cAddEventThread AddEventThread;

// ---

void AddEvent(cEvent *Event, tChannelID ChannelID)
{
  AddEventThread.AddEvent(Event, ChannelID);
  if (!AddEventThread.Active())
     AddEventThread.Start();
}

// --- Listitem ----------------------------------------

cListItem::cListItem()
{
  enabled = false;
  string = NULL;
  numchannels = 0;
  channels_num = NULL;
  channels_id = NULL;
}

cListItem::~cListItem(void)
{
  Free();
}

void cListItem::Free(void)
{
  FREE(channels_num);
  FREE(channels_id);
  FREE(string);
  numchannels = 0;
  enabled = false;
}

tChannelID *cListItem::GetChannelID(int index)
{
  if (channels_id && index >= 0 && index < numchannels)
     return &channels_id[index];
  else
     return NULL;
}

int cListItem::GetChannelNum(int index)
{
  if (channels_num && index >= 0 && index < numchannels)
     return channels_num[index];
  else
     return 0;
}

bool cListItem::IsActive(tChannelID ChannelID)
{
  bool active = false;
  if (numchannels > 0) {
     int i = 0;
     int channel_number = Channels.GetByChannelID(ChannelID)->Number();
     while (i < numchannels) {
           if ((channel_number == GetChannelNum(i)) ||
               (GetChannelID(i) && (ChannelID == *GetChannelID(i)))) {
              active = true;
              break;
              }
           ++i;
           }
     }
  else
     active = true;
  return active;
}

int cListItem::LoadChannelsFromString(const char *string)
{
  numchannels = 0;
  bool numbers = false;
  if (string != NULL) {
     if (atoi(string))
        numbers = true;
     char *tmpstring = strdup(string);
     char *c = strtok(tmpstring, ",");
     while (c) {
           ++numchannels;
           char *d = 0;
           if (numbers && (d = strchr(c, '-')))// only true if numbers are used
              numchannels = numchannels + atoi(d+1) - atoi(c);
           c = strtok(NULL, ",");
           }
     free(tmpstring);
     }
  if (numchannels > 0) {
     char *tmpstring = strdup(string);
     // Use channel numbers
     if (numbers)
        channels_num = (int *)malloc(sizeof(int)*numchannels);
     else// use channel IDs
        channels_id = (tChannelID *)malloc(sizeof(tChannelID)*numchannels);
     int i = 0;
     char *c = strtok(tmpstring, ",");
     while (i < numchannels) {
           // Use channel numbers
           if (numbers) {
              channels_num[i] = atoi(c);
              if (char *d = strchr(c, '-')) {
                 int count = atoi(d+1) - channels_num[i] + 1;
                 int j = 1;
                 while (j < count) {
                       channels_num[i+j] = channels_num[i] + j;
                       ++j;
                       }
                 i = i + count;
                 }
              }
           else // use channel IDs
              channels_id[i] = tChannelID::FromString(c);
           c = strtok(NULL, ",");
           ++i;
           }
     free(tmpstring);
     }
  return numchannels;
}

void cListItem::ToggleEnabled(void)
{
  enabled = !enabled;
}

void cListItem::PrintConfigLineToFile(FILE *f)
{
  if (f)
     fprintf(f, "%s%s\n", (!enabled && string && *string != '#')  ? "!" : "", string);
}
