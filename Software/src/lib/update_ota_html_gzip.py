import json
import gzip
from pathlib import Path

script_dir = Path(__file__).parent
libpath = script_dir / "ayushsharma82-ElegantOTA"

plain_html = libpath / "CurrentPlainHTML.txt"
gzipped = libpath / "CurrentPlainHTML.txt.gz"
header = libpath / "src/elop.h"
cpp = libpath / "src/elop.cpp"

if not plain_html.exists():
    print(f"❌ Error: Could not find {plain_html.resolve()}")
    raise SystemExit(1)

print("📦 Compressing HTML file...")
with open(plain_html, 'rb') as f_in:
    with gzip.open(gzipped, 'wb', compresslevel=9) as f_out:
        f_out.writelines(f_in)

gzipbytes = gzipped.read_bytes()
intlist = [int(one) for one in gzipbytes]
content = json.dumps(intlist).replace("[","{").replace("]","}").replace(" ", "")

headertext = header.read_text()
header.write_text(headertext[:1+headertext.find("[")]+str(len(gzipbytes))+headertext[headertext.find("]"):])

cpptext = cpp.read_text()
first_bracket = cpptext.find("[")
second_bracket = cpptext.find("]")
corrected_bytes = cpptext[:1+first_bracket]+str(len(gzipbytes))+cpptext[second_bracket:]
cppout = corrected_bytes[:corrected_bytes.find("{")]+content+corrected_bytes[corrected_bytes.find(";"):]+"\n"
cpp.write_text(cppout)

print("✅ Success! File content updated from:", gzipped.resolve())
print("📊 Bytes fixed:", cpptext[1+first_bracket:second_bracket], "to", len(gzipbytes))