```
g++ -O3 main.cpp
./a.out 2> /dev/null > result.csv
```

TODO: Check if VTune tells me about the false sharing. Huawei's vtune clone for their arm cpus says it will

<img src="image.png" alt="Graph showing a steep drop in time spent after a 64 byte offset is applied. After that point, the two threads are no longer falsely sharing." width="80%">
