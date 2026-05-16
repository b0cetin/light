#!/bin/bash
TRIGGER=".qemu-trigger"
DIR="$(cd "$(dirname "$0")" && pwd)"

cleanup() { rm -f "$TRIGGER"; echo "Host runner stopped."; }
trap cleanup EXIT

rm -f "$TRIGGER"
echo "Host runner ready."

launch() {
    osascript -e "tell application \"Terminal\"
        do script \"cd '$DIR' && $1\"
        activate
    end tell"
}

while true; do
    if [ -f "$TRIGGER" ]; then
        MODE=$(cat "$TRIGGER")
        rm -f "$TRIGGER"
        case "$MODE" in
            run)   launch "./run.sh" ;;
            debug) launch "DEBUG=1 ./run.sh" ;;
            *)     echo "Unknown mode: $MODE" ;;
        esac
    fi
    sleep 0.1
done