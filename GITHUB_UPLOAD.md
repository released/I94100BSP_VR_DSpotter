# GitHub 上傳操作

專案倉庫：[released/I94100BSP_VR_DSpotter](https://github.com/released/I94100BSP_VR_DSpotter)，可見性為 Public。

## 1. 上傳目前修改

完成模型／音訊產生及相關測試後，雙擊專案根目錄 **[github_push.bat](github_push.bat)**。

批次檔依序：

1. 核對目前分支為 `main`，且 `origin` 只有本專案的 HTTPS 推送網址。
2. 執行 `git add -A`，加入新增、修改及刪除項目，套用 `.gitignore`。
3. 有新變更時，以 `Update bilingual voice project` 作為提交訊息；沒有新變更時略過提交。
4. 執行 `git push -u origin main`，也會推送先前已提交但尚未上傳的內容。
5. 顯示成功或錯誤結果，按任意鍵關閉。

執行時需要登入，依 Git Credential Manager 提示完成 GitHub 驗證。任何一步失敗會停止；推送失敗時，本機提交仍保留，修正後可再次執行同一批次檔。進行中的 merge、cherry-pick、revert，以及已強制納入的忽略檔案需先完成處理。

從 PowerShell 執行且完成後不暫停：

```powershell
.\github_push.bat --no-pause
```

個別指定提交訊息時，可使用等效命令：

```powershell
Set-Location "D:\SourceCode\_Avery_I94100\I94100BSP_VR_DSpotter"
git status --short
git add -A
git --no-pager diff --cached --stat
git commit -m "Describe the change"
git push -u origin main
```

`git add -A` 會納入新增、修改及刪除項目，並依 `.gitignore` 排除暫存。`git commit` 保存到本機；`git push` 才上傳至 GitHub。

每條命令獨立一行，上一條完成後再執行下一條。若分頁畫面底部出現 `(END)`，按 `q` 離開，再輸入 Git 命令。

目前分支為 `main`，`origin` 已設定為：

```text
https://github.com/released/I94100BSP_VR_DSpotter.git
```

核對命令：

```powershell
git branch --show-current
git remote -v
```

## 2. 專案納入範圍

| 內容 | 用途 |
|---|---|
| `Src/`、`DNN_VR/`、`Play/`、`SH_Audio/`、`Lib/` | 韌體、BSP 與必要函式庫 |
| `Prj/KEIL/` | Keil 工程及記憶體配置 |
| `DSMT/VoiceCommands.ini`、`DSMT/Import/` | 辨識規格及 DSMT 匯入資料 |
| `DSMT/Combine/Working/` | 國語／台語原生工程、必要模型及匯出詞條 |
| `DSMT/Input/` | 功能清單、模型狀態及原始需求 Excel |
| `DSMT/` 工具原始碼與 `AirconBundleBuilder.exe` | 無需 Python 的一鍵產生工具與開發參考 |
| `AudioRes/ReplyPhrases/` | 回覆 WAV、設定、Audio Tool 工程與必要編碼產物 |
| `CMDSet/ReleaseV005/` | 最新 APROM、DataFlash、容量報告及檔案指紋 |
| `DSMT/Test/` 程式與三份使用者測試文件 | 可重現的自動驗證與逐句板測 |
| README 及操作教學 | INI、DSMT、Audio Tool、燒錄及測試說明 |

`.gitignore` 排除歷史備份、舊版發布、Keil `obj/lst`、本機除錯設定、測試輸出與 Audio Tool 快取。工具執行後重新產生的暫存不會納入正常提交。

`.gitattributes` 保留 DSMT 匯出、音訊來源與二進位產物的原始位元內容，供 SHA-256 核對。

推送包含 `main` 尚未上傳的提交及其歷史。`.gitignore` 處理未追蹤檔案；既有歷史中的檔案仍屬於提交歷史。批次檔沿用現有歷史，遠端有不同提交時由 Git 拒絕推送，整合後再重跑。

第三方元件說明見 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 3. 日後更新

依 INI／DSMT 或 Audio Tool 教學完成修改，雙擊 `BuildLatestModel.cmd`，核對發布檔與相關測試，再雙擊 **`github_push.bat`**。

手動上傳的等效命令：

```powershell
git add -A
git --no-pager diff --cached --stat
git commit -m "Describe the change"
git push
```

Git 顯示 `nothing to commit` 表示沒有待提交變更。若推送被拒絕，先取得遠端狀態並檢查兩端提交：

```powershell
git fetch origin
git --no-pager log --oneline --all --graph -20
```

## 4. 首次設定 Git 身分

如 `git commit` 提示尚未設定身分，在本專案設定一次：

```powershell
git config user.name "<Git 顯示名稱>"
git config user.email "<GitHub noreply 電子郵件>"
```

推送時依 Git Credential Manager 提示登入 GitHub。

官方操作說明：[Adding locally hosted code to GitHub](https://docs.github.com/en/migrations/importing-source-code/using-the-command-line-to-import-source-code/adding-locally-hosted-code-to-github)。
