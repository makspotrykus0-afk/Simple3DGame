import os
import shutil

src_path = 'vendor/raylib/rmodels.c'
dst_path = 'raylib/src/rmodels.c'

# Ensure destination directory exists
os.makedirs(os.path.dirname(dst_path), exist_ok=True)

# Copy the file first because it seems truncated in the destination
shutil.copy2(src_path, dst_path)

# Now read and patch
path = dst_path
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Fix unused baseDir and missing CHDIR for OBJ loading
s1 = '    if (fileData != NULL)\n    {\n        int result = tinyobj_parse_obj'
r1 = '    if (fileData != NULL)\n    {\n        char currentDir[1024] = { 0 };\n        strcpy(currentDir, GetWorkingDirectory());\n        CHDIR(baseDir);\n\n        int result = tinyobj_parse_obj'

if s1 in content:
    content = content.replace(s1, r1)
    print("Applied patch part 1")
else:
    print("Failed to find patch part 1 target")

# Restore directory before unloading file data
s2 = '        UnloadFileData(fileData);'
r2 = '        CHDIR(currentDir);\n\n        UnloadFileData(fileData);'

if s2 in content:
    content = content.replace(s2, r2)
    print("Applied patch part 2")
else:
    print("Failed to find patch part 2 target")

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)