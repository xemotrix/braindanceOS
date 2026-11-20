







```
[xxx] == sizeof xxx
0xXXXX - +Y == from 0xXXXX to 0xXXXX+Y
```


## Layout:
```
|- low memory (1MB)
|0x0
|0x7c00    - 7e00         boot_sector (loaded here by the BIOS)
|          - 0x9000       stack in real mode
|0x10000   - +[kernel]    kernel (temporary real mode copy, loaded here in the boot_sector)
|          - 0x90000      stack in pm

|- kernel zone (1MB)
|0x100000           - +sizeofkernel
|0x200000-[bitmap]  - 0x200000

```
