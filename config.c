/*
 * config.c: Global configuration and user settings
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

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

  // Debug flags - all disabled by default
  debug_epghandler = 0;
  debug_channelfilter = 0;
  debug_regexp = 0;
  debug_charset = 0;
  debug_epgclone = 0;
  debug_blacklist = 0;
  debug_htmlstrip = 0;
  debug_bugfixes = 0;
}
