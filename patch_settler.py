
import os
file_path = 'game/Settler.cpp'

search_text = 'if (skill.priority <= 0) continue;'

replace_text = '// if (skill.priority <= 0) continue; // Priority check removed to allow tasks enabled via UI checkboxes'
with open(file_path, 'r', encoding='utf-8') as f:

content = f.read()
if search_text in content:

new_content = content.replace(search_text, replace_text)

with open(file_path, 'w', encoding='utf-8') as f:

f.write(new_content)

print(f"Successfully replaced text in {file_path}")

else:

print(f"Could not find exact text: '{search_text}'")

idx = content.find("sortedSkills")

if idx != -1:

print("Context around 'sortedSkills':")

print(content[idx:idx+200])
