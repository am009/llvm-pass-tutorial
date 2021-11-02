# ollvm/armariris/hikari port to llvm10 in 2020

### 配置环境

安装Vscode的Remote-Containers插件，Ctrl-Shift-P执行build and open in devcontainer

### 编译

```bash
export CC=clang-12 CXX=clang++-12
cmake -S . -B build
cmake --build build
```

### 运行

首先使用wllvm获取程序的LLVM-IR字节码文件。如tshd.bc，转换为可读的IR文件

```bash
llvm-dis-12 ./tshd.bc -o tshd.ll
```

执行运算指令替换：

```bash
opt-12 -load build/ollvm/libollvm.so --substitution --sub_loop=1  < ./tshd.bc > out.bc
```

最后再转换为可读的IR文件，

```bash
llvm-dis-12 ./out.bc -o out.ll
```

最后用vscode diff一下两个ll文件查看变化了哪里

再最后编译成可执行文件看能不能运行

```bash
clang-12 ./out.bc -lutil -o tshd.out
./tshd.out --help
```


### 执行混淆的命令

可以在help里面搜索对应的命令行。

```bash
opt-12 -load build/ollvm/libollvm.so --help > help.txt
```

1. Pass的开启

    代码中命令行的注册是
    ```cpp
    static RegisterPass<Flattening> X("flattening", "Call graph flattening");
    ```
    开启的时候就使用 `--flattening` 即可。

2. Pass的命令行参数

    Pass的命令行参数在代码中通过`cl::opt`注册。

3. 常用命令

```bash
opt-12 -load build/ollvm/libollvm.so --flattening  < ./tshd.bc > out.bc # 控制流平坦化
opt-12 -load build/Armariris/libArmariris.so --GVDiv < ./tshd.bc > out.bc # Armariris里有全局变量字符串加密Pass
```

### 混淆哪些函数

主要看`bool Flattening::runOnFunction(Function &F)`这种runOnFunction函数。

那些被包围在`if (toObfuscate(flag, tmp, "fla"))`里的代码基本上都不会执行到，因为它需要在源码给函数标attribute。所以都注释了。

1. 让Pass只跑某一个函数：用`F.getName().compare`

```cpp
  auto name = F.getName();
  if (name.compare("file_chmod") == 0) {
    substitute(tmp);
    return true;
  }
```

```cpp
      if(toObfuscate(flag,&F,"bcf") || F.getName().compare("file_chmod") == 0) {
        bogus(F);
        doF(*F.getParent());
        return true;
      }
```

2. 让Pass跑所有函数：把包围在`if (toObfuscate(flag, tmp, "fla"))`里的代码拿出来即可。
