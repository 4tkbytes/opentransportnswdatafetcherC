# OpenTransportNSWDataFetcher in C

This C project uses libcurl, cjson, the transportnsw public api data, and more.

## How to use

This is actually a binary so far, but there are plans to turn it into a library. 

To run the program, you must first create an API Key and store it in a .env file (anywhere).
You will also need the stopID for your bus stop. 

```.env
API_KEY=
STOP_ID=
```

Then, you must edit the MSYS2 path in the CMAKE (if needed) and install the dependencies. I am using UCRT, so I am installing through UCRT. 

The following packages are required:
- libcurl (or curl)
- cjson
- clang (recommended, not required)

Then, you build using the following commands: 
```bash
mkdir build # if required

cd build
cmake .. -G "MinGW Makefiles" # recommended to use mingw makefiles
cmake --build .
```

---

Enjoy!