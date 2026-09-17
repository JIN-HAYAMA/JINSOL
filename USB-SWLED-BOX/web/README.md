# USB-SWLED BOX Web Control v1.0

JIN Solution / USB-SWLED BOX の完成版Webコントロールソフトです。

## 主な機能

- Web Serial APIによるUSBシリアル接続
- 115200bps / 8N1 / CR+LF
- スイッチ状態100ms周期リアルタイム監視
- 緑LED ON / OFF / 500ms点滅
- 赤LED ON / OFF / 500ms点滅
- ALL ON / ALL OFF
- 通信ログ表示
- 通信ログをTXT保存
- USB取り外し検出
- レスポンシブUI
- JIN Solution ブランド表記

## BOX通信仕様

- `i1` → `i11` : スイッチON
- `i1` → `i10` : スイッチOFF
- `o11` → `o1` : 緑LED ON
- `o10` → `o1` : 緑LED OFF
- `o21` → `o2` : 赤LED ON
- `o20` → `o2` : 赤LED OFF

すべてCR+LF終端。

## 対応環境

推奨:
- Windows 10 / 11
- Google Chrome
- Microsoft Edge

Web Serial APIは、通常 `localhost` または `https://` のセキュアコンテキストで使用してください。

## ローカル起動

### Pythonがある場合

このフォルダ内でコマンドプロンプトを開きます。

```bash
python -m http.server 8000
```

その後、ChromeまたはEdgeで:

```text
http://localhost:8000
```

を開いてください。

## 公開する場合

HTTPS対応のWebサーバーへ `index.html`, `style.css`, `app.js`, `favicon.svg` をそのままアップロードします。

## 注意

ブラウザのセキュリティ仕様により、初回接続時はユーザー自身がシリアルポートを選択する必要があります。
