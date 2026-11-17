# 实验三：设备树

本实验使用 C 编写 FDT 解析器，读取 QEMU `virt` 机器导出的 `virt.dtb`。课程网页标注本实验为可选，实验内容是依据设备树规范选择熟悉的语言解析 `virt.dtb`。

## 目录内容

```text
实验三_设备树_GitHub版/
|-- README.md
|-- 官方网页原文.rst
|-- 验收终端操作.md
|-- virt.dts
|-- parse_fdt.c                 快速运行用源码
|-- virt.dtb                    快速运行用输入文件
|-- 解析结果.txt                 快速运行结果
|-- 作业/                       原目录中的作业源码、DTB、结果和可执行文件
`-- 实验截图/                   实验过程截图
```

实验报告未放入本目录。根目录的 `parse_fdt.c`、`virt.dtb` 和 `解析结果.txt` 是便于快速查看与运行的副本；`作业/` 保留原有交付结构。

## 编译与运行

需要支持 C11 的编译器：

```sh
clang -std=c11 -Wall -Wextra -Werror -pedantic parse_fdt.c -o parse_fdt
./parse_fdt virt.dtb > 解析结果.txt
```

若要重新导出 DTB，需要安装支持 AArch64 的 QEMU：

```sh
qemu-system-aarch64 -machine virt,dumpdtb=virt.dtb -cpu cortex-a53 -m 128M -nographic
```

## 实验结果

解析器按 FDT header 中的偏移定位结构块和字符串块，将大端 32 位字段转换为主机整数，再按 FDT token 遍历节点。运行结果共 286 行，包含整棵节点树。

- `memory@40000000` 的 `reg` 表示起始地址 `0x40000000`、长度 128 MB。
- `pl011@9000000` 的寄存器起始地址为 `0x09000000`；中断规格为 SPI 1、高电平触发，对应 GIC INTID 33。
- `intc@8000000` 的 phandle 为 `0x8002`。
- `chosen/stdout-path` 指向 `/pl011@9000000`。

## 关于 `virt.dts`

根目录的 `virt.dts` 是一份简化设备树示例，与 `作业/virt.dtb` 的实际内容不一致，也不是本实验导出该 DTB 的来源。实验步骤使用 QEMU `dumpdtb` 生成 `virt.dtb`，解析器直接读取这个二进制文件；因此该差异不影响实验目标，但不应把 `virt.dts` 描述成该 DTB 的源码。

## 发布前提示

`验收终端操作.md` 中的命令包含原电脑上的绝对路径；在其他环境使用前需替换成对应路径。
