Panasonic 冷氣國台語專案 V005 文件入口

完整操作手冊：專案根目錄 README.md。
燒錄入口：CMDSet/ReleaseV005/ProgrammingLayout.txt。
一般 ICP 燒錄：APROM 選 DNN_NuOne_UAC.bin，DataFlash 選 AirconDataFlash.bin。
DataFlash 整合檔包含雙語模型與內部 NuOne 回覆音訊。

國語工程：Combine/Working/ChineseTWN/PanasonicAirConZHTW.dsmt。
台語工程：Combine/Working/Taiwanese/Taiwanese.dsmt。
DSMT 修改完整儲存後，雙擊根目錄 BuildLatestModel.cmd 更新發布檔，不需 Python。
新增／改名詞條、MapID、發音與需求同步方式見 README.md「DSMT 如何設定及操作」。

逐句測試：Test/UserCommandTestGuide.txt。
兩語命令、MapID 與回覆：Test/CommandMapIDQuickReference.md。
國語 84 種文字／9 組，台語 53 種文字／9 組，兩語同時辨識。
nanoe 讀「奈諾伊」；台語「溫度攑懸」中的攑採 gia3。

兩語命令共用國語回覆，音訊位於內部 Flash 0x70000，格式為 16 kHz、NuOne 1.0 bits/sample。
溫度 16～30 度共用「好的，溫度已設定」；定時 1～12 小時共用「好的，定時已設定」。
回覆定義：AudioRes/ReplyPhrases/ReplyProfile.json，操作見 README.md 音訊維護章節。
冷氣實際控制仍由應用層串接；UART 保留功能、區域、溫度及小時數。

控制等待預設 60 秒，於 DNN_VR/AirconVR.h 的 AIRCON_CONTROL_TIMEOUT_SECONDS 修改後建置。
整句播放結束後恢復辨識；逾時、-1105 音訊遺失或 -1106 兩語衝突後重新喚醒。
喚醒與命令成功均有 LED 短閃；命令正確性依 UART 與預期回覆核對。

歷史工程、來源與發布備份位於根目錄 _Archive/。
