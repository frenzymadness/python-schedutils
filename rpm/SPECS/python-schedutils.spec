%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}
%{!?python_ver: %define python_ver %(%{__python} -c "import sys ; print sys.version[:3]")}

Summary: Linux scheduler python bindings
Name: python-schedutils
Version: 0.5
Release: 1%{?dist}
License: GPLv2
URL: https://rt.wiki.kernel.org/index.php/Tuna
Source: https://cdn.kernel.org/pub/software/libs/python/%{name}/%{name}-%{version}.tar.xz
http://userweb.kernel.org/~acme/python-schedutils/%{name}-%{version}.tar.bz2
Group: System Environment/Libraries
BuildRequires: python-devel
BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
Python interface for the Linux scheduler sched_{get,set}{affinity,scheduler}
functions and friends.

%prep
%setup -q

%build
%{__python} setup.py build

%install
rm -rf %{buildroot}
%{__python} setup.py install --skip-build --root %{buildroot}
mkdir -p %{buildroot}%{_bindir}
cp -p pchrt.py %{buildroot}%{_bindir}/pchrt
cp -p ptaskset.py %{buildroot}%{_bindir}/ptaskset
mkdir -p %{buildroot}%{_mandir}/man1
gzip -c pchrt.1 > %{buildroot}%{_mandir}/man1/pchrt.1.gz
gzip -c ptaskset.1 > %{buildroot}%{_mandir}/man1/ptaskset.1.gz

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc COPYING README
%{_bindir}/pchrt
%{_bindir}/ptaskset
%{python_sitearch}/schedutils.so
%if "%{python_ver}" >= "2.5"
%{python_sitearch}/*.egg-info
%endif
%{_mandir}/man1/pchrt.1.gz
%{_mandir}/man1/ptaskset.1.gz

%changelog
* Thu Dec 22 2016 Jiri Kastner <jkastner@redhat.com> - 0.5-1
- added basic support for SCHED_DEADLINE
- fixed URL and Source in specfile

* Tue May 10 2016 John Kacur <jkacur@redhat.com> - 0.4-2
- Add man pages for pchrt and ptaskset
- Fix and update usage messages for pchrt and ptaskset

* Mon Aug  1 2011 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.4-1
- New upstream release.

* Tue May 17 2011 Clark Williams <williams@redhat.com> - 0.3-1
- reworked get_affinity() and set_affinity() to use dynamic CPU_* macros

* Thu Aug 28 2008 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.2-2
- Fix build and install sections as suggested by the fedora rewiewer
  (BZ #460387)

* Wed Aug 27 2008 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.2-1
- Add get_priority_{min,max} methods
- Add constants for SCHED_{BATCH,FIFO,OTHER,RR}
- Implement get_priority method
- Add pchrt utility for testing the bindings, chrt clone
- Add ptaskset utility for testing the bindings, taskset clone

* Tue Jun 10 2008 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-3
- add dist to the release tag

* Wed Dec 19 2007 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-2
- First build into rhel5-rt

* Wed Dec 19 2007 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-1
- Initial package
