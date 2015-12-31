Apache Clownfish symbiotic object system -- runtime Go bindings
---------------------------------------------------------------

These are temporary installation instructions for developers working on the
Apache Clownfish Go bindings.

As a prerequisite, install the Go bindings for the Clownfish compiler (CFC).
This will entail cloning the Git repository which is shared by the runtime.

    # After installing CFC, switch to the runtime/go directory.
    cd $GOPATH/src/git-wip-us.apache.org/repos/asf/lucy-clownfish.git/runtime/go

From the `runtime/go` directory within the checkout, run `build.go`:

    go run build.go
    go run build.go test
    go run build.go install
    go run build.go clean

