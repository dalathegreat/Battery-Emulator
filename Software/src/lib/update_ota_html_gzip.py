import json
from pathlib import Path
libpath = Path("ayushsharma82-ElegantOTA")
gzipped=libpath/"CurrentPlainHTML.txt.gz"
header=libpath/"src/elop.h"
cpp=libpath/"src/elop.cpp"
if not gzipped.exists():
    print(f"Please create {gzipped.resolve()} to replace OTA file.")
    print(f"Example: zopfli -v --i10000 {libpath.resolve()}/CurrentPlainHTML.txt")
    raise SystemExit(1)
gzipbytes=gzipped.read_bytes()
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
print("File content updated from", gzipped.resolve())
print("Bytes fixed:", cpptext[1+first_bracket:second_bracket], "to", len(gzipbytes))
