cd /d %~dp0

D:\Project\Smarthome\esp\nanopb\generator-bin\protoc.exe --nanopb_out=. RelayConfig.proto
copy RelayConfig.pb.h ..\include /y
copy RelayConfig.pb.c ..\src /y
del RelayConfig.pb.h
del RelayConfig.pb.c
