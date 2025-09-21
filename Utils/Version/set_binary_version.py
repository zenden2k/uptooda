import sys
import re
import datetime
import subprocess

if len(sys.argv) < 3:
    print ("You should provide path to file versioninfo.h as first argument and path to exe or dll as second argument.")
    sys.exit(1)
filename = sys.argv[1]
exename = sys.argv[2]

if len(sys.argv) == 4 and sys.argv[3]!= "1" and sys.argv[3] != "Release" and sys.argv[3] != "MinSizeRel":
    print ("set_binary_version.py: command ommitted for non-release build")
    sys.exit(0)
    
with open(filename) as f:
    content = f.readlines()

params = {}
    
content = [x.strip() for x in content] 
reg = re.compile("#define ([a-zA-Z0-9_]+) \"(.*?)\"")
out_text = ""
for line in content:
    res = reg.match(line)
    if res:
        params[res.group(1)] = res.group(2)
        
version = params["IU_APP_VER_CLEAN"]
tokens = version.split('.')
ver_major = int(tokens[0])
ver_minor = int(tokens[1])
ver_release = int(tokens[2].replace("-beta", "").replace("-RC", ""))
ver_build = int(params["IU_BUILD_NUMBER"])
file_version = "{}.{}.{}.{}".format(ver_major, ver_minor, ver_release, ver_build)
rcedit_path = sys.path[0] + "\\rcedit.exe"
rcedit_arguments =  [rcedit_path, exename, '--set-file-version', file_version, '--set-product-version', file_version]
if "openssl" in exename and ".exe" in exename:
    rcedit_arguments += ["--set-version-string", "FileDescription", "Image Uploader (with OpenSSL)"]
#print rcedit_arguments
print(subprocess.check_output(rcedit_arguments).strip())
print ("Successfully patched")