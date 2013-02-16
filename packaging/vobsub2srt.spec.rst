    pushd ~/rpmbuild/SOURCE
    wget https://github.com/ruediger/VobSub2SRT/archive/1746781ee4a98d92d70ba7198246c58285296437/vobsub2srt-0.1-1746781.tar.gz
    popd
    rpmbuild -ba vobsub2srt.spec
    yum install -y `find ~/rpmbuild/RPMS -name vobsub2srt-0.1.*`

