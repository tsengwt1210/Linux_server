# 作業二：HTTP 與 CGI

Server 在 `127.0.0.1:9998` 接收 GET 請求，為每條連線建立子行程。請求 `/` 時回傳工作目錄內的 `index.html`；請求 `/cgi` 時執行獨立的 CGI 程式，顯示伺服器時間、連線端 IP 與 port。

## 準備與執行

在 repository 根目錄執行：

```bash
make
mkdir -p run/www
cp examples/www/index.html run/www/index.html
cp build/bin/cgi run/www/cgi
cd run/www
../../build/bin/http_server
```

在同一台 Linux／WSL 裝置上，以瀏覽器或另一個終端機開啟：

```bash
curl -i http://127.0.0.1:9998/
curl -i http://127.0.0.1:9998/cgi
curl -i http://127.0.0.1:9998/missing.html
```

前兩個請求應回傳 `200 OK`，缺少的 HTML 檔應回傳 `404 Not Found`。CGI 的時間與 Client port 隨執行而變化。完成後在 Server 終端機按 `Ctrl+C`。

WSL 的 Windows 主機若無法直接以瀏覽器連線，先用 WSL 終端機的 `curl` 確認程式；跨 Windows／WSL 的 localhost 轉送取決於環境設定。

## 靜態檔案與 CGI 的差別

靜態請求由 [WWW_server.cpp](../ConsoleApplication/Assignment2_WWW/WWW_server.cpp) 開啟檔案，產生 Content-Type／Content-Length 標頭，再將檔案內容送進 socket。可提供 HTML、JPG／JPEG、PNG、GIF，副檔名不區分大小寫。

CGI 請求先設定 `REMOTE_ADDR`、`REMOTE_PORT`，再以 `dup2()` 把 socket 接到標準輸入、輸出與錯誤輸出。`execl()` 將子行程替換為 `./cgi`，[cgi.cpp](../ConsoleApplication/Assignment2_WWW/cgi.cpp) 自行輸出 HTTP 標頭與 HTML。HTTP Server 和 CGI 是兩個執行檔，不能把兩個 `main()` 連結成同一個程式。

## 請求範圍與錯誤處理

| 情況 | 回應 |
|---|---|
| `/`、`/index.html` 或支援類型的檔案 | 檔案存在時回傳 200 |
| `/cgi` | 執行工作目錄的 `cgi`；缺少執行檔時回傳 500 |
| 支援類型但不存在的檔案 | 404 |
| 支援範圍以外的副檔名 | 415 |
| 非 GET 方法 | 405 |
| 子目錄、`..`、百分比編碼、query string 等路徑 | 403 |
| 超過 1023 bytes、尚未結束的請求標頭 | 431 |

本版將靜態檔案限制為工作目錄的單一檔名，拒絕 symbolic link；來源程式與私人檔案不應放在示範的 `run/www/` 中。CGI 只有固定 `/cgi` 入口，沒有依 URL 任意執行程式。

這份程式用來觀察基本 HTTP 請求與行程 I/O。每條連線處理一個請求後關閉，沒有 keep-alive、POST body、chunked encoding、TLS、登入或完整 CGI 環境；目前也未驗證高連線量與長時間運作。
