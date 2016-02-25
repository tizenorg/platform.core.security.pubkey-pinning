%define pubkey_pinning_test_build 0

Name:       pubkey-pinning
Summary:    Https Public Key Pinning for Tizen platform
Version:    0.0.3
Release:    0
Group:      Security/Libraries
License:    Apache-2.0 and BSD-2.0 and MPL-1.1
Source0:    %name-%version.tar.gz
Source1:    %name.manifest
Requires(post): /sbin/ldconfig
Requires(post): /usr/bin/systemctl
Requires(postun): /sbin/ldconfig
Requires(postun): /usr/bin/systemctl
Requires(preun): /usr/bin/systemctl
BuildRequires: cmake
BuildRequires: gettext-tools
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: pkgconfig(libiri)
BuildRequires: pkgconfig(libcurl)
BuildRequires: pkgconfig(gnutls)
BuildRequires: pkgconfig(openssl)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(libsystemd-daemon)
BuildRequires: pkgconfig(vconf)
%{?systemd_requires}

%description
Https Public Key Pinning for Tizen platform system framework.

%package devel
Summary:  Tizen HPKP library development files
Group:    Development/Libraries
Requires: %name = %version-%release

%description devel
Tizen HPKP library development files including headers and
pkgconfig.

%if 0%{?pubkey_pinning_test_build}
%package test
Summary:  Tizen HPKP library internal test
Group:    Security/Testing
BuildRequires: boost-devel
Requires: %name = %version-%release

%description test
Tizen HPKP library internal test with boost test framework.
%endif

%prep
%setup -q
cp %SOURCE1 .

%build
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"

export LDFLAGS+="-Wl,--rpath=%_prefix/lib"

%global sock_path /tmp/.%name-popup.sock

%{!?build_type:%define build_type "Release"}
%cmake . -DCMAKE_INSTALL_PREFIX=%_prefix \
        -DVERSION=%version               \
        -DINCLUDEDIR=%_includedir        \
        -DSOCK_PATH=%sock_path           \
        -DCMAKE_BUILD_TYPE=%build_type   \
%if 0%{?pubkey_pinning_test_build}
        -DPUBKEY_PINNING_TEST_BUILD=1    \
%endif
        -DCMAKE_VERBOSE_MAKEFILE=ON \
        -DSYSTEMD_UNIT_DIR=%_unitdir_user

make %{?_smp_mflags}

%install
%make_install
mkdir -p %buildroot%_unitdir_user/default.target.wants
mkdir -p %buildroot%_unitdir_user/sockets.target.wants
ln -sf ../%name-popup.service %buildroot%_unitdir_user/default.target.wants/%name-popup.service
ln -sf ../%name-popup.socket %buildroot%_unitdir_user/sockets.target.wants/%name-popup.socket
%find_lang %{name}

%post
/sbin/ldconfig
systemctl daemon-reload
if [ $1 == 1 ]; then
    systemctl restart %name-popup.service
fi

%preun
if [ $1 == 0 ]; then
    systemctl stop %name-popup.service
    systemctl disable %name-popup
fi

%postun
/sbin/ldconfig
systemctl daemon-reload

%files -f %{name}.lang
%manifest %name.manifest
%license LICENSE
%license LICENSE.BSD-3-Clause
%license LICENSE.MPL-1.1
%_libdir/libtpkp-common.so.*
%_libdir/libtpkp-curl.so.*
%_libdir/libtpkp-gnutls.so.*
%_unitdir_user/%name-popup.service
%_unitdir_user/%name-popup.socket
%_unitdir_user/default.target.wants/%name-popup.service
%_unitdir_user/sockets.target.wants/%name-popup.socket
%_bindir/tpkp-popup

%files devel
%_includedir/tpkp/common/tpkp_error.h
%_includedir/tpkp/curl/tpkp_curl.h
%_includedir/tpkp/gnutls/tpkp_gnutls.h
%_libdir/pkgconfig/tpkp-curl.pc
%_libdir/pkgconfig/tpkp-gnutls.pc
%_libdir/libtpkp-common.so
%_libdir/libtpkp-curl.so
%_libdir/libtpkp-gnutls.so

%if 0%{?pubkey_pinning_test_build}
%files test
%_bindir/tpkp-internal-test
%_bindir/tpkp-internal-test-popup
%endif
