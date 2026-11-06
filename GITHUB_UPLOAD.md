# GitHub 上傳操作

倉庫名稱：`I94100BSP_VR_DSpotter`。可見性：**Public**。

本文件使用 Windows PowerShell。將 `<GitHub帳號>` 替換為個人帳號或組織名稱。

## 1. 建立 GitHub 倉庫

1. 登入 GitHub，選擇 **New repository**。
2. 選擇 Owner，Repository name 輸入 `I94100BSP_VR_DSpotter`。
3. 選擇 **Public**。
4. 保持空白倉庫：README、`.gitignore`、License 均不勾選；這些檔案由本機專案管理。
5. 按 **Create repository**，複製 HTTPS 網址：

   ```text
   https://github.com/<GitHub帳號>/I94100BSP_VR_DSpotter.git
   ```

專案內包含既有 SDK、辨識模型及音訊資源；各元件的來源與授權狀態見 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。公開發布的範圍依取得的原廠授權辦理。

## 2. 檢查本機內容

開啟專案根目錄的 PowerShell：

```powershell
Set-Location "<專案根目錄>"
if (-not (Test-Path -LiteralPath .git)) { git init -b main }
git status --short
git add --dry-run .
```

`.gitignore` 的主要規則：

| 類別 | 處理方式 |
|---|---|
| 韌體原始碼、Keil 工程、必要 `.lib` | 納入 |
| `DSMT/Combine/Working/` 原生工程、模型、群組匯出 | 納入 |
| `DSMT/Input/` 來源 Excel、需求清單、MapID | 納入 |
| `DSMT/AirconBundleBuilder.exe` 與工具原始碼 | 納入，支援無 Python 的一鍵封裝 |
| `AudioRes/ReplyPhrases/` 音訊來源、設定、原生工程與封裝所需產物 | 納入 |
| `CMDSet/ReleaseV005/` 燒錄檔、指紋及燒錄說明 | 納入 |
| README、逐句測試表、主機測試原始碼 | 納入 |
| `_Archive/`、舊版 `ReleaseV001`～`ReleaseV004` | 留在本機，上傳時排除 |
| Keil `obj/`、`lst/`、視窗與本機除錯設定、根目錄 `.icp` | 留在本機，上傳時排除 |
| 測試執行紀錄、暫存、Audio Tool 快取及開發紀錄 | 留在本機，上傳時排除 |

`.gitattributes` 預設保留檔案位元內容；文件與一般腳本另行指定換行格式。DSMT 匯出及音訊來源清單使用 SHA-256，下載及簽出時須保留這些檔案的原始換行與編碼。

## 3. 設定提交者

檢查目前設定：

```powershell
git var GIT_AUTHOR_IDENT
```

若尚未設定，使用下列命令。電子郵件可填入 GitHub 帳號設定頁所提供的 `noreply` 地址：

```powershell
git config user.name "<Git 顯示名稱>"
git config user.email "<GitHub noreply 電子郵件>"
```

上述設定僅套用此專案。

## 4. 建立第一次提交

```powershell
git add .
git diff --cached --stat
git diff --cached --name-only
git commit -m "Initial import: I94124 bilingual air-conditioner voice control"
```

提交內容包含 V005 發布檔。實機燒錄入口為 `CMDSet/ReleaseV005/ProgrammingLayout.txt`，APROM 使用 `DNN_NuOne_UAC.bin`，DataFlash 使用 `AirconDataFlash.bin`。

## 5. 設定遠端並上傳

首次設定遠端：

```powershell
git remote add origin "https://github.com/<GitHub帳號>/I94100BSP_VR_DSpotter.git"
git remote -v
git push -u origin main
```

依 Git 的登入提示完成 GitHub 驗證。推送完成後，在 GitHub 開啟 README，確認 `CMDSet/ReleaseV005/`、`DSMT/Combine/Working/`、來源 Excel 與一鍵封裝執行檔已列出。

若 `origin` 已存在，先用 `git remote -v` 核對；需要變更網址時使用 `git remote set-url origin "<正確網址>"`。遠端已有提交而推送被拒絕時，先以 `git fetch origin`、`git log --oneline --all --graph` 核對兩端歷史，再整合提交。

## 6. 後續更新

1. 依 README 完成 DSMT／韌體修改、封裝與相關測試。
2. 更新操作文件及 `CMDSet/ReleaseV005/` 的整套產物。
3. 提交並推送：

   ```powershell
   git status --short
   git add .
   git diff --cached --stat
   git commit -m "Describe the model, firmware or documentation change"
   git push
   ```

新增發布版本時，更新 `.gitignore` 的版本目錄規則、封裝工具的目標版本及文件入口。使用 GitHub Releases 發布下載包時，附上 APROM、DataFlash、`ModelManifest.json` 與 `ProgrammingLayout.txt`。

GitHub 操作參考：[Adding locally hosted code to GitHub](https://docs.github.com/en/migrations/importing-source-code/using-the-command-line-to-import-source-code/adding-locally-hosted-code-to-github)。
