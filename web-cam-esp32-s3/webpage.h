#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <pgmspace.h>

// HTML page with video stream and audio
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-S3 IP Camera</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background: #1a1a1a;
            color: #fff;
        }
        .container {
            max-width: 900px;
            margin: 0 auto;
        }
        h1 {
            text-align: center;
            color: #4CAF50;
        }
        .video-container {
            position: relative;
            width: 100%;
            background: #000;
            border-radius: 8px;
            overflow: hidden;
            box-shadow: 0 4px 6px rgba(0,0,0,0.3);
        }
        #stream {
            width: 100%;
            height: auto;
            display: block;
        }
        .controls {
            margin-top: 20px;
            display: flex;
            gap: 10px;
            flex-wrap: wrap;
            justify-content: center;
        }
        button {
            padding: 12px 24px;
            font-size: 16px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            background: #4CAF50;
            color: white;
            transition: background 0.3s;
        }
        button:hover {
            background: #45a049;
        }
        button:disabled {
            background: #666;
            cursor: not-allowed;
        }
        .status {
            text-align: center;
            margin-top: 15px;
            padding: 10px;
            border-radius: 5px;
            background: #333;
        }
        .status.connected {
            background: #2d5016;
        }
        .status.disconnected {
            background: #5a1a1a;
        }
        .audio-controls {
            display: flex;
            gap: 10px;
            align-items: center;
            justify-content: center;
            margin-top: 15px;
        }
        .volume-control {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        input[type="range"] {
            width: 150px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32-S3 IP Camera</h1>

        <div class="video-container">
            <img id="stream" src="/stream">
        </div>

        <div class="controls">
            <button onclick="location.reload()">Refresh Stream</button>
            <button onclick="toggleAudio()" id="audioBtn">Enable Audio</button>
            <button onclick="toggleMic()" id="micBtn" disabled>Enable Microphone</button>
        </div>

        <div class="audio-controls">
            <div class="volume-control">
                <label>Speaker Volume:</label>
                <input type="range" id="speakerVolume" min="0" max="100" value="50" onchange="updateVolume()">
                <span id="volumeValue">50%</span>
            </div>
        </div>

        <div class="status disconnected" id="status">
            Audio: Disconnected
        </div>

        <div class="status" id="securityWarning" style="background: #5a1a1a;">
            <p><strong>⚠ MICROPHONE ACCESS:</strong></p>
            <p>Browsers block microphone over HTTP by default.</p>
            <p><strong>Chrome/Edge Workaround:</strong></p>
            <ol style="text-align: left; margin: 10px auto; max-width: 500px;">
                <li>Open: <code style="background:#333;padding:2px 5px;border-radius:3px;">chrome://flags/#unsafely-treat-insecure-origin-as-secure</code></li>
                <li>Add: <strong><span id="espUrl">http://...</span></strong></li>
                <li>Set to "Enabled"</li>
                <li>Click "Relaunch" button</li>
            </ol>
            <p style="font-size: 0.9em; color: #aaa;">Audio from ESP32 mic works without this workaround</p>
        </div>

        <script>
            // Update ESP32 URL
            document.getElementById('espUrl').textContent = 'http://' + window.location.hostname;
        </script>
    </div>

    <script>
        let ws = null;
        let audioContext = null;
        let micStream = null;
        let audioEnabled = false;
        let micEnabled = false;
        let audioWorkletNode = null;

        function toggleAudio() {
            const btn = document.getElementById('audioBtn');
            const micBtn = document.getElementById('micBtn');
            const status = document.getElementById('status');

            if (!audioEnabled) {
                // Connect WebSocket (always WS over HTTP)
                ws = new WebSocket('ws://' + window.location.hostname + ':81');
                ws.binaryType = 'arraybuffer';

                ws.onopen = function() {
                    console.log('✓ WebSocket Connected');
                    status.textContent = 'Audio: Connected';
                    status.className = 'status connected';
                    btn.textContent = 'Disable Audio';
                    micBtn.disabled = false;
                    audioEnabled = true;

                    // Initialize Audio Context for playback
                    if (!audioContext) {
                        audioContext = new (window.AudioContext || window.webkitAudioContext)({
                            sampleRate: 16000
                        });
                        console.log('✓ AudioContext created, sample rate:', audioContext.sampleRate);
                    }
                };

                let msgCount = 0;
                ws.onmessage = function(event) {
                    // Received audio from ESP32 microphone
                    if (event.data instanceof ArrayBuffer) {
                        msgCount++;
                        if (msgCount % 50 === 0) {
                            console.log('Received audio packet:', event.data.byteLength, 'bytes');
                        }
                        playAudio(event.data);
                    }
                };

                ws.onclose = function() {
                    console.log('WebSocket Disconnected');
                    status.textContent = 'Audio: Disconnected';
                    status.className = 'status disconnected';
                    btn.textContent = 'Enable Audio';
                    micBtn.disabled = true;
                    audioEnabled = false;
                };

                ws.onerror = function(error) {
                    console.error('WebSocket Error:', error);
                    status.textContent = 'Audio: Connection Error';
                    status.className = 'status disconnected';
                };
            } else {
                // Disconnect
                if (ws) {
                    ws.close();
                }
                if (micStream) {
                    micStream.getTracks().forEach(track => track.stop());
                    micStream = null;
                }
                audioEnabled = false;
                micEnabled = false;
                btn.textContent = 'Enable Audio';
                micBtn.textContent = 'Enable Microphone';
                micBtn.disabled = true;
            }
        }

        let playCount = 0;
        function playAudio(arrayBuffer) {
            if (!audioContext || !audioEnabled) {
                console.warn('Cannot play audio: context not ready');
                return;
            }

            const int16Array = new Int16Array(arrayBuffer);
            const float32Array = new Float32Array(int16Array.length);

            // Convert Int16 to Float32
            for (let i = 0; i < int16Array.length; i++) {
                float32Array[i] = int16Array[i] / 32768.0;
            }

            playCount++;
            if (playCount % 50 === 0) {
                console.log('Playing audio buffer:', float32Array.length, 'samples');
            }

            const audioBuffer = audioContext.createBuffer(1, float32Array.length, 16000);
            audioBuffer.getChannelData(0).set(float32Array);

            const source = audioContext.createBufferSource();
            source.buffer = audioBuffer;

            const gainNode = audioContext.createGain();
            gainNode.gain.value = document.getElementById('speakerVolume').value / 100;

            source.connect(gainNode);
            gainNode.connect(audioContext.destination);
            source.start();
        }

        async function toggleMic() {
            const btn = document.getElementById('micBtn');

            if (!micEnabled) {
                try {
                    console.log('Requesting microphone access...');
                    // Request microphone access
                    micStream = await navigator.mediaDevices.getUserMedia({
                        audio: {
                            sampleRate: 16000,
                            channelCount: 1,
                            echoCancellation: true,
                            noiseSuppression: true
                        }
                    });
                    console.log('✓ Microphone access granted');

                    if (!audioContext) {
                        audioContext = new (window.AudioContext || window.webkitAudioContext)({
                            sampleRate: 16000
                        });
                    }

                    const source = audioContext.createMediaStreamSource(micStream);
                    const processor = audioContext.createScriptProcessor(512, 1, 1);

                    let sendCount = 0;
                    processor.onaudioprocess = function(e) {
                        if (ws && ws.readyState === WebSocket.OPEN) {
                            const inputData = e.inputBuffer.getChannelData(0);
                            const int16Array = new Int16Array(inputData.length);

                            // Convert Float32 to Int16
                            for (let i = 0; i < inputData.length; i++) {
                                const s = Math.max(-1, Math.min(1, inputData[i]));
                                int16Array[i] = s < 0 ? s * 0x8000 : s * 0x7FFF;
                            }

                            ws.send(int16Array.buffer);

                            sendCount++;
                            if (sendCount % 50 === 0) {
                                console.log('Sending audio to ESP32:', int16Array.length, 'samples');
                            }
                        }
                    };

                    source.connect(processor);
                    processor.connect(audioContext.destination);

                    btn.textContent = 'Disable Microphone';
                    micEnabled = true;

                } catch (err) {
                    console.error('Microphone access denied:', err);
                    alert('Cannot access microphone. Please grant permission.');
                }
            } else {
                // Disable microphone
                if (micStream) {
                    micStream.getTracks().forEach(track => track.stop());
                    micStream = null;
                }
                btn.textContent = 'Enable Microphone';
                micEnabled = false;
            }
        }

        function updateVolume() {
            const volume = document.getElementById('speakerVolume').value;
            document.getElementById('volumeValue').textContent = volume + '%';
        }
    </script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H
