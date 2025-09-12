(function() {
  const windowTitle = 'Media';
  const customHTML = `
  <div style="border-radius: 9px">
  <input type="file" id="audioFile" accept="audio/*" />
  <center>
  <audio style="width: 100%" id="audioPlayer" controls>
        Your browser does not support the audio element.
    </audio>
    </center>
    </div>
    <canvas id="visualizer"></canvas>`;
  createWindow(windowTitle, customHTML, 600, 350);
})();

const canvas = document.getElementById("visualizer");
const ctx = canvas.getContext("2d");
let time = 0;

function animate() {
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.save();
  ctx.translate(canvas.width / 2, canvas.height / 2);

  for (let i = 0; i < 360; i += 10) {
      let angle = (i + time * 1) * (Math.PI / 180); // Slowed down
      let radius = (Math.min(canvas.width, canvas.height) / 3) + Math.sin(time * 0.02 + i * 0.1) * 70; // Dynamic scaling
      let x = Math.cos(angle) * radius;
      let y = Math.sin(angle) * radius;
      let hue = (i * 2 + time * 2) % 360;
      
      ctx.fillStyle = `hsl(${hue}, 100%, 50%)`;
      ctx.beginPath();
      ctx.arc(x, y, 15 + Math.sin(time * 0.05 + i) * 5, 0, Math.PI * 2);
      ctx.fill();
  }
  
  ctx.restore();
  time += 0.3; // Slower movement
  requestAnimationFrame(animate);
}

animate();

const audioFileInput = document.getElementById('audioFile');
        const audioPlayer = document.getElementById('audioPlayer');

        // Event listener for file input change
        audioFileInput.addEventListener('change', function(event) {
            const file = event.target.files[0];

            if (file) {
                const objectURL = URL.createObjectURL(file);
                audioPlayer.src = objectURL;
                audioPlayer.play(); // Play automatically after selection
                notifier.create('Now Playing:' + file.name, 'info', 5000);
            }
        });