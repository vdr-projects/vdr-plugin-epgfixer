# EPGfixer Debug Guide

This guide shows you how to use journal logs to debug and understand EPG processing.

## Table of Contents
1. [Basic Journal Commands](#basic-journal-commands)
2. [Debug by Component](#debug-by-component)
3. [Searching for Specific Events](#searching-for-specific-events)
4. [Tracing Full Processing Pipeline](#tracing-full-processing-pipeline)
5. [Common Troubleshooting Patterns](#common-troubleshooting-patterns)

---

## Basic Journal Commands

### View Recent EPGfixer Logs
```bash
# Last 100 lines
journalctl -u vdr --no-pager | grep epgfixer | tail -100

# Last 5 minutes
journalctl -u vdr --since "5 minutes ago" --no-pager | grep epgfixer

# Last hour
journalctl -u vdr --since "1 hour ago" --no-pager | grep epgfixer

# Follow live (like tail -f)
journalctl -u vdr -f | grep epgfixer
```

### Filter by Debug Level
```bash
# Only show changes (most useful!)
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "CHANGED"

# Show errors only
journalctl -u vdr --since "10 minutes ago" --no-pager | grep -i "error"

# Show specific component
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "epgfixer\[regexp\]"
```

---

## Debug by Component

### 1. Regexp Processing (regexp.c)

**Find all regexp changes:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "epgfixer\[regexp\].*CHANGED"
```

**Example output:**
```
[regexp]: Apply() - Line 121 CHANGED title: 'Rote Rosen (1615)' => 'Rote Rosen'
```

**Find which rule applied to specific field:**
```bash
# Changes to titles only
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "CHANGED title:"

# Changes to shorttext only
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "CHANGED shorttext:"

# Changes to description only
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "CHANGED description:"
```

**Find conditional rule failures:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "Conditional check FAILED"
```

**Example output:**
```
[regexp]: Apply() - Line 44: Conditional check FAILED on title, rule skipped
```

**Find conditional rule successes:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "Conditional check PASSED"
```

**Find specific rule by line number:**
```bash
# Show what Line 121 did
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "Line 121"
```

**Count how many times each rule was applied:**
```bash
journalctl -u vdr --since "1 hour ago" --no-pager | \
    grep "epgfixer\[regexp\].*Line [0-9]* CHANGED" | \
    sed 's/.*Line \([0-9]*\).*/\1/' | \
    sort | uniq -c | sort -rn
```

**Example output:**
```
    245 121   <- Line 121 made 245 changes
     89 34
     12 126
```

---

### 2. HTML Stripping (tools.c)

**Find all HTML stripping changes:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "epgfixer\[htmlstrip\].*=>"
```

**Example output:**
```
[htmlstrip]: StripHTML() - Title: '<b>News</b>' => 'News'
[htmlstrip]: StripHTML() - Description: 'Text<br />More text' => 'Text\nMore text'
```

**Find events with no HTML changes:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "htmlstrip.*No changes"
```

**Count HTML changes:**
```bash
journalctl -u vdr --since "1 hour ago" --no-pager | \
    grep -c "epgfixer\[htmlstrip\].*=>"
```

---

### 3. Character Set Conversion (charset.c)

**Find all charset conversions:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "epgfixer\[charset\].*=>"
```

**Example output:**
```
[charset]: Apply() - Title: 'Mörder' => 'Mörder'
```

**Find charset conversion attempts:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "epgfixer\[charset\].*Conversion:"
```

**Example output:**
```
[charset]: Apply() - Event='Tatort', Channel='S19.2E-1-1-12345', Conversion: iso8859-9 -> iso8859-1
```

**Find events with no charset changes:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "charset.*No changes needed"
```

---

### 4. EPG Bug Fixes (tools.c)

**Find what bug fixes were applied:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "epgfixer\[bugfixes\].*Applied:"
```

**Example output:**
```
[bugfixes]: FixOriginalEpgBugs() - Applied: RemoveFormatting
[bugfixes]: FixOriginalEpgBugs() - Applied: QuotedShortText
[bugfixes]: FixOriginalEpgBugs() - Applied: RepeatedTitle
```

**Find repeated title removals:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "RepeatedTitle"
```

---

### 5. Channel Filtering (tools.c)

**Find channel filter matches:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "MATCHED filter"
```

**Example output:**
```
[channelfilter]: IsActive() - MATCHED filter at index 0
```

**Find channel not found errors:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "Channel NOT FOUND"
```

**Example output:**
```
[channelfilter]: IsActive() - Channel NOT FOUND for ChannelID='-0-0-0'
```

**Find fallback ChannelID usage (VDR 2.3.4+):**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "fallback ChannelID"
```

**Example output:**
```
[regexp]: Apply() - Using fallback ChannelID='S19.2E-1-1089-12003' (Event ChannelID was invalid)
```

---

### 6. EPG Cloning (epgclone.c)

**Find all EPG cloning activity:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "epgfixer\[epgclone\]"
```

**Example output:**
```
[epgclone]: Apply() - Cloning Event='Movie Title' from Channel='S19.2E-1-1-12345'
[epgclone]: CloneEvent() - Destination channel number=3, ChannelID='S19.2E-1-1-12346'
```

---

### 7. Blacklist (blacklist.c)

**Find blacklisted channels:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "epgfixer\[blacklist\]"
```

**Example output:**
```
[blacklist]: Apply() - Ignoring Event='Program' on Channel='S19.2E-1-1-12345'
```

---

## Searching for Specific Events

### By Event Title

**Find all processing for a specific event:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "Event='Sportschau'"
```

**Find only changes to specific event:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "Event='Sportschau'" | grep "CHANGED"
```

### By Channel

**Find all events on a specific channel:**
```bash
# By channel number
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "Channel found: number=18"

# By channel name
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "Channel found:.*name='ONE HD'"

# By channel ID
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "S19.2E-1-1089-12003"
```

### By Time Range

**Find events processed in specific timeframe:**
```bash
# Between two times today
journalctl -u vdr --since "13:30" --until "14:00" --no-pager | grep epgfixer

# Specific date and time
journalctl -u vdr --since "2026-01-10 13:30:00" --until "2026-01-10 14:00:00" --no-pager | grep epgfixer
```

---

## Tracing Full Processing Pipeline

### Complete Pipeline for One Event

To see every step of processing for a specific event:

```bash
# Method 1: Using event title
journalctl -u vdr --since "5 minutes ago" --no-pager | \
    grep -A 100 "FixOriginalEpgBugs() START - Event='Rote Rosen'" | \
    head -50

# Method 2: Using process ID (more accurate)
# First, find the process that handled the event
PID=$(journalctl -u vdr --since "5 minutes ago" --no-pager | \
    grep "Event='Rote Rosen'" | head -1 | sed -n 's/.*\[\([0-9]*\)\].*/\1/p')

# Then show all logs from that process
journalctl -u vdr --since "5 minutes ago" --no-pager | grep "\[$PID\]"
```

**Example pipeline output:**
```
[79375] epgfixer[bugfixes]: FixOriginalEpgBugs() START - Event='Sportschau'
[79375] epgfixer[bugfixes]: FixOriginalEpgBugs() - Applied: RemoveFormatting
[79375] epgfixer[htmlstrip]: StripHTML() - No changes for Event='Sportschau'
[79375] epgfixer[channelfilter]: IsActive() - ChannelID='I-1-10301-10301', numchannels=0
[79375] epgfixer[regexp]: Apply() - Line 34: Event='Sportschau', Field=shorttext
[79375] epgfixer[regexp]: Apply() - Line 34: Pattern matched but no change to shorttext
```

---

## Common Troubleshooting Patterns

### Problem: Rule isn't applying

**Step 1: Check if the event is being processed**
```bash
journalctl -u vdr --since "5 minutes ago" --no-pager | grep "Event='Your Event Name'"
```

**Step 2: Check if your rule line is being tested**
```bash
journalctl -u vdr --since "5 minutes ago" --no-pager | grep "Line 121"
```

**Step 3: Check for conditional failures**
```bash
journalctl -u vdr --since "5 minutes ago" --no-pager | grep "Line 121.*Conditional check FAILED"
```

**Step 4: Check channel filtering**
```bash
journalctl -u vdr --since "5 minutes ago" --no-pager | grep "channelfilter"
```

---

### Problem: Rule matches but no change

This is NORMAL when the pattern matches but the replacement produces the same result.

**Example:**
```
[regexp]: Apply() - Line 52: Pattern matched but no change to shorttext
```

**Meaning:** Line 52 is `s/Comedy Reihe/Comedy-Reihe/`, but the shorttext doesn't contain "Comedy Reihe", so nothing changes.

**To verify:**
```bash
# Check what Line 52 does
sed -n '52p' /etc/vdr/plugins/epgfixer/regexp.conf

# See which events it did change
journalctl -u vdr --since "1 hour ago" --no-pager | grep "Line 52 CHANGED"
```

---

### Problem: Too much log noise

**Reduce debug output by disabling components in VDR setup menu:**

1. Go to VDR Setup → Plugins → epgfixer
2. Set debug levels:
   - `Debug regexp: 0` (disable)
   - `Debug htmlstrip: 0` (disable)
   - `Debug charset: 0` (disable)
   - Keep others at 0 except what you're debugging

**Or only show actual changes:**
```bash
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "CHANGED"
```

---

### Problem: Understanding line numbers

**Find what a line number does:**
```bash
# Show line 121 in regexp.conf
sed -n '121p' /etc/vdr/plugins/epgfixer/regexp.conf

# Show line 121 with context (5 lines before/after)
sed -n '116,126p' /etc/vdr/plugins/epgfixer/regexp.conf | cat -n
```

**Find which rule changed an event:**
```bash
# Shows the actual change with line number
journalctl -u vdr --since "10 minutes ago" --no-pager | \
    grep "Event='Rote Rosen'" | grep "CHANGED"
```

Output: `Line 121 CHANGED title: 'Rote Rosen (1615)' => 'Rote Rosen'`

---

## Debug Log Format Reference

### Log Entry Anatomy

```
Jan 10 13:32:03 vdrserver vdr[79350]: [79375] epgfixer[regexp]: Apply() - Line 121 CHANGED title: 'Before' => 'After'
│              │         │         │ │      │         │          │                   │       │          │
│              │         │         │ │      │         │          │                   │       │          └─ New value
│              │         │         │ │      │         │          │                   │       └─ Old value
│              │         │         │ │      │         │          │                   └─ Field changed
│              │         │         │ │      │         │          └─ Config line number
│              │         │         │ │      │         └─ Function name
│              │         │         │ │      └─ Component
│              │         │         │ └─ Thread/Process ID
│              │         │         └─ VDR main process
│              │         └─ VDR hostname
│              └─ Timestamp
```

### Component Names

- `[bugfixes]` - VDR's built-in EPG bug fixes
- `[htmlstrip]` - HTML tag stripping
- `[charset]` - Character set conversion
- `[regexp]` - Regular expression processing
- `[channelfilter]` - Channel filter matching
- `[epgclone]` - EPG cloning
- `[blacklist]` - Channel blacklisting
- `[epghandler]` - EPG handler coordination

---

## Advanced Examples

### Find Top 10 Most Modified Events

```bash
journalctl -u vdr --since "1 hour ago" --no-pager | \
    grep "CHANGED" | \
    sed -n "s/.*Event='\([^']*\)'.*/\1/p" | \
    sort | uniq -c | sort -rn | head -10
```

### Find All Rules That Made Changes

```bash
journalctl -u vdr --since "1 hour ago" --no-pager | \
    grep "CHANGED" | \
    sed -n 's/.*Line \([0-9]*\).*/\1/p' | \
    sort -n | uniq | \
    while read line; do
        echo "Line $line: $(sed -n "${line}p" /etc/vdr/plugins/epgfixer/regexp.conf)"
    done
```

### Monitor EPG Changes in Real-Time

```bash
# Watch changes as they happen
journalctl -u vdr -f | grep --line-buffered "CHANGED" | \
    while read line; do
        echo "$(date '+%H:%M:%S') $line"
    done
```

### Export Debug Session for Analysis

```bash
# Export last hour of EPG processing to file
journalctl -u vdr --since "1 hour ago" --no-pager | \
    grep epgfixer > epgfixer-debug-$(date +%Y%m%d-%H%M%S).log

# Analyze the export
grep "CHANGED" epgfixer-debug-*.log | wc -l  # Count total changes
grep "Line [0-9]*" epgfixer-debug-*.log | less  # Review all rule applications
```

---

## Tips and Best Practices

1. **Start with CHANGED filter** - Most of the time you only care about actual changes
2. **Use time ranges** - Don't search entire journal, limit to recent timeframe
3. **Use line numbers** - They directly correlate to your regexp.conf file
4. **Check conditionals** - Rules often fail because conditionals don't match
5. **Watch process IDs** - One event = one PID, helps trace full pipeline
6. **Test incrementally** - Enable one rule, check logs, enable next rule
7. **Comment out rules** - Add `!` prefix to disable rule and see impact
8. **Keep backups** - Save working regexp.conf before making changes

---

## Quick Reference Card

```bash
# Most useful commands for daily use:

# See what changed in last 5 minutes
journalctl -u vdr --since "5 minutes ago" --no-pager | grep "CHANGED"

# Debug specific event
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "Event='Title Here'"

# Check specific rule (line 121)
journalctl -u vdr --since "10 minutes ago" --no-pager | grep "Line 121"

# See rule statistics
journalctl -u vdr --since "1 hour ago" --no-pager | \
    grep "CHANGED" | sed 's/.*Line \([0-9]*\).*/\1/' | \
    sort | uniq -c | sort -rn

# Live monitoring
journalctl -u vdr -f | grep --line-buffered "CHANGED"
```

---

## Getting Help

If you still can't figure out what's happening:

1. Enable only the component you're debugging (regexp/htmlstrip/charset)
2. Set other components to debug level 0
3. Capture logs for 5 minutes: `journalctl -u vdr --since "5 minutes ago" > debug.log`
4. Search for your event: `grep "Event='YourEvent'" debug.log`
5. Share the relevant section (with line numbers) when asking for help

Remember: Line numbers in the logs directly correspond to lines in your config files!
