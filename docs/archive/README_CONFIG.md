## Config file ##

# ServerConfig.hpp #
1. listen :
    a. Allowed port numbers are 0-65535.
    b. Must start with non-zero.
    c. Must be numeric.
2. host :
    a. [IP_ADDRESS] could be multiple.
3. server_name :
    a. Could be multiple.
4. client_max_body_size :
    a. Body size limit
5. error_page:
    a. [error code] - [file path](could be multiple) : [key] - [value]


# LocationConfig.hpp #
## Location Block ##
URL Path processing Rules

1. root :
    a. Base directory path on the server where web resources (HTML, files) are stored.
    b. Actual file path : [root path] + [requested URI]

2. index :
    a. Default index files to serve when a directory is requested.
    b. Default value is "index.html".
    c. Can specify multiple files in priority order (e.g., index.html index.htm).

3. allow_methods :
    a. Allowed HTTP methods for this location (GET, POST, DELETE, etc.).

4. autoindex :
    a. Directory listing flag when no index file exists (on / off).

5. redirection :
    a. [status code] - [target url]: [key] - [value]

6. CGI_pass:
    a. Common Gateway Interface : excute external program(python, php, c++, etc.) and get output as html
    b. [path]: [value]
