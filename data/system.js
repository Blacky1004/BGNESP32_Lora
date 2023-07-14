
var useLora = false;
var useWLAN = false;
var loraText = 'LoraWAN steht für deinen Standort leider nicht zur Verfügung bzw. ist die Abdeckung nicht aussreichend.';
var lcnt=0;
var hasInternet = false;
var sensorDatas = {
    pm10: [0,0,0,0,0,0,0,0,0,0],
    pm25 : [0,0,0,0,0,0,0,0,0,0],
    tmp: [0,0,0,0,0,0,0,0,0,0],
    hum: [0,0,0,0,0,0,0,0,0,0]
};
var pmChart;
var tcChart;
$(document).ready(function() {
    //Beim start immer die DashbordDiv aktivieren, den rest disablen
    showConnections();
    if(!loraAvailable){
        document.getElementById("ltitle").innerHTML = loraText;
        var loraNodes = document.getElementById('loraSetup').getElementsByTagName('*');
        for(var i = 0; i < loraNodes.length; i++){
            loraNodes[i].disabled = true;
        }
        document.getElementById("useLora").checked = true;
    }
    else {
        loraText = "Bei Unsicherheit bei diesen Einstellungen wende dich bitte an die Mitglieder des Bürgernetz Gera-Greiz e.V";
        document.getElementById("ltitle").innerHTML = loraText;
        var loraNodes = document.getElementById('loraSetup').getElementsByTagName('*');
        for(var i = 0; i < loraNodes.length; i++){
            loraNodes[i].disabled = false;
        }        
    }        
    const pmData = {
            labels: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10"],
            datasets: [
                {
                    label: '2.5 µg/m3',
                    data: sensorDatas["pm25"],
                },
                {
                    label: '10 µg/m3',
                    data: sensorDatas["pm10"],
                }
            ]
        };
    const tcData = {
        labels: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10"],
        datasets:[
            {
                label: 'Temperaturn in °C',
                data: sensorDatas["tmp"]
            },
            {
                label: 'Humidity in %',
                data: sensorDatas["hum"]
            }
        ]
    };
    const pmConfig = {
        type: 'line',
        data: pmData,
    };
    const tcConfig = {
        type: 'line',
        data: tcData
    };
    const cPMChart = document.getElementById("fsdaten");
    pmChart = new Chart(cPMChart, pmConfig);
    const cTcChart = document.getElementById("tempdaten");
    tcChart = new Chart(cTcChart, tcConfig);            
        
    
    getConfigDatas();
    setInterval(function() {
        lcnt = 0;
        getLoraInfo();
        // try {
        //     var testImg = new Image();
        //     testImg.src = "https://www.freifunk-gera-greiz.de/images/logo-buergernetzgeragreiz.png";
        //     if(testImg.height > 0){
        //         hasInternet = true;
        //     }
        //     else{
        //         hasInternet = false;
        //     }
        // } catch (error) {
        //     hasInternet = false;
        // }
    }, 10000);
    setInterval(() => {
        ajaxCharts();
    }, 60000);
    ajaxCharts();

    setTimeout(function(){ 
        $('.preloader').addClass('preloader-deactivate');
    }, 3000);

});

document.getElementById("useLora").addEventListener('click', function(e) {
    if(checkConnectionAvailable()){
        document.getElementById("useLora").checked = true;
    }
    var loraNodes = document.getElementById('loraSetup').getElementsByTagName('*');
    for(var i = 0; i < loraNodes.length; i++){
        if(document.getElementById("useLora").checked) {
            loraNodes[i].disabled = false;
        }
        else {
            loraNodes[i].disabled = true;
            document.getElementById("useLora").disabled = false;
            
        }                    
    }
    document.getElementById("loracheck").disabled = false;        
});
document.getElementById("loracheck").addEventListener('click', function(e) {
    let nkey = document.getElementById("nwkskey").value.match(/.{1,2}/g) ?? [];
    let akey = document.getElementById("appskey").value.match(/.{1,2}/g) ?? [];
    let did = document.getElementById("devaddr").value;
    let devId = document.getElementById("devid").value;
    if(nkey.length != 16){

    }
    if(akey.length != 16){

    }
    let akeyint = [];
    let nkeyInt=[];
    let devkey = hex2num(did);
    for(var i=0; i < nkey.length; i++){
        nkeyInt[i] = hex2num(nkey[i]);
    }

    for(var i= 0; i < akey.length; i++){
    akeyint[i] = hex2num(akey[i]);
    }

    let json = {
        nwkey: nkeyInt,
        apkey: akeyint,
        devkey: devkey,
        devid: devId,
        enabled: document.getElementById("useLora").checked
    };

    $.ajax({
        url: '/test_lora',
        type: 'post',
        data: JSON.stringify(json),
        dataType: "json",
        contentType: "application/json",
        success: function(response) {
            if(response["code"] == 200){
                if(response["need_restart"] == true){
                    //Öffnen des ModalDialoges
                    $('#restartmodal').modal('show');
                    //reset des ESP senden
                    $.getJSON("/restart",{});
                    // 30 Sekunden warten dann neu laden
                    window.setTimeout(function() {
                        window.location.href = 'http://'+response["uri"];
                    }, 30000);
                } else{
                    toastr.success('Die Änderungen wurden erfolgreich gespeichert.', "Änderungen gespeichert", {timeOut: 5000});
                }
            } else{
                toastr.error(response['message'], "FEHLER!!!", {timeOut: 5000});
            }
        },
        error: function(error) {
            console.log(error);
        }
    });    
});
document.getElementById("lgimg").addEventListener('click', function(e) {
    if(expertMode) return;
    lcnt++;
    if(lcnt == 5){
        expertMode = true;
        let expertMenu = '<li class="nav-item"><a class="nav-link" href="javascript:void(0)" id="lnkSystem" onclick="showSystem()">System</a></li>';
        document.getElementById("nmenu").insertAdjacentHTML('beforeend', expertMenu);
    }
});
document.getElementById("checkgps").addEventListener("click", function(e) {
    getGpsLocation();
});


function getConfigDatas(){
    
    getLoraInfo();
    $.getJSON("/get_sensors", function(result) {
        if(result["gps"])
        {
            if(result["gps"]["enabled"] && result["gps"]["valid"]){
                document.getElementById("lat").value = result["gps"]["lat"];
                document.getElementById("lng").value = result["gps"]["lng"];
            }
        }
        if(result["cfg"]) {
            document.getElementById("fsize").innerHTML = result["cfg"]["flash"] + "MB";
            document.getElementById("fversion").innerHTML =  result["cfg"]["version"];
            document.getElementById("fheap").innerHTML =  formatSizeUnits(result["cfg"]["freeheap"]) + " frei von " + formatSizeUnits(result["cfg"]["heap"]);
            
        }
        if(result["wlan"]) {
            document.getElementById("wmode").innerHTML = result["wlan"]["mode"];
            document.getElementById("wssid").innerHTML = result["wlan"]["ssid"];
            document.getElementById("wip").innerHTML = result["wlan"]["ip"];
        }
    });

    $.getJSON("/get_wifi_list", function(result) {
        $("#ssid").empty();
        $("#ssid").append($('<option></option>').val("-1").html("--- Nur als Accesspoint ---"));
        
        if(result["wifis"] != undefined && result["wifis"].length > 0) {                        
            for(var i = 0; i < result["wifis"].length; i++) {
                $("#ssid").append($('<option></option>').val(result["wifis"][i]["ssid"]).html(result["wifis"][i]["ssid"]));
            }                 
        }
    });
}
function getWifiImage(rssi){
    if(rssi >= 0)
        return "wifi_err.png";
    
    let image = "wifi_err.png";
    if(rssi <= -30 && rssi > -59)
        image = "wifi_4.png";
    else if(rssi <= -60 && rssi > -69)
        image = "wifi_3.png";
    else if(rssi <= -70 && rssi > -79)
        image = "wifi_2.png";
    else if(rssi <= -80 && rssi >- 89)
        image = "wifi_1.png";
    else if(rssi <= -90)
        image = "wifi_0.png";
}

function ajaxCharts() {
    $.getJSON("/get_chartdatas", function(result) {
        sensorDatas = result;
        pmChart.data.datasets[0].data = sensorDatas["pm25"];
        pmChart.data.datasets[1].data = sensorDatas["pm10"];
        tcChart.data.datasets[0].data = sensorDatas["tmp"];
        tcChart.data.datasets[1].data = sensorDatas["hum"];
        pmChart.update();
        tcChart.update();
    });
}
function checkwifi(){
    let ssid = document.getElementById("ssid").value;
    let pasw = document.getElementById("ssidpasw").value;
    let json = {
        ssid: ssid,
        pasw: pasw
    };
    if(ssid == ""){
        toastr.error("Das Feld SSID darf nicht leer sein!", "FEHLER", {timeOut: 5000});
        return;
    }



    $.ajax({
        url: "/save_wifi",
            type: 'post',
            dataType: "json",
            contentType: "application/json",
            data: JSON.stringify(json),
            success: function(response) {
                if(response && response["code"]) {
                    switch(response["code"]) {
                        case 200:
                            toastr.success("Die Speicherung Der WLAN-Verbindung war erfolgreich. Das System startet nun mit deinen Einstellungen neu.", "Erfolgreich", {timeOut: 2500})
                            setTimeout( function() {
                                window.open("http:\\" + response["ip"]);
                            }, 3000);
                        break;

                        case 300:
                            toastr.warning(response["message"], "Warnung", {timeOut: 5000})
                        default:
                        case 400:
                            toastr.error(response["message"], "FEHLER", {timeOut: 5000})
                        break;
                    }
                }
                
            },
            error: function(error) {
                toastr.error(error["message"], "Fehler!",   {timeOut: 5000});
            }
    });
}
function showConnections(){
    document.getElementById("lnkHome").classList.remove("active");
    document.getElementById("lnkConnections").classList.remove("active");
    document.getElementById("lnkSensors").classList.remove("active");
    if(expertMode)
        document.getElementById("lnkSystem").classList.remove("active");
    document.getElementById("lnkConnections").classList.add("active");

    document.getElementById("pDashboerd").style.display = "none"
    document.getElementById("pConnections").style.display = "flex"
    document.getElementById("pSensors").style.display = "none"
    document.getElementById("pSystem").style.display = "none"

    document.getElementById("street").disabled = !hasInternet;
    document.getElementById("plz").disabled = !hasInternet;
    document.getElementById("ort").disabled = !hasInternet;
    document.getElementById("checkgps").disabled = !hasInternet;
}
function showSensors() {
    document.getElementById("lnkHome").classList.remove("active");
    document.getElementById("lnkConnections").classList.remove("active");
    document.getElementById("lnkSensors").classList.remove("active");
    if(expertMode)
        document.getElementById("lnkSystem").classList.remove("active");

    document.getElementById("lnkSensors").classList.add("active");
    document.getElementById("pDashboerd").style.display = "none"
    document.getElementById("pConnections").style.display = "none"
    document.getElementById("pSensors").style.display = "flex"           
    document.getElementById("pSystem").style.display = "none"
}
function showDasboard(){
    document.getElementById("lnkHome").classList.remove("active");
    document.getElementById("lnkConnections").classList.remove("active");
    document.getElementById("lnkSensors").classList.remove("active");

    if(expertMode)
        document.getElementById("lnkSystem").classList.remove("active");

    document.getElementById("lnkHome").classList.add("active");

    document.getElementById("pDashboerd").style.display = "flex"
    document.getElementById("pConnections").style.display = "none"
    document.getElementById("pSensors").style.display = "none"
    document.getElementById("pSystem").style.display = "none"
    getLoraInfo();
}
function showSystem() {
    document.getElementById("lnkHome").classList.remove("active");
    document.getElementById("lnkConnections").classList.remove("active");
    document.getElementById("lnkSensors").classList.remove("active");
    document.getElementById("lnkSystem").classList.remove("active");

    document.getElementById("lnkSystem").classList.add("active");

    document.getElementById("pSystem").style.display = "flex"
    document.getElementById("pDashboerd").style.display = "none"
    document.getElementById("pConnections").style.display = "none"
    document.getElementById("pSensors").style.display = "none"
    getAllCfgFiles();
}
function hex2num(hexcode){ return Number(  '0x' + hexcode.split(/[^0-9a-fA-F]+/).join('')  ) }

function formatSizeUnits(bytes){
    if      (bytes >= 1073741824) { bytes = (bytes / 1073741824).toFixed(2) + " GB"; }
    else if (bytes >= 1048576)    { bytes = (bytes / 1048576).toFixed(2) + " MB"; }
    else if (bytes >= 1024)       { bytes = (bytes / 1024).toFixed(2) + " KB"; }
    else if (bytes > 1)           { bytes = bytes + " bytes"; }
    else if (bytes == 1)          { bytes = bytes + " byte"; }
    else                          { bytes = "0 bytes"; }
    return bytes;
}

function checkConnectionAvailable(){
    if((loraAvailable == false || document.getElementById("useLora").checked == false) &&  document.getElementById("enableWlan").checked  == false) {
        alert("Es muss mindestens eine Verbindung konfiguriert sein!");
        document.getElementById("savewlan").disabled = true;
        document.getElementById("loracheck").disabled = true;
        return true;
    } else{
        document.getElementById("savewlan").disabled = false;
        document.getElementById("loracheck").disabled = false;
        return false;
    }
}

function getLoraInfo() {
    $.getJSON("/get_lora_info", function(response) {
        let loraOn = false;
        document.getElementById("loramode").innerHTML = "";
        document.getElementById("lorastatus").classList.remove("text-danger", "text-primary", "text-success");
        document.getElementById("dlstatus").classList.remove("text-danger", "text-primary", "text-success");
        //document.getElementById("devid").value = response["lora"]["devid"];
        if(response) {
            if(response["status"] > 0)
                loraOn = true;
            
            switch(response["status"]) {
                case 0: 
                    document.getElementById("lorastatus").innerHTML = "n/a"; 
                    document.getElementById("lorastatus").classList.add("text-danger"); 
                    document.getElementById("dlstatus").innerHTML = "n/a"; 
                    document.getElementById("dlstatus").classList.add("text-danger"); 
                    break;
                case 1:
                    document.getElementById("lorastatus").innerHTML = "Initialisierung"; 
                    document.getElementById("lorastatus").classList.add("text-primary"); 
                    document.getElementById("dlstatus").innerHTML = "Initialisierung"; 
                    document.getElementById("dlstatus").classList.add("text-primary"); 
                    break;
                case 2:
                    document.getElementById("lorastatus").innerHTML = "Initialisiert";
                    document.getElementById("lorastatus").classList.add("text-primary");
                    document.getElementById("dlstatus").innerHTML = "Initialisiert";
                    document.getElementById("dlstatus").classList.add("text-primary");
                    break;
                case 3:
                    document.getElementById("lorastatus").innerHTML = "Joining";
                    document.getElementById("lorastatus").classList.add("text-primary");
                    document.getElementById("dlstatus").innerHTML = "Joining";
                    document.getElementById("dlstatus").classList.add("text-primary");
                   break;
                case 5:
                    document.getElementById("lorastatus").innerHTML = "Wait Join";
                    document.getElementById("lorastatus").classList.add("text-primary"); 
                    document.getElementById("dlstatus").innerHTML = "Wait Join";
                    document.getElementById("dlstatus").classList.add("text-primary"); 
                    break;
                case 4:
                    document.getElementById("lorastatus").innerHTML = "Joined";
                    document.getElementById("lorastatus").classList.add("text-success");
                    document.getElementById("dlstatus").innerHTML = "Joined";
                    document.getElementById("dlstatus").classList.add("text-success");
                    break;
            }
            if(response["mode"] == "OTAA") {
                let otamode =`<div class="mb-3"><input type="checkbox" class="form-check-input" id="useLora" name="useLora" ${response["status"] > 0 ? "checked" : ""}/><label for="useLora">LoRaWAN verwenden</label></div>`;
                                
                let deveui = "";
                if(response["deveui"] && response["deveui"].length > 0){
                    for(var i = 0; i < response["deveui"].length; i++) {
                        deveui += response["deveui"][i].toString(16);
                    }
                }
                otamode += `<div class="mb-3"><label for="deveui">DeviceEUI</label><input id="deveui" name="deveui" class="form-control" value="${deveui.toUpperCase()}" placeholder="eui-0000000000000000" /></div>`;
                
                let appeui = "";
                if(response["appeui"] && response["appeui"].length > 0){
                    for(var i = 0; i < response["appeui"].length; i++) {
                        appeui += response["appeui"][i].toString(16);
                    }
                }
                otamode += `<div class="mb-3"><label for="appeui">AppEUI</label><input id="appeui" name="appeui" class="form-control" value="${appeui.toUpperCase()}" placeholder="eui-0000000000000000" /></div>`

                let appkey= "";
                if(response["appkey"] && response["appkey"].length > 0){
                    for(var i = 0; i < response["appkey"].length; i++) {
                        appkey += response["appkey"][i].toString(16);
                    }
                }      
                otamode += `<div class="mb-3"><label for="appkey">AppKey<span class="text-danger">*</span></label><input type="text" class="form-control" id="appkey" name="appkey" value="${appkey.toUpperCase()}" required placeholder="00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"/></div>`;
                document.getElementById("loramode").innerHTML = otamode;
            }
            if(response["devaddr"] && response["devaddr"].length > 0) {
                document.getElementById("ldid").innerHTML = "0x" + response["devaddr"].toUpperCase();
            } else {
                document.getElementById("ldid").innerHTML = "-";
            }
            if(response["netid"] && response["netid"] > 0) {
                document.getElementById("netid").innerHTML = response["netid"];
            } else {
                document.getElementById("netid").innerHTML = "-";
            }
            document.getElementById("lcycle").innerHTML = response["lcycle"] + " Sek"
            document.getElementById("lqueue").innerHTML = response["lwaitings"];
            if(response["lpayload"] && response["lpayload"] != 0) {
               
                var pdate = new Date(response["lpayload"]* 1000);
                var day = pdate.getDay() < 10 ? "0"+pdate.getDay() : pdate.getDay();
                var mon = pdate.getMonth() < 10 ? "0"+pdate.getMonth() : pdate.getMonth();
                var hour = pdate.getHours() < 10 ? "0" + pdate.getHours() : pdate.getHours();
                var minute = pdate.getMinutes() < 10 ? "0" + pdate.getMinutes() : pdate.getMinutes();
                var seks = pdate.getSeconds() < 10 ? "0" + pdate.getSeconds() : pdate.getSeconds();
                //document.getElementById("npl").innerHTML = `${day}.${mon}.${pdate.getFullYear()} ${hour}:${minute}:${seks}`;
                document.getElementById("npl").innerHTML = `${pdate.toLocaleDateString("de-DE")} ${pdate.toLocaleTimeString("de-DE")}`;
            } else {
                document.getElementById("npl").innerHTML = "-";
            }
            document.getElementById("lradio").innerHTML = response["rparams"];
        }                
    });
}

document.getElementById("btnSetDF").addEventListener("click", function(e) {
    $("#m_factoryDefault").modal("hide");
});

document.getElementById("btncloseDF").addEventListener("click", function(e) {
    $.getJSON("/factory_defaults", function(result) {
        if(result["code"] == 200) {
            toastr.success("Das System wurde erfolgreich auf Werkseinstellung zurückgesetzt und startet nun neu.", "Werkseintellungen",   {timeOut: 5000});
        }
    })
});

document.getElementById("sysRestart").addEventListener("click", function(e) {
    $.getJSON("/restart", function(result) {
        toastr.success("Das System wird nun neu gestartet und sollte in ca. 30 Sekunden wieder zur Verfügung stehen.", "Systemneustart",   {timeOut: 5000});
    });

    this.style.disabled = true;
});
document.getElementById("savelocation").addEventListener("click", function(e) {
    let lat = document.getElementById("lat").value;
    let lon = document.getElementById("lng").value;
    if((lat != "" || lat > 0) && (lon != "" && lon) > 0){
        let json = {
            latitude: lat,
            longitude: lon
        };
        $.ajax({
            url: "/save_location",
            type: 'post',
            dataType: "json",
            contentType: "application/json",
            data: JSON.stringify(json),
            success: function(response) {
                toastr.success("Die Speicherung deiner Standortdaten war erfolgreich.", "Erfolgreich", {timeOut: 5000})
            },
            error: function(error) {
                toastr.error(error["message"], "Fehler!",   {timeOut: 5000});
            }
        });
    }
});
function setFactoryDefault() {
    //Sicherheitsabfrage ob das wirklich gemacht werden soll
    $("#m_factoryDefault").modal("show");
    //JA
    $.getJSON("/factory_defaults",{});
}

function getAllCfgFiles() {
    var fileList = "";
    $.getJSON("/get_cfiles", function(result) {
        if(result["code"] == 200){
            for(var i = 0; i < result["files"].length; i++){
                fileList += '<li onclick="getFileContent('+ result["files"][i]+')" style="cursor: pointer;">';
                fileList += '<img src="configuration.png" width="32">';
                fileList += '<span class="text-field">'+ result["files"][i]+'</span>';
            }
        }
    });
    document.getElementById("files").innerHTML = fileList;
}

function getGpsLocation(){
    if(hasInternet){
        let street = $.trim(document.getElementById("street").value);
        let plz = $.trim(document.getElementById("plz").value);
        let ort = $.trim(document.getElementById("ort").value);
        let url = 'https://nominatim.openstreetmap.org/search?q=' + street.replace(" ", "+") + "+" + plz + "+" + ort.replace(" ", "+") + '&format=json&polygon=1&addressdetails=1';
        $.getJSON(url, function(result) {
            if(result[0] != null) {
                document.getElementById("lat").value = result[0]["lat"];
                document.getElementById("lng").value = result[0]["lon"];
            }
        });

    }
}