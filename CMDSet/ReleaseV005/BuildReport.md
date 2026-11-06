# 發布檔與記憶體用量

此表由一鍵產生工具依本目錄產物更新。1 KiB＝1024 bytes。

| 燒錄區域 | 起點 | 檔案 |
|---|---|---|
| APROM | `0x0` | `DNN_NuOne_UAC.bin` |
| DataFlash | `0x15000` | `AirconDataFlash.bin` |

| 部位 | 分配 bytes | 使用 bytes | 剩餘 bytes |
|---|---:|---:|---:|
| APP | 86016 | 75132 | 10884 |
| 國語＋台語模型 | 372736 | 336108 | 36628 |
| 回覆音訊 | 65536 | 59992 | 5544 |

| Keil 項目 | bytes |
|---|---:|
| Code | 50940 |
| RO | 16384 |
| RW | 7808 |
| ZI | 151992 |
| RAM 配置 RW＋ZI | 159800 |
| RAM 配置後剩餘 | 36808 |

RW 在 Flash 與執行時 RAM 各占一份；ZI 只占 RAM。RAM 數值包含啟動檔預留的 stack，實際執行水位及辨識池需求由板上量測。

| 產物 | bytes | SHA-256 |
|---|---:|---|
| `ChineseTWN.pack.bin` | 164768 | `4cb89c5dc78ff0cad35d221fbe097d9578aea8d4fe11ff14121c7a7375d4a1d5` |
| `Taiwanese.pack.bin` | 171292 | `16fd9d1705c4ad09d2a12a66edcc0204390c21e7803cfb382e83746f75014852` |
| `AirconModels.bundle.bin` | 336108 | `d103cc98b39ff4e798f37e7cf8cf0d7e1f5c2fe0bd241dfb6ae6354d621147b5` |
| `DNN_NuOne_UAC.bin` | 75132 | `d73975f1064c9c4ce3238d3af9d94bbd41d9f22d42a2ddf273f1560643141493` |
| `AirconReplies.bin` | 59992 | `7c7c2f98024d881b6421343a7327ddeff5352698bb87a60344ffc2f4bb9096e8` |
| `AirconDataFlash.bin` | 432728 | `77d62afdac2b12564953b2a8762f3b2f63e9f31207019fd3b9173274984c99fb` |
