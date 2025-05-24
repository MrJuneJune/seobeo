#!/bin/bash

SOURCE_DIR="../src"
WATCH_FILES="$SOURCE_DIR/*.c"

LAST_MODIFIED_TIME=$(stat --format=%Y $WATCH_FILES)

# Function to restart the server
restart_server() {
    echo "Restarting server..."
    pkill -f example_server
    ./example_server &
}

compile() {
    echo "Static library changed, recompiling..."
    # Create the static library (.a)
    gcc -I../include -c ../src/*.c -o object_files/
    ar rcs libseobeo.a object_files/*.o

    echo "Compiling server"
    gcc -I../include main.c -L. -lseobeo -Wl,-rpath,. -o example_server
}

# Watch for changes and restart the server
while true; do
    CURRENT_MODIFIED_TIME=$(stat --format=%Y $WATCH_FILES)

    if [ "$LAST_MODIFIED_TIME" != "$CURRENT_MODIFIED_TIME" ]; then
        compile
        restart_server
        LAST_MODIFIED_TIME=$CURRENT_MODIFIED_TIME
    fi

    sleep 1
done

