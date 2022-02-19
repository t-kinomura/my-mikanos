# 動かし方

1. 環境変数設定
```
$ OS_DIR=/workspace/my-mikanos
```

2. edkのディレクトリからシンボリックリンクを貼る
```
$ cd ~/edk2
$ ln -s /workspace/my-mikanos/MikanLoaderPkg ./
```

3. edkの設定
```
$ source edksetup.sh
// 必要に応じてConf/target.txtを編集する
```

4. ビルド実行
```
$ build
$  ~/osbook/devenv/run_qemu.sh Build/MikanLoaderPkg/DEBUG_CLANG38/X64/Loader.efi
```