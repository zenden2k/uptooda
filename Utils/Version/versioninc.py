import sys
import re
import datetime
import subprocess

if len(sys.argv) < 2:
    print ("You should provide path to versioninfo.h as argument.")
    sys.exit(1)
filename = sys.argv[1]

if len(sys.argv) == 3 and sys.argv[2]!= "1" and sys.argv[2] != "Release" and sys.argv[2] != "MinSizeRel":
    print ("versioninc.py: command ommitted for non-release build")
    sys.exit(0)

with open(filename) as f:
    content = f.readlines()
    
content = [x.strip() for x in content] 
reg = re.compile("#define ([a-zA-Z0-9_]+) \"(.*?)\"")
out_text = ""
for line in content:
    res = reg.match(line)
    if res:
        define_name = res.group(1) 
        if define_name == "IU_BUILD_NUMBER":
            build_number = int(res.group(2))+1
            print ("New IU build: {}".format(build_number))
            out_text += "#define {} \"{}\"\n".format(define_name, str(build_number))
        elif define_name == "IU_BUILD_DATE":
            now = datetime.datetime.now()
            out_text += "#define {} \"{}\"\n".format(define_name, now.strftime("%d.%m.%Y"))   
        elif define_name == "IU_COMMIT_HASH":
            git_hash = subprocess.check_output(['git', 'rev-parse', 'HEAD']).decode('utf-8').strip()
            out_text += "#define {} \"{}\"\n".format(define_name, git_hash)
        elif define_name == "IU_COMMIT_HASH_SHORT":
            git_hash = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode('utf-8').strip()
            out_text += "#define {} \"{}\"\n".format(define_name, git_hash)              
        elif define_name == "IU_BRANCH_NAME":        
            git_branch_name = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).decode('utf-8').strip()
            out_text += "#define {} \"{}\"\n".format(define_name, git_branch_name)  
        else:
            out_text += line + "\n"
    else:
         out_text += line + "\n"
        
text_file = open(filename, "w")
text_file.write(out_text)
text_file.close()
print ("Successfully generated versioninfo.h")