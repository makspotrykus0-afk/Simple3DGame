import os

file_path = 'raylib/src/rmodels.c'

with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Target block from LoadOBJ
target_block = """    if (ret != TINYOBJ_SUCCESS)
    {
        TRACELOG(LOG_WARNING, "MODEL: Unable to read obj data %s", fileName);
        return model;
    }

    UnloadFileText(fileText);"""

replacement_block = """    if (ret != TINYOBJ_SUCCESS)
    {
        TRACELOG(LOG_WARNING, "MODEL: Unable to read obj data %s", fileName);
        CHDIR(currentDir);
        return model;
    }

    CHDIR(currentDir);
    UnloadFileText(fileText);"""

if target_block in content:
    content = content.replace(target_block, replacement_block)
    print("Applied LoadOBJ CHDIR fix")
else:
    print("Target block not found in LoadOBJ. Content might be different.")
    # Debug: print nearby content
    idx = content.find('if (ret != TINYOBJ_SUCCESS)')
    if idx != -1:
        print("Found 'if (ret != TINYOBJ_SUCCESS)' at", idx)
        print("Context:", content[idx:idx+200])
    else:
        print("Could not find start of block.")

with open(file_path, 'w', encoding='utf-8') as f:
    f.write(content)