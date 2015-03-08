#
# Spec file for package monav-light
#

Name:           monav-light
Version:        0.1
Release:        1
Url:            https://github.com/M4rtinK/monav-light
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  qt5-qttools
BuildRequires:  pkgconfig(Qt5Core)
Source:         %{name}-%{version}.tar.gz
Summary:        Lightweight Monav based offline routing software
License:        GPL-3.0+
Group:          Productivity/Other

%description
Monav-light is a lightweight and portable offline routing software based on MoNav,
working with preprocessed OpenStreetMap data and using JSON for input and output.

%prep
%setup -q

%build
export CFLAGS="$RPM_BUILD_OPTS -g"
qmake-qt5 QMAKE_CXXFLAGS_RELEASE+="$CFLAGS" monav-light.pro
make

%install
install -d -m 0755 $RPM_BUILD_ROOT/usr/bin
install -m 0755 bin/monav* $RPM_BUILD_ROOT/usr/bin/

%clean
rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(-,root,root,-)
/usr/bin/*

%changelog
