# Cgvhid

## Basic

Cloud Gaming Virtual Human Interface Device

## Building and Testing

- Install latest WDK.

- Open solution file with VS, and press F7.

- Enable Test Mode:

```
bcdedit.exe -set nointegritychecks on
bcdedit.exe -set loadoptions nointegritychecks
bcdedit.exe -set testsigning on
```

- Restart Windows.

- Install driver via `devmgmt.msc`.

- Run `cgvhid_test`.
