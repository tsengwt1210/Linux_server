# 變更紀錄

## 2026-10-14

- 補上 README、三項作業的執行與設計說明、實際驗證紀錄。
- 從本機課程來源補入 fork／CGI 版 HTTP Server 與 UTF-8 版群播聊天室；原 Git 提交歷史保留。
- 新增 Makefile，將 TCP Server、TCP Client、HTTP Server、CGI、群播聊天室分開編譯。
- 新增可公開的示範文字檔、HTML 頁面及忽略編譯產物的規則。
- TCP 與 HTTP 預設綁定 loopback；補上完整讀寫、系統呼叫中斷與斷線時的錯誤處理。
- TCP 檔名改為固定 256 bytes 欄位，驗證檔名與 offset；Client、Server 需使用本次一起編譯的版本。
- 保留 TCP 的 PID 存檔方式，文件明確標示跨連線續傳尚未完成。
- HTTP 補上分段標頭接收、請求長度與路徑限制、靜態檔案長度，以及 400／403／404／405／415／431／500 回應；CGI 僅由 `/cgi` 啟動。
- 新增 10 項 loopback 整合測試與 GitHub Actions；WSL Ubuntu 的編譯與測試結果見 [驗證紀錄](docs/validation.md)。

這次整理沒有加入課堂講義、作業報告、個人照片、學號或編譯執行檔。
