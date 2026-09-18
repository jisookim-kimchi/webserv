# HTTP
HTTP is a stateless, request-response protocol for transferring web resources between clients and servers.  
## Flow the parsing request.
```
 Raw Request Buffer 
 -> parseRequestLine(Method,URI,Path,QueryString,Version)  
 -> parseHeaders(Key-Value Map & Validate Host Header)  
 -> parseBody(Content-Length / Chunked Body Unchunking)  
```

## 1. Request Line (`parseRequestLine`)
- **Method**: First token (`GET`, `POST`, `DELETE`, etc.)
- **URI**: Split by `?` into `Path` and `QueryString`
- **URL Decode**: Decodes `%....` (e.g. `%20` -> space)
- **Version**: Checks `HTTP/1.1`

## 2. Headers (`parseHeaders`)
- reads each line (`\r\n`) into `Key: Value` pairs
- converts key to lowercase (case-insensitive)
- checks that `Host` header exists and is not duplicated

## 3. Body (`parseBody`)
- **Content-Length**: reads the exact number of bytes specified
- **Chunked (`Transfer-Encoding: chunked`)**: reads size in hex -> reads data chunk -> stops at `0\r\n\r\n`

# Reference
> https://developer.mozilla.org/ko/docs/Web/HTTP/Guides/Messages
