# RIDE Platform Installation

The RIDE platform requires Fast-DDS as a system dependency.

## Prerequisites

### macOS (Homebrew)
```bash
brew install tinyxml2
```

### Build Fast-CDR from source
```bash
git clone https://github.com/eProsima/Fast-CDR.git
cd Fast-CDR && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4 && sudo make install
```

### Build foonathan_memory from source
```bash
git clone https://github.com/foonathan/memory.git
cd memory && mkdir build && cd build  
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4 && sudo make install
```

### Build Fast-DDS from source
```bash
git clone https://github.com/eProsima/Fast-DDS.git
cd Fast-DDS && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4 && sudo make install
```

## Building RIDE Platform

After installing dependencies:
```bash
mkdir build && cd build
cmake ..
make
```

## Testing Installation

Verify Fast-DDS is properly installed by testing the examples:
```bash
cd /path/to/Fast-DDS/examples/cpp/hello_world && mkdir build && cd build
cmake .. && make
./hello_world publisher &
./hello_world subscriber
```