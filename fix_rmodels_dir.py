import os

file_path = 'raylib/src/rmodels.c'

with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Fix 1: Restore directory on failure
search_fail = 'TRACELOG(LOG_WARNING, "MODEL: Unable to read obj data %s", fileName);\n        return model;'
replace_fail = 'TRACELOG(LOG_WARNING, "MODEL: Unable to read obj data %s", fileName);\n        CHDIR(currentDir);\n        return model;'

if search_fail in content:
    content = content.replace(search_fail, replace_fail)
    print("Applied Fix 1 (Restore dir on failure)")
else:
    print("Fix 1 not applied (pattern not found)")

# Fix 2: Restore directory on success (before UnloadFileText)
search_success = 'UnloadFileText(fileText);'
replace_success = 'CHDIR(currentDir);\n    UnloadFileText(fileText);'

# Check if it's already patched to avoid double patching or misplacement
if 'CHDIR(currentDir);\n    UnloadFileText(fileText);' not in content:
    # We need to be careful not to replace every UnloadFileText, only the one in LoadOBJ.
    # Since we read the whole file, searching for just UnloadFileText(fileText) is risky as it might appear elsewhere.
    # However, in LoadOBJ it appears right after the check.
    
    # Let's try to find a larger block to be unique.
    block_search = '    if (ret != TINYOBJ_SUCCESS)\n    {\n        TRACELOG(LOG_WARNING, "MODEL: Unable to read obj data %s", fileName);\n        CHDIR(currentDir);\n        return model;\n    }\n\n    UnloadFileText(fileText);'
    # Note: I used the result of Fix 1 in the search block for Fix 2 context, assuming Fix 1 ran.
    
    # Alternative: Just use a unique enough context around UnloadFileText inside LoadOBJ.
    # The code has:
    #     if (ret != TINYOBJ_SUCCESS)
    #     {
    #         ...
    #     }
    #
    #     UnloadFileText(fileText);
    
    # Let's try to match the unique variable name 'ret' check closing brace.
    
    unique_search = '    if (ret != TINYOBJ_SUCCESS)\n    {\n        TRACELOG(LOG_WARNING, "MODEL: Unable to read obj data %s", fileName);\n        CHDIR(currentDir);\n        return model;\n    }\n\n    UnloadFileText(fileText);'
    unique_replace = '    if (ret != TINYOBJ_SUCCESS)\n    {\n        TRACELOG(LOG_WARNING, "MODEL: Unable to read obj data %s", fileName);\n        CHDIR(currentDir);\n        return model;\n    }\n\n    CHDIR(currentDir);\n    UnloadFileText(fileText);'
    
    if unique_search in content:
        content = content.replace(unique_search, unique_replace)
        print("Applied Fix 2 (Restore dir on success)")
    else:
        # If Fix 1 wasn't applied (already present?), try finding the original block
        unique_search_orig = '    if (ret != TINYOBJ_SUCCESS)\n    {\n        TRACELOG(LOG_WARNING, "MODEL: Unable to read obj data %s", fileName);\n        return model;\n    }\n\n    UnloadFileText(fileText);'
        unique_replace_orig = '    if (ret != TINYOBJ_SUCCESS)\n    {\n        TRACELOG(LOG_WARNING, "MODEL: Unable to read obj data %s", fileName);\n        CHDIR(currentDir);\n        return model;\n    }\n\n    CHDIR(currentDir);\n    UnloadFileText(fileText);'
        
        if unique_search_orig in content:
            content = content.replace(unique_search_orig, unique_replace_orig)
            print("Applied Fix 1 & 2 together")
        else:
            print("Fix 2 not applied (pattern not found)")

with open(file_path, 'w', encoding='utf-8') as f:
    f.write(content)