#!/bin/bash
find ../Source/ -type d \( -path  "../Source/qimageuploader" -o -path  "../Source/QtIUHelper" -\) -prune -o -type f \( -iname \*.cpp -o -iname \*.h \)  -print | xargs -d '\n' xgettext   --from-code=UTF-8 --default-domain=imageuploader --keyword=_ --keyword=_c:1c,2 --keyword=TRCC:2c,3 --keyword=_n:1,2 --keyword=_nc:1c,2,3 --keyword=tr --keyword=TR --keyword=TR_COND --keyword=TRC:2 --keyword=TR_CONST --language=C++ --sort-output --package-name=imageuploader --copyright-holder="Sergey Svistunov <zenden2k@gmail.com>" -o imageuploader.pot
for PO_FILE in locale/*/LC_MESSAGES/imageuploader.po
do
    msgmerge -U --previous --backup=off "$PO_FILE" imageuploader.pot
done