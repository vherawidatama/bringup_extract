module android/soong/hidl

require (
	android/soong v0.0.0
	github.com/google/blueprint v0.0.0
)

require google.golang.org/protobuf v1.26.0-rc.1 // indirect

replace google.golang.org/protobuf v0.0.0 => ../../../../external/golang-protobuf

replace github.com/google/blueprint v0.0.0 => ../../../../build/blueprint

replace android/soong v0.0.0 => ../../../../build/soong

replace github.com/google/go-cmp v0.5.5 => ../../../../external/go-cmp

go 1.23

toolchain go1.24.1
