/* -*- Mode: JavaScript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
(function() {
  const windowTitle = 'Media';
  const customHTML = `
  <div>
  <input type="file" id="audioFile" accept="audio/*" style="display:none;"/>
  <center>
    <img id="albumArt" width="250" style="display:none; border-radius: 3px;" />
    <h2><span id="title">-</span></h2>
    <p><strong>by <span id="artist">-</span></strong> - <strong><span id="album">-</span></strong></p>
    <p></p>
<br>
  <audio style="object-fit: contain; border: none;" id="audioPlayer" controls >
        Your browser does not support the audio element.
    </audio>
  <video style="display:none; max-width:100%; max-height:320px;" id="videoPlayer" controls></video>
  <img id="openedImage" style="display:none; max-width:100%; max-height:320px;" />
  <br><br>
  <label for="audioFile" class="uploadlabel">Upload</label>
    </center>
    </div>`;
  createWindow(windowTitle, customHTML, 600, 580);
})();

(function() {
  if (!window.openedFile) return;
  const opened = window.openedFile;
    const source = './' + opened.path.replace(/^\/+/, '');
    const image = document.getElementById('openedImage');
    const audio = document.getElementById('audioPlayer');
    const video = document.getElementById('videoPlayer');
    if (opened.mimeType.indexOf('image/') === 0 && image) {
        image.src = source;
        image.style.display = 'block';
        audio.style.display = 'none';
        video.style.display = 'none';
    } else if (opened.mimeType.indexOf('audio/') === 0) {
        audio.src = source;
        audio.play();
        video.style.display = 'none';
    } else if (opened.mimeType.indexOf('video/') === 0 && video) {
        video.src = source;
        video.style.display = 'block';
        audio.style.display = 'none';
    }
    const title = document.getElementById('title');
    if (title) title.textContent = opened.name;
})();

const audioFileInput = document.getElementById('audioFile');
        const audioPlayer = document.getElementById('audioPlayer');

        // Event listener for file input change
        audioFileInput.addEventListener('change', function(event) {
            const file = event.target.files[0];

            if (file) {
                const objectURL = URL.createObjectURL(file);
                audioPlayer.src = objectURL;
                audioPlayer.play(); // Play automatically after selection
            }
        });
document.getElementById('audioFile').addEventListener('change', function(event) {
      const file = event.target.files[0];
      if (!file) return;

      jsmediatags.read(file, {
        onSuccess: function(tag) {
          const tags = tag.tags;

          // Set text fields
          document.getElementById('title').textContent = tags.title || 'Unknown';
          document.getElementById('artist').textContent = tags.artist || 'Unknown';
          document.getElementById('album').textContent = tags.album || 'Unknown';

          // Handle album art
          const picture = tags.picture;
          if (picture) {
            const base64String = picture.data
              .map(byte => String.fromCharCode(byte))
              .join('');
            const imageUrl = `data:${picture.format};base64,${btoa(base64String)}`;
            const img = document.getElementById('albumArt');
            img.src = imageUrl;
            img.style.display = 'block';
          } else {
            document.getElementById('albumArt').style.display = 'none';
            console.log('No album art found.');
          }
        },
        onError: function(error) {
          console.error("Error reading file:", error);
          alert("Failed to read file metadata.");
        }
      });
    });
