
# ble_door_nrf52832
基于nRF52832控制的蓝牙门锁<br>
主要有触摸按键、LED显示、RTC时钟、指纹模块、蜂鸣器、电机6个部分。<br>

## 2017.9.28 test
指纹模块已测试注册和搜索功能
## 2017.9.30
内部flash存储的相关程序，必须设置若干pstorage_handle_t，保证读取的flash的handle是全局变量