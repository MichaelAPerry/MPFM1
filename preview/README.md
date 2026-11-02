# Stamp Composer browser preview

Serve this folder over localhost or HTTPS:

```sh
python3 -m http.server 8080 --directory preview
```

Open http://localhost:8080, select **Start composing**, then **Load Playground demo → Play song**. See `manual.html` for controls. Loading the demo replaces the song after confirmation and can be undone.

The native C app is compiled to WebAssembly with Emscripten 4.0.20. This is a host simulation, not execution of a PI32 firmware binary. Audio remains an original placeholder bank. Saves belong to this browser/origin; export `.scf` copies to keep songs independently.

Matching source is in `app.c`, `dsp/` and `web/emu/`, with the pinned `base` submodule. Rebuild with `./build.sh --web`; generated output is in `build/emu`. Character provenance is in `../assets/NOTICE.md`; dependency notices are in `licenses/`. Original application code is under the root MIT license, the platform shell retains MPL-2.0 notices, and character artwork has separate rights.
