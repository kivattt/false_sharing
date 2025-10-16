```
g++ -O3 main.cpp
./a.out 2> /dev/null > result.csv
```

TODO: Check if VTune tells me about the false sharing. Huawei's vtune clone for their arm cpus says it will
