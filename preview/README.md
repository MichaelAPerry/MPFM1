# Prebuilt native-app preview

Serve this directory over localhost with Python's HTTP server. Built using Emscripten 4.0.20 from the C sources and pinned fm1-base in this branch. `app.wasm` is host-simulation code, not a device firmware binary. Original app sounds are placeholders.

Rebuild: `./build.sh --web`, then copy `build/emu/` here. This snapshot was tested with `node tests/web.mjs preview/app.wasm examples/playground.scf`. Full browser audio/device testing remains outstanding.

Sources: parent repository (MIT) and its `base` submodule at 02f422c7d9de121ca099bb2fdebe5c035f59e8ad (MPL-2.0); virtual panel HTML and worklet retain upstream copyright headers. License texts for the platform, font and Emscripten runtime are in `licenses/`. No SDK firmware blobs or Nintendo samples are included.
