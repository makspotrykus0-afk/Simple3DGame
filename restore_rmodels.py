import shutil
import os

src = 'vendor/raylib/rmodels.c'
dst = 'raylib/src/rmodels.c'

try:
    if os.path.exists(src):
        shutil.copyfile(src, dst)
        print(f"Successfully restored {dst} from {src}")
    else:
        print(f"Error: Source file {src} not found.")
except Exception as e:
    print(f"Error restoring file: {e}")