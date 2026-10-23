import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:syncfusion_flutter_gauges/gauges.dart';
import 'package:shared_preferences/shared_preferences.dart';

// =====================================================================
// NOTA DI REVISIONE (v2 di questo file):
//
// Il file originale fornito conteneva diversi punti troncati e non
// compilabili (liste terminate con ":," invece del contenuto reale).
// La logica di business (scansione BLE, riconnessione automatica,
// parsing pacchetti, sync offline) era gia' corretta e completa ed e'
// stata mantenuta. La UI era stata ricostruita in una revisione
// precedente con un semplice gauge + storico piatto.
//
// Questa versione introduce un modello dati Sessione/Lancio ispirato a
// un'app concorrente ("Beycombat Logger") di cui sono stati mostrati
// screenshot: statistiche aggregate (All Time / Last Session), Top 5
// lanci, storico con conteggio totale, e un flusso di conferma del
// lancio (schermata Results con Save/Cancel) prima di salvarlo
// definitivamente nello storico.
//
// ASSUNZIONE DICHIARATA: l'app di riferimento mostra i lanci in una
// unita' chiamata "LP" (Launch Power) con valori tipici 7000-9900.
// Questo range e' compatibile con gli RPM che il firmware di questo
// progetto misura direttamente (vedi sezione firmware): per questo,
// in questa app, "LP" e semplicemente un'etichetta per lo stesso
// valore RPM restituito dal sensore, NON una formula di conversione
// separata. Se in futuro si scoprisse che l'app concorrente applica
// una formula diversa, va isolata in un'unica funzione di conversione
// (vedi TODO su convertRpmToLaunchPower) senza toccare il resto.
// =====================================================================

void main() {
  FlutterBluePlus.setLogLevel(LogLevel.error);
  runApp(const BBXBattlePassApp());
}

class BBXBattlePassApp extends StatelessWidget {
  const BBXBattlePassApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Beyblade X - Battle Pass Pro',
      theme: ThemeData.dark().copyWith(
        primaryColor: const Color(0xFFCCFF00),
        scaffoldBackgroundColor: const Color(0xFF111111),
        cardColor: const Color(0xFF1C1C1C),
        appBarTheme: const AppBarTheme(
          backgroundColor: Color(0xFFFFFFFF),
          foregroundColor: Color(0xFF000000),
          centerTitle: true,
          elevation: 0,
          titleTextStyle: TextStyle(
            color: Colors.black,
            fontSize: 24,
            fontWeight: FontWeight.w900,
            letterSpacing: 1.5,
            fontStyle: FontStyle.italic,
          ),
        ),
      ),
      home: const DashboardScreen(),
    );
  }
}

// ---------------------------------------------------------------------
// MODELLO DATI
// ---------------------------------------------------------------------

class Launch {
  final double launchPower;
  final int sessionNumber;
  final DateTime timestamp;
  final String source;

  Launch({
    required this.launchPower,
    required this.sessionNumber,
    required this.timestamp,
    required this.source,
  });

  Map<String, dynamic> toJson() => {
        'launchPower': launchPower,
        'sessionNumber': sessionNumber,
        'timestamp': timestamp.toIso8601String(),
        'source': source,
      };

  factory Launch.fromJson(Map<String, dynamic> json) => Launch(
        launchPower: (json['launchPower'] as num).toDouble(),
        sessionNumber: json['sessionNumber'] as int,
        timestamp: DateTime.parse(json['timestamp'] as String),
        source: json['source'] as String,
      );
}

double convertRpmToLaunchPower(double rpm) => rpm;

class LaunchRepository {
  static const _storageKey = 'bbx_launch_history_v1';
  static const _sessionKey = 'bbx_current_session_v1';

  Future<List<Launch>> loadAll() async {
    final prefs = await SharedPreferences.getInstance();
    final raw = prefs.getString(_storageKey);
    if (raw == null) return [];
    final List<dynamic> decoded = jsonDecode(raw);
    return decoded.map((e) => Launch.fromJson(e as Map<String, dynamic>)).toList();
  }

  Future<void> saveAll(List<Launch> launches) async {
    final prefs = await SharedPreferences.getInstance();
    final encoded = jsonEncode(launches.map((l) => l.toJson()).toList());
    await prefs.setString(_storageKey, encoded);
  }

  Future<void> addLaunch(Launch launch) async {
    final all = await loadAll();
    all.insert(0, launch);
    await saveAll(all);
  }

  Future<int> getOrCreateSessionNumber(List<Launch> existingLaunches) async {
    final prefs = await SharedPreferences.getInstance();
    if (existingLaunches.isEmpty) {
      await prefs.setInt(_sessionKey, 1);
      return 1;
    }
    final lastLaunch = existingLaunches.first;
    final elapsed = DateTime.now().difference(lastLaunch.timestamp);
    if (elapsed.inHours >= 1) {
      final nextSession = lastLaunch.sessionNumber + 1;
      await prefs.setInt(_sessionKey, nextSession);
      return nextSession;
    }
    return lastLaunch.sessionNumber;
  }

  Future<void> clearAll() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove(_storageKey);
    await prefs.remove(_sessionKey);
  }
}

// ---------------------------------------------------------------------
// SCHERMATA PRINCIPALE
// ---------------------------------------------------------------------

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({Key? key}) : super(key: key);
  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  BluetoothDevice? targetDevice;
  BluetoothCharacteristic? txCharacteristic, rxCharacteristic;
  StreamSubscription? txSubscription;
  StreamSubscription<BluetoothConnectionState>? connectionSubscription;

  bool isScanning = false, isConnected = false, isSyncing = false;
  double currentRPM = 0.0;

  final LaunchRepository _repo = LaunchRepository();
  List<Launch> allLaunches = [];
  int currentSession = 1;

  double? pendingLaunchPower;

  final String deviceName = "BBX_BattlePass_PRO";
  final String serviceUuid = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
  final String txUuid = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
  final String rxUuid = "beb5483e-36e1-4688-b7f5-ea07361b26a9";

  @override
  void initState() {
    super.initState();
    _loadHistory();
    _requestPermissionsAndStart();
  }

  Future<void> _loadHistory() async {
    final launches = await _repo.loadAll();
    final session = await _repo.getOrCreateSessionNumber(launches);
    setState(() {
      allLaunches = launches;
      currentSession = session;
    });
  }

  Future<void> _requestPermissionsAndStart() async {
    if (Platform.isAndroid) {
      await [
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
        Permission.locationWhenInUse,
      ].request();
    }
    scanForBattlePass();
  }

  void scanForBattlePass() async {
    if (isScanning) return;
    setState(() {
      isScanning = true;
      currentRPM = 0.0;
    });

    await FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));

    FlutterBluePlus.scanResults.listen((results) {
      for (var r in results) {
        if (r.device.platformName == deviceName ||
            r.device.platformName.startsWith("BBX")) {
          FlutterBluePlus.stopScan();
          setState(() {
            isScanning = false;
            targetDevice = r.device;
          });
          connectToDevice(r.device);
          break;
        }
      }
    });

    Future.delayed(const Duration(seconds: 10), () {
      if (mounted) setState(() => isScanning = false);
    });
  }

  void connectToDevice(BluetoothDevice device) async {
    try {
      connectionSubscription = device.connectionState.listen((state) {
        if (state == BluetoothConnectionState.disconnected) {
          setState(() {
            isConnected = false;
            currentRPM = 0.0;
          });
          scanForBattlePass();
        } else if (state == BluetoothConnectionState.connected) {
          setState(() => isConnected = true);
        }
      });

      await device.connect();
      if (Platform.isAndroid) await device.requestMtu(512);

      List<BluetoothService> services = await device.discoverServices();
      for (var service in services) {
        if (service.uuid.toString() == serviceUuid) {
          for (var char in service.characteristics) {
            if (char.uuid.toString() == txUuid) {
              txCharacteristic = char;
              await char.setNotifyValue(true);
              txSubscription = char.onValueReceived.listen(parseBluetoothPacket);
            } else if (char.uuid.toString() == rxUuid) {
              rxCharacteristic = char;
            }
          }
        }
      }
    } catch (e) {
      setState(() => isConnected = false);
    }
  }

  void parseBluetoothPacket(List<int> value) {
    String data = utf8.decode(value);

    if (data.startsWith("LIVE:")) {
      setState(() => currentRPM = double.tryParse(data.substring(5)) ?? 0.0);
    } else if (data.startsWith("RT:")) {
      double maxRPM = double.tryParse(data.substring(3)) ?? 0.0;
      setState(() {
        currentRPM = maxRPM;
        pendingLaunchPower = convertRpmToLaunchPower(maxRPM);
      });
      _showResultsSheet(source: "Live");
    } else if (data.startsWith("SYNC_COUNT:")) {
      setState(() => isSyncing = true);
      ScaffoldMessenger.of(context).showSnackBar(const SnackBar(
        content: Text("Download lanci offline..."),
        backgroundColor: Color(0xFFCCFF00),
      ));
    } else if (data.startsWith("S:")) {
      double offRPM = double.tryParse(data.substring(2)) ?? 0.0;
      _persistLaunch(convertRpmToLaunchPower(offRPM), source: "Offline");
    } else if (data == "SYNC_END") {
      setState(() => isSyncing = false);
      ScaffoldMessenger.of(context).showSnackBar(const SnackBar(
        content: Text(
          "Sincronizzazione completata!",
          style: TextStyle(color: Colors.black),
        ),
        backgroundColor: Color(0xFFCCFF00),
      ));
    }
  }

  Future<void> _persistLaunch(double launchPower, {required String source}) async {
    final launch = Launch(
      launchPower: launchPower,
      sessionNumber: currentSession,
      timestamp: DateTime.now(),
      source: source,
    );
    await _repo.addLaunch(launch);
    final updated = await _repo.loadAll();
    setState(() => allLaunches = updated);
  }

  void _showResultsSheet({required String source}) {
    showModalBottomSheet(
      context: context,
      isDismissible: false,
      enableDrag: false,
      backgroundColor: Theme.of(context).cardColor,
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(20)),
      ),
      builder: (ctx) {
        return Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              const Text(
                "Results",
                style: TextStyle(fontSize: 22, fontWeight: FontWeight.bold),
              ),
              const SizedBox(height: 16),
              SizedBox(
                height: 220,
                child: SfRadialGauge(
                  axes: <RadialAxis>[
                    RadialAxis(
                      minimum: 0,
                      // CORRETTO: allineato al tetto teorico del firmware
                      // dopo la ricalibrazione del debounce (~40.000 RPM),
                      // non piu' 30.000 che tagliava il fondoscala troppo
                      // vicino ai lanci forti osservati (15.000+ RPM).
                      maximum: 40000,
                      startAngle: 150,
                      endAngle: 30,
                      axisLineStyle: const AxisLineStyle(
                        thickness: 0.15,
                        thicknessUnit: GaugeSizeUnit.factor,
                      ),
                      pointers: <GaugePointer>[
                        NeedlePointer(
                          value: (pendingLaunchPower ?? 0).clamp(0, 40000),
                          needleColor: Colors.white,
                          knobStyle: const KnobStyle(color: Color(0xFFCCFF00)),
                        ),
                      ],
                      annotations: <GaugeAnnotation>[
                        GaugeAnnotation(
                          positionFactor: 0.1,
                          angle: 90,
                          widget: Text(
                            "Max Launch Power:\n${(pendingLaunchPower ?? 0).toStringAsFixed(0)}",
                            textAlign: TextAlign.center,
                            style: const TextStyle(
                              fontSize: 20,
                              fontWeight: FontWeight.bold,
                              color: Colors.white,
                            ),
                          ),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
              const SizedBox(height: 20),
              Row(
                children: [
                  Expanded(
                    child: OutlinedButton(
                      onPressed: () {
                        setState(() => pendingLaunchPower = null);
                        Navigator.pop(ctx);
                      },
                      child: const Text("Cancel"),
                    ),
                  ),
                  const SizedBox(width: 12),
                  Expanded(
                    child: ElevatedButton(
                      style: ElevatedButton.styleFrom(
                        backgroundColor: const Color(0xFFCCFF00),
                        foregroundColor: Colors.black,
                      ),
                      onPressed: () async {
                        if (pendingLaunchPower != null) {
                          await _persistLaunch(pendingLaunchPower!, source: source);
                        }
                        setState(() => pendingLaunchPower = null);
                        Navigator.pop(ctx);
                      },
                      child: const Text("SAVE"),
                    ),
                  ),
                ],
              ),
            ],
          ),
        );
      },
    );
  }

  void requestSync() {
    if (rxCharacteristic != null && isConnected) {
      rxCharacteristic!.write(utf8.encode("SYNC"));
    }
  }

  Future<void> clearOfflineData() async {
    if (rxCharacteristic != null && isConnected) {
      rxCharacteristic!.write(utf8.encode("CLEAR"));
    }
  }

  Future<void> clearLocalHistory() async {
    await _repo.clearAll();
    await _loadHistory();
  }

  @override
  void dispose() {
    txSubscription?.cancel();
    connectionSubscription?.cancel();
    targetDevice?.disconnect();
    super.dispose();
  }

  double get allTimeLaunchPower =>
      allLaunches.isEmpty ? 0 : allLaunches.map((l) => l.launchPower).reduce((a, b) => a > b ? a : b);

  double get lastSessionLaunchPower {
    final lastSessionLaunches = allLaunches.where((l) => l.sessionNumber == currentSession).toList();
    if (lastSessionLaunches.isEmpty) return 0;
    return lastSessionLaunches.map((l) => l.launchPower).reduce((a, b) => a > b ? a : b);
  }

  int get launchCount => allLaunches.length;

  int get sessionCount => allLaunches.map((l) => l.sessionNumber).toSet().length;

  List<Launch> get top5Launches {
    final sorted = [...allLaunches]..sort((a, b) => b.launchPower.compareTo(a.launchPower));
    return sorted.take(5).toList();
  }

  Widget _buildConnectionBadge() {
    final Color color = isConnected ? const Color(0xFFCCFF00) : Colors.redAccent;
    final String label = isConnected
        ? "CONNESSO"
        : (isScanning ? "RICERCA IN CORSO..." : "NON CONNESSO");
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        Container(
          width: 10,
          height: 10,
          decoration: BoxDecoration(color: color, shape: BoxShape.circle),
        ),
        const SizedBox(width: 8),
        Text(
          label,
          style: TextStyle(color: color, fontWeight: FontWeight.bold, letterSpacing: 1.2),
        ),
      ],
    );
  }

  Widget _buildStatBox(String title, String subtitle, String value, String unit) {
    return Expanded(
      child: Container(
        margin: const EdgeInsets.all(4),
        padding: const EdgeInsets.all(14),
        decoration: BoxDecoration(
          color: Theme.of(context).cardColor,
          borderRadius: BorderRadius.circular(14),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: const TextStyle(fontSize: 14, color: Colors.white70)),
            Text(subtitle, style: const TextStyle(fontSize: 11, color: Colors.grey)),
            const SizedBox(height: 10),
            Row(
              crossAxisAlignment: CrossAxisAlignment.baseline,
              textBaseline: TextBaseline.alphabetic,
              children: [
                Text(
                  value,
                  style: const TextStyle(
                    fontSize: 26,
                    fontWeight: FontWeight.bold,
                    color: Color(0xFFCCFF00),
                  ),
                ),
                const SizedBox(width: 4),
                Text(unit, style: const TextStyle(fontSize: 12, color: Colors.grey)),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildTop5Card() {
    return Container(
      padding: const EdgeInsets.all(14),
      decoration: BoxDecoration(
        color: const Color(0xFFCCFF00),
        borderRadius: BorderRadius.circular(16),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            "Top 5 Launches",
            style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Colors.black),
          ),
          const SizedBox(height: 10),
          if (top5Launches.isEmpty)
            const Padding(
              padding: EdgeInsets.symmetric(vertical: 8),
              child: Text("Nessun lancio ancora registrato", style: TextStyle(color: Colors.black87)),
            )
          else
            ...top5Launches.map((l) => Container(
                  margin: const EdgeInsets.symmetric(vertical: 4),
                  padding: const EdgeInsets.all(10),
                  decoration: BoxDecoration(
                    color: const Color(0xFF111111),
                    borderRadius: BorderRadius.circular(10),
                  ),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            "${l.launchPower.toStringAsFixed(0)} LP",
                            style: const TextStyle(
                              color: Color(0xFFCCFF00),
                              fontWeight: FontWeight.bold,
                              fontSize: 16,
                            ),
                          ),
                          Text("Session ${l.sessionNumber}", style: const TextStyle(color: Colors.grey, fontSize: 12)),
                        ],
                      ),
                      Text(
                        "${l.timestamp.day}/${l.timestamp.month}/${l.timestamp.year}",
                        style: const TextStyle(color: Colors.white70, fontSize: 12),
                      ),
                    ],
                  ),
                )),
        ],
      ),
    );
  }

  Widget _buildHistoryList() {
    return Expanded(
      child: Container(
        decoration: BoxDecoration(
          color: Theme.of(context).cardColor,
          borderRadius: BorderRadius.circular(16),
        ),
        child: Column(
          children: [
            Padding(
              padding: const EdgeInsets.all(12),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Text("Launches (${allLaunches.length})",
                      style: const TextStyle(fontWeight: FontWeight.bold)),
                  IconButton(
                    icon: const Icon(Icons.delete_outline, size: 20),
                    tooltip: "Cancella storico locale",
                    onPressed: allLaunches.isEmpty ? null : clearLocalHistory,
                  ),
                ],
              ),
            ),
            Expanded(
              child: allLaunches.isEmpty
                  ? const Center(
                      child: Text("Nessun lancio ancora registrato", style: TextStyle(color: Colors.grey)),
                    )
                  : ListView.builder(
                      itemCount: allLaunches.length,
                      itemBuilder: (c, i) {
                        final l = allLaunches[i];
                        return ListTile(
                          leading: Icon(
                            l.source == "Live" ? Icons.speed : Icons.cloud_download,
                            color: l.source == "Live" ? Colors.white : const Color(0xFFCCFF00),
                          ),
                          title: Text(
                            "${l.launchPower.toStringAsFixed(0)} LP",
                            style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 18),
                          ),
                          subtitle: Text("Session ${l.sessionNumber}"),
                          trailing: Text(
                            "${l.timestamp.day}/${l.timestamp.month}/${l.timestamp.year}",
                            style: const TextStyle(color: Colors.white70, fontSize: 12),
                          ),
                        );
                      },
                    ),
            ),
          ],
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('BEYBLADE X')),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            children: [
              _buildConnectionBadge(),
              const SizedBox(height: 12),
              Row(
                children: [
                  _buildStatBox("All Time", "Launch Power", allTimeLaunchPower.toStringAsFixed(0), "LP"),
                  _buildStatBox("Last Session", "Launch Power", lastSessionLaunchPower.toStringAsFixed(0), "LP"),
                ],
              ),
              Row(
                children: [
                  _buildStatBox("Launch Count", "Total Launches", "$launchCount", "Launches"),
                  _buildStatBox("Session Count", "Total Sessions", "$sessionCount", "Sessions"),
                ],
              ),
              const SizedBox(height: 8),
              _buildTop5Card(),
              const SizedBox(height: 12),
              _buildHistoryList(),
              const SizedBox(height: 10),
              Row(
                children: [
                  Expanded(
                    child: ElevatedButton.icon(
                      onPressed: isConnected && !isSyncing ? requestSync : null,
                      icon: const Icon(Icons.sync),
                      label: Text(isSyncing ? "Sincronizzo..." : "Sincronizza offline"),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: const Color(0xFFCCFF00),
                        foregroundColor: Colors.black,
                      ),
                    ),
                  ),
                  const SizedBox(width: 10),
                  IconButton(
                    onPressed: isConnected ? clearOfflineData : null,
                    icon: const Icon(Icons.delete_forever_outlined),
                    tooltip: "Cancella lanci offline salvati sul firmware",
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }
}
