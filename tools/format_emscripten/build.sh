#!/bin/bash

emcc -std=c++17 -Oz -flto -I src/ -I tools/format/ -isystem src/third-party/ \
    -s SINGLE_FILE --shell-file tools/format_emscripten/shell.html -s WASM=1 \
    -s ENVIRONMENT=web -s MODULARIZE=1 -s 'EXPORT_NAME=json_formatter' -s NO\_FILESYSTEM=1 \
    -s LLD_REPORT_UNDEFINED -s MINIFY_HTML=0 -s NO_DISABLE_EXCEPTION_CATCHING \
    -s EXPORTED_FUNCTIONS=_json_format -s EXPORTED_RUNTIME_METHODS=cwrap \
    src/wcwidth.cpp src/json.cpp tools/format/format.cpp tools/format_emscripten/format_emscripten.cpp \
    -o formatter.html
