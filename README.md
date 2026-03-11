# Benchmark: array 2D contiguo vs vector\<vector\<int\>>

Compara tiempo y memoria entre:

- **Array contiguo:** `int[32*1e6][2]` en el heap (un solo bloque).
- **Vector de vectores:** `vector<vector<int>>(32*1e6, vector<int>(2))` (32M bloques).

## Compilar

### Windows (Visual Studio / MSVC)
```cmd
cl /EHsc /O2 benchmark_array_vs_vector.cpp psapi.lib
```

### Windows (MinGW/g++)
```cmd
g++ -O2 -o benchmark_array_vs_vector.exe benchmark_array_vs_vector.cpp -lpsapi
```

### Linux
```bash
g++ -O2 -o benchmark_array_vs_vector benchmark_array_vs_vector.cpp
```

## Ejecutar

```cmd
benchmark_array_vs_vector.exe
```

o en Linux:

```bash
./benchmark_array_vs_vector
```

**Nota:** Cada estructura usa ~256 MB solo en datos; el proceso puede superar 500 MB con `vector<vector>`. Ejecutar en una máquina con suficiente RAM.
