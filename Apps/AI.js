async function sendMessage() {
  let message = document.getElementById("message").value;
  let imageInput = document.getElementById("image").files[0];

  if (!message) {
    notifier.create('Enter your message first', 'error', 5000);
      return;
  }

  let formData = new FormData();
  formData.append("message", message);
  if (imageInput) {
      formData.append("image", imageInput);
  }

  try {
      let response = await fetch("http://104.171.203.107:6969/chat", {
          method: "POST",
          body: formData
      });

      let data = await response.json();
      document.getElementById("response").innerText = data.response;
  } catch (error) {
    notifier.create('Error generating a response', 'error', 5000);
  }
}
function openFile() {
  document.getElementById('image').click();
}
(function() {
  const windowTitle = 'Snap';
  const customHTML = `

  <p style="position: absolute; top: 20px; left:0; padding: 15px; height: calc(100% - 35px)" id="response"></p>
  <div id="chat-container" style="position: absolute; bottom: 10px; width: calc(100% - 35px);">
        <div style="display: flex;">
        <input type="file" id="image" style="display: none;">
        <button style="min-width: 50px; height: 40px; margin-right: 10px; padding-top: 5px; border-radius: 4px;" onclick="openFile()"><img src="../Assets/AI/paperclip.svg"></button>
        <input style="height: 34px; width: calc(100% - 120px); border-radius: 4px; padding-left: 15px;" type="text" id="message" placeholder="Ask me anything">
        <button style="min-width: 50px; height: 40px; margin-left: 10px; padding-top: 5px; border-radius: 4px;" onclick="sendMessage()"><img src="../Assets/AI/arrow-up-from-line.svg"></button>
    </div>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 600, 400);
})();