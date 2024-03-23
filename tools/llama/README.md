## Using llama to accelerate your build

[llama](https://github.com/nelhage/llama) is a CLI for outsourcing computation to AWS Lambda.

You can use llama to accelerate your CDDA builds.  To help you set that up,
this directory contains a suitable image for compiling CDDA on.

After bootstrapping llama, you can use for example

```bash
llama update-function --create --timeout=120s --memory=4096 --build=tools/llama/gcc-focal gcc
```

This will configure llama to use the `gcc-focal` image here.  Note that it's
important to have larger-than-default timeout and memory settings.

### Troubleshooting

If, when building, you get errors about failed includes of SDL2 headers, then
you can work around these via a hack I discovered, by setting a carefully
chosen CXXFLAGS.  Your build command might look something like this:

```bash
CXXFLAGS=-I/x/../../../../usr/include/SDL2 CXX=llamac++ make PCH=0 TILES=1 -j100
```

Or, if using cmake, something like this:
```bash
cmake \
    -DTILES=ON \
    -DSOUND=ON \
    -DCMAKE_C_COMPILER=llamacc \
    -DCMAKE_CXX_COMPILER=llamac++ \
    -DCMAKE_CXX_FLAGS="-I/x/../../../../usr/include/SDL2" \
    ..
```

(`/x` has to be a directory that doesn't exist on your system)
