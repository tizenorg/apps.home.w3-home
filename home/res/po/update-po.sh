#!/bin/sh

PACKAGE=volume
SRCROOT=../src
POTFILES=POTFILES.in

#ALL_LINGUAS= ar bg ca cs da de_DE el_GR en en_GB en_US es_ES fi fr_FR he hi hr hu id it_IT ja_JP ko_KR lt lv ms nl_NL no pl pt_PT ro ru_RU sk sl sr sv th tr_TR uk vi zh_CN zh_HK zh_TW
ALL_LINGUAS="ar bg ca cs da de_DE el_GR en en_GB en_US es_ES fi fr_FR he hi hr hu id it_IT ja_JP ko_KR lt lv ms nl_NL no pl pt_PT ro ru_RU sk sl sr sv th tr_TR uk vi zh_CN zh_HK zh_TW"

XGETTEXT=/usr/bin/xgettext
MSGMERGE=/usr/bin/msgmerge

echo -n "Make ${PACKAGE}.pot  "
if [ ! -e $POTFILES ] ; then
	echo "$POTFILES not found"
	exit 1
fi

$XGETTEXT --default-domain=${PACKAGE} --directory=${SRCROOT} \
		--add-comments --keyword=_ --keyword=N_ --files-from=$POTFILES
if [ $? -ne 0 ]; then
	echo "xgettext error"
	exit 1
fi

if [ ! -f ${PACKAGE}.po ]; then
	echo "No such file: ${PACKAGE}.po"
	exit 1
fi

rm -f ${PACKAGE}.pot && mv ${PACKAGE}.po ${PACKAGE}.pot
echo "done"

for LANG in $ALL_LINGUAS; do
	echo "$LANG : "

	if [ ! -e $LANG.po ] ; then
		sed 's/CHARSET/UTF-8/g' ${PACKAGE}.pot > ${LANG}.po
		echo "${LANG}.po created"
	else
		if $MSGMERGE ${LANG}.po ${PACKAGE}.pot -o ${LANG}.new.po ; then
			if cmp ${LANG}.po ${LANG}.new.po > /dev/null 2>&1; then
				rm -f ${LANG}.new.po
			else
				if mv -f ${LANG}.new.po ${LANG}.po; then
					echo ""
				else
					echo "msgmerge for $LANG.po failed: cannot move $LANG.new.po to $LANG.po" 1>&2
					rm -f ${LANG}.new.po
					exit 1
				fi
			fi
		else
			echo "msgmerge for $LANG failed!"
			rm -f ${LANG}.new.po
		fi
	fi
	echo ""
done

