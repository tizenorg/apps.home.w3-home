Name:	org.tizen.w-home
Summary:	Home for the wearable devices
Version:	0.1.0
Release:	1
Group:	Applications/System
License:	Flora-1.1
Source0:	%{name}-%{version}.tar.gz

%if "%{?tizen_profile_name}"=="mobile"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires: cmake, gettext-tools, smack, coreutils
BuildRequires: pkgconfig(ail)
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(badge)
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-media-player)
BuildRequires: pkgconfig(capi-media-sound-manager)
BuildRequires: pkgconfig(capi-appfw-preference)
BuildRequires: pkgconfig(capi-system-runtime-info)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(deviced)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(ecore-x)
BuildRequires: pkgconfig(efl-assist)
BuildRequires: pkgconfig(efl-extension)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(feedback)
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: pkgconfig(widget_service)
BuildRequires: pkgconfig(widget_viewer_evas)
BuildRequires: pkgconfig(minicontrol-monitor)
BuildRequires: pkgconfig(minicontrol-viewer)
BuildRequires: pkgconfig(pkgmgr)
BuildRequires: pkgconfig(pkgmgr-info)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(utilX)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(appsvc)
BuildRequires: pkgconfig(capi-base-utils-i18n)
BuildRequires: pkgconfig(capi-system-system-settings)
BuildRequires: pkgconfig(capi-message-port)
BuildRequires: pkgconfig(capi-media-image-util)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(rua)

%ifarch %{arm}
%define ARCH arm
BuildRequires: pkgconfig(dbus-1)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(dbus-glib-1)
BuildRequires: pkgconfig(json)
BuildRequires: pkgconfig(journal)
BuildRequires: pkgconfig(tapi)
%else
%define ARCH emulator
%endif

BuildRequires: cmake
BuildRequires: edje-bin
BuildRequires: embryo-bin
BuildRequires: gettext-devel
BuildRequires: hash-signer
BuildRequires: model-build-features

%description
Home for wearable devices

%prep
%setup -q

%define PREFIX /usr/apps/%{name}
%define DATADIR /opt%{PREFIX}/data

%build

%if "%{model_build_feature_formfactor}" == "circle"
	%define CIRCLE "circle"
%else
	%define CIRCLE "rectangle"
%endif

%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif

%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif
RPM_OPT=`echo $CFLAGS|sed 's/-Wp,-D_FORTIFY_SOURCE=2//'`
export CFLAGS=$RPM_OPT
cmake  -DCMAKE_INSTALL_PREFIX="%{PREFIX}" -DARCH=%{ARCH} -DCIRCLE="%{CIRCLE}"
make %{?jobs:-j%jobs}



%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
mkdir -p %{buildroot}/opt/usr/share/w-launcher
%define tizen_sign 1
%define tizen_sign_base /usr/apps/%{name}
%define tizen_sign_level platform
%define tizen_author_sign 1
%define tizen_dist_sign 1

%post
/usr/bin/signing-client/hash-signer-client.sh -a -d -p platform /usr/apps/%{name}

INHOUSE_ID="5000"
make_data_directory()
{
	I="%{DATADIR}"
	if [ ! -d $I ]; then
		mkdir -p $I
	fi
	chmod 775 $I
	chown :$INHOUSE_ID $I
}
make_data_directory

sqlite3 %{DATADIR}/.home.db 'PRAGMA journal_mode = PERSIST;
	CREATE TABLE IF NOT EXISTS home (
		id			TEXT,
		subid		TEXT,
		ordering	INTEGER
	);
'
chmod 666 %{DATADIR}/.home.db*

sqlite3 %{DATADIR}/.home_tts.db 'PRAGMA journal_mode = PERSIST;
	CREATE TABLE IF NOT EXISTS home (
		id			TEXT,
		subid		TEXT,
		ordering	INTEGER
	);
'
chmod 666 %{DATADIR}/.home_tts.db*

# apps
sqlite3 %{DATADIR}/.apps.db 'PRAGMA journal_mode = PERSIST;
	CREATE TABLE IF NOT EXISTS apps (
		id			TEXT NOT NULL PRIMARY KEY,
		ordering	INTEGER
	);
'
chmod 666 %{DATADIR}/.apps.db*

if [ -f /usr/lib/rpm-plugins/msm.so ]
then
chsmack -a '%{name}' /opt%{PREFIX}
chsmack -a '%{name}' %{DATADIR}
chsmack -a '%{name}' %{DATADIR}/.home.db*
chsmack -a '%{name}' %{DATADIR}/.home_tts.db*
chsmack -a '%{name}' %{DATADIR}/.apps.db*
fi

vconftool set -t int "memory/private/org.tizen.w-home/tutorial" 0 -i -g $INHOUSE_ID -f -s %{name}
vconftool set -t int "db/private/org.tizen.w-home/enabled_tutorial" 0 -g $INHOUSE_ID -f -s %{name}
vconftool set -t int "db/private/org.tizen.w-home/apps_first_boot" 1 -g $INHOUSE_ID -f -s %{name}
vconftool set -t int "db/private/org.tizen.w-home/apps_flickup_count" 0 -g $INHOUSE_ID -f -s %{name}
vconftool set -t int "db/private/org.tizen.w-home/apps_initial_popup" 0 -g $INHOUSE_ID -f -s %{name}
vconftool set -t string "db/private/org.tizen.w-home/logging" ";" -g $INHOUSE_ID -f -s system::vconf_system
vconftool set -t int "memory/homescreen/clock_visibility" 0 -i -g $INHOUSE_ID -f -s system::vconf_system
vconftool set -t string "memory/homescreen/music_status" ";" -i -g $INHOUSE_ID -f -s system::vconf_system
vconftool set -t int "memory/private/org.tizen.w-home/auto_feed" 1 -i -g $INHOUSE_ID -f -s %{name}
vconftool set -t int "memory/private/org.tizen.w-home/sensitive_move" 1 -i -g $INHOUSE_ID -f -s %{name}
vconftool set -t string "db/wms/clocks_set_idle" "org.tizen.idle-clock-digital" -u $INHOUSE_ID -s system::vconf

%files
%manifest home/%{name}.manifest
%defattr(-,root,root,-)
/usr/share/license/%{name}
/usr/share/packages/%{name}.xml
/usr/share/icons/default/small/%{name}*.png
/etc/smack/accesses.d/%{name}.efl
/etc/opt/upgrade/*.sh
%{PREFIX}/*.xml
%{PREFIX}/bin/*
%{PREFIX}/shared/*
%{PREFIX}/res/*.xml
%{PREFIX}/res/*.list
%{PREFIX}/res/*.sh
%{PREFIX}/res/edje/*
%{PREFIX}/res/images/*
%{PREFIX}/res/locale/*/*/*.mo
/opt/etc/dump.d/module.d/dump_w-home.sh
