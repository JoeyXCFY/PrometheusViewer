# PrometheusViewer

PrometheusViewer 是一個使用 **Unreal Engine 5.4.4** 開發的監控視覺化面板，能透過 Prometheus API 即時顯示系統資源 (CPU、Memory 等) 狀態。  
此專案的目標是提供比 Grafana 更直觀、模組化的可擴充 UI，並以 EXE 應用程式形式提供使用。

---

## 📦 前置需求

要讓 PrometheusViewer 正常運作，使用者需要先在欲監測的系統上部署 [dockprom](https://github.com/stefanprodan/dockprom) 環境：

```bash
git clone https://github.com/stefanprodan/dockprom
cd dockprom
ADMIN_USER='admin' ADMIN_PASSWORD='admin' ADMIN_PASSWORD_HASH='$2a$14$1l.IozJx7xQRVmlkEQ32OeEEfP5mRxTpbDTCTcXRqn19gXD8YK1pO' docker-compose up -d
```

這將會在本機啟動 Prometheus + cAdvisor + Node Exporter + Grafana 等容器，PrometheusViewer 會透過 Prometheus API 抓取資料。

🚀 使用方式
下載已編譯好的 EXE (Google Drive 下載連結)：

Google Drive - PrometheusViewer EXE
https://drive.google.com/file/d/190U52EkNFpEg6FtEdLaVxMfZh_Vj78vS/view?usp=drive_link

解壓縮並執行 PrometheusViewer.exe

在登入畫面中輸入：

IP：部署有 dockprom 的主機位址

帳號/密碼：與 Prometheus/Grafana 相同 (預設 admin / admin)

登入成功後會進入 Dashboard 畫面，可以：

新增監測項目

選擇 Metric 與 Type

即時查看數據或繪製圖表

⚙️ 開發資訊
Unreal Engine：5.4.4

Visual Studio：2022

語言：C++ / Blueprint 混合

專案結構：

PrometheusManager：負責與 Prometheus API 溝通

DashboardWidget：主控台介面

MonitoringItemWidget：單一監控項目模組

LoginWidget：登入介面

📝 待辦 / Roadmap
 支援更多圖表樣式 (Bar/Donut/Heatmap)

 提供使用者自訂 PromQL 查詢

 Alert 異常通知

 多用戶登入與權限管理
 
📄 License
前置部分環境參考
https://github.com/stefanprodan/dockprom

本專案僅作為個人學習與展示使用，無商業授權。
