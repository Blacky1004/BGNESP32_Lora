#pragma once;
const char setup_index[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    
    <title>LoRa ESP32</title>
    <style>
        %ESP_STYLES%
    </style>
</head>
<body>
    <nav class="navbar navbar-expand navbar-light bg-light">
        <div class="container px-5">
            <a class="navbar-brand" href="javascript:void(0)">BGN32-CHIPID</a>
            <div class="collapse navbar-collapse">
                <ul class="navbar-nav ms-auto mb-2 mb-lg-0">
                    <li class="nav-item">
                        <a class="nav-link active" aria-current="page" href="/wifisetup">WiFi Setup</a>
                    </li>
                    <li class="nav-item">
                        <a class="nav-link" href="/mcusetup">MCU Setup</a>
                    </li>
                </ul>
            </div>
        </div>
    </nav>
    <div class="container px-4 px-lg-5">
        <div class="row gx-4 gx-lg-5 align-items-center my-5">
            <div class="col-lg-4">
                <img class="img-fluid rounded mb-4 mb-lg-0" src="img/logo-buergernetzgeragreiz.png" alt="logo">
            </div>
            <div class="col-lg-8">
                <h1 class="font-weight-light">Bürgernetz Gera-Greiz Feinstaubsensor</h1>
                <p>Hier ne kurze Einführung etc. zum Projekt und wo man sich z.B. bei Problemen hinwenden kann.</p>
            </div>
        </div>
        <div class="card text-white bg-primary my-5 py-4 text-center">
            <div class="card-body">
                <p class="text-white m-0">
                    Wichtige Hinweise zu den untenstehenden Einstellungen
                </p>
            </div>
        </div>
        <div class="row gx-4 gx-lg-5">
            <div class="col-md-6 mb-5">
                <div class="card h-100">
                    <form>
                        <div class="card-body">
                            <h3 class="card-title">WLAN Einstellungen</h3>
                            <p class="card-text">
                                Die MCU benötigt für die Versendung der Sensordaten eine aktive Internetverbindung.
                                Auch wird diese Verbindung für Updates etc. verwendet, da dies nicht über ein verfügbares loRaWAN realisiert werden kann.
                            </p>
                            <div class="mb-3">
                                <label>SSID</label>
                                <select class="form-control" id="ssid" name="ssid">
                                    <option value="null">-- Auswahl --</option>
                                </select>
                            </div>
                            <div class="mb-4">
                                <label>WLAN Passwort</label>
                                <input type="password" class="form-control" id="ssidpasw" name="ssidpasw" />
                            </div>
                        </div>
                        <div class="card-footer">
                            <div class="mt-4 mt-md-0 ms-auto">
                                <button class="btn btn-primary font-weight-medium rounded-pill px-4">Speichern</button>
                            </div>
                        </div>
                    </form>
                </div>
            </div>
            <div class="col-md-6 mb-5">
                <div class="card h-100" id="loraSetup">
                    <form>
                        <div class="card-body">
                            <h3 class="card-title">LoRaWAN Einstellungen</h3>
                            <p class="card-text" id="ltitle"></p>
                            <div class="mb-3">
                                <input type="checkbox" class="form-check-input" id="useLora" name="useLora" />
                                <label for="useLora">LoRaWAN verwenden</label>
                            </div>
                            <div class="mb-3">
                                <p>Sollte der Gateyway ein Single-Gateway sein, so ist hier nur der festeingestellte Kanal 1 zu verwenden.
                                    Die übrigen Kanäle sind zu deaktivieren.
                                </p>
                                <div class="row">
                                    <div class="col-md-3">
                                        <input type="checkbox" class="form-check-input" disabled checked/>
                                        <label>Kanal 1</label>
                                    </div>
                                    <div class="col-md-3">
                                        <input type="checkbox" class="form-check-input"  checked/>
                                        <label>Kanal 2</label>
                                    </div>
                                    <div class="col-md-3">
                                        <input type="checkbox" class="form-check-input"  checked/>
                                        <label>Kanal 3</label>
                                    </div>
                                    <div class="col-md-3">
                                        <input type="checkbox" class="form-check-input"  checked/>
                                        <label>Kanal 4</label>
                                    </div>
                                </div>
                                <div class="row">
                                    <div class="col-md-3">
                                        <input type="checkbox" class="form-check-input"  checked/>
                                        <label>Kanal 5</label>
                                    </div>
                                    <div class="col-md-3">
                                        <input type="checkbox" class="form-check-input"  checked/>
                                        <label>Kanal 6</label>
                                    </div>
                                    <div class="col-md-3">
                                        <input type="checkbox" class="form-check-input"  checked/>
                                        <label>Kanal 7</label>
                                    </div>
                                    <div class="col-md-3">
                                        <input type="checkbox" class="form-check-input"  checked/>
                                        <label>Kanal 8</label>
                                    </div>
                                </div>
                            </div>
                        </div>
                        <div class="card-footer">
                            <div class="mt-3 mt-md-0 ms-auto">
                                <button class="btn btn-primary font-weight-medium rounded-pill px-4">Speichern</button>
                            </div>
                        </div>
                    </form>
                </div>                
            </div>
        </div>
    </div>
    <footer class="py-5 footer-light">

    </footer>
    <script>
        var loraAvailable = true;
        var useLora = false;
        var loraText = 'LoraWAN steht für deinen Standort leider nicht zur Verfügung bzw. ist die Abdeckung nicht aussreichend.';
        document.addEventListener("DOMContentLoaded", () => {
            if(!loraAvailable){
                document.getElementById("ltitle").innerHTML = loraText;
                var loraNodes = document.getElementById('loraSetup').getElementsByTagName('*');
                for(var i = 0; i < loraNodes.length; i++){
                    loraNodes[i].disabled = true;
                }
            }
            else {
                loraText = "Bei Unsicherheit bei diesen Einstellungen wende dich bitte an die Mitglieder des Bürgernetz Gera-Greiz e.V";
                document.getElementById("ltitle").innerHTML = loraText;
                var loraNodes = document.getElementById('loraSetup').getElementsByTagName('*');
                for(var i = 0; i < loraNodes.length; i++){
                    loraNodes[i].disabled = false;
                }
            }
        });
    </script>
    <div class="modal" id="systemRestart">
        <div class="modal-content">
            
        </div>
    </div>
</body>
</html>
)rawliteral";