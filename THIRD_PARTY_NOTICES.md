# 第三方元件

本專案含既有 Nuvoton BSP、Cyberon DSpotter、NuOne 解碼庫、CMSIS 標頭及音訊／模型資料。既有檔案中的著作權與授權標示保留。

| 元件 | 專案位置 | 說明 |
|---|---|---|
| Nuvoton I94100 BSP 與範例 | `Src/`、`Lib/`、`SH_Audio/`、`readme.txt` 等 | 既有檔案含 Nuvoton 著作權標示；使用及散布範圍依原始套件授權 |
| Cyberon DSpotter SDK 2.2.25.2 | `DNN_VR/DSpotterSDKApi*.h`、`DNN_VR/libDSpotterSDK_16k23d_I94100_arm-cm4_V2.2.25.2_A_b20260330.lib` | Keil 工程使用的辨識引擎；SDK 使用及散布依原廠授權 |
| Cyberon 語音模型與 DSMT 產物 | `DSMT/Combine/Working/`、`CMDSet/` | 包含基礎模型及命令模型；產生模型需 DSMT 服務與相應帳號 |
| Nuvoton NuOne 解碼庫 | `Play/NuOneEx_Keil.lib` | 語音回覆解碼所需；使用及散布依原廠授權 |
| Arm CMSIS／DSP 標頭 | `Lib/DSP_LIB/Include/` 等 | 部分標頭明列 Apache-2.0；以各檔案的授權標示為準 |
| 回覆音訊 | `AudioRes/ReplyPhrases/` | 來源及指紋見 `PhraseSourceManifest.json`、`AudioBuildManifest.json`；合成聲音設定見 `ReplyProfile.json` |

目前專案未附 Cyberon／Nuvoton 元件的完整公開再散布授權文件，也未設定整個倉庫的統一開源授權。公開倉庫的可見性不會變更各元件的授權條件。發布者須確認 SDK、模型、合成音訊及來源需求資料的公開散布權限；相關授權文件可依原廠提供的正式版本補入。
