import os
import xml.etree.ElementTree as ET

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
FAVICONS_DIR = os.path.join(SCRIPT_DIR, "..", "Data", "Favicons")
SERVERS_XML = os.path.join(SCRIPT_DIR, "..", "Data", "servers.xml")

IGNORED_FILES = {"default.ico", "directory.ico", "ftp.ico"}

def get_server_names(xml_path):
    tree = ET.parse(xml_path)
    root = tree.getroot()

    names = set()
    for server in root.iter("Server"):
        name = server.get("Name")
        if name:
            names.add(name.lower())
    return names

def get_ico_files(folder):
    return [
        f for f in os.listdir(folder)
        if f.lower().endswith(".ico") and f.lower() not in IGNORED_FILES
    ]

def main():
    server_names = get_server_names(SERVERS_XML)
    ico_files = get_ico_files(FAVICONS_DIR)

    orphan_icos = []
    for ico in ico_files:
        server_name = os.path.splitext(ico)[0].lower()
        if server_name not in server_names:
            orphan_icos.append(ico)

    if orphan_icos:
        print("Иконки, для которых нет сервера в servers.xml:")
        for ico in sorted(orphan_icos):
            print(f"  {ico}")
    else:
        print("Все иконки соответствуют серверам.")

if __name__ == "__main__":
    main()