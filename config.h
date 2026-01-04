/*
 * config.h: Global configuration and user settings
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __EPGFIXER_CONFIG_H_
#define __EPGFIXER_CONFIG_H_

struct cEpgfixerSetup
{
  int quotedshorttext;
  int blankbeforedescription;
  int repeatedtitle;
  int doublequotedshorttext;
  int removeformatting;
  int longshorttext;
  int equalshorttextanddescription;
  int nobackticks;
  int components;
  int striphtml;

  // Debug logging options
  int debug_epghandler;
  int debug_channelfilter;
  int debug_regexp;
  int debug_charset;
  int debug_epgclone;
  int debug_blacklist;
  int debug_htmlstrip;
  int debug_bugfixes;

  cEpgfixerSetup();
};

// Global instance
extern cEpgfixerSetup EpgfixerSetup;

// Granular debug logging macros
#define DEBUG_EPGHANDLER(x...) \
  do { if (EpgfixerSetup.debug_epghandler) dsyslog("epgfixer[epghandler]: " x); } while (0)

#define DEBUG_CHANNELFILTER(x...) \
  do { if (EpgfixerSetup.debug_channelfilter) dsyslog("epgfixer[channelfilter]: " x); } while (0)

#define DEBUG_REGEXP(x...) \
  do { if (EpgfixerSetup.debug_regexp) dsyslog("epgfixer[regexp]: " x); } while (0)

#define DEBUG_CHARSET(x...) \
  do { if (EpgfixerSetup.debug_charset) dsyslog("epgfixer[charset]: " x); } while (0)

#define DEBUG_EPGCLONE(x...) \
  do { if (EpgfixerSetup.debug_epgclone) dsyslog("epgfixer[epgclone]: " x); } while (0)

#define DEBUG_BLACKLIST(x...) \
  do { if (EpgfixerSetup.debug_blacklist) dsyslog("epgfixer[blacklist]: " x); } while (0)

#define DEBUG_HTMLSTRIP(x...) \
  do { if (EpgfixerSetup.debug_htmlstrip) dsyslog("epgfixer[htmlstrip]: " x); } while (0)

#define DEBUG_BUGFIXES(x...) \
  do { if (EpgfixerSetup.debug_bugfixes) dsyslog("epgfixer[bugfixes]: " x); } while (0)

#endif //__EPGFIXER_CONFIG_H_
