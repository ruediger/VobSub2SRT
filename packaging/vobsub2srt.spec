%global commit 1746781ee4a98d92d70ba7198246c58285296437
%global shortcommit %(c=%{commit}; echo ${c:0:7})
%global commitdate 20130216
%global gittag %{commitdate}git%{shortcommit}

Name:		vobsub2srt
Version:	1.0
Release:	1%{?dist}.%{gittag}
Summary:	VobSub2SRT .sub/.idx to .srt subtitle converter

Group:		Applications/Multimedia
License:	GPLv3+
URL:		https://github.com/ruediger/VobSub2SRT
Source0:	https://github.com/ruediger/VobSub2SRT/archive/%{commit}/%{name}-%{version}-%{shortcommit}.tar.gz

BuildRequires: cmake
BuildRequires: tesseract-devel
BuildRequires: libtiff-devel

%description
VobSub2SRT is a simple command line program to convert .idx / .sub subtitles
into .srt text subtitles by using OCR. It is based on code from the MPlayer
project.

%prep
%setup -qn VobSub2SRT-%{commit}

%build
mkdir -p %{_target_platform}
pushd %{_target_platform}
%{cmake} \
    -D INSTALL_DOC_DIR=%{_docdir}/%{name}-%{version} \
    ..
popd

make %{?_smp_mflags} -C %{_target_platform}

%install
make install DESTDIR=%{buildroot} -C %{_target_platform}
mv %{buildroot}/%{_docdir}/%{name}/copyright .
mv %{buildroot}/%{_docdir}/%{name}/README .


%files
%doc README copyright
%{_mandir}/man1/vobsub2srt.1.gz
%{_bindir}/vobsub2srt


%changelog
* Sat Feb 16 2013 Vinzenz Feenstra <evilissimo@gmail.com> - 1.0-1.20130216git1746781
- Initial package

