# ProtoC

## What is this?
This is a simple [Google's Protocol Buffers](https://protobuf.dev/) compiler for C.

## LI Usage?

`bin/protoc -h`

## API Usage?

See `src/test`

## How do I compile this?

`cmake .`

followed by 

`make`

in root directory. You can then use `make install` to install the utility and `make test` to run tests.

---

If you happen to use any other build system then the general gist is:

`src/protoc/*.c -> bin/protoc`

`bin/protoc -m -o include/protocols/* src/protocols/*.proto`

`src/test/*.c -> bin/test_*`

## Feature complete?
Nope. A LOT of things are missing.

## Compiler support?
Probably supports anything C99 compliant.