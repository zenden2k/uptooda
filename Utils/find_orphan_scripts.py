import os
import xml.etree.ElementTree as ET

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.join(SCRIPT_DIR, "..", "Data", "Scripts")
SERVERS_XML = os.path.join(SCRIPT_DIR, "..", "Data", "servers.xml")

IGNORED_DIRS = {"Test", "UploadFilters", "Utils"}
IGNORED_FILES = {"directory.nut", "ftp.nut", "sftp.nut", "webdav.nut"}

def get_plugin_names(xml_path):
    tree = ET.parse(xml_path)
    root = tree.getroot()

    names = set()
    for server in root.iter("Server"):
        plugin = server.get("Plugin")
        if plugin:
            names.add(plugin.lower())
    return names

def get_nut_files(folder):
    nut_files = []
    for dirpath, dirnames, filenames in os.walk(folder):
        # Исключаем игнорируемые подпапки из дальнейшего обхода
        dirnames[:] = [d for d in dirnames if d not in IGNORED_DIRS]

        for filename in filenames:
            if filename.lower().endswith(".nut") and filename.lower() not in IGNORED_FILES:
                full_path = os.path.join(dirpath, filename)
                rel_path = os.path.relpath(full_path, folder)
                rel_path = rel_path.replace(os.sep, "/")
                nut_files.append(rel_path)
    return nut_files

def main():
    plugin_names = get_plugin_names(SERVERS_XML)
    nut_files = get_nut_files(SCRIPTS_DIR)

    orphan_nuts = []
    for nut in nut_files:
        plugin_name = nut[:-4].lower()  # убираем ".nut"
        if plugin_name not in plugin_names:
            orphan_nuts.append(nut)

    if orphan_nuts:
        print("Скрипты, для которых нет сервера в servers.xml:")
        for nut in sorted(orphan_nuts):
            print(f"  {nut}")
    else:
        print("Все скрипты соответствуют серверам.")

if __name__ == "__main__":
    main()