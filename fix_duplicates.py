
import os
file_path = 'game/Settler.cpp'

with open(file_path, 'r', encoding='utf-8') as f:

lines = f.readlines()
cutoff_index = 1398

new_lines = lines[:cutoff_index]
new_lines.append("\n")

new_lines.append("    m_state = SettlerState::IDLE;\n")

new_lines.append("}\n")
with open(file_path, 'w', encoding='utf-8') as f:

f.writelines(new_lines)

print(f"Fixed {file_path}")
file_path = 'systems/UISystem.cpp'

with open(file_path, 'r', encoding='utf-8') as f:

lines = f.readlines()
cutoff_index = 1423

new_lines = lines[:cutoff_index]
new_lines.append('    DrawText(owner.c_str(), x + 10, y + 30, 10, WHITE);\n')

new_lines.append('}\n')
with open(file_path, 'w', encoding='utf-8') as f:

f.writelines(new_lines)

print(f"Fixed {file_path}")
