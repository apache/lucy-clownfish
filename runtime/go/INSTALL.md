Apache Clownfish symbiotic object system -- runtime Go bindings
---------------------------------------------------------------

As a prerequisite, install the Go bindings for the Clownfish compiler (CFC).
This will entail installing the shared source tree for Clownfish, which
includes the source code for both the compiler and the runtime.

After installing CFC, switch to the `runtime/go` directory and run `build.go`:

    cd $GOPATH/src/git-wip-us.apache.org/repos/asf/lucy-clownfish.git/runtime/go
    go run build.go
    go run build.go test
    go run build.go install
    go run build.go clean

