"""Copy first-party simulator assets after fm1-base produces the WASM module."""
from pathlib import Path
import shutil
root=Path(__file__).resolve().parents[1]
out=root/'build/emu'
for name in ('theme.css','manual.html'):
 shutil.copyfile(root/'web/emu'/name,out/name)
shutil.copytree(root/'assets/stamps',out/'stamps',dirs_exist_ok=True)
print('Preview theme, manual and stamps copied.')
