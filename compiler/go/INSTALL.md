Apache Clownfish symbiotic object system -- CFC compiler Go bindings
--------------------------------------------------------------------

To install from an official release of Clownfish:

    mkdir -p $GOPATH/src/git-wip-us.apache.org/repos/asf/lucy.git
    tar -C $GOPATH/src/git-wip-us.apache.org/repos/asf/lucy.git \
        -zxf apache-clownfish-0.6.0.tgz

Developers actively working on Clownfish may instead clone from the Clownfish
Git repository:

    mkdir -p $GOPATH/src/git-wip-us.apache.org/repos/asf
    cd $GOPATH/src/git-wip-us.apache.org/repos/asf
    git clone https://git-wip-us.apache.org/repos/asf/lucy-clownfish.git lucy-clownfish.git

Once the source code is in place, from the `compiler/go` directory within the
checkout, run `build.go`:

    cd $GOPATH/src/git-wip-us.apache.org/repos/asf/lucy-clownfish.git/compiler/go
    go run build.go
    go run build.go test
    go run build.go install
    go run build.go clean

