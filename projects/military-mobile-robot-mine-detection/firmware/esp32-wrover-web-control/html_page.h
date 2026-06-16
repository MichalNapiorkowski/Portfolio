#ifndef HTML_PAGE_H
#define HTML_PAGE_H

const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" />
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
            outline: none;
        }
        
        html, body {
            width: 100%;
            height: 100%;
            overflow: hidden;
        }
       body { 
            margin: 0; 
            padding: 0;
            font-family: Arial, sans-serif; 
            background-image: url('https://static.vecteezy.com/system/resources/previews/013/166/648/non_2x/winter-camouflage-pattern-background-illustration-vector.jpg');
            background-size: 300%; 
            background-repeat: no-repeat;
            display: flex;
            justify-content: center;
            background-position: center;
            align-items: center;
            user-select: none; 
            overflow: hidden;
            height: 90vh; 
        }
        .page-container {
            height: 100%;
            max-width: 700px;
            margin: auto 0;
        }
        .top-bar {
            width: 100%;
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 10px 20px;
        }
        .battery {
            display: inline-block;
            position: baseline;
            width: 80px;
            height: 40px;
            border: 2px solid #333;  
            border-radius: 4px;
            background-color: #fff;  
            box-sizing: border-box;
            transform: scale(0.7);
            transform-origin: center;
            margin: 0px 60px; 
            margin-top: 20px;
        }
        .battery-head {
            position: absolute;
            right: -10px;
            top: 6px;
            width: 8px;
            height: 24px;
            background-color: #333;
            border-radius: 2px;
        }
        .battery-body {
            width: 100%;
            height: 100%;
            display: flex;
            align-items: center;
            justify-content: space-evenly;
            padding: 0 2px; 
            box-sizing: border-box;
        }
        .battery-bar {
            width: 12px;
            height: 50%;
            background-color: green; 
            border-radius: 1px; 
        }
        .wifi-indicator {
            transform: scale(1.4);
            transform-origin: right top;
            margin: 0px 60px; 
            margin-top: 20px;
            display: flex;
            align-items: flex-end;
            gap: 4px;
        }
        .wifi-bar {
            display: inline-block;
            width: 6px; 
            margin: 0 -1px; 
            background-color: #ddd; 
            transition: 0.3s; 
            border-radius: 2px;
            vertical-align: bottom;
        }
        #wifi-bar1 { height: 6px; }
        #wifi-bar2 { height: 10px; }
        #wifi-bar3 { height: 14px; }
        #wifi-bar4 { height: 18px; }
        #wifi-bar5 { height: 22px; }
        .wifi-bar.active {
            background-color: #0000FF; 
        }

        .stream-container {
            text-align: center;
            margin: 0;
            padding: 0 20px;
        }
        img {
            margin-top: 1px;
            max-width: 640px;
            max-height: 480px;
            width: 100%;
            height: auto;
            border: 2px solid #ccc;
            border-radius: 10px;
            transform: scale(0.85);
        }

        .controls-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin: 10px 0;
            padding: 0 10px;
        }

        .distance-indicator-container,
        .movement-buttons,
        .sensitivity-container {
            flex: 1; 
            display: flex;
            flex-direction: column;
            align-items: center;
        }

        .distance-label {
            font-size: 20px;
            font-weight: bold;
            margin-bottom: 5px;
            text-align: center;
            margin: 0px -20px;
        }
        .distance-indicator {
            width: 40px;
            height: 180px;
            background-color: #ccc;
            position: relative;
            border-radius: 5px;
            overflow: hidden;
        }
        .distance-fill {
            position: absolute;
            bottom: 0;
            width: 100%;
            height: 0%; 
            background: linear-gradient(to top, green, red); 
            transition: height 0.3s;
        }

        .movement-buttons {
            justify-content: center;
        }
        .movement-buttons button {
            width: 110px; 
            height: 60px; 
            margin: 4px; 
            font-size: 20px; 
            background-color: #0000FF; 
            color: white; 
            border: none; 
            border-radius: 20px; 
            cursor: pointer;
            transition: background-color 0.3s, transform 0.2s;
        }
        .movement-buttons button.active { 
            background-color: #45c049; 
            transform: scale(1.1);
        }
        .left-right-row {
            display: flex;
            justify-content: center;
        }

        .sensitivity-container {
            height: 200px;
        }
        .sens-label {
            font-size: 20px; 
            font-weight: bold; 
            margin-bottom: 5px;
            text-align: center;
            margin: 0px -20px;
        }
        .vertical-slider {
            -webkit-appearance: slider-vertical; 
            writing-mode: bt-lr; 
            width: 30px; 
            height: 150px;  
            margin: 0;
        }
        .sens-value {
            font-size: 20px; 
            margin-top: 5px; 
            font-weight: bold;
        }

        .bottom-row {
            display: flex;
            justify-content: space-between;
            align-items: baseline;
            margin: 10px 0;
            padding: 0 20px;
        }
        .speed-block, .blockade-block {
            display: flex;
            flex-direction: column;
            align-items: center;
            flex: 1;
        }
        .speed-label {
            font-size: 20px;
            font-weight: bold;
            margin-bottom: 5px;
            text-align: center;
        }
        .speed-controller {
            display: flex; 
            align-items: center; 
            justify-content: center; 
        }
        .speed-controller button {
            width: 40px; 
            height: 40px; 
            font-size: 24px; 
            font-weight: bold; 
            margin: 5px;
            background-color: #0000FF; 
            color: white; 
            border: none; 
            border-radius: 20px; 
            cursor: pointer;
            transition: transform 0.2s, background-color 0.3s;
        }
        .speed-controller button:active { 
            transform: scale(1.2); 
        }
        .speed-display {
            font-size: 24px; 
            font-weight: bold; 
            color: #111; 
            width: 30px;
            text-align: center;
        }

        .blockade-label {
            font-size: 20px;
            font-weight: bold;
            margin-bottom: 5px;
        }
        .toggle-container {
            display: flex; 
            align-items: center; 
            justify-content: center; 
        }
        .toggle-switch {
            position: relative; 
            width: 60px; 
            height: 34px;
        }
        .toggle-switch input {
            opacity: 0; 
            width: 0; 
            height: 0;
        }
        .slider {
            position: absolute; 
            cursor: pointer; 
            top: 0; 
            left: 0; 
            right: 0; 
            bottom: 0;
            background-color: #FF0000; 
            transition: .4s; 
            border-radius: 34px;
        }
        .slider:before {
            position: absolute; 
            content: ""; 
            height: 26px; 
            width: 26px; 
            left: 4px; 
            bottom: 4px;
            background-color: white; 
            transition: .4s; 
            border-radius: 50%;
        }
        input:checked + .slider {
            background-color: #50FF00;
        }
        input:checked + .slider:before {
            transform: translateX(26px);
        }
        #toggle-status {
            margin-left: 10px; 
            font-size: 20px; 
            font-weight: bold;
        }
    </style>
</head>
<body>
    <div class="page-container">
        <div class="top-bar">
            <div class="battery">
                <div class="battery-head"></div>
                <div class="battery-body">
                    <div class="battery-bar"></div>
                    <div class="battery-bar"></div>
                    <div class="battery-bar"></div>
                    <div class="battery-bar"></div>
                </div>
            </div>
            <div class="wifi-indicator">
                <div class="wifi-bar" id="wifi-bar1"></div>
                <div class="wifi-bar" id="wifi-bar2"></div>
                <div class="wifi-bar" id="wifi-bar3"></div>
                <div class="wifi-bar" id="wifi-bar4"></div>
                <div class="wifi-bar" id="wifi-bar5"></div>
            </div>
        </div>

        <div class="stream-container">
            <img src="" id="photo" alt="stream">
        </div>

        <div class="controls-row">
            <div class="distance-indicator-container">
                <div class="distance-label">Detection</div>
                <div class="distance-indicator">
                    <div class="distance-fill" id="distance-fill"></div>
                </div>
            </div>

            <div class="movement-buttons">
                <button id="up" onmousedown="startSendingCommand('up')" onmouseup="stopSendingCommand('up')" ontouchstart="startSendingCommand('up')" ontouchend="stopSendingCommand('up')">FORWARD</button>
                <div class="left-right-row">
                    <button id="left" onmousedown="startSendingCommand('left')" onmouseup="stopSendingCommand('left')" ontouchstart="startSendingCommand('left')" ontouchend="stopSendingCommand('left')">LEFT</button>
                    <button id="right" onmousedown="startSendingCommand('right')" onmouseup="stopSendingCommand('right')" ontouchstart="startSendingCommand('right')" ontouchend="stopSendingCommand('right')">RIGHT</button>
                </div>
                <button id="down" onmousedown="startSendingCommand('down')" onmouseup="stopSendingCommand('down')" ontouchstart="startSendingCommand('down')" ontouchend="stopSendingCommand('down')">BACK</button>
            </div>

            <div class="sensitivity-container">
                <div class="sens-label">Detection<br>sensitivity</div>
                <input type="range" id="sensitivity-slider" class="vertical-slider" min="1" max="10" step="1" value="2" oninput="onSensitivityChange(this.value)">
                <div class="sens-value" id="sens-value">2</div>
            </div>
        </div>

        <div class="bottom-row">
            <div class="speed-block">
                <div class="speed-label">Speed</div>
                <div class="speed-controller">
                    <button onclick="changeSpeed(-1)">-</button>
                    <span class="speed-display" id="speed-display">3</span>
                    <button onclick="changeSpeed(1)">+</button>
                </div>
            </div>
            
            <div class="blockade-block">
                <div class="blockade-label">Auto Stop</div>
                <div class="toggle-container">
                    <label class="toggle-switch">
                        <input type="checkbox" id="enableToggle" onclick="toggleEnable()" checked>
                        <span class="slider"></span>
                    </label>
                </div>
            </div>
        </div>
    </div>

    <script>
        let activeButton = null;
        let intervalId = null;
        let speed = 3;

        function startSendingCommand(action) {
            if (activeButton !== action) {
                activeButton = action;
                document.getElementById(action).classList.add('active');
                sendCommand(action);
                intervalId = setInterval(() => {
                    sendCommand(action);
                }, 85);
            }
        }

        function stopSendingCommand() {
            if (intervalId) {
                clearInterval(intervalId);
                intervalId = null;
                if (activeButton) {
                      document.getElementById(activeButton).classList.remove('active');
                      activeButton = null;
                }
            }
        }
        function sendCommand(action) {
            fetch(`/button?action=${action}`).catch(err => console.log(err));
        }
        function changeSpeed(delta) {
            speed = Math.max(1, Math.min(5, speed + delta));
            document.getElementById("speed-display").textContent = speed;
            fetch(`/button?action=speed${speed}`).catch(err => console.log(err));
        }
        function toggleEnable() {
            const isChecked = document.getElementById("enableToggle").checked;
            const action = isChecked ? 'enable' : 'disable';
            fetch(`/button?action=${action}`).catch(err => console.log(err));
        }
        function onSensitivityChange(value) {
            const mappedValue = 10 - value;
            document.getElementById('sens-value').textContent = value;
            fetch(`/button?action=sens${mappedValue}`).catch(err => console.log(err));
        }
        function updateDistanceIndicator() {
            fetch('/distance')
                .then(response => response.text())
                .then(distStr => {
                    const dist = parseInt(distStr);
                    const fillElem = document.getElementById('distance-fill');
                    if (isNaN(dist) || dist < 1) {
                        fillElem.style.height = '0%';
                    } else {
                        const fillPercent = (dist / 9) * 100;
                        fillElem.style.height = fillPercent + '%';
                    }
                })
                .catch(err => console.log(err));
        }
        function updateWifiIndicator() {
            fetch('/signal')
                .then(response => response.text())
                .then(barsStr => {
                    const bars = parseInt(barsStr);
                    for (let i = 1; i <= 5; i++) {
                        const bar = document.getElementById('wifi-bar' + i);
                        if (bars >= i) {
                            bar.classList.add('active');
                        } else {
                            bar.classList.remove('active');
                        }
                    }
                })
                .catch(err => console.log(err));
        }
        window.onload = () => {
            document.getElementById("photo").src = window.location.href.slice(0, -1) + ":81/stream";
            const defaultSensitivity = 5;
            document.getElementById('sensitivity-slider').value = defaultSensitivity;
            onSensitivityChange(defaultSensitivity);
            setInterval(updateDistanceIndicator, 250);
            setInterval(updateWifiIndicator, 1000);
            updateDistanceIndicator();
            updateWifiIndicator();
        };
    </script>
</body>
</html>
)rawliteral";

#endif // HTML_PAGE_H
