Name:       buteo-sync-plugins-email
Summary:    Syncs email accounts
Version:    0.1.10
Release:    1
License:    LGPLv2
URL:        https://github.com/sailfishos/buteo-sync-plugins-email
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(QmfClient)
BuildRequires:  pkgconfig(accounts-qt5)
BuildRequires:  nemo-qml-plugin-email-qt5-devel >= 0.6.0
BuildRequires:  buteo-syncfw-qt5-devel >= 0.10.0
Requires: buteo-syncfw-qt5-msyncd

%description
A plugin which provides data synchronization for email accounts.

%files
%defattr(-,root,root,-)
%license LICENSE.LGPL
%{_libdir}/buteo-plugins-qt5/oopp/libsyncemail-client.so
%config %{_sysconfdir}/buteo/profiles/client/*.xml
%config %{_sysconfdir}/buteo/profiles/sync/*.xml

%prep
%autosetup -n %{name}-%{version}

%build
%qmake5
%make_build

%install
%qmake5_install

%post
systemctl-user restart msyncd.service || :

