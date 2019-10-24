set architecture i386:x86-64:intel

echo + target remote localhost:32678\n
target remote localhost:32678

echo + symbol-file build/kernel\n
symbol-file build/kernel

continue
