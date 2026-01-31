# merge_blocks.py
import re
import json
from collections import defaultdict

input_file = "snbt_convert.txt"
output_file = "block_state_templates.json"


templates = defaultdict(list)

def parse_state(block_line):
    m = re.match(r'.*?\[(.*)\]', block_line)
    if not m:
        return {}
    states_str = m.group(1)
    pairs = states_str.split(",")
    state_dict = {}
    for p in pairs:
        k, v = p.split("=")
        state_dict[k.strip()] = v.strip().strip('"')
    return state_dict

def parse_block_name(line):
    if "[" in line:
        return line[:line.find("[")]
    else:
        return line.strip()

with open(input_file, "r", encoding="utf-8") as f:
    lines = [l.strip() for l in f if l.strip()]

for i in range(0, len(lines), 3):
    in_line = lines[i][5:].strip()
    uni_line = lines[i+1][4:].strip()
    out_line = lines[i+2][4:].strip()

    base_in = parse_block_name(in_line)
    base_uni = parse_block_name(uni_line)

    in_state = parse_state(in_line)
    uni_state = parse_state(uni_line)
    out_state = parse_state(out_line)

    key_str = f"{base_in}||{base_uni}"  # tuple 改为字符串
    templates[key_str].append({
        "in_state": in_state,
        "uni_state": uni_state,
        "out_state": out_state
    })

with open(output_file, "w", encoding="utf-8") as f:
    json.dump(templates, f, indent=2)