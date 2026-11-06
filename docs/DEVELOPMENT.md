# 工程維護

一般操作見 [README](../README.md)。本文件供韌體與工具維護者使用。

## Keil 建置與發布

1. 開啟 `Prj/KEIL/I94_Cyberon.uvproj`，Target 選 `UsbCallBack`，使用 Arm Compiler 6。
2. Build／Rebuild，確認 0 errors；輸出為 `Prj/KEIL/obj/DNN_NuOne_UAC.bin`。
3. 完成 DSMT／Audio Tool 的來源變更後，執行根目錄 `BuildLatestModel.cmd`。
4. 核對 `CMDSet/ReleaseV005/BuildReport.md` 及燒錄檔指紋，再進行板測。

一鍵工具優先使用 Keil `obj` 中的相容韌體；無建置產物時，核對發布指紋後沿用發布韌體。`obj` 是建置輸出，`CMDSet/ReleaseV005` 是供燒錄的整套發布檔。

## 一鍵工具結構

| 檔案 | 用途 |
|---|---|
| `BuildLatestModel.cmd` | 唯一的使用者產生入口 |
| `DSMT/BuildProject.ps1` | 暫存建置、來源快照核對、發布與清理 |
| `DSMT/VoiceCommands.ini` | 使用者維護的辨識規格與完整發音候選 |
| `DSMT/VoiceIni.ps1` | INI 解析、官方匯入格式、候選核對、發音寫回與模型狀態比對 |
| `DSMT/SyncNativeInputs.ps1` | 由已儲存 DSMT 詞條及既有功能定義同步輸入清單 |
| `DSMT/ReplyAudioBuild.ps1` | WAV 格式、音訊來源及編碼時效核對 |
| `DSMT/AirconBundleBuilder.cs/.exe` | 原生模型、音訊結構及 Flash 邊界核對、二進位封裝 |
| `DSMT/UpdateVoiceDocs.ps1` | 逐句表、MapID 及台語拼音文件 |
| `DSMT/WriteBuildReport.ps1` | 發布檔與記憶體報告 |
| `DSMT/VoiceMaintenance.ps1` | 共用序列化、指紋、備份與檔案更新 |

自動化驗證可用 `BuildLatestModel.cmd --no-pause`。此模式不啟動 Audio Tool 或產生新 WAV，來源尚未準備完成時回傳失敗。一般雙擊模式會依回覆文字引導 WAV 產生及 Audio Tool F7。

`DSMT/build_bundle_tool.cmd` 重新編譯原生封裝 exe。Python 檔為開發參考及回歸測試，正常操作不依賴 Python。

## 韌體修改位置

| 檔案 | 用途 |
|---|---|
| `DNN_VR/AirconCommands.c` | MapID、功能、溫度／小時及區域解碼 |
| `DNN_VR/AirconVR.c/.h` | 雙語辨識、候選處理、等待時間 |
| `DNN_VR/AirconAudio.c/.h` | 音訊格式及 MapID→播放索引 |
| `Play/PlayNuOne.c` | NuOne 解碼、片語佇列及 DMA 銜接 |
| `Src/app/src/main.c` | UART、LED、回覆與應用層控制入口 |
| `Prj/KEIL/Aircon.sct` | APP、RW、ZI 記憶體配置 |

新增實際冷氣控制介面時，在解碼後的應用層串接 GPIO／通訊，依實際控制結果決定回覆。

## 新增功能及回覆片段

新增既有 MapID 的說法依 INI／DSMT 流程操作。新功能或新的獨立回覆片段需要同步：

1. 定義 MapID、功能、分類、兩語說法及回覆；更新 `AirconCommands_Decode()` 與需求功能模板。
2. 更新 `AirconAudio_Reply()`、音訊片段數及索引配置。
3. 在 Profile、WAV、UID 新增來源，Audio Tool F7 產生新的 ROM 與標頭。
4. 同步封裝核對中的片段數、來源數、MapID 覆蓋及索引規則；位置為 `AirconBundleBuilder.cs`、`ReplyAudioBuild.ps1`、`SealAudioBuild.ps1`。Python 參考及測試一併維護。
5. 重新編譯封裝 exe 與韌體，執行一鍵產生，再完成容量與完整板測。

內部音訊分區為 64 KiB；目前配置 23 個片段與 49 個 MapID。既有溫度及定時範圍分別為 16～30 度、1～12 小時，擴充範圍時同步解碼與詞條展開定義。

## 候選紀錄與分數設定

`AIRCON_RESULT_DIAGNOSTICS` 控制 UART 候選紀錄；資料在辨識結束後輸出，`matched=1` 表示 SDK 曾產生候選，`filtered=1` 表示此候選未通過控制門檻。

| 紀錄 | 意義 |
|---|---|
| `VR_DECISION stage/outcome/frames` | 階段、結果及此次 PCM frame 數 |
| `filtered` | 此輪低品質候選計數 |
| `score_resolved` | 兩語不同 MapID 已依分數差距選出結果 |
| `VR_CANDIDATE index/frame` | SDK 原始命令索引及候選出現時間；PCM frame 為 30 ms |
| `confi/sg/fil/score_ret` | SDK 原始分數及查詢回傳值 |
| `dura/end_sil/latency/epd_ret` | SDK 詞長、靜音、延遲及命令索引回傳值 |

控制分數處理由 `AirconVR.h` 設定，與 UART 顯示開關分開：

| define | 預設 |
|---|---:|
| `AIRCON_CONTROL_SCORE_FILTER` | 1 |
| `AIRCON_CONTROL_MIN_CONFIDENCE` | 5 |
| `AIRCON_CONTROL_MIN_SG_DIFF` | 15 |
| `AIRCON_CONFLICT_SCORE_ARBITRATION` | 1 |
| `AIRCON_CONFLICT_MIN_CONFIDENCE` | 25 |
| `AIRCON_CONFLICT_CONFIDENCE_GAP` | 20 |
| `AIRCON_CONFLICT_SG_GAP` | 10 |

低品質控制候選會重設該引擎並繼續原控制視窗。不同 MapID 只在一方達到勝方最低 Confidence，且 Confidence／SG 均領先設定差距時採用；差距不足回傳 `-1106`。規則對兩語相同，喚醒仍依原生模型條件。

數值為專案的起始設定，需以不同使用者、語言、口音、距離與環境測試調整。分數意義見 [Cyberon 官方指南](https://tool.cyberon.com.tw/DSMT_V2/index.php?lang=en)；分數差距不代表錯誤率或機率。

## 驗證

| 入口 | 範圍 |
|---|---|
| `DSMT/Test/run_host_tests.cmd` | 解碼、PCM、雙語候選處理、記憶體、音訊及播放流程；需 Visual Studio C 工具 |
| `DSMT/Test/test_project_build.py` | 單一入口、來源變動、編碼核對、發布及清理 |
| `DSMT/Test/test_bundle_builder.py` | 原生封裝與 Python 參考一致性、容量、鎖定及錯誤處理 |
| `DSMT/Test/test_voice_maintenance.py` | Windows 需求／文件產生器與來源保留 |
| `DSMT/Test/test_voice_ini.ps1` | INI 欄位、299 候選往返、匯出檔一致性與模型狀態比對 |
| `DSMT/Test/test_github_push.py` | 本機 Git 倉庫驗證一鍵提交／推送、忽略規則、錯誤停止及重試；不連線 GitHub |

主機測試不取代實機辨識率、回覆讀音、麥克風／喇叭及即時處理驗證。執行紀錄與測試暫存放在 `DSMT/Test/`，由 Git 排除。

INI 的 No 只決定匯入排序。Phoneme 明確填寫時，依序核對每個候選；留白時允許 DSMT 將該句展開為連續的有效發音候選，成功發布時寫回 INI。每句候選必須共用 MapID。

ExtEnd 的 INI 欄位與 DSMT 顯示值使用秒；原生 `extra_endsil_frame` 及 `-extsil` 使用 30 ms 格數，範圍 0～16。此換算依現有 DSMT 2.2.25.116 的欄位處理確認，與聲學設定的 100 fps 分開。更新 DSMT 版本時重新核對匯出格式及參數單位。

`Input/IniModelState.json` 保存每組有效發音、MapID、Reward、SGReward、ExtEnd、FastSpeech 與 Global 設定的摘要及模型指紋。摘要變更而模型指紋未變時，一鍵工具要求 DSMT 重新產生模型。顯示文字改名不納入模型摘要，因原生模型依音素與 MapID 辨識；INI 與群組文字仍需一致。此核對不取代 DSMT 的合法發音處理及板測。

`UpdateVoiceInputs.ps1` 與 Python Excel 匯入程式保留供工程移植參考，一般使用者以 INI 維護。新專案的群組及路徑變更需同步 `SyncNativeInputs.ps1`、`Get-VoicePronunciations`、封裝器、功能模板及韌體。
