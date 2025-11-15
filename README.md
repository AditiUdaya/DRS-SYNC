# DRS-SYNC

structure:

DRS-SYNC/
├── include/drs_sync/
│   ├── packet.hpp
│   ├── network_interface.hpp
│   ├── transfer_engine.hpp
│   ├── congestion_control.hpp
│   ├── simple_checkpoint.hpp
│   └── integrity.hpp
├── src/
│   ├── network_interface.cpp
│   ├── transfer_engine.cpp
│   ├── congestion_control.cpp
│   ├── simple_checkpoint.cpp
│   ├── integrity.cpp
│   └── api_server.cpp
├── frontend/
│   └── (covered separately)
└── CMakeLists.txt





frontend/
├── package.json
├── vite.config.js
├── index.html
└── src/
    ├── main.jsx
    ├── App.jsx
    ├── App.css
    └── components/
        ├── Dashboard.jsx
        ├── Speedometer.jsx
        ├── TelemetryPanel.jsx
        └── FileUploader.jsx