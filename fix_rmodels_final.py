import os

file_path = 'raylib/src/rmodels.c'

with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Fix compilation error in UpdateModelAnimationBones
# Error: argument of type "int" is incompatible with parameter of type "Matrix"
# The issue is likely a mix of types or incorrect casting/function call
# Looking at the code, it seems like QuaternionToMatrix expects a Quaternion but maybe got something else?
# Actually, wait. Let's look at lines 2316 and 2322 again.
# 2316: QuaternionToMatrix(bindTransform->rotation)),
# 2322: QuaternionToMatrix(targetTransform->rotation)),
# If bindTransform->rotation is a Quaternion, this should be fine.
# Let's check if QuaternionToMatrix returns a Matrix. Yes it does.
# But wait, MatrixMultiply takes two Matrices.
# The structure is:
# MatrixMultiply(
#    MatrixMultiply(MatrixScale(...), QuaternionToMatrix(...)),
#    MatrixTranslate(...)
# )
# This looks correct structure-wise.
# The error says: argument of type "int" is incompatible...
# This usually happens if a function is not declared or returns int by default in C?
# But raymath.h is included.
# Maybe MatrixMultiply expects pointers? No, it expects values.
# Maybe QuaternionToMatrix is not defined? It is in raymath.h.

# Wait, looking at the error log again:
# 2316 | QuaternionToMatrix(bindTransform->rotation)), : argument of type "int" is incompatible with parameter of type "Matrix"
# This suggests that the RESULT of QuaternionToMatrix is being treated as an int? Or one of the arguments to MatrixMultiply is an int?
# Or maybe MatrixScale returns void? No.

# Let's check if raymath.h is included properly. Yes, line 54.

# However, there are other errors about 'currentDir' being undefined in many places.
# These are introduced by my previous patch which inserted CHDIR(currentDir) in places where currentDir was not defined.
# I need to remove those invalid CHDIR calls or define currentDir in those scopes.
# The previous patch was a bit aggressive and replaced strings in functions other than LoadOBJ presumably.

# Let's revert the invalid CHDIR(currentDir) insertions first.
# I will remove "CHDIR(currentDir);" from lines where it causes errors.

# Lines: 4657, 4666, 4984, 4993, 6452, 6563, 6574, 6896, 6925, 6937, 7013
# These are likely in LoadIQM, LoadGLTF, LoadVOX, LoadM3D etc. where I didn't define currentDir.
# I should only have applied it to LoadOBJ.

# Strategy:
# 1. Remove CHDIR(currentDir); from everywhere EXCEPT LoadOBJ.
# LoadOBJ is roughly lines 4290-4350.
# I will assume that any CHDIR(currentDir) outside of LoadOBJ is wrong.
# But wait, I can't easily know line numbers in replace.

# Let's look at the context.
# In LoadOBJ, we added:
# char currentDir[1024] = { 0 };
# strcpy(currentDir, GetWorkingDirectory());
# ...
# CHDIR(currentDir);

# If I search for CHDIR(currentDir) and check if currentDir is defined above it?
# Easier: I know I introduced it. I can revert it globally and then re-apply correctly ONLY to LoadOBJ.

# Reverting CHDIR(currentDir)
if 'CHDIR(currentDir);' in content:
    content = content.replace('CHDIR(currentDir);', '')
    print("Reverted global CHDIR(currentDir) insertions")

# Reverting CHDIR(currentDir) with newlines if necessary to clean up
content = content.replace('\n        \n        UnloadFileData(fileData);', '\n        UnloadFileData(fileData);')
content = content.replace('\n    \n    UnloadFileText(fileText);', '\n    UnloadFileText(fileText);')

# Now re-apply ONLY to LoadOBJ.
# I need to find LoadOBJ function body.
start_marker = 'static Model LoadOBJ(const char *fileName)'
end_marker = 'static Model LoadIQM(const char *fileName)'

start_idx = content.find(start_marker)
end_idx = content.find(end_marker)

if start_idx != -1 and end_idx != -1:
    load_obj_body = content[start_idx:end_idx]
    
    # Apply fixes strictly within this block
    
    # Fix 1: Restore dir on failure (if not already there)
    # The previous revert removed it, so we add it back.
    # Search for the failure return
    fail_search = 'TRACELOG(LOG_WARNING, "MODEL: Unable to read obj data %s", fileName);\n        return model;'
    fail_replace = 'TRACELOG(LOG_WARNING, "MODEL: Unable to read obj data %s", fileName);\n        CHDIR(currentDir);\n        return model;'
    
    if fail_search in load_obj_body:
        load_obj_body = load_obj_body.replace(fail_search, fail_replace)
        print("Re-applied Fix 1 to LoadOBJ")
        
    # Fix 2: Restore dir on success (before UnloadFileText)
    success_search = 'UnloadFileText(fileText);'
    # We need to be careful, UnloadFileText appears twice in LoadOBJ?
    # No, looking at the file read (lines 4290-4350), it appears once at line 4326.
    # Wait, line 4326 is BEFORE the loop.
    
    # Let's verify the structure from the read file:
    # 4318: int ret = tinyobj_parse_obj(...)
    # 4320: if (ret != TINYOBJ_SUCCESS) { ... return model; }
    # 4326: UnloadFileText(fileText);
    
    # So yes, replace UnloadFileText(fileText) with CHDIR(currentDir); UnloadFileText(fileText);
    # But only the first occurrence in this block?
    # The function returns model at the end, but UnloadFileText is called early.
    # Actually, is it called early? Yes.
    # So restoring directory there is correct.
    
    if success_search in load_obj_body:
        # Check if already applied (it shouldn't be after revert)
        if 'CHDIR(currentDir);\n    UnloadFileText(fileText);' not in load_obj_body:
             load_obj_body = load_obj_body.replace(success_search, 'CHDIR(currentDir);\n    UnloadFileText(fileText);')
             print("Re-applied Fix 2 to LoadOBJ")

    # Inject the patched body back
    content = content[:start_idx] + load_obj_body + content[end_idx:]

else:
    print("Could not isolate LoadOBJ function")

# Now about the compilation error in UpdateModelAnimationBones.
# Error: argument of type "int" is incompatible with parameter of type "Matrix"
# Line 2316: QuaternionToMatrix(bindTransform->rotation)),
# Line 2322: QuaternionToMatrix(targetTransform->rotation)),
#
# The code looks like this:
# Matrix bindMatrix = MatrixMultiply(MatrixMultiply(
#     MatrixScale(...),
#     QuaternionToMatrix(bindTransform->rotation)),
#     MatrixTranslate(...)
# );
#
# If this compiles on other platforms, maybe it's a macro expansion issue?
# Or maybe QuaternionToMatrix is NOT defined in this scope?
# It should be in raymath.h.
# raymath.h is included.
#
# Wait, maybe MatrixMultiply is defined as a macro that gets confused?
# Or maybe C compiler (MSVC) sees something else.
#
# Let's try to break it down into separate lines to debug/fix.
# Instead of nested calls:
# Matrix mScale = MatrixScale(...);
# Matrix mRot = QuaternionToMatrix(...);
# Matrix mTrans = MatrixTranslate(...);
# Matrix mTemp = MatrixMultiply(mScale, mRot);
# Matrix bindMatrix = MatrixMultiply(mTemp, mTrans);
#
# This is safer and easier to debug. I will apply this change.

search_complex_block = """                Matrix bindMatrix = MatrixMultiply(MatrixMultiply(
                    MatrixScale(bindTransform->scale.x, bindTransform->scale.y, bindTransform->scale.z),
                    QuaternionToMatrix(bindTransform->rotation)),
                    MatrixTranslate(bindTransform->translation.x, bindTransform->translation.y, bindTransform->translation.z));

                Transform *targetTransform = &anim.framePoses[frame][boneId];
                Matrix targetMatrix = MatrixMultiply(MatrixMultiply(
                    MatrixScale(targetTransform->scale.x, targetTransform->scale.y, targetTransform->scale.z),
                    QuaternionToMatrix(targetTransform->rotation)),
                    MatrixTranslate(targetTransform->translation.x, targetTransform->translation.y, targetTransform->translation.z));"""

replace_simple_block = """                Matrix bindScale = MatrixScale(bindTransform->scale.x, bindTransform->scale.y, bindTransform->scale.z);
                Matrix bindRot = QuaternionToMatrix(bindTransform->rotation);
                Matrix bindTrans = MatrixTranslate(bindTransform->translation.x, bindTransform->translation.y, bindTransform->translation.z);
                Matrix bindMatrix = MatrixMultiply(MatrixMultiply(bindScale, bindRot), bindTrans);

                Transform *targetTransform = &anim.framePoses[frame][boneId];
                Matrix targetScale = MatrixScale(targetTransform->scale.x, targetTransform->scale.y, targetTransform->scale.z);
                Matrix targetRot = QuaternionToMatrix(targetTransform->rotation);
                Matrix targetTrans = MatrixTranslate(targetTransform->translation.x, targetTransform->translation.y, targetTransform->translation.z);
                Matrix targetMatrix = MatrixMultiply(MatrixMultiply(targetScale, targetRot), targetTrans);"""

if search_complex_block in content:
    content = content.replace(search_complex_block, replace_simple_block)
    print("Applied simplification to UpdateModelAnimationBones")
else:
    # Try to match loosely if indentation or newlines vary
    # The original code has standard indentation.
    print("Could not find complex block in UpdateModelAnimationBones. Attempting partial match.")
    
    # Let's try just replacing the bindMatrix part
    part1 = """Matrix bindMatrix = MatrixMultiply(MatrixMultiply(
                    MatrixScale(bindTransform->scale.x, bindTransform->scale.y, bindTransform->scale.z),
                    QuaternionToMatrix(bindTransform->rotation)),
                    MatrixTranslate(bindTransform->translation.x, bindTransform->translation.y, bindTransform->translation.z));"""
    
    repl1 = """Matrix bindScale = MatrixScale(bindTransform->scale.x, bindTransform->scale.y, bindTransform->scale.z);
                Matrix bindRot = QuaternionToMatrix(bindTransform->rotation);
                Matrix bindTrans = MatrixTranslate(bindTransform->translation.x, bindTransform->translation.y, bindTransform->translation.z);
                Matrix bindMatrix = MatrixMultiply(MatrixMultiply(bindScale, bindRot), bindTrans);"""

    if part1 in content:
        content = content.replace(part1, repl1)
        print("Part 1 applied")
    
    part2 = """Matrix targetMatrix = MatrixMultiply(MatrixMultiply(
                    MatrixScale(targetTransform->scale.x, targetTransform->scale.y, targetTransform->scale.z),
                    QuaternionToMatrix(targetTransform->rotation)),
                    MatrixTranslate(targetTransform->translation.x, targetTransform->translation.y, targetTransform->translation.z));"""
    
    repl2 = """Matrix targetScale = MatrixScale(targetTransform->scale.x, targetTransform->scale.y, targetTransform->scale.z);
                Matrix targetRot = QuaternionToMatrix(targetTransform->rotation);
                Matrix targetTrans = MatrixTranslate(targetTransform->translation.x, targetTransform->translation.y, targetTransform->translation.z);
                Matrix targetMatrix = MatrixMultiply(MatrixMultiply(targetScale, targetRot), targetTrans);"""

    if part2 in content:
        content = content.replace(part2, repl2)
        print("Part 2 applied")


with open(file_path, 'w', encoding='utf-8') as f:
    f.write(content)