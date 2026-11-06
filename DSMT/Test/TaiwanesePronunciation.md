# 台語 DSMT 羅馬拼音與逐句測試索引

拼音直接取自 `DSMT/Combine/Working/Taiwanese/Taiwanese.dsmt` 的 `phoneme`，保留全部候選與聲調數字。`<zh-NAN>` 是語言標記，句點分隔音節；候選序號不用念出。

先照模型候選發音，再依 UART 的 MapID 判讀結果。不同口音或自然說法可依 INI 新增候選或同 MapID 別名，再由 DSMT 建模；操作見 [DSMT 教學](../../DSMT_OPERATION_GUIDE.md)。nanoe 讀「奈諾伊」，「攑」使用 `gia3`（台語第三調低降）。

| 測試編號 | 分類 | 台語詞條 | MapID | DSMT 羅馬拼音（全部候選） |
|---|---|---|---|---|
| T001 | 喚醒 | 房間冷氣 | 1001 | `<zh-NAN>pang7.king7.ling1.khi3` |
| T002 | 喚醒 | 客廳冷氣 | 1002 | `<zh-NAN>kheh8.thiann7.ling1.khi3` |
| T003 | 電源 | 開冷氣 | 1 | `<zh-NAN>khui7.ling1.khi3`<br>`<zh-NAN>khai7.ling1.khi3` |
| T004 | 電源 | 打開冷氣 | 1 | `<zh-NAN>thau1.khui7.ling1.khi3` |
| T005 | 電源 | 關冷氣 | 2 | `<zh-NAN>kuainn7.ling1.khi3`<br>`<zh-NAN>kuan7.ling1.khi3` |
| T006 | 電源 | 冷氣關起來 | 2 | `<zh-NAN>ling1.khi2.kuainn1.khi3.lai5` |
| T007 | 電源 | 冷氣關掉 | 2 | `<zh-NAN>ling1.khi2.kuainn1.tiau3` |
| T008 | 模式 | 吹冷氣 | 3 | `<zh-NAN>tshui7.ling1.khi3`<br>`<zh-NAN>tshue7.ling1.khi3`<br>`<zh-NAN>phuh8.ling1.khi3`<br>`<zh-NAN>tshe7.ling1.khi3` |
| T009 | 模式 | 吹暖氣 | 4 | `<zh-NAN>tshui7.luan1.khi3`<br>`<zh-NAN>tshue7.luan1.khi3`<br>`<zh-NAN>phuh8.luan1.khi3`<br>`<zh-NAN>tshe7.luan1.khi3`<br>`<zh-NAN>tshui7.sio7.khi3` |
| T010 | 模式 | 開除濕 | 5 | `<zh-NAN>khai7.ti7.sip4`<br>`<zh-NAN>khai7.tu7.sip4` |
| T011 | 模式 | 開自動 | 6 | `<zh-NAN>khui7.tsu3.tong7`<br>`<zh-NAN>khai7.tsu3.tong7` |
| T012 | 溫度 | 溫度調高 | 7 | `<zh-NAN>un7.too3.tiau7.kuan5` |
| T013 | 溫度 | 溫度攑懸 | 7 | `<zh-NAN>un7.too7.gia3.kuan5` |
| T014 | 溫度 | 溫度調低 | 8 | `<zh-NAN>un7.too3.tiau7.ke7` |
| T015 | 溫度 | 溫度降落 | 8 | `<zh-NAN>un7.too3.kang2.loh8` |
| T016 | 溫度 | 設定十六度 | 116 | `<zh-NAN>siat8.ting3.tsap4.lak4.to5`<br>`<zh-NAN>siat8.ting3.sip4.lak4.to5`<br>`<zh-NAN>siat8.ting3.tsap4.liok4.to5`<br>`<zh-NAN>siat8.ting3.sip4.liok4.to5`<br>`<zh-NAN>siat8.ting3.tsap4.lak4.too7`<br>`<zh-NAN>siat8.ting3.sip4.lak4.too7`<br>`<zh-NAN>siat8.ting3.tsap4.liok4.too7`<br>`<zh-NAN>siat8.ting3.sip4.liok4.too7` |
| T017 | 溫度 | 設定十七度 | 117 | `<zh-NAN>siat8.ting3.tsap4.tshit8.to5`<br>`<zh-NAN>siat8.ting3.sip4.tshit8.to5`<br>`<zh-NAN>siat8.ting3.tsap4.tshit8.too7`<br>`<zh-NAN>siat8.ting3.sip4.tshit8.too7` |
| T018 | 溫度 | 設定十八度 | 118 | `<zh-NAN>siat8.ting3.tsap4.pueh8.to5`<br>`<zh-NAN>siat8.ting3.sip4.pueh8.to5`<br>`<zh-NAN>siat8.ting3.tsap4.pat8.to5`<br>`<zh-NAN>siat8.ting3.sip4.pat8.to5`<br>`<zh-NAN>siat8.ting3.tsap4.peh8.to5`<br>`<zh-NAN>siat8.ting3.sip4.peh8.to5` |
| T019 | 溫度 | 設定十九度 | 119 | `<zh-NAN>siat8.ting3.tsap4.kau1.to5`<br>`<zh-NAN>siat8.ting3.sip4.kau1.to5`<br>`<zh-NAN>siat8.ting3.tsap4.kiu1.to5`<br>`<zh-NAN>siat8.ting3.sip4.kiu1.to5`<br>`<zh-NAN>siat8.ting3.tsap4.kiau1.to5`<br>`<zh-NAN>siat8.ting3.sip4.kiau1.to5` |
| T020 | 溫度 | 設定二十度 | 120 | `<zh-NAN>siat8.ting3.ji3.tsap4.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.to5`<br>`<zh-NAN>siat8.ting3.ji3.tsap4.too7`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.too7`<br>`<zh-NAN>siat8.ting3.ji3.sip4.too7`<br>`<zh-NAN>siat8.ting3.nng3.sip4.too7` |
| T021 | 溫度 | 設定二十一度 | 121 | `<zh-NAN>siat8.ting3.ji3.tsap4.bat4`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.bat4`<br>`<zh-NAN>siat8.ting3.ji3.sip4.bat4`<br>`<zh-NAN>siat8.ting3.nng3.sip4.bat4` |
| T022 | 溫度 | 設定二十二度 | 122 | `<zh-NAN>siat8.ting3.ji3.tsap4.ji3.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.ji3.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.ji3.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.ji3.to5`<br>`<zh-NAN>siat8.ting3.ji3.tsap4.nng3.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.nng3.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.nng3.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.nng3.to5` |
| T023 | 溫度 | 設定二十三度 | 123 | `<zh-NAN>siat8.ting3.ji3.tsap4.sam7.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.sam7.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.sam7.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.sam7.to5` |
| T024 | 溫度 | 設定二十四度 | 124 | `<zh-NAN>siat8.ting3.ji3.tsap4.su2.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.su2.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.su2.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.su2.to5`<br>`<zh-NAN>siat8.ting3.ji3.tsap4.si2.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.si2.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.si2.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.si2.to5` |
| T025 | 溫度 | 設定二十五度 | 125 | `<zh-NAN>siat8.ting3.ji3.tsap4.goo3.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.goo3.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.goo3.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.goo3.to5`<br>`<zh-NAN>siat8.ting3.ji3.tsap4.ngoo1.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.ngoo1.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.ngoo1.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.ngoo1.to5` |
| T026 | 溫度 | 設定二十六度 | 126 | `<zh-NAN>siat8.ting3.ji3.tsap4.lak4.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.lak4.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.lak4.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.lak4.to5`<br>`<zh-NAN>siat8.ting3.ji3.tsap4.liok4.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.liok4.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.liok4.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.liok4.to5` |
| T027 | 溫度 | 設定二十七度 | 127 | `<zh-NAN>siat8.ting3.ji3.tsap4.tshit8.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.tshit8.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.tshit8.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.tshit8.to5` |
| T028 | 溫度 | 設定二十八度 | 128 | `<zh-NAN>siat8.ting3.ji3.tsap4.pueh8.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.pueh8.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.pueh8.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.pueh8.to5`<br>`<zh-NAN>siat8.ting3.ji3.tsap4.pat8.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.pat8.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.pat8.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.pat8.to5` |
| T029 | 溫度 | 設定二十九度 | 129 | `<zh-NAN>siat8.ting3.ji3.tsap4.kau1.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.kau1.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.kau1.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.kau1.to5`<br>`<zh-NAN>siat8.ting3.ji3.tsap4.kiu1.to5`<br>`<zh-NAN>siat8.ting3.nng3.tsap4.kiu1.to5`<br>`<zh-NAN>siat8.ting3.ji3.sip4.kiu1.to5`<br>`<zh-NAN>siat8.ting3.nng3.sip4.kiu1.to5` |
| T030 | 溫度 | 設定三十度 | 130 | `<zh-NAN>siat8.ting3.sam7.tsap4.to5`<br>`<zh-NAN>siat8.ting3.sann7.tsap4.to5`<br>`<zh-NAN>siat8.ting3.sam7.sip4.to5`<br>`<zh-NAN>siat8.ting3.sann7.sip4.to5`<br>`<zh-NAN>siat8.ting3.sam7.tsap4.too7` |
| T031 | 風量 | 風量大一點 | 9 | `<zh-NAN>hong7.ling3.tai3.it8.tiam2`<br>`<zh-NAN>hong7.niu7.tai3.it8.tiam2` |
| T032 | 風量 | 風量細一點 | 10 | `<zh-NAN>hong7.ling3.se2.tam3.poh8`<br>`<zh-NAN>hong7.niu7.se2.tam3.poh8` |
| T033 | 風量 | 風量自動 | 11 | `<zh-NAN>hong7.ling3.tsu3.tong7`<br>`<zh-NAN>hong7.niu7.tsu3.tong7` |
| T034 | 風向 | 風向自動 | 12 | `<zh-NAN>hong7.hiang2.tsu3.tong7`<br>`<zh-NAN>hong7.hiong2.tsu3.tong7` |
| T035 | 風向 | 固定風向 | 13 | `<zh-NAN>koo2.ting3.hong7.hiang3`<br>`<zh-NAN>koo2.ting3.hong7.hiong3` |
| T036 | 健康 | 開 nanoe | 14 | `<zh-NAN>khui7.nai3.noo3.i1` |
| T037 | 健康 | 關 nanoe | 15 | `<zh-NAN>kuainn7.nai3.noo3.i1` |
| T038 | 音量 | 聲音大一點 | 16 | `<zh-NAN>siann7.im7.tai3.it8.tiam2` |
| T039 | 音量 | 聲音細一點 | 17 | `<zh-NAN>siann7.im7.se2.tam3.poh8`<br>`<zh-NAN>siann7.im7.sue2.tam3.poh8` |
| T040 | 音量 | 關閉語音 | 18 | `<zh-NAN>kuainn1.tiam3.gi1.im1`<br>`<zh-NAN>kuainn1.tiam3.gu1.im1` |
| T041 | 定時 | 一小時後關機 | 201 | `<zh-NAN>it8.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>tsit4.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>it8.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>tsit4.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>it8.sio1.si7.hau3.kuainn7.ki1`<br>`<zh-NAN>tsit4.sio1.si7.hau3.kuainn7.ki1` |
| T042 | 定時 | 兩小時後關機 | 202 | `<zh-NAN>niu1.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>nng3.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>ling1.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>liong1.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>niu1.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>nng3.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>ling1.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>liong1.sio1.si7.hoo3.kuainn7.ki1` |
| T043 | 定時 | 三小時後關機 | 203 | `<zh-NAN>sam7.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>sann7.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>sam7.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>sann7.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>sam7.sio1.si7.hau3.kuainn7.ki1` |
| T044 | 定時 | 四小時後關機 | 204 | `<zh-NAN>su2.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>si2.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>su2.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>si2.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>su2.sio1.si7.hau3.kuainn7.ki1`<br>`<zh-NAN>si2.sio1.si7.hau3.kuainn7.ki1` |
| T045 | 定時 | 五小時後關機 | 205 | `<zh-NAN>goo3.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>ngoo1.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>goo3.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>ngoo1.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>goo3.sio1.si7.hau3.kuainn7.ki1`<br>`<zh-NAN>ngoo1.sio1.si7.hau3.kuainn7.ki1` |
| T046 | 定時 | 六小時後關機 | 206 | `<zh-NAN>lak4.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>liok4.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>lak4.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>liok4.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>lak4.sio1.si7.hau3.kuainn7.ki1`<br>`<zh-NAN>liok4.sio1.si7.hau3.kuainn7.ki1` |
| T047 | 定時 | 七小時後關機 | 207 | `<zh-NAN>tshit8.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>tshit8.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>tshit8.sio1.si7.hau3.kuainn7.ki1` |
| T048 | 定時 | 八小時後關機 | 208 | `<zh-NAN>pueh8.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>pat8.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>peh8.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>pueh8.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>pat8.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>peh8.sio1.si7.hoo3.kuainn7.ki1` |
| T049 | 定時 | 九小時後關機 | 209 | `<zh-NAN>kau1.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>kiu1.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>kiau1.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>kau1.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>kiu1.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>kiau1.sio1.si7.hoo3.kuainn7.ki1` |
| T050 | 定時 | 十小時後關機 | 210 | `<zh-NAN>tsap4.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>sip4.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>tsap4.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>sip4.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>tsap4.sio1.si7.hau3.kuainn7.ki1`<br>`<zh-NAN>sip4.sio1.si7.hau3.kuainn7.ki1` |
| T051 | 定時 | 十一小時後關機 | 211 | `<zh-NAN>tsap4.it8.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>sip4.it8.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>tsap4.tsit4.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>sip4.tsit4.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>tsap4.it8.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>sip4.it8.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>tsap4.tsit4.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>sip4.tsit4.sio1.si7.hoo3.kuainn7.ki1` |
| T052 | 定時 | 十二小時後關機 | 212 | `<zh-NAN>tsap4.ji3.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>sip4.ji3.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>tsap4.nng3.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>sip4.nng3.sio1.si7.au3.kuainn7.ki1`<br>`<zh-NAN>tsap4.ji3.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>sip4.ji3.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>tsap4.nng3.sio1.si7.hoo3.kuainn7.ki1`<br>`<zh-NAN>sip4.nng3.sio1.si7.hoo3.kuainn7.ki1` |
| T053 | 定時 | 取消定時 | 19 | `<zh-NAN>tshu1.siau7.u3.si7.u3.tsun7` |
