# config file parser.

1: Configuration File Parser
Parse Nginx-style .conf files into ServerConfig and LocationConfig structures/classes.
-Need to talk with Team

1. server found -> {
2. ServerConfig object create ->
3. set server information ->
4. location found -> {
5. LocationConfig object create ->
6. set location information -> 
7. } save
8. } found server's end
9. save server object into vector
10. repeat from 1. until end of file.
