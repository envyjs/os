
function execCommand(command, value = null) {
  document.execCommand(command, false, value);
}

function openFile() {
  document.getElementById('fileInput').click();
}

// Function to load and parse RTF file
function loadRTFFile(event) {
  const file = event.target.files[0];
  if (file && file.type === 'application/rtf') {
      const reader = new FileReader();
      reader.onload = function (e) {
          const rtfContent = e.target.result;
          const plainText = rtfToPlainText(rtfContent);
          document.querySelector('.editor').innerHTML = plainText;
      };
      reader.readAsText(file);
  } else {
      alert('Please select a valid .RTF file.');
  }
}

// Simple RTF to Plain Text parser
function rtfToPlainText(rtf) {
  // Remove RTF control words and groups using regex
  return rtf.replace(/\\[a-z]+(\d+)?(?:-?\d+)?[ ]?/gi, '')
            .replace(/[\{\}\*]/g, '')
            .replace(/\\par/g, '<br>');
}

function cutText() {
  document.execCommand('cut');
}

function copyText() {
  document.execCommand('copy');
}

function pasteText() {
  // Using the Clipboard API for paste
  navigator.clipboard.readText()
      .then(text => {
          document.execCommand('insertText', false, text);
      })
      .catch(err => {
          alert('Failed to paste content. Please use Ctrl+V as a fallback.');
      });
}

function changeFontSize(size) {
  if (size) {
      document.execCommand('fontSize', false, size);
  }
}

function changeAlignment(alignment) {
  if (alignment) {
      execCommand(alignment);
  }
}

function changeFontFamily(font) {
  if (font) {
      document.execCommand('fontName', false, font);
  }
}

function saveAsRTF() {
  const editorContent = document.querySelector('.editor').innerHTML;

  // Wrap content in RTF headers
  const rtfHeader = `
      {\\rtf1\\ansi\\deff0
      {\\fonttbl{\\f0 Arial;}}
      {\\colortbl;\\red0\\green0\\blue0;}
      \\viewkind4\\uc1\\pard\\f0\\fs24 
  `;
  const rtfFooter = '}';
  const rtfContent = rtfHeader + htmlToRtf(editorContent) + rtfFooter;

  // Create a Blob for the RTF file
  const blob = new Blob([rtfContent], { type: 'application/rtf' });

  // Create a link and trigger download
  const link = document.createElement('a');
  link.href = URL.createObjectURL(blob);
  link.download = 'document.rtf';
  link.click();
  notifier.create('Document saved!', 'success', 5000);
}

function htmlToRtf(html) {
  // Replace basic HTML tags with RTF equivalents
  return html
      .replace(/<b>(.*?)<\/b>/g, '\\b $1\\b0 ')
      .replace(/<i>(.*?)<\/i>/g, '\\i $1\\i0 ')
      .replace(/<u>(.*?)<\/u>/g, '\\ul $1\\ulnone ')
      .replace(/<br\s*\/?>/g, '\\par ')
      .replace(/<div>(.*?)<\/div>/g, '$1\\par ')
      .replace(/<p>(.*?)<\/p>/g, '\\par $1');
}

(function() {
  const windowTitle = 'Lexis';
  const customHTML = `
  <div class="ribbon lx">
        <button onclick="pasteText()" style="height: 68px; max-width: 60px !important;">Paste</button>
        <div>
        <button onclick="cutText()" style="width: 56px;">Cut</button>
        <br>
        <button onclick="copyText()" style="margin-top: -1px">Copy</button>
        </div>
        <select onchange="changeFontFamily(this.value)" style="height: 30px;">
            <option value="">Font</option>
            <option value="Arial">Arial</option>
            <option value="Courier New">Courier New</option>
            <option value="Georgia">Georgia</option>
            <option value="Times New Roman">Times New Roman</option>
            <option value="Verdana">Verdana</option>
        </select>
        <select onchange="changeFontSize(this.value)" style="height: 30px;">
            <option value="">Font Size</option>
            <option value="1">Very Small</option>
            <option value="2">Small</option>
            <option value="3">Normal</option>
            <option value="4">Large</option>
            <option value="5">Very Large</option>
            <option value="6">Huge</option>
            <option value="7">Gigantic</option>
        </select>
        <select onchange="changeAlignment(this.value)" style="height: 30px; position: absolute; left: 316px; top: 48px;">
            <option value="">Alignment</option>
            <option value="justifyLeft">Align Left</option>
            <option value="justifyCenter">Center</option>
            <option value="justifyRight">Align Right</option>
            <option value="justifyFull">Justify</option>
        </select>
        <button onclick="openFile()">Open</button>
        <button onclick="saveFile()" style="height: 30px; width: 58px; position: absolute; left: 435px; top: 48px;">Save</button>
        <button onclick="execCommand('bold')" style="height: 30px; width: 45px; position: absolute; left: 146px; top: 48px;"><b>B</b></button>
        <button onclick="execCommand('italic')" style="height: 30px; width: 45px; position: absolute; left: 203px; top: 48px;"><i>I</i></button>
        <button onclick="execCommand('underline')" style="height: 30px; width: 45px; position: absolute; left: 260px; top: 48px;"><u>U</u></button>
        <input type="file" id="fileInput" accept=".rtf" style="display: none;" onchange="loadRTFFile(event)">
    </div>

    <div class="editor-container">
        <div class="editor" id="fileContent" contenteditable="true">
        </div>
    </div>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 600, 400);
})();