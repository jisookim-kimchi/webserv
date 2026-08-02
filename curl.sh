#!/bin/bash

echo "[GET TEST]"
curl -i http://localhost:8080/

echo "[POST TEST]"
curl -i -X POST http://localhost:8080/deleted_file.txt -H "Content-Type: text/plain" -d "Hallo Hallo File"

echo "[DELETE TEST]"
curl -i -X DELETE http://localhost:8080/deleted_file.txt
