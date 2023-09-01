
# x86_code_on_x64_demo

実用性は皆無な遊びですので、ご了解ください。

WSL にて、以下の要領で作成した 32 ビット向けの機械語である x86.bin  
ファイルを 64 ビットモードの Windows アプリとして実行するデモです。  
bin フォルダの x86_code_on_x64_demo.exe をコマンドプロンプトにて  
実行してください。実行に成功すると、x86_func() = 4 と出力されます。  

```
$ cat x86.s

BITS 32

_start:
    push ebp
    call proc
    mov eax, dword [ebp-4] ; return code
    pop ebp
    ret

proc:
    mov ebp, esp
    mov dword [ebp-4],4
    ret

$ nasm x86.s -o x86.bin
$ objdump -D -Mintel -b binary -m i386 x86.bin > x86.txt
$ cat x86.txt

x86.bin:     file format binary


Disassembly of section .data:

00000000 <.data>:
   0:	55                   	push   ebp
   1:	e8 05 00 00 00       	call   0xb
   6:	8b 45 fc             	mov    eax,DWORD PTR [ebp-0x4]
   9:	5d                   	pop    ebp
   a:	c3                   	ret    
   b:	89 e5                	mov    ebp,esp
   d:	c7 45 fc 04 00 00 00 	mov    DWORD PTR [ebp-0x4],0x4
  14:	c3                   	ret    
```

x86_code_on_x64_demo.exe のヘッダ情報に Application can handle  
large (>2GB) addresses が含まれていないため、64 ビットモードで  
あっても 2GB を超えるメモリ空間は使用されないことが確認できます。

```
C:\> dumpbin /headers x86_code_on_x64_demo.exe > x86_code_on_x64_demo.txt
(略)
FILE HEADER VALUES
            8664 machine (x64)
               B number of sections
        64F1A5AF time date stamp Fri Sep  1 17:49:51 2023
               0 file pointer to symbol table
               0 number of symbols
              F0 size of optional header
               2 characteristics
                   Executable

OPTIONAL HEADER VALUES
             20B magic # (PE32+)
(略)
```

## ファイルのダウンロード時の注意事項

コマンドプロンプトで以下のコマンドを投入し、ゾーン情報を削除します。

```
C:\>echo.>x86_code_on_x64_demo-main.zip:Zone.Identifier
```

## 開発環境

* NASM(WSL 上で、 sudo apt install nasm すること)

```
$ nasm -v
NASM version 2.14.02
```

* Visual Studio Community 2022

Windows SDK and developer tools - Windows app development | Microsoft Developer  
https://developer.microsoft.com/en-us/windows/downloads/

以前のバージョンの Visual Studio のダウンロード - 2019、2017、2015 以前のバージョン  
https://visualstudio.microsoft.com/ja/older-downloads/

Download the Windows Driver Kit (WDK) - Windows drivers | Microsoft Learn  
https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk

* x64 版 Windows 11

## ビルド方法

* x86_code_on_x64_demo.sln ファイルをダブルクリックします。

* F7 キーを押下します。

ビルド後に bin フォルダに x86_code_on_x64_demo.exe が出力されます。

## 参考文献・資料

インサイドWindows　第7版　上 システムアーキテクチャ、プロセス、スレッド、メモリ管理、他  
(原題：Windows Internals, Seventh Edition, Part 1: System architecture, processes, threads, memory management, and more)  
日経BP

Exploring the x64  
https://www.ffri.jp/assets/files/research/research_papers/psj10-murakami_JP.pdf

/LARGEADDRESSAWARE (Handle Large Addresses) | Microsoft Learn  
https://learn.microsoft.com/en-us/cpp/build/reference/largeaddressaware-handle-large-addresses?view=msvc-170

PE Format - Win32 apps | Microsoft Learn  
https://learn.microsoft.com/en-us/windows/win32/debug/pe-format

IsWow64Process function (wow64apiset.h) - Win32 apps | Microsoft Learn  
https://learn.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-iswow64process

_alloca | Microsoft Learn  
https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/alloca?view=msvc-170

NASM - The Netwide Assembler  
https://www.nasm.us/xdoc/2.16.01/html/nasmdoc0.html

X86アセンブラ/NASM構文 - Wikibooks  
https://ja.wikibooks.org/wiki/X86%E3%82%A2%E3%82%BB%E3%83%B3%E3%83%96%E3%83%A9/NASM%E6%A7%8B%E6%96%87

## ライセンス

[MIT ライセンス](LICENSE)

## 作者

市川 真一(Shin'ichi Ichikawa) <tenpoku1000@outlook.com>

