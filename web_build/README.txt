This folder contains the web demo build artifacts.

If you ran `scripts/build_web.sh`, open this directory and run a static file server to test locally:

  cd web_build
  python3 -m http.server 8000

Then open http://localhost:8000 in your browser.

The generated index.html mounts an IDBFS filesystem at `/save` and will sync on load and before unload so your save files persist between sessions.

If the build fails due to missing raylib for Emscripten, you may need to build or install an Emscripten-compatible raylib and link it into the emcc command.
