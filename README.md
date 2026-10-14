# Linux 網路程式設計作業

[![Linux build and loopback tests](https://github.com/tsengwt1210/Linux_server/actions/workflows/build.yml/badge.svg)](https://github.com/tsengwt1210/Linux_server/actions/workflows/build.yml)

這個 repository 收錄 TCP 檔案傳送、HTTP／CGI 伺服器與 UDP 群播聊天室三項課程練習。程式使用 C/C++ 與 Linux socket API，從連線建立、檔案讀寫，延伸到 `fork()` 多行程與群播收發。

## 先看哪裡

| 作業 | 實作內容 | 程式入口 | 操作與設計說明 |
|---|---|---|---|
| TCP 檔案傳送 | Client 上傳檔案；Server 為每條連線建立子行程並存檔 | [Server](ConsoleApplication/ConsoleApplication/try.cpp)、[Client](ConsoleApplication/ClientApplication/client.cpp) | [作業一](docs/01-tcp-file-transfer.md) |
| HTTP 與 CGI | 讀取靜態網頁；以 `dup2()`、`exec()` 執行 CGI | [HTTP Server](ConsoleApplication/Assignment2_WWW/WWW_server.cpp)、[CGI](ConsoleApplication/Assignment2_WWW/cgi.cpp) | [作業二](docs/02-http-cgi.md) |
| UDP 群播聊天室 | 加入 multicast 群組，以父、子行程分別發送與接收訊息 | [Multicast chat](ConsoleApplication/multicast_chat/multicast_chat.cpp) | [作業三](docs/03-multicast-chat.md) |

想快速確認能否執行，可以先完成下方編譯，再跑 `make test`。想看程式設計細節，可從作業一的連線流程開始。

## 編譯與驗證

環境需求：Linux 或 Windows 的 WSL Ubuntu、支援 C++17 的 `g++`、GNU Make。整合測試另外使用 Python 3，無第三方 Python 套件依賴。

在 Linux／WSL 終端機執行：

```bash
git clone https://github.com/tsengwt1210/Linux_server.git
cd Linux_server
make -j2
make test
```

若 Ubuntu 尚未安裝編譯工具，可自行安裝 `build-essential` 與 `python3`。程式依賴 POSIX socket、`fork()` 與 Linux 標頭，應在 Linux 環境編譯；Windows 的一般 MSVC console project 不能直接建置。

`make` 會產生五個獨立執行檔：

```text
build/bin/
├── tcp_server
├── tcp_client
├── http_server
├── cgi
└── multicast_chat
```

HTTP Server 與 CGI 各有自己的 `main()`，因此分開編譯。`make test` 會在暫存目錄啟動 TCP／HTTP Server，僅連線到 `127.0.0.1`；測試結束後清理它建立的行程與檔案。群播程式只做編譯檢查。

## TCP 範例

在 repository 根目錄準備兩個工作目錄：

```bash
mkdir -p run/server run/client
cp examples/homework.txt run/client/homework.txt
```

終端機 A：

```bash
cd run/server
../../build/bin/tcp_server
```

終端機 B（從 repository 根目錄開始）：

```bash
cd run/client
../../build/bin/tcp_client
```

Server 會在 `run/server/` 留下 `Client_<PID>_homework.txt`。第一次示範完成後，可回到 repository 根目錄比對內容：

```bash
sha256sum run/client/homework.txt run/server/Client_*_homework.txt
```

使用 `Ctrl+C` 停止 Server。HTTP 與群播的完整執行步驟見各作業說明。

## 程式中的重點

TCP 作業使用 `socket → bind → listen → accept` 建立連線，再用 `fork()` 把檔案接收交給子行程。Client 和 Server 交換固定長度檔名與檔案偏移量；[共用 I/O 函式](ConsoleApplication/io_helpers.h) 處理一次讀寫不完整與中斷的情況。

HTTP 作業把網路連線接到 CGI 的標準輸入輸出，讓獨立程式直接產生動態頁面。靜態檔案與 CGI 共用連線入口，但產生回應的方式不同。

群播作業不另外設置中央聊天室 Server。每個程式加入 `239.0.0.1:8888`，父行程從鍵盤讀取訊息，子行程接收群組封包。

## 使用範圍

這些程式保留課程練習的簡化設計。TCP 與 HTTP 預設只綁定 `127.0.0.1`，適合在本機理解與驗證流程。

| 項目 | 目前範圍 |
|---|---|
| TCP 傳輸協定 | 自訂協定，沒有實作 FTP；offset 使用本機 `long long` 格式 |
| 續傳 | 程式有交換 offset，但 PID 存檔名稱會隨連線改變，尚不能可靠跨連線續傳 |
| 大檔案 | 以 64 位元檔案 offset 編譯；目前測試未涵蓋 4 GB 以上檔案 |
| HTTP | 支援短 GET 請求、根目錄單檔與固定 `/cgi`；沒有 TLS、登入或完整 HTTP/1.1 功能 |
| 群播 | 保留手動聊天室操作；是否能跨主機接收取決於網卡、路由與防火牆設定 |

2026-10-14 已在 WSL Ubuntu 編譯五個入口，通過 10 項本機整合測試。詳細條件與未測範圍記在 [驗證紀錄](docs/validation.md)。

## 目錄

```text
ConsoleApplication/  程式來源，沿用原作業目錄名稱
docs/                每項作業的流程、操作方式與限制
examples/            可公開的文字檔與示範網頁
tests/               TCP、HTTP、CGI 的本機整合測試
Makefile             五個入口的編譯設定
CHANGELOG.md         本次整理與程式修正紀錄
```

原始課程進度保留在 Git 歷史；公開版的來源補齊與修正詳見 [變更紀錄](CHANGELOG.md)。
