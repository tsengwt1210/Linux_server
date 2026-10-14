# 驗證紀錄

## 2026-10-14：本機編譯與整合測試

| 項目 | 實際環境／結果 |
|---|---|
| 作業系統 | WSL2，Ubuntu 24.04.4 LTS |
| Linux kernel | `6.6.87.2-microsoft-standard-WSL2` |
| 編譯器 | g++ 13.3.0 |
| 編譯工具 | GNU Make 4.3 |
| 測試工具 | Python 3.12.3，標準庫 unittest |
| 編譯設定 | C++17、`-Wall -Wextra -Wpedantic`、64 位元 file offset |
| 編譯結果 | 五個執行檔完成，未輸出編譯警告 |
| 測試結果 | 10 項整合測試全部通過 |

執行指令：

```bash
make -j2
make test
```

## 測試內容

| 測試 | 檢查方式 |
|---|---|
| TCP 二進位檔 | 使用 1,048,580 bytes 合成資料，核對接收檔長度與 SHA-256 |
| TCP 空檔 | 確認成功建立零位元組檔案 |
| TCP 缺少輸入 | Client 回傳非零狀態並指出 homework.txt |
| TCP 檔名分段 | 分多次發送 256 bytes 檔名欄位，確認 offset 與內容 |
| TCP 路徑拒絕 | 對 `../outside.txt` 拒絕接收，不在目錄外建立檔案 |
| HTTP 靜態頁面 | 比對標頭、Content-Length 與 HTML 完整內容 |
| HTTP 分段請求 | 將 request 分次送出，仍取得 200 |
| CGI | 確認動態頁面包含時間、loopback IP 與 Client port |
| HTTP 錯誤處理 | 檢查 404、415、403、405，並確認不能讀取目錄外或 symbolic link 指向的檔案 |
| HTTP 過長標頭 | 1023 bytes 內尚未結束的 header 回傳 431 |

測試只連線到 `127.0.0.1:12345` 與 `127.0.0.1:9998`，檔案位於本次測試的暫存目錄。啟動前先確認 port 可用；結束時只關閉測試自行建立的 process group。

## 未涵蓋

此次沒有驗證 4 GB 以上檔案、跨連線續傳、不同 CPU 架構之間交換 offset、高連線量、長時間執行或跨主機網路。群播聊天室僅完成編譯，未發送群播封包。

GitHub Actions 使用 Ubuntu 24.04 runner 執行相同的 `make` 與 `make test`。每次提交的結果可從 [Actions](https://github.com/tsengwt1210/Linux_server/actions/workflows/build.yml) 查閱；上方的本機結果只對應本次記錄的環境。
