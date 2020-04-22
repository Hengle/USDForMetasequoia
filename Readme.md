# USD For Metasequoia
![](https://user-images.githubusercontent.com/1488611/79929701-74983080-8481-11ea-882f-5f56d513223e.jpg)  
[メタセコイア](https://www.metaseq.net/) で USD と Alembic を扱えるようにするプラグインです。  

USD も Alembic も主に映像業界でよく使われるファイルフォーマットです。  
USD は FBX の後継的なものと考えて差し支えないと思われます。FBX が保持できるデータは大体 USD でも保持でき、他ファイルへの参照、非破壊編集などのより高次な機能を備えます。
Alembic はジオメトリを時系列で保持するファイルフォーマットで、全フレームの全ポリゴンを Alembic でエクスポートし、レンダラーに渡す、といった使い方をされます。  
本プラグインは USD と Alembic のインポート、エクスポートを両方サポートします。
USD はボーンとモーフのインポート、エクスポートもサポートしています。
また、モデリングの過程を Alembic で 3D 録画する機能も備えています。

Windows (32bit / 64bit) 版、Mac 版共にサポートしていますが、**USD プラグインは、メタセコイアが 32bit 版であっても Windows が 64bit 版でないと動作しない** のでご注意ください。
これは USD のライブラリが 64bit しか対応していないため、32bit 版のプラグインは内部的に 64bit の別プロセスを経由してインポート/エクスポートを行うためです。
Alembic プラグインにはこの制限はなく、32bit 版 Windows でも動作します。

## 使い方
- [mqusd.zip](https://github.com/i-saint/USDForMetasequoia/releases/download/1.0.0/mqusd.zip) をダウンロード＆解凍し、install_Windows.bat を実行することでインストールします
  - もし install_Windows.bat が機能しない場合、C:\Users\ユーザー名\AppData\Roaming\tetraface\Metasequoia4_x64\Plugins に mqusd_Windows_64bit 内のディレクトリ全てをコピーしてください。
  - **メタセコイア内の "プラグインについて" -> "インストール" では正しくインストールできません**。この方法では関連する他のファイルがコピーされないためです。
- インストール後はファイルの 開く/保存 で abc や usd も認識するようになります
- Alembic によるレコーディング機能は "パネル" -> "Alembic Recorder" からアクセスします
  - Alembic Recorder の "Capture Interval" はモデルを記録する間隔で、5.0 なら 5 秒毎に記録します。ただし、モデルに変更がない場合は変更が来るまで記録を先延ばしにします。よって、0.0 にしてもメタセコイアが固まるということはありません。(変更が加わるたびに記録、という挙動になります)

## 注意点
- **ファイル名やディレクトリ名に日本語が含まれる場合、インポート/エクスポートが失敗します**。
  - 残念ながら Alembic や USD のライブラリ側が日本語を考慮していないため対応が難しく、現在対応の予定はありません。
- 日本語を含むオブジェクト名は扱いが特殊なので注意が必要です。
  - USD はオブジェクトの名前には英数字と _ しか使えない仕様になっています。このため、日本語名などは文字コードの数値の羅列に変換してエクスポートし、インポート時はその逆の変換を行っています。
    このため、本プラグインでエクスポートしてインポートした場合は日本語名は復元できますが、Maya や Blender などの他のツールに持っていった場合、数値の羅列がそのままオブジェクト名になってしまいます。

## ライセンス
- [MIT](LICENSE.txt)
- [Third Party Notices](Third%20Party%20Notices.md)
