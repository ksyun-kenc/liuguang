# [microsoft](https://github.com/microsoft)/[GSL](https://github.com/microsoft/GSL)

```
commit 4377f6e603c64a86c934f1546aa9db482f2e1a4e (HEAD -> main, origin/main, origin/HEAD)
Author: Werner Henze <34543625+beinhaerter@users.noreply.github.com>
Date:   Mon Jan 31 22:06:42 2022 +0100

    quoted form of #include when GSL includes GSL files (#1030)

    [SF.12: Prefer the quoted form of #include for files relative to the including file and the angle bracket form everywhere else](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rs-incform)

    Additionally changed #include order in `span` so that all `span_ext` is in the GSL include block and not in the STL include block.

    Fixes issues #1025.

    Co-authored-by: Werner Henze <w.henze@avm.de>
```
