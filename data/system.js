
var useLora = false;
var useWLAN = false;
var loraText = 'LoraWAN steht für deinen Standort leider nicht zur Verfügung bzw. ist die Abdeckung nicht aussreichend.';
var lcnt=0;

var sensorDatas = {
    pm10: [0,0,0,0,0,0,0,0,0,0],
    pm25 : [0,0,0,0,0,0,0,0,0,0],
    tmp: [0,0,0,0,0,0,0,0,0,0]
};
var pmChart;
var tcChart;
document.addEventListener("DOMContentLoaded", () => {
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
    
    document.getElementById("fszize").innerHTML =  formatSizeUnits(flashSize);
    document.getElementById("freeheap").innerHTML = formatSizeUnits(heap) + " frei: " + formatSizeUnits(freeHeap);
    setInterval(function() {
        lcnt = 0;
    }, 10000);
    setInterval(() => {
        ajaxCharts();
    }, 60000);
    ajaxCharts();
    getConfigDatas();
    setTimeout(function(){ 
        $('.preloader').addClass('preloader-deactivate');
    }, 3000);
    adressCoding();
});
document.getElementById("enableWlan").addEventListener('click', function(e){
    if(checkConnectionAvailable()){
        document.getElementById("enableWlan").checked = true;
    }
    var wlanNodes = document.getElementById("wlansetup").getElementsByTagName("*");
    for(var i = 0; i < wlanNodes.length; i++) {
        if(document.getElementById("enableWlan").checked){
            wlanNodes[i].disabled = false;
        }
        else {
            wlanNodes[i].disabled = true;
            document.getElementById("enableWlan").disabled = false;
            document.getElementById("savewlan").disabled = false;            
        }        
    }    
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
    var xhttp = new XMLHttpRequest();
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

function adressCoding(){
    var geoUrl = "https://nominatim.openstreetmap.org/search?q=Eichenstr.+19+07549+Gera&format=json&polygon=1&addressdetails=1";
    // $.getJSON(geoUrl, function(result) {

    // });
}

function getConfigDatas(){
    $.getJSON("/myconfig", function(response) {
        document.getElementById("useLora").checked = response["lora"]["enabled"];
        document.getElementById("devid").value = response["lora"]["devid"];
        let strNKey="";
        let strAppKey = "";
        let devaddr = "";
        if(response["lora"]["newskey"] != null) {
            for(var i = 0; i < response["lora"]["newskey"].length; i++) {
                strNKey += response["lora"]["newskey"][i].toString(16);
            }
        }
        if(response["lora"]["appskey"] != null) {
            for(var i = 0; i < response["lora"]["appskey"].length; i++) {
                strAppKey += response["lora"]["appskey"][i].toString(16);
            }
        }
        if(response["lora"]["devaddr"] != null) {
            devaddr = response["lora"]["devaddr"].toString(16);
        }
        document.getElementById("nwkskey").value = strNKey;
        document.getElementById("appskey").value = strAppKey;
        document.getElementById("devaddr").value = devaddr;

        document.getElementById("enableWlan").checked = response["wifi"]["enabled"];   
        if(response["wifi"]["enabled"] == true && response["wifi"]["ssid"].length > 0) {
            var wsel = document.getElementById("ssid");
            for(var i=0; i < wsel.options.length; ++i){
                if(wsel.options[i].text == response["wifi"]["ssid"]){
                    wsel.options[i].selected = true;
                }
            }
        }             
        document.getElementById("ssidpasw").value = response["wifi"]["password"];
    });
}
function ajaxCharts() {
    $.getJSON("/getchartdatas", function(result) {
        sensorDatas = result;
        pmChart.data.datasets[0].data = sensorDatas["pm25"];
        pmChart.data.datasets[1].data = sensorDatas["pm10"];
        tcChart.data.datasets[0].data = sensorDatas["tmp"];
        pmChart.update();
        tcChart.update();
    });
}
function checkwifi(){
    let ssid = document.getElementById("ssid").value;
    let pasw = document.getElementById("ssidpasw").value;
    let json = {
        ssid: ssid,
        pasw: pasw,
        enabled : document.getElementById("enableWlan").checked
    };
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function(){
        console.log(this.responseText);
    }
    xhttp.open("Post","/test_wifi", true);
    xhttp.setRequestHeader("Content-Type", "application/json");
    xhttp.send(JSON.stringify(json));
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
        alert("Es muss midestens eine Verbindung konfiguriert sein!");
        document.getElementById("savewlan").disabled = true;
        document.getElementById("loracheck").disabled = true;
        return true;
    } else{
        document.getElementById("savewlan").disabled = false;
        document.getElementById("loracheck").disabled = false;
        return false;
    }
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