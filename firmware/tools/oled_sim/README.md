# Simulatore dell'interfaccia OLED

Compila la logica del firmware (`../../xbattle_firmware/xbattle_firmware.ino`) sul PC con librerie finte
(`stubs/`), simula due lanci con impulsi realistici e registra ogni disegno dell'OLED.
Serve per iterare sull'interfaccia animata **senza hardware**.

```bash
cd firmware/tools/oled_sim
g++ -std=c++17 -Istubs -I. -x c++ sim.cpp -o sim     # serve g++
./sim                                                # scrive frames.jsonl e ble.log
python3 render_preview.py                            # serve Pillow + numpy -> anteprima_oled.png
```
- `ble.log`: ogni notifica BLE con il suo timestamp (controllo di RT, LIVE, SYNC).
- Cosa verifica: compila, il picco misurato (14189 vs 14200 simulato), ~25 Hz di LIVE, frame OLED a 33 ms.
- Cosa NON verifica: font reali, tempi reali dell'I2C, BLE reale, rumore del sensore. Va provato su hardware.
- File generati (`sim`, `frames.jsonl`, `ble.log`, `anteprima_oled.png`) sono ignorati da git.
