#!/usr/bin/env bash
set -euo pipefail

# Build script for Emscripten web demo of rcktmn
# Requirements: emcc (Emscripten SDK) installed and on PATH.

OUT_DIR="web_build"
mkdir -p "$OUT_DIR"

echo "Checking for emcc..."
if ! command -v emcc >/dev/null 2>&1; then
  echo "emcc not found. Install Emscripten SDK and ensure 'emcc' is on PATH." >&2
  exit 2
fi

# Compiler flags
EMCC_FLAGS=(
  -std=c++17
  -O2
  -s WASM=1
  -s ALLOW_MEMORY_GROWTH=1
  -s ASYNCIFY=1
  -s USE_GLFW=3
  -DWEB_BUILD
  -s "EXPORTED_RUNTIME_METHODS=['FS','callMain']"
  -s "FORCE_FILESYSTEM=1"
  -s "ENVIRONMENT=web"
  -s "MODULARIZE=0"
  -s "NO_EXIT_RUNTIME=0"
  -s "INITIAL_MEMORY=134217728" # 128MB
)

# Source files
SRCS=(rcktmn.cpp src/*.cpp)

OUT_JS="$OUT_DIR/rcktmn.js"
OUT_HTML="$OUT_DIR/index.html"

echo "Compiling to $OUT_JS ..."

# If a prebuilt emscripten-compatible raylib is provided via RAYLIB_DIR, include/link it
EMCC_EXTRA=()
if [[ -n "${RAYLIB_DIR:-}" ]]; then
  if [[ -f "$RAYLIB_DIR/include/raylib.h" ]]; then
    echo "Using raylib from $RAYLIB_DIR"
    EMCC_EXTRA+=( -I"$RAYLIB_DIR/include" )
    # Link against static library if present
    if [[ -f "$RAYLIB_DIR/lib/libraylib.a" ]]; then
      EMCC_EXTRA+=( "$RAYLIB_DIR/lib/libraylib.a" )
    else
      echo "Warning: libraylib.a not found in $RAYLIB_DIR/lib — you may need to build raylib for Emscripten." >&2
    fi
  else
    echo "RAYLIB_DIR defined but include/raylib.h not found inside it." >&2
    exit 3
  fi
else
  echo "No RAYLIB_DIR set. Building raylib for Emscripten in scripts/raylib_web/ (this may take several minutes)."

  # Use scripts/raylib_web for Emscripten raylib
  SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  RAYLIB_DIR="$SCRIPTS_DIR/raylib_web"
  RAYLIB_SRC="$RAYLIB_DIR/raylib"
  mkdir -p "$RAYLIB_DIR"
  
  if [[ ! -d "$RAYLIB_SRC" ]]; then
    echo "Cloning raylib into $RAYLIB_DIR..."
    git clone --depth 1 https://github.com/raysan5/raylib.git "$RAYLIB_SRC"
  else
    echo "Found existing raylib in $RAYLIB_SRC"
  fi

  pushd "$RAYLIB_SRC" >/dev/null
  # Build with emcmake/emmake
  if ! command -v emcmake >/dev/null 2>&1; then
    echo "emcmake not found — ensure Emscripten SDK is active (source emsdk_env.sh)." >&2
    exit 4
  fi

  BUILD_DIR="$RAYLIB_SRC/build_ems"
  mkdir -p "$BUILD_DIR"
  echo "Configuring raylib for Emscripten in $BUILD_DIR..."
  emcmake cmake -S . -B "$BUILD_DIR" -DPLATFORM=Web -DBUILD_EXAMPLES=OFF || true

  echo "Building raylib (this may take a while)..."
  emmake make -C "$BUILD_DIR" -j$(nproc) || true

  # Try to locate libraylib.a in build output
  POSS_LIBS=(
    "$BUILD_DIR/libraylib.a"
    "$BUILD_DIR/src/libraylib.a"
    "$BUILD_DIR/src/release/libraylib.a"
    "$BUILD_DIR/raylib/libraylib.a"
    "$BUILD_DIR/raylib/src/libraylib.a"
  )
  FOUND_LIB=""
  for p in "${POSS_LIBS[@]}"; do
    if [[ -f "$p" ]]; then
      FOUND_LIB="$p"
      break
    fi
  done

  if [[ -z "$FOUND_LIB" ]]; then
    echo "Warning: could not find built libraylib.a in $BUILD_DIR. Build may have failed." >&2
    echo "Please inspect $RAYLIB_SRC/$BUILD_DIR for build errors." >&2
    popd >/dev/null
    exit 5
  fi

  FOUND_LIB="$(realpath "$FOUND_LIB")"

  # Set RAYLIB_DIR to the source root for includes, and lib path for linking
  EMCC_EXTRA+=( -I"$RAYLIB_SRC/src" "$FOUND_LIB" )
  popd >/dev/null

  emcc "${SRCS[@]}" "${EMCC_FLAGS[@]}" "${EMCC_EXTRA[@]}" -o "$OUT_JS"
fi

# Generate a small wrapper HTML that mounts IDBFS at /save and syncs on load/unload
cat > "$OUT_HTML" <<'HTML'
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>RCKTMN Web Demo</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html, body {
      margin: 0;
      padding: 0;
      width: 100%;
      height: 100%;
      background: #111;
      color: #eee;
      font-family: sans-serif;
      overflow: hidden;
    }
    body {
      position: fixed;
      inset: 0;
    }
    #game-shell {
      position: fixed;
      inset: 0;
      display: flex;
      flex-direction: column;
      background: #111;
    }
    #topbar {
      display: none;
    }
    #topbar h3 {
      margin: 0;
      font-size: 18px;
      font-weight: 700;
    }
    #topbar p {
      margin: 0;
      opacity: 0.8;
      font-size: 13px;
    }
    #fs-button {
      margin-left: auto;
      padding: 8px 12px;
      border: 1px solid #555;
      border-radius: 6px;
      background: #222;
      color: #fff;
      cursor: pointer;
    }
    #fs-button:hover {
      background: #2d2d2d;
    }
    #stage {
      position: relative;
      flex: 1 1 auto;
      min-height: 0;
      overflow: hidden;
    }
    canvas {
      display: block;
      width: 100vw;
      height: 100vh;
      image-rendering: pixelated;
      background: #000;
    }
  </style>
</head>
<body>
  <div id="game-shell">
    <div id="topbar">
      <h3>RCKTMN Web Demo</h3>
      <p>Click fullscreen, then click the game to capture input.</p>
      <button id="fs-button" type="button">Fullscreen</button>
    </div>
    <div id="stage">
      <canvas id="canvas" width="1920" height="1080" tabindex="1" autofocus></canvas>
    </div>
  </div>

<script>
// Mount IDBFS at /save and sync before runtime starts
const canvas = document.getElementById('canvas');
const fsButton = document.getElementById('fs-button');

function focusGame() {
  canvas.focus();
}

function enterFullscreen() {
  const target = document.getElementById('game-shell');
  if (target.requestFullscreen) {
    target.requestFullscreen().catch(() => {});
  } else if (canvas.requestFullscreen) {
    canvas.requestFullscreen().catch(() => {});
  }
  focusGame();
}

fsButton.addEventListener('click', enterFullscreen);
canvas.addEventListener('click', focusGame);
window.addEventListener('load', focusGame);

const preventKeys = new Set([
  'Tab', ' ', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight',
  'PageUp', 'PageDown', 'Home', 'End'
]);

window.addEventListener('keydown', function (e) {
  const key = e.key;
  const ctrlCombo = e.ctrlKey || e.metaKey;
  const specialControl = ctrlCombo && ['s', 'l', 'm'].includes(key.toLowerCase());
  if (preventKeys.has(key) || specialControl) {
    e.preventDefault();
    e.stopPropagation();
  }
}, true);

window.addEventListener('wheel', function (e) {
  if (document.activeElement === canvas) {
    e.preventDefault();
  }
}, { passive: false });

window.addEventListener('contextmenu', function (e) {
  if (document.activeElement === canvas) {
    e.preventDefault();
  }
});

var Module = {
  canvas: canvas,
  preRun: [function() {
    try {
      FS.mkdir('/save');
    } catch (e) {}
    try {
      FS.mount(IDBFS, {}, '/save');
      // Populate FS from IndexedDB
      FS.syncfs(true, function (err) {
        console.log('IDBFS sync (load) complete', err);
      });
    } catch (e) {
      console.warn('IDBFS mount failed:', e);
    }
  }],
  onRuntimeInitialized: function() {
    console.log('Runtime initialized');
  }
};
</script>

<script src="rcktmn.js"></script>

<script>
// Ensure /save is persisted before the page unloads
window.addEventListener('beforeunload', function (e) {
  try {
    FS.syncfs(false, function (err) {
      console.log('IDBFS sync (save) complete', err);
    });
  } catch (e) {}
});
</script>

</body>
</html>
HTML

chmod +x "$OUT_HTML"

echo "Build finished. Open $OUT_HTML in a static web server (e.g., 'python3 -m http.server 8000' in $OUT_DIR)."

echo "Note: This script assumes raylib is available for Emscripten builds. If you need to compile/link raylib for Web, install or build the emscripten-compatible raylib and adjust the command to include it."
