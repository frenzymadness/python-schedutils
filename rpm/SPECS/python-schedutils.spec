%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}
%{!?python_ver: %define python_ver %(%{__python} -c "import sys ; print sys.version[:3]")}

Summary: Linux scheduler python bindings
Name: python-schedutils
Version: 0.1
Release: 3%{?dist}
Source0: %{name}-%{version}.tar.bz2
License: GPLv2+
Group: System Environment/Libraries
BuildRequires: python-devel
BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
Python interface for the Linux scheduler sched_{get,set}{affinity,scheduler}
functions and friends.

%prep
%setup -q

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=${RPM_BUILD_ROOT} install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc COPYING
%{python_sitearch}/schedutils.so
%if "%{python_ver}" >= "2.5"
%{python_sitearch}/*.egg-info
%endif

%changelog
* Tue Jun 10 2008 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-3
- add dist to the release tag

* Tue Dec 19 2007 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-2
- First build into rhel5-rt

* Tue Dec 19 2007 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-1
- Initial package
