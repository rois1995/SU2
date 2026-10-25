#!/bin/bash
# DC-16 machine-safety watchdog: kills the named test_driver binary when its RSS exceeds a ceiling (kB). Usage: rss_guard.sh <binary basename> <ceiling kB>
B=$1;C=$2
while true; do P=$(pgrep -f "$B \[Native" | head -1); [ -z "$P" ] && { sleep 10; P=$(pgrep -f "$B \[Native" | head -1); [ -z "$P" ] && exit 0; }
  R=$(ps -o rss= -p $P 2>/dev/null | tr -d ' '); [ -n "$R" ] && [ "$R" -gt "$C" ] && { echo "$(date -Is) killing $P rss=$R > $C"; kill -9 $P; exit 1; }; sleep 5; done
