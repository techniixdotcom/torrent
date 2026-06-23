# PQL - the NiiX Torrent Query Language

PQL is a simple query language for filtering torrents in NiiX Torrent.


## Building

```
java -jar .\antlr-4.8-complete.jar -Dlanguage=Cpp -package pt::PQL -visitor -no-listener -o generated .\Query.g4
```
