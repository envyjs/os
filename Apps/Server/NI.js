// NI.js
// Network Information app for Envy Server
// (C) 2025-2027 Envy Group

        // Get local IP (WebRTC method)
        function getLocalIP(callback) {
            const rtc = new RTCPeerConnection({iceServers: []});
            rtc.createDataChannel('');
            rtc.createOffer().then(offer => rtc.setLocalDescription(offer));
            rtc.onicecandidate = event => {
                if (event && event.candidate && event.candidate.candidate) {
                    const ipMatch = event.candidate.candidate.match(/([0-9]{1,3}(\.[0-9]{1,3}){3})/);
                    if (ipMatch) {
                        callback(ipMatch[1]);
                        rtc.close();
                    }
                }
            };
        }
        getLocalIP(ip => {
            document.getElementById('local-ip').textContent = ip || 'Unavailable';
        });

        // Get public IP (using ipify API)
        fetch('https://api.ipify.org?format=json')
            .then(res => res.json())
            .then(data => {
                document.getElementById('public-ip').textContent = data.ip || 'Unavailable';
            })
            .catch(() => {
                document.getElementById('public-ip').textContent = 'Unavailable';
            });

(function() {
  const windowTitle = 'Network Information';
const customHTML = `
    <h2>Network Status</h2>
    <ul>
        <li>Online Status: <span id="online-status">${navigator.onLine ? 'Online' : 'Offline'}</span></li>
        <li>Connection Type: <span id="connection-type">${navigator.connection ? navigator.connection.effectiveType : 'Unavailable'}</span></li>
    </ul>
    <h2>IP Address</h2>
    <ul>
        <li>Local IP Address: <span id="local-ip">Loading...</span></li>
        <li>Public IP Address: <span id="public-ip">Loading...</span></li>
    </ul>
`;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 600, 350);
})();

notifier.create('Hello from the Example App', 'info', 5000);