#!/bin/sh
# POST_BUILD guard for orbis/orbis_wchar32.c.
#
#   check-wchar-link.sh <why-extract.txt>
#
# The SDK's libc.a was built with a 16-bit wchar_t. orbis_wchar32.c (an object, so it wins) replaces
# its wide-character members; the regcomp/regexec/vfscanf members are linked as renamed copies (see
# CMakeLists.txt). This fails the build if the linker nevertheless EXTRACTED one of those libc.a
# members - a newly referenced wide symbol that orbis_wchar32.c does not define, or a libc member that
# passes a 16-bit wchar_t buffer to the (now 32-bit) mbtowc/mbrtowc. --allow-multiple-definition,
# which the Mesa link needs, would let either case through silently.
set -eu
why="$1"
[ -s "$why" ] || { echo "!! check-wchar-link: no linker --why-extract output at $why" >&2; exit 1; }

members='btowc c16rtomb c32rtomb mblen mbrlen mbrtoc16 mbrtoc32 mbrtowc mbsinit mbsnrtowcs mbsrtowcs
mbstowcs mbtowc wcrtomb wcsnrtombs wcsrtombs wcstombs wctob wctomb __ctype_get_mb_cur_max
wcpcpy wcpncpy wcscasecmp wcscasecmp_l wcscat wcschr wcscmp wcscpy wcscspn wcsdup wcslen wcsncasecmp
wcsncasecmp_l wcsncat wcsncmp wcsncpy wcsnlen wcspbrk wcsrchr wcsspn wcsstr wcstok wcswcs wmemchr
wmemcmp wmemcpy wmemmove wmemset wcscoll wcsxfrm iswspace wcswidth wcwidth wcstod wcstol wcsftime
swprintf vswprintf fnmatch getopt iconv regcomp regexec vfscanf
fgetwc fgetws fputwc fputws ungetwc fwide vfwprintf vfwscanf vswscanf swscanf fwprintf fwscanf
wprintf wscanf vwprintf vwscanf getwc getwchar putwc putwchar open_wmemstream'

bad=""
for m in $members; do
	if grep -F "libc.a(${m}.lo)" "$why" | awk -F'\t' -v m="libc.a(${m}.lo)" 'index($2, m) { found=1 } END { exit !found }'; then
		bad="$bad $m.lo"
	fi
done
if [ -n "$bad" ]; then
	echo "!! 16-bit wchar_t libc.a members were linked:$bad" >&2
	echo "!! see Oxygen/sonic3air/build/_ps4/orbis/orbis_wchar32.c - define the symbol there (or add a renamed copy)." >&2
	grep -F -e "$(echo "$bad" | sed 's/ \([^ ]*\)/libc.a(\1)\n/g' | sed '/^$/d')" "$why" >&2 || true
	exit 1
fi
echo "check-wchar-link: no 16-bit wchar_t libc.a member extracted"
