# cgi 设计

## 1. 基本功能

- 运行游戏

- 注入 Hook DLL

- 运行 cge

## 2. 参数

- --exec：游戏路径名，例如 C:\games\usf4.exe

- --cd：游戏的当前目录，如果没有设置则取游戏所在目录，例如 C:\games

- --dynamic：true 则使用动态注入，默认为 false

- --imagename：要注入的进程名。有可能和 --exec 中的文件名不同，因为有些游戏采用加载器、主程序分离的设计。若不提供，则取 --exec 中的文件名部分，例如 usf4.exe。

- --wait：等待 exec 进程，单位 ms。

- --lx86：要注入的 dll 路径名，32 位版本

- --lx64：要注入的 dll 路径名，x64 版本

- --cge：cge 路径名

- --cgedir：cge 的运行目录，即依赖的 dll 或配置文件的所在目录，默认为 cge 所在目录。

## 3. 设计

- dynamic 方式时，如果找不到 imagename 指定的进程，用 ShellExecuteEx 运行 exec，这种方式比 CreateProcess 的兼容性更好。

- 由于 exec 可能是个加载器，所以需要 wait 一定时间，再去查找 imagename 进程。
