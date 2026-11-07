I94124 國語／台語語音控制範例文件入口

完整操作手冊：專案根目錄 README.md。
燒錄入口：CMDSet/ReleaseV005/ProgrammingLayout.txt。
一般 ICP 燒錄：APROM 選 DNN_NuOne_UAC.bin，DataFlash 選 AirconDataFlash.bin。
DataFlash 整合檔包含雙語模型與內部 NuOne 回覆音訊。

國語工程：Combine/Working/ChineseTWN/AirconZHTW.dsmt。
台語工程：Combine/Working/Taiwanese/Taiwanese.dsmt。
DSMT 修改完整儲存後，雙擊根目錄 BuildLatestModel.cmd 更新發布檔，不需 Python。
辨識規格維護於 VoiceCommands.ini；一鍵工具產生 Import/ 匯入檔，交由 DSMT 取得發音並儲存模型。
回覆 WAV 由 Audio Tool 按 F7 編碼。
最後都只需雙擊根目錄 BuildLatestModel.cmd，自動同步、核對、封裝及更新文件。
操作教學：DSMT_OPERATION_GUIDE.md、AUDIO_TOOL_GUIDE.md。
新增／改名詞條、MapID、發音與 INI 欄位格式見 DSMT_OPERATION_GUIDE.md。

逐句測試：Test/UserCommandTestGuide.txt。
兩語命令、MapID 與回覆：Test/CommandMapIDQuickReference.md。
台語 DSMT 羅馬拼音與全部候選：Test/TaiwanesePronunciation.md。
國語 84 種文字／9 組，台語 53 種文字／9 組，兩語同時辨識。
nanoe 讀「奈諾伊」；台語「溫度攑懸」中的攑採 gia3。

兩語命令共用國語回覆，音訊位於內部 Flash 0x70000，格式為 16 kHz、NuOne 1.0 bits/sample。
溫度 16～30 度共用「好的，溫度已設定」；定時 1～12 小時共用「好的，定時已設定」。
回覆定義：AudioRes/ReplyPhrases/ReplyProfile.json，操作見 README.md 音訊維護章節。
冷氣實際控制仍由應用層串接；UART 保留功能、區域、溫度及小時數。

控制等待預設 60 秒，於 DNN_VR/AirconVR.h 的 AIRCON_CONTROL_TIMEOUT_SECONDS 修改後建置。
整句播放結束後恢復辨識；逾時、-1105 音訊遺失或 -1106 兩語衝突後重新喚醒。
喚醒與命令成功均有 LED 短閃；命令正確性依 UART 與預期回覆核對。

工具更新產物時的備份位於根目錄 _Archive/，由 .gitignore 排除。
