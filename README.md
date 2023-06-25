# marbleel.exe : Wheel Emulator for Trackman Marble

[Logitech](https://www.logitech.com/)(ja;[Logicool](https://www.logicool.co.jp/)) [Trackman Marble](https://duckduckgo.com/?q=trackman+marble) (2022年頃廃番) という今時ホイールのない4ボタン人差し指式有線トラックボールで玉をホイールとして使うためのソフトウェアです。もちろんTrackman Marble以外の製品でも使えます。メーカー公式ドライバや世に出回っている同様の主旨のソフトよりも低機能で遥かに小さいのが売りですが意味はありません。

Windows 2000(x86), Windows XP(x86), Windows Vista(x86), Windows 7(x64), Windows 10(x64), Windows 11 で動作確認していますがあらゆる環境での動作を保証するものではありません。

## 使い方

本プログラムを起動するとタスクトレイにマウスのアイコンが出ます。アイコンはコントロールパネルのアイコン(main.cplの0番など)を読み込んで表示します。左ダブルクリックで終了します。終了確認のダイアログボックスが出ます。シフトキーを押下しながらダブルクリックすると確認なしに終了します。コントロールキーを押下しながらダブルクリックすると設定内容を表示します。オルトキーを押下しながらダブルクリックするとバージョン情報を表示します。右ダブルクリックでも設定内容を、中ダブルクリックでもバージョン情報を表示しますが、近頃のトレイは左以外のダブルクリックを受け付けないようです。

ホイール駆動ボタンを**押したまま**玉を動かすとホイールになります。

Trackman Marbleが廃番になってしまったので代替品を想定して同時押し機能を付加しました。しかし同時押し判定の待ちが生じるため、クリック速度に敏感な人やドラッグ操作を多用する人にはお勧めできません。

タスクスケジューラを用いて管理者権限で実行することもできます。その際はタスクトレイのアイコンを操作できませんのでご注意ください。終了するにはタスクスケジューラで停止する([schtasks /end /tn ...](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/schtasks-end))か、管理者権限で `marbleel.exe /q` としてください。

バージョン1.3では設定はコマンドラインオプションで行います。

## コマンドラインオプション

| 指示 | 引数 | 既定値 | 範囲 | 意味 |
| -- | ---- | ---- | ------ | -------------------------------- |
| `/a`  | S D | (無効) | 1-5 | ボタンSをボタンDに読み替え。複数、入替も可能ですが多用は推奨しません。 |
| `/b`  | S | (無効) | 1-5 | ボタンSをホイール駆動ボタンにする |
| `/c`  | A B D | (無効) | 1-5 | (実験的) ボタンAとボタンBを同時に押すとボタンDに読み替え。1つだけ。 |
| `/ct` | 整数 | 75 | 30-500 | (実験的) 同時押し判定(ミリ秒) |
| `/vd` | 整数 | 120 | 15-1920 | 縦の [WHEEL_DELTA](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousewheel) 値。変更すべきでない。 |
| `/hd` | 整数 | 120 | 15-1920 | 横の [WHEEL_DELTA](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousewheel) 値。変更すべきでない。 |
| `/vs` | 整数 | 1 | 1-127 | このポイント以上カーソルを動かしたら縦を検知 |
| `/hs` | 整数 | 1 | 1-127 | このポイント以上カーソルを動かしたら横を検知 |
| `/vc` | 整数 | 1 | 1-127 | この回数以上検知したら縦ホイールを送信 |
| `/hc` | 整数 | 1 | 1-127 | この回数以上検知したら横ホイールを送信 |
| `/e`  | (なし) | (無効) | | ホイール駆動ボタンを押してホイールしなかったらクリックを送信 |
| `/h`  | (なし) | (無効) | | 横ホイール有効化 |
| `/r`  | (なし) | (無効) | | ホイール逆回転 |
| `/q`  | (なし) | (無効) | | 終了 |

## ボタンの番号

Windowsがサポートするマウスボタンは5つのみです。このプログラムでは次の番号を割り振っています。順序はWindows APIに準拠しています。

| 番号 | WinAPI上の呼称 | 自然言語 | Trackman Marble |
| - | ---------------- | ---------------- | ---------------- |
| 1 | LBUTTON | 左ボタン | 左大ボタン |
| 2 | RBUTTON | 右ボタン | 右大ボタン |
| 3 | MBUTTON | 中ボタン | |
| 4 | XBUTTON1 | 後退ボタン | 左小ボタン |
| 5 | XBUTTON2 | 前進ボタン | 右小ボタン |

## 設定例

| コマンドラインオプション | 意味 |
| -------- | -------------------------------- |
| `/b 4 /a 5 3` | 後退ボタンでホイール駆動。前進ボタンを中ボタンとする。(筆者の設定) |
| `/b 4 /a 5 3 /vc 5` | 同上。ただしホイールの動きを鈍くする。 |
| `/b 4 /e /a 4 3 /a 5 3` | 後退ボタンでホイール駆動。後退ボタンを単純にクリックしたら中ボタンクリックとする。前進ボタンを中ボタンとする。 |
| `/c 1 2 3` | 左ボタンと右ボタンの同時押しを中ボタンとする。 |
| `/c 1 2 3 /b 3 /e` | 左ボタンと右ボタンの同時押しをホイール駆動ボタンとする。ただし同時クリックは中ボタンクリックとする。 |
| `/q` | 既に実行中の本プログラムを終了する。 |

## 原始プログラムを翻訳する

ご覧の通りx86は [Borland C++ Compiler 5.5](https://www.embarcadero.com/free-tools/ccompiler) と [Netwide Assembler](https://nasm.us/) 、x64は [Tiny C Compiler](https://bellard.org/tcc/) を使用しましたがお好みの環境で適当にやってください。

なお現在(2023年6月現在)配布されているTiny Cには `lib\shell32.def` が添付されていないので `tcc -impdef C:\Windows\System32\shell32.dll -o tcc\lib\shell32.def` のようにして定義体を作成してください。またWindows APIのヘッダーファイルは標準添付されていないので忘れず同時入手してください。

## 改訂履歴

- 1.3 (2023-06-08)

    マルチスレッド方式に変更。マウス操作の受信、ボタンメッセージ発行、トレイアイコン指示待ち(メイン)の3本立て。
    設定をコマンドラインオプション方式に変更。
    同時押し機能を追加。ホイールの回数判定を追加。設定内容の表示機能を追加。

    ファイルサイズ 13KB(32bit) | 20.5KB(64bit)

- 1.2 (2019-10-04)

    Windows 10 対応のためマウス操作の受信とボタンメッセージの発行を分離。

    ファイルサイズ 8KB(32bit)

- 1.1 (紛失)

- 1.0 (2013-08-08)

    ファイルサイズ 4KB(32bit)
