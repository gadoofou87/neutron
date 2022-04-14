# Neutron
My unfinished С++20 pet project, which is supposed to be a very fast cross-platform messenger

## Structure
`3rdparty` - contains CMake scripts that fetch third-party libraries  
`client` - Qt/QML cross-platform client (not started)  
`lib/api` - client-server api (not started)  
`lib/communication` - rpc-like wrapper over a connection (almost done)  
`lib/crypto` - contains C++ wrappers over crypto algorithms (almost done, partially covered with tests)  
`lib/protocol` - custom SCTP-like protocol over UDP (almost done)  
`lib/serialization` - header-only library for writing zero-copy serializable structures (almost done)  
`lib/utils` - common logic  
`server` - server (not started)  
`tests` - tests
