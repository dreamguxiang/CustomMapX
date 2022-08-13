cd GOHelper
go build -v -ldflags "-s -w"  -trimpath -buildmode=c-shared -o MAP_Golang_Module.dll