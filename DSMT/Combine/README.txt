Panasonic 國台語 DSMT 工程操作入口
更新：2026-11-06，V004

請從專案根目錄 README.md 閱讀完整設定、Mermaid流程、發音處理、建置、燒錄與測試步驟。
自行維護詞條請讀 README.md「DSMT 如何設定及操作 → 新增或修改詞條」：
  A 新增同功能別名（例：開啟冷氣 → MapID 1）。
  B 修改發音／台語候選／辨識參數；Edit改名不會自動改發音。
  C 國台語多詞共用MapID；功能、參數、單位必須一致。
  D 新增功能與參數範圍；須同步韌體解碼與測試。
新增別名還需同步Excel、Manifest及腳本固定筆數；完整教學包含對照範例。
後續依「DSMT改完後取得最新bin」重新封裝；更新同一發布目錄前先備份原檔與已填測試紀錄。
不需Python：儲存正式Working工程後，雙擊根目錄BuildLatestModel.cmd；發布檔變動時工具先自動ZIP備份。
逐句測試表：DSMT\Test\UserCommandTestGuide.txt，每個案例明列喚醒＋命令，含台語發音提示。

本目錄目前工作工程：
  Working\ChineseTWN\PanasonicAirConZHTW.dsmt（84種文字，9組，含國語nanoe）
  Working\Taiwanese\Taiwanese.dsmt（53種文字，215筆發音候選，9組，含台語nanoe）

nanoe兩語都讀「奈諾伊」（nano-E）；使用者已確認台語候選試聽符合讀法，正式Working及V004已納入開／關兩條。
台語MapID14：khui7.nai3.noo3.i1；MapID15：kuainn7.nai3.noo3.i1。健康組未變；本次只修正MapID7「溫度攑懸」為un7.too7.gia3.kuan5。
各語言 NanoePronunciation.json 保存實際處理紀錄，ProjectStatus.json 保存當前產物雜湊與待補項目。
不要自行拼造音素ID，或把國語CYBase當成台語使用。
控制窗口預設60秒，以DNN_VR\AirconVR.h定義修改；分類不需由使用者先用語音選取。

封裝產物在 CMDSet\ReleaseV004\；本次gia3修訂只改模型，已是相容V004韌體的板子可只更新DataFlash模型包。
兩份DNN韌體bin目前內容相同；obj是Keil輸出，CMDSet是封裝時複製的發布副本。
舊版與原始種子工程已封存於根目錄 _Archive\；過往完整舊說明也在修改前ZIP保留。
