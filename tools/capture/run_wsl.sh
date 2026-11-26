#!/bin/bash
# Runs REVSNAP and a key script against the original under DOSBox-X, headless
# (dummy SDL video and audio, DISPLAY and WAYLAND_DISPLAY unset), on Linux or
# WSL. One run at a time, nice'd.
#
#   run_wsl.sh NAME KEYS.BIN [CYCLES [LIMIT_SECONDS]]
#
# Environment:
#   REVSNAP_HOME  the work folder: game/ holds a copy of the game (only
#                 read: each run copies it), REVSNAP.COM is beside it; each run
#                 gets run-NAME/ and run-NAME.conf/.log there
#   DOSBOX_X      the dosbox-x binary (default: dosbox-x on PATH). An unpacked
#                 Debian package works without root when LD_LIBRARY_PATH names
#                 its library folders.
#
# The captures are run-NAME/RVSnnnnn.BIN, in the script's @snap order.
# REVSNAPE.TXT marks the end of the script; NOPATCH.TXT, KEYFAIL.TXT and
# ITEMFAIL.TXT report problems (see revsnap.asm).
set -u
NAME=$1; KEYS=$2; CYCLES=${3:-3000}; LIMIT=${4:-300}
HOME_DIR=${REVSNAP_HOME:?set REVSNAP_HOME}; X=${DOSBOX_X:-dosbox-x}
RUN=$HOME_DIR/run-$NAME
rm -rf "${RUN:?}" && mkdir -p "$RUN" || exit 1
cp -r "$HOME_DIR/game/." "$RUN/" && cp "$HOME_DIR/REVSNAP.COM" "$RUN/" && cp "$KEYS" "$RUN/KEYS.BIN" || exit 1
cat > "$HOME_DIR/run-$NAME.conf" <<EOC
[sdl]
autolock=false
[dosbox]
machine=svga_s3
memsize=16
quit warning=false
[cpu]
core=normal
cputype=386
cycles=fixed $CYCLES
[mixer]
nosound=true
[speaker]
pcspeaker=false
[autoexec]
mount c "$RUN"
c:
REVSNAP
REVENGE
EOC
env -u DISPLAY -u WAYLAND_DISPLAY SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    nice -n 10 "$X" -conf "$HOME_DIR/run-$NAME.conf" -nopromptfolder -fastlaunch \
    < /dev/null > "$HOME_DIR/run-$NAME.log" 2>&1 &
pid=$!; start=$SECONDS
while kill -0 $pid 2>/dev/null; do
    [ -e "$RUN/REVSNAPE.TXT" ] && { sleep 2; echo "script done at $((SECONDS - start)) s"; break; }
    [ -e "$RUN/NOPATCH.TXT" ] && { echo "NOPATCH at $((SECONDS - start)) s"; break; }
    [ $((SECONDS - start)) -ge "$LIMIT" ] && { echo "deadline $LIMIT s"; break; }
    sleep 1
done
kill -9 $pid 2>/dev/null; wait $pid 2>/dev/null   # SIGTERM would wait at a prompt
echo "captures: $(ls "$RUN" | grep -c '^RVS[0-9]'); flags: $(ls "$RUN" | grep -E '^(NOPATCH|KEYFAIL|ITEMFAIL)' | tr '\n' ' ')"
