var content = document.createElement("div");
content.innerHTML = `
<div class="hidden">
      <div id="samba" style="height: 100%">
        <div
          id="menu"
          style="
            -webkit-backdrop-filter: blur(10px) saturate(180%);
            backdrop-filter: blur(10px) saturate(180%);
            min-height: 100%;
            padding: 25px;
            background-color: #0000004f;
          "
        >
          <div id="smbheader" style="padding: 10px 0px">
            <h1 style="display: inline">Apps</h1>
            <img
              width="25"
              height="25"
              src="./Assets/legacy/icons/lock.png"
              style="border-radius: 10px; float: right"
              onclick="showlock()"
            />
          </div>
          <div
            id="grid"
            style="
              width: 100%;
              display: flex;
              justify-content: center;
              margin-top: 15px;
            "
          >
            <div
              id="appgrid"
              style="
                display: grid;
                grid-template-columns: repeat(3, 1fr);
                grid-column-gap: 10px;
                grid-row-gap: 25px;
                max-width: 250px;
              "
            >
              <div
                id="app"
                style="
                  display: flex;
                  flex-direction: column;
                  gap: 5px;
                  align-items: center;
                  width: 70px;
                "
                onclick="showstore();closeTeresa();"
              >
                <img
                  width="50"
                  height="50"
                  src="./Assets/legacy/apps/store/icon.png"
                  style="border-radius: 10px"
                />
                <span>Store</span>
              </div>
              <div
                id="app"
                style="
                  display: flex;
                  flex-direction: column;
                  gap: 5px;
                  align-items: center;
                  width: 70px;
                "
                onclick="useSambaHelp();closeTeresa();"
              >
                <img
                  width="50"
                  height="50"
                  src="./Assets/legacy/apps/help/icon.png"
                  style="border-radius: 10px"
                />
                <span>Help</span>
              </div>
              <div
                id="app"
                style="
                  display: flex;
                  flex-direction: column;
                  gap: 5px;
                  align-items: center;
                  width: 70px;
                "
                onclick="id3();closeTeresa();"
              >
                <img
                  width="50"
                  height="50"
                  src="./Assets/legacy/apps/browser/icon.png"
                  style="border-radius: 10px"
                />
                <span>Browser</span>
              </div>
              <div
                id="app"
                style="
                  display: flex;
                  flex-direction: column;
                  gap: 5px;
                  align-items: center;
                  width: 70px;
                "
                onclick="aspenplus();closeTeresa();"
              >
                <img
                  width="50"
                  height="50"
                  src="./Assets/legacy/apps/aspen/icon.png"
                  style="border-radius: 10px"
                />
                <span>Aspen</span>
              </div>
              <div
                id="app"
                style="
                  display: flex;
                  flex-direction: column;
                  gap: 5px;
                  align-items: center;
                  width: 70px;
                "
                onclick="showWeather();closeTeresa();"
              >
                <img
                  width="50"
                  height="50"
                  src="./Assets/legacy/icons/wttr.png"
                  style="border-radius: 10px"
                />
                <span>Weather</span>
              </div>
              <div
                id="app"
                style="
                  display: flex;
                  flex-direction: column;
                  gap: 5px;
                  align-items: center;
                  width: 70px;
                "
                onclick="calc();closeTeresa();"
              >
                <img
                  width="50"
                  height="50"
                  src="./Assets/legacy/apps/calc/icon.png"
                  style="border-radius: 10px"
                />
                <span>Calculator</span>
              </div>
              <div
                id="app"
                style="
                  display: flex;
                  flex-direction: column;
                  gap: 5px;
                  align-items: center;
                  width: 70px;
                "
                onclick="showSettingsWindow();closeTeresa();"
              >
                <img
                  width="50"
                  height="50"
                  src="./Assets/legacy/apps/prefs/icon.png"
                  style="border-radius: 10px"
                />
                <span>Settings</span>
              </div>
            </div>
          </div>
        </div>
      </div>
      <div id="teresalogin">
        <style>
          #teresalogin {
            background: rgb(0, 35, 94);
            background: linear-gradient(
              0deg,
              rgba(0, 35, 94, 1) 0%,
              rgba(0, 0, 0, 1) 80%
            );
            background-size: cover;
            background-position: center;
            background-attachment: fixed;
            background-repeat: no-repeat;
            height: 100%;
            display: flex;
            flex-direction: column;
            gap: 8px;
            justify-content: center;
            align-items: center;
          }
        </style>
        <script>
          // Delete lockscreen func.

          function RemoveLockScreen2() {
            let element = document.querySelector(".winbox #teresalogin");
            if (element) element = element.closest(".winbox").remove();
          }
        </script>

        <center>
          <img
            src="./Assets/legacy/brand/zeon.png"
            style="border-radius: 10px"
            height="100"
            width="100"
          />
          <h2>Zeon user</h2>
          <input type="password" placeholder="Password" /><button
            class="ripple"
            onclick="RemoveLockScreen2()"
          >
            Go
          </button>
          <br /><br />
          <button class="ripple">Switch users</button>
        </center>
      </div>
      <div id="teresadual">
        <center>
          <br />
          <img src="./Assets/legacy/icons/tops.png" onclick="showDualWindow()" />
          <img src="./Assets/legacy/icons/term.png" onclick="showTelegram()" />
        </center>
      </div>
      <div id="teresaoobe">
        <style>
          #teresaoobe {
            height: 100%;
            display: flex;
            flex-direction: column;
            gap: 8px;
            justify-content: center;
            align-items: center;
            background-clip: text;
            background: linear-gradient(
              45deg,
              rgba(131, 58, 180, 1) 0%,
              rgba(253, 29, 29, 1) 50%,
              rgba(252, 176, 69, 1) 100%
            );
            background-size: 200% 200%;
            animation: gradientAnimation 2s linear forwards;
          }

          @keyframes gradientAnimation {
            0%,
            100% {
              background-position: 0% 0%;
            }
            50% {
              background-position: 100% 0%;
            }
          }

          @keyframes slideFadeIn {
            from {
              transform: translateY(100%);
              opacity: 0;
            }
            to {
              transform: translateY(0);
              opacity: 1;
            }
          }
        </style>
        <script>
          function nosetup() {
            let element = document.querySelector(".winbox #teresaoobe");
            if (element) element = element.closest(".winbox").remove();
            new WinBox({
              title: "Welcome to Envy",
              class: ["no-max", "no-min", "no-close", "eclipsemono"],
              x: "center",
              y: "center",
              width: "50%",
              height: "75%",
              header: 0,
              top: 0,
              right: 0,
              bottom: 45,
              left: 0,
              modal: true,
              mount: document.getElementById("stage2").cloneNode(true),
            });
          }
        </script>
        <center>
          <img
            style="animation: slideFadeIn 1s ease-out; border-radius: 20px"
            src="./Assets/legacy/brand/boo.png"
            height="64"
            width="64"
          />
          <h2 style="animation: slideFadeIn 1s ease-out">Welcome to Envy.</h2>
          <h3 style="animation: slideFadeIn 1s ease-out">
            Let's reinvent the desktop.
          </h3>
          <button
            class="ripple"
            style="animation: slideFadeIn 1s ease-out"
            onclick="nosetup()"
          >
            Let's start</button
          ><br /><br />
          <img
            style="animation: slideFadeIn 1s ease-out"
            src="./Assets/legacy/icons/access.png"
            onclick="accesspanel()"
          />
        </center>
      </div>
      <div id="stage2">
        <style>
          #stage2 {
            height: 100%;
            display: flex;
            flex-direction: column;
            gap: 8px;
            justify-content: center;
            align-items: center;
            background: rgb(131, 58, 180);
            background: linear-gradient(
              45deg,
              rgba(131, 58, 180, 1) 0%,
              rgba(253, 29, 29, 1) 50%,
              rgba(252, 176, 69, 1) 100%
            );
          }
        </style>
        <script>
          function skipsetup() {
            let element = document.querySelector(".winbox #stage2");
            if (element) element = element.closest(".winbox").remove();
          }

          function zeonsetup() {
            let element = document.querySelector(".winbox #stage2");
            if (element) element = element.closest(".winbox").remove();
            new WinBox({
              title: "Welcome to Envy",
              class: ["no-max", "no-min", "no-close", "eclipsemono"],
              x: "center",
              y: "center",
              width: "50%",
              height: "75%",
              header: 0,
              top: 0,
              right: 0,
              bottom: 45,
              left: 0,
              modal: true,
              mount: document.getElementById("zeonsetup").cloneNode(true),
            });
          }
        </script>
        <center>
          <img
            src="./Assets/legacy/brand/zeon.png"
            style="border-radius: 10px"
            height="64"
            width="64"
          />
          <h2>Do you want to login with Zeon?</h2>
          <button class="ripple" onclick="zeonsetup()">Yes</button>
          <button class="ripple" onclick="skipsetup()">Skip</button>
        </center>
      </div>
      <div id="zeonsetup">
        <style>
          #zeonsetup {
            height: 100%;
            display: flex;
            flex-direction: column;
            gap: 8px;
            justify-content: center;
            align-items: center;
            background: rgb(131, 58, 180);
            background: linear-gradient(
              45deg,
              rgba(131, 58, 180, 1) 0%,
              rgba(253, 29, 29, 1) 50%,
              rgba(252, 176, 69, 1) 100%
            );
          }
        </style>
        <script>
          function finishzeonsetup() {
            let element = document.querySelector(".winbox #zeonsetup");
            if (element) element = element.closest(".winbox").remove();
          }
          function backtostage2() {
            let element = document.querySelector(".winbox #zeonsetup");
            if (element) element = element.closest(".winbox").remove();
            nosetup();
          }
        </script>
        <center>
          <img
            src="./assets/legacy/brand/zeon.png"
            style="border-radius: 10px"
            height="64"
            width="64"
          />
          <h2>Sign into Zeon</h2>
          <button class="ripple" onclick="zeoniframe()">
            Don't have a Zeon account?</button
          ><br /><br />
          <input id="zuser" type="text" placeholder="Username" /><br /><br />
          <input
            id="zpass"
            type="password"
            placeholder="Password"
          /><br /><br />
          <button class="ripple" onclick="oobelogin()">Go!</button>
          <button class="ripple" onclick="backtostage2()">Back</button>
        </center>
      </div>
      <div id="aspenplus">
        <center>
          <style>
            #aspenplus {
              height: 100%;
              background: rgb(180, 152, 35);
              background: linear-gradient(
                45deg,
                rgba(180, 152, 35, 1) 0%,
                rgba(203, 20, 20, 1) 100%
              );
            }
          </style>
          <br />
          <b style="margin-right: 10px">Aspen Start </b>
          <button class="ripple" onclick="flipify()">Flipify</button>
          <br />
          <h2>Welcome to Aspen Start</h2>
          <input type="text" placeholder="Search Aspen..." /><br /><br />
          <button class="ripple">Search Aspen</button
          ><button class="ripple">Search Flipify</button>
        </center>
      </div>
      <div id="files">
        <center id="filelist">
          <br />
          <button class="ripple">&lt;</button
          ><input type="text" placeholder="Address bar" /><button
            class="ripple"
          >
            &gt;
          </button>
        </center>
      </div>
      <div id="log">
        <center>
          <br />
          <img src="./assets/legacy/brand/boo.png" width="64" height="64" /><img />
          <h1>Credits</h1>
          <p>
            <b
              >This site uses the
              <a href="http://dsans.davidstudios.uk/">DSans font.</a></b
            >
          </p>
        </center>
      </div>
      <div id="net">
        <center>
          <br />
          <p>(empty)</p>
        </center>
      </div>
      <div id="modernprefs" style="height: 100%">
        <style>
          .puivisiblecol {
            display: block;
          }
          .puihiddencol {
            display: none;
          }
          .puitabactive {
            background: #ffffff0d;
          }
          .puitabinactive {
            background: transparent;
          }
          [data-theme="light"] .puilcolicon {
            filter: invert(1);
          }
          .selectionstatus:not(.checked):after,
          .qselectionstatus:not(.checked):after {
            content: "-";
          }
          .selectionstatus.checked:after,
          .qselectionstatus.checked:after {
            content: "✓";
          }
        </style>
      <div id="ccaccess">
        <center>
          <br />
          <h2>Accessbility</h2>
          <button class="ripple">Enable OpenDyslexic font</button><br /><br />
          <button class="ripple">High contrast themes</button>
        </center>
      </div>
      <div id="backup2">
        <center>
          <br />
          <h2>Backup</h2>
          <h3>How often do you want to backup Envy?</h3>
          <button class="ripple">Never</button>
          <button class="ripple">Every day</button>
          <button class="ripple">Every week</button>
          <button class="ripple">Every month</button>
          <h3>Where do you want to backup Envy?</h3>
          <input type="text" />
        </center>
      </div>
      <div id="ccbackup">
        <center>
          <br />
          <h2>Backup and reset</h2>
          <button class="ripple" onclick="backup2panel()">
            Backup your system</button
          ><br /><br />
          <button class="ripple" onclick="resetconfirm()">
            Reset your system
          </button>
        </center>
      </div>
      <div id="ccdisplay">
        <center>
          <br />
          <h2>Envy themes</h2>
          <button class="ripple" onclick="dark()">Dark (default)</button>
          <button class="ripple" onclick="light()">Light</button>
          <h2>Custom themes</h2>
          <button class="ripple" onclick="urlbox()">Type in an image URL</button
          ><br />
          <button class="ripple" onclick="winboxbox()">
            Choose a window color</button
          ><br />
          <h2>Aspen colors</h2>
        </center>
      </div>
      <div id="ccinter">
        <center>
          <br />
          <h2>Let's talk language.</h2>
          <h3>What language do you speak?</h3>
          <form action="/action_page.php">
            <select id="tasks" name="tasks">
              <option value="tasks">Select an option</option>
              <option value="tasks">Arabic (???)</option>
              <option value="tasks">English (UK)</option>
              <option value="tasks">German (Germany)</option>
              <option value="tasks">Italian (Italy)</option>
              <option value="tasks">Japanese (Japan)</option>
              <option value="tasks">Polish (Poland)</option>
              <option value="tasks">Russian (Russia)</option>
              <option value="tasks">Spanish (Spain)</option>
              <option value="tasks">Turkish (Turkey)</option>
              <option value="tasks">Ukrainian (Ukraine)</option>
            </select>
            <!-- <input type="submit"> -->
          </form>
        </center>
      </div>
      <div id="cctask">
        <center>
          <br />
          <h2>Taskbar functions</h2>
          <h3>Search</h3>
          <form action="/action_page.php">
            <label for="tasks">Search appearence</label>
            <select id="tasks" name="tasks">
              <option value="tasks">Icon</option>
              <option value="tasks">Box</option>
            </select>
            <!-- <input type="submit"> -->
          </form>
          <h2>Taskbar theming</h2>
          <button class="ripple" onclick="colorbox()">
            Choose a taskbar color</button
          ><br />
        </center>
      </div>
      <div id="urlbox">
        <center>
          <br />
          <h2>Type in your URL</h2>
          <input type="text" id="newid" />
          <button class="ripple" onclick="setWallpaper()">Apply</button>
        </center>
      </div>
      <div id="colorbox">
        <center>
          <br />
          <h2>Select your color</h2>
          <button class="ripple" onclick="changetaskbarcolor('#992d1f')">
            Red
          </button>
          <button class="ripple" onclick="changetaskbarcolor('#99661f')">
            Orange
          </button>
          <button class="ripple" onclick="changetaskbarcolor('#1f993d')">
            Green
          </button>
          <button class="ripple" onclick="changetaskbarcolor('#0b2247')">
            Blue
          </button>
          <br />
          <button class="ripple" onclick="changetaskbarcolor('#210b38')">
            Purple
          </button>
          <button class="ripple" onclick="changetaskbarcolor('#000000')">
            Black
          </button>
          <br />
        </center>
      </div>
      <div id="winboxbox">
        <center>
          <br />
          <h2>Select your window color</h2>
          <button class="ripple" onclick="setWinboxColor('#992d1f')">
            Red
          </button>
          <button class="ripple" onclick="setWinboxColor('#99661f')">
            Orange
          </button>
          <button class="ripple" onclick="setWinboxColor('#1f993d')">
            Green
          </button>
          <button class="ripple" onclick="setWinboxColor('#0b2247')">
            Blue
          </button>
          <br />
          <button class="ripple" onclick="setWinboxColor('#210b38')">
            Purple
          </button>
          <button class="ripple" onclick="setWinboxColor('#000000')">
            Black
          </button>
          <br />
        </center>
      </div>
      <div id="viewww">
        <center>
          <br />
          <button class="ripple">
            Controller extension (Installed by system)
          </button>
        </center>
      </div>
      <div id="addextension">
        <center>
          <br />
          <input type="text" placeholder="Type a filename" />
          <button class="ripple">Add</button>
        </center>
      </div>
      <div id="duallink">
        <center>
          <br />
          <h2>Extensions</h2>
          <button class="ripple" onclick="extensionview()">
            View installed extensions</button
          ><br /><br />
          <button class="ripple" onclick="addextension()">
            Apply an extension to your system
          </button>
        </center>
      </div>
      <div id="cctrans">
        <center>
          <br />
          <h2>Controllers</h2>
          <h3>Please type in your speed below:</h3>
          <input type="text" placeholder="1 - 20" />
        </center>
      </div>
      <div id="ccuser">
        <center>
          <br />
          <h2>Change or create a user</h2>
          <span id="loginStatus"></span><br /><br />
          <button class="ripple" onclick="envynet()">Use a Zeon account</button
          ><br /><br />
          <button class="ripple" onclick="logout()">
            Log out of your Zeon account</button
          ><br /><br />
          <button class="ripple">Manage your Zeon account</button>
        </center>
      </div>
      <div id="envynet">
        <center>
          <br />
          <h2>Sign into Zeon</h2>
          <button class="ripple" onclick="zeoniframe()">
            Don't have a Zeon account?</button
          ><br /><br />
          <input id="zuser" type="text" placeholder="Username" /><br /><br />
          <input
            id="zpass"
            type="password"
            placeholder="Password"
          /><br /><br />
          <button class="ripple" onclick="loginGUI()">Go!</button>
        </center>
      </div>
      <div id="zeonerr">
        <center>
          <br />
          <img src="./Assets/legacy/icons/info.png" />
          <h3>Some information you entered is incorrect. Please try again.</h3>
        </center>
      </div>
      <div id="zeonsuccess">
        <center>
          <br />
          <img src="./Assets/legacy/icons/info.png" />
          <h2 id="zwelcomename"></h2>
          <h3>Welcome back!</h3>
        </center>
      </div>
      <div id="err">
        <center>
          <br />
          <img src="./Assets/legacy/icons/info.png" width="64" height="64" /><img />
          <!-- May replace info icon with an error icon/SVG (David)-->
          <h1>Oops!</h1>
          <p>
            It seems like Envy has encountered an error. Please ignore this
            unless you have seen this multiple times.
          </p>
        </center>
      </div>
      <div id="resetconfirm">
        <script>
          function noreset() {
            let element = document.querySelector(".winbox #resetconfirm");
            if (element) element = element.closest(".winbox").remove();
          }
          function yesreset() {
            let element = document.querySelector(".winbox #resetconfirm");
            if (element) element = element.closest(".winbox").remove();
            resetinit();
          }
        </script>
        <center>
          <br />
          <img src="./Assets/legacy/icons/info.png" width="64" height="64" /><img />
          <h1>Woah!</h1>
          <p>Are you sure you want to reset your system?</p>
          <p>Resetting your system does the following:</p>
          <p>- Log you out of Zeon</p>
          <p>- Remove all data</p>
          <button class="ripple" onclick="yesreset()">Yes</button
          ><button class="ripple" onclick="noreset()">No</button>
        </center>
      </div>
      <div id="resetinit">
        <center>
          <br />
          <img src="./Assets/legacy/icons/info.png" width="64" height="64" /><img />
          <h1>Resetting system...</h1>
          <h3>Do not exit this page.</h3>
        </center>
      </div>
      <div id="widgallery">
        <center>
          <h4>Quick Dial</h4>
          <button class="ripple" onclick="id3()">Aspen Browser</button><br />
          <button class="ripple" onclick="flipify()">Aspen Flipify</button>
        </center>
      </div>
      <div id="emoji">
        <emoji-picker></emoji-picker>
      </div>
      <div id="content"></div>
    </div>
`;
document.getElementById("userland").appendChild(content);
// todo
// none atm
// Define a variable to keep track of WiFi WinBox status
var isTeresaOpen = false;
var teresaMenu = null; // Variable to store the WinBox instance

// Function to show or close the WiFi WinBox
function openTeresa() {
  // Check if Teresa WinBox is already open
  if (isTeresaOpen) {
    // Close the Teresa WinBox if it's already open
    closeTeresa();
  } else {
    // Open the Teresa WinBox
    teresaBox = new WinBox({
      title: " ",
      class: [
        "no-max",
        "no-min",
        "no-close",
        "no-title",
        "eclipselock",
        "noshadow",
      ],
      x: "15",
      y: "bottom",
      width: "30%",
      height: "55%",
      // top: 0,
      bottom: 70,
      // left: 15,
      mount: document.getElementById("samba").cloneNode(true),
      onclose: function () {
        // Set the WiFi WinBox status to closed when it's closed
        isTeresaOpen = false;
      },
    });

    // Set the teresa WinBox status to open
    isTeresaOpen = true;
  }
}

function closeTeresa() {
  // Check if the teresa WinBox is open before closing
  if (teresaBox) {
    // Close the teresa WinBox
    teresaBox.close();
    // Set the teresa WinBox status to closed
    isTeresaOpen = false;
  }
}

// if (/Mobi/.test(navigator.userAgent) === true)
//  location.replace("mobile/index.html");

function SearchQuery(val) {
  document.getElementById("searchButton").href =
    "https://www.bing.com/search?q=" + val;
}

function buttonClick() {
  document.getElementById("searchBox").value = "";
}

// Define a variable to keep track of WiFi WinBox status
var isWifiOpen = false;
var wifiWinBox = null; // Variable to store the WinBox instance

// Function to show or close the WiFi WinBox
function toggleWifi() {
  // Check if WiFi WinBox is already open
  if (isWifiOpen) {
    // Close the WiFi WinBox if it's already open
    closeWifi();
  } else {
    // Open the WiFi WinBox
    wifiWinBox = new WinBox({
      title: " ",
      class: [
        "no-max",
        "no-min",
        "no-close",
        "no-title",
        "eclipselock",
        "noshadow",
      ],
      x: "right",
      y: "bottom",
      width: "30%",
      height: "36%",
      top: 0,
      right: 15,
      bottom: 70,
      left: 0,
      url: "assets/qs/internetStatus.htm",
      onclose: function () {
        // Set the WiFi WinBox status to closed when it's closed
        isWifiOpen = false;
      },
    });

    // Set the WiFi WinBox status to open
    isWifiOpen = true;
  }
}

// Function to close the WiFi WinBox
function closeWifi() {
  // Check if the WiFi WinBox is open before closing
  if (wifiWinBox) {
    // Close the WiFi WinBox
    wifiWinBox.close();
    // Set the WiFi WinBox status to closed
    isWifiOpen = false;
  }
}

// Example: Assuming you have a WiFi icon that triggers the toggleWifi function


function hideMenu() {
  document.getElementById("contextMenu").style.display = "none";
}

function rightClick(e) {
  e.preventDefault();

  if (document.getElementById("contextMenu").style.display == "flex")
    hideMenu();
  else {
    var menu = document.getElementById("contextMenu");

    menu.style.display = "flex";

    var bcr = menu.getBoundingClientRect();

    // Context menu screen positioning logic
    if (e.pageX > window.innerWidth - bcr.width) {
      menu.style.left = e.pageX - bcr.width + "px";
    } else {
      menu.style.left = e.pageX + "px";
    }
    if (e.pageY > window.innerHeight - bcr.height) {
      menu.style.top = e.pageY - bcr.height + "px";
    } else {
      menu.style.top = e.pageY + "px";
    }
  }
}

let Html;

import("https://unpkg.com/@datkat21/html").then((h) => {
  Html = h.default;

  taskbarBar = new Html("div")
    .styleJs({
      // position: "absolute",
      // left: "400px",
      // bottom: "0",
      display: "flex",
      height: "70px",
      gap: "6px",
      alignItems: "center",
    })
    .appendTo(".bar-placeholder");
});

const taskbarItems = [];

let taskbarBar;

function addTaskbarItem(name, icon = "/dave.jpg", winboxRef = null) {
  let element = new Html("div")
    .styleJs({
      /*backgroundColor: "#2f2f2f",*/
      /*border: "1px solid #2f2f2f",*/
      userSelect: "none",
      /*borderRadius: "6px",*/
      marginRight: "8px",
      display: "flex",
      alignItems: "center",
      gap: "8px",
      flexShrink: "0",
    })
    .appendMany(
      new Html("img")
        .style({ width: "28px", height: "28px" })
        .attr({ src: icon })
      // new Html("span").text(name)
    )
    .appendTo(taskbarBar);

  if (winboxRef !== null) {
    winboxRef.onclose = function () {
      removeTaskbarIem(index);
    };
    winboxRef.onfocus = function () {
      element.styleJs({
        /*backgroundColor: "#2f2f2f",*/
      });
    };
    winboxRef.onblur = function () {
      element.styleJs({
        backgroundColor: "transparent",
      });
    };
    winboxRef.onminimize = function (e) {
      winboxRef.window.style.display = "none";
    };
    element.on("click", (e) => {
      winboxRef.focus();
      winboxRef.window.style.display = "unset";
      if (!winboxRef.max) winboxRef.restore();
    });
  }

  let index = taskbarItems.push({ element, winboxRef }) - 1;

  return index;
}
function removeTaskbarIem(index) {
  let item = taskbarItems[index];
  item.element.cleanup();
  taskbarItems[index] = null;
  // taskbarItems.splice(index, 1);
}

// new WinBox({
//   title: "Welcome to Envy",
//   class: ["no-min", "no-max", "no-title","eclipsemono"],
//   x: "center",
//   y: "center",
//   width: "50%",
//   height: "75%",
//   top: 0,
//   right: 0,
//   bottom: 70,
//   left: 0,
//   modal: true,
//   mount: document.getElementById("teresaoobe").cloneNode(true),
// });

// Envy logon is deprecated and removed.

function showInvidious() {
  let appName = "YouTube",
    appIcon =
      "https://cdn3.iconfinder.com/data/icons/social-network-30/512/social-06-512.png";
  let wb = new WinBox({
    title: appName,
    icon: appIcon,
    icon: "./Assets/legacy/icons/invidious.png",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "https://ivp.cherries.to/",
  });

  addTaskbarItem(appName, appIcon, wb);
}

function showNotepad() {
  new WinBox({
    title: "Notepad",
    icon: "./Assets/legacy/icons/Win-Notepad.png",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "https://zen.unit.ms/",
  });
}

function showVirtualEMU() {
  new WinBox({
    title: "Virtual x86",
    icon: "https://upload.wikimedia.org/wikipedia/commons/thumb/6/6d/Windows_Logo_%281992-2001%29.svg/1385px-Windows_Logo_%281992-2001%29.svg.png",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "https://copy.sh/v86/",
  });
}

function showDiscordWindow() {
  new WinBox({
    title: "Discord",
    icon: "./Assets/legacy/icons/discross.png",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "https://corsproxy.io/?https://www.discord.com/app');",
  });
}

function showWeather() {
  let appName = "Weather",
    appIcon = "./Assets/legacy/icons/wttr.png";
  let wb = new WinBox({
    title: appName,
    icon: appIcon,
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "20%",
    height: "20%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "https://wttr.in/",
  });
  addTaskbarItem(appName, appIcon, wb);
}

function showTab(tabId) {
  // Hide all tabs
  const tabs = document.querySelectorAll(".tab-content");
  tabs.forEach((tab) => tab.classList.remove("active"));

  // Show the selected tab
  const selectedTab = document.getElementById(tabId);
  selectedTab.classList.add("active");
}

function showTelegram() {
  let appName = "JS/Linux",
    appIcon = "./Assets/legacy/icons/term.png";

  let wb = new WinBox({
    title: appName,
    icon: appIcon,
    class: ["no-min", "eclipsemono"],
    x: "center",
    y: "center",
    width: "50%",
    height: "80%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "https://bellard.org/jslinux/vm.html?url=alpine-x86.cfg&mem=270",
  });

  addTaskbarItem(appName, appIcon, wb);
}

function showmoreTelegram() {
  new WinBox({
    title: "test",
    icon: "./Assets/legacy/icons/telegram_700x700.png",
    class: ["no-min", "eclipsemono"],
    x: "center",
    y: "center",
    width: "50%",
    height: "80%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "https://bellard.org/jslinux/vm.html?url=win2k.cfg&mem=512&graphic=1&w=800&h=700",
  });
}

function showDualWindow() {
  new WinBox({
    title: "Dual",
    class: ["no-min", "eclipsemono"],
    x: "center",
    y: "center",
    width: "75%",
    height: "75%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: location.href,
  });
}

function useSambaHelp() {
  let appName = "Envy Help",
    appIcon = "./Assets/legacy/apps/help/icon.png";

  let wb = new WinBox({
    title: appName,
    icon: appIcon,
    class: ["eclipsemono"],
    x: "center",
    y: "center",
    width: "50%",
    height: "70%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("sambahelp").cloneNode(true),
  });

  addTaskbarItem(appName, appIcon, wb);
}

function manageDual() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "left",
    y: "bottom",
    width: "100%",
    height: "17%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("teresadual").cloneNode(true),
  });
}

function showlogin() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-close", "no-title", "eclipselock"],
    x: "left",
    y: "bottom",
    width: "100%",
    height: "100%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 0,
    left: 0,
    modal: true,
    mount: document.getElementById("teresalogin").cloneNode(true),
  });
}

function showlock() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-close", "no-title", "eclipselock"],
    x: "left",
    y: "bottom",
    width: "100%",
    height: "100%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 0,
    left: 0,
    modal: true,
    mount: document.getElementById("teresalock").cloneNode(true),
  });
}

function setWallpaper() {
  localStorage.setItem("bg", document.querySelector(".winbox #newid").value);
  document.body.style.backgroundImage =
    'url("' + document.querySelector(".winbox #newid").value + '")';
}

function dark() {
  localStorage.setItem("bg", "./assets/wp.jpg");
  document.body.style.backgroundImage = 'url("' + "assets/wp.jpg" + '")';
  document.querySelector(".bar").style.backgroundColor = "#0000004f";
  setWinboxColor("rgba(var(--bg-color-darker), 0.5)");
  document.documentElement.setAttribute("data-theme", "dark");
}

function light() {
  localStorage.setItem("bg", "./assets/wp2.png");
  document.body.style.backgroundImage = 'url("' + "assets/wp.jpg" + '")';
  document.querySelector(".bar").style.backgroundColor = "#e6e6e6a0";
  setWinboxColor("#e6e6e6");
  // add data-theme to html
  document.documentElement.setAttribute("data-theme", "light");
}

function qson() {
  document.documentElement.setAttribute("qsvisible", "true");
}

function qsoff() {
  document.documentElement.setAttribute("qsvisible", "false");
}

function setWinboxColor(color) {
  var e = `.winbox.eclipsemono { background-color: ${color} !important;`;
  var s = document.querySelector("#s") || document.createElement("style");
  s.innerHTML = e;
  s.id = "s";
  document.head.appendChild(s);
}

function changetaskbarcolor(color) {
  document.querySelector(".bar").style.backgroundColor = color;
}

function shownewsearch() {
  new WinBox({
    title: "Search results",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "left",
    y: "bottom",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "https://example.com",
  });
}

function summongamehub() {
  new WinBox({
    title: "Gaming",
    icon: "./Assets/legacy/icons/game.png",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "apps/gamehub.html",
  });
}

function widgetinit() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "15%",
    height: "30%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("teresadrei").cloneNode(true),
  });
}

function showall() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "30%",
    height: "20%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("teresaplus").cloneNode(true),
  });
}

function urlbox() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "25%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("urlbox").cloneNode(true),
  });
}

function colorbox() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "35%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("colorbox").cloneNode(true),
  });
}

function winboxbox() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "35%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("winboxbox").cloneNode(true),
  });
}

function addextension() {
  new WinBox({
    title: "Select an extension",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "40%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("addextension").cloneNode(true),
  });
}

function extensionview() {
  new WinBox({
    title: "Installed extensions",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "40%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("viewww").cloneNode(true),
  });
}

function duallink() {
  new WinBox({
    title: "Envy extensions",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "35%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("duallink").cloneNode(true),
  });
}

function imgbox() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "25%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("imgbox").cloneNode(true),
  });
}

function community() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "25%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("communitywall").cloneNode(true),
  });
}

function envynet() {
  new WinBox({
    title: "Use a Zeon account",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "30%",
    height: "40%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("envynet").cloneNode(true),
  });
}

function zeoniframe() {
  new WinBox({
    title: "Zeon",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "35%",
    height: "65%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "https://zeon.dev/signup",
  });
}

function music() {
  let appName = "Envy Music",
    appIcon = "./Assets/legacy/icons/music.png";

  let wb = new WinBox({
    title: appName,
    icon: appIcon,
    class: ["no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "./apps/music/index.html",
  });

  let index = addTaskbarItem(appName, appIcon, wb);
}

function aspenplus() {
  let appName = "Aspen Network",
    appIcon = "./Assets/legacy/apps/aspen/favicon.png";

  let wb = new WinBox({
    title: appName,
    icon: appIcon,
    class: ["no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("aspenplus").cloneNode(true),
  });

  let index = addTaskbarItem(appName, appIcon, wb);
}

function flipify() {
  let appName = "Aspen Flipify",
    appIcon = "./Assets/legacy/icons/aspen.webp";

  let wb = new WinBox({
    title: appName,
    icon: appIcon,
    class: ["no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "https://envyjs.github.io/flipify",
  });

  let index = addTaskbarItem(appName, appIcon, wb);
}

function showClock() {
  new WinBox({
    title: "Clock",
    icon: "./Assets/legacy/icons/clock.png",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "15%",
    height: "14%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("clock").cloneNode(true),
  });
}

function showlog() {
  new WinBox({
    title: "Credits",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("log").cloneNode(true),
  });
}

function showmore() {
  new WinBox({
    title: "Advanced information",
    class: ["eclipsemono"],
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("more").cloneNode(true),
  });
}

function showSettingsWindow() {
  let appName = "Settings",
    appIcon = "./Assets/legacy/apps/prefs/favicon.png";

  const wb = new WinBox({
    title: appName,
    icon: appIcon,
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "70%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("modernprefs").cloneNode(true),
  });

  addTaskbarItem(appName, appIcon, wb);
}
/* note to anyone who needs it, legacy control center is id "cc" */

function showAboutWindow() {
  let appName = "About this device",
    appIcon = "./Assets/legacy/icons/info.png";
  let wb = new WinBox({
    title: appName,
    class: ["no-max", "no-min", "eclipsemono"],
    x: "center",
    y: "center",
    width: "30%",
    height: "28%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("about").cloneNode(true),
  });

  addTaskbarItem(appName, appIcon, wb);
}

function shownet() {
  new WinBox({
    title: "Overview of this device",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "30%",
    height: "30%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("net").cloneNode(true),
  });
}

function displaypanel() {
  new WinBox({
    title: "Display options",
    class: ["no-min", "eclipsemono"],
    x: "center",
    y: "center",
    width: "27%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("ccdisplay").cloneNode(true),
  });
}

function taskpanel() {
  new WinBox({
    title: "Taskbar options",
    class: ["no-min", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "45%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("cctask").cloneNode(true),
  });
}

function accesspanel() {
  new WinBox({
    title: "Accessbility options",
    class: ["no-min", "no-max", "eclipsemono"],
    x: "center",
    y: "center",
    width: "27%",
    height: "40%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("ccaccess").cloneNode(true),
  });
}

function interpanel() {
  new WinBox({
    title: "International options",
    class: ["no-min", "no-max", "eclipsemono"],
    x: "center",
    y: "center",
    width: "27%",
    height: "40%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("ccinter").cloneNode(true),
  });
}

function backuppanel() {
  new WinBox({
    title: "Backup and reset options",
    class: ["no-min", "no-max", "eclipsemono"],
    x: "center",
    y: "center",
    width: "27%",
    height: "40%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("ccbackup").cloneNode(true),
  });
}

function backup2panel() {
  new WinBox({
    title: "Backup and reset options",
    class: ["no-min", "no-max", "eclipsemono"],
    x: "center",
    y: "center",
    width: "27%",
    height: "40%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("backup2").cloneNode(true),
  });
}

function transpanel() {
  new WinBox({
    title: "Controller options",
    class: ["no-max", "no-min", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "40%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("cctrans").cloneNode(true),
  });
}

function userpanel() {
  new WinBox({
    title: "User options",
    class: ["no-max", "no-min", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("ccuser").cloneNode(true),
  });
  loadLoginStatus();
}

function showstore() {
  let appName = "Envy Store",
    appIcon = "./Assets/legacy/apps/store/icon.png";
  let wb = new WinBox({
    title: appName,
    icon: appIcon,
    class: ["eclipsemono"],
    x: "center",
    y: "center",
    width: "50%",
    height: "70%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("smbmetropole").cloneNode(true),
  });

  addTaskbarItem(appName, appIcon, wb);
}

function showmark() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "right",
    y: "bottom",
    width: "20%",
    height: "100%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("TeresaMark").cloneNode(true),
  });
}

function showerror() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "33%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("err").cloneNode(true),
  });
}

function files() {
  new WinBox({
    title: "File Explorer",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "30%",
    height: "35%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("files").cloneNode(true),
  });
}

function resetconfirm() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-close", "eclipsemono"],
    x: "center",
    y: "center",
    width: "20%",
    height: "70%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    modal: true,
    mount: document.getElementById("resetconfirm").cloneNode(true),
  });
}

function resetinit() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-close", "eclipsemono"],
    x: "center",
    y: "center",
    width: "30%",
    height: "40%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    modal: true,
    mount: document.getElementById("resetinit").cloneNode(true),
  });
  localStorage.clear();
  logout();
  location.reload();
}

// Main window section over
// Widget section start under this line

function widgetgallery() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "right",
    y: "bottom",
    width: "10%",
    height: "22%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("widgallery").cloneNode(true),
  });
}

function emoji() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "right",
    y: "bottom",
    width: "19%",
    height: "51%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    mount: document.getElementById("emoji").cloneNode(true),
  });
}

function calc() {
  let appName = "Calculator",
    appIcon = "./Assets/legacy/apps/calc/favicon.png";
  let wb = new WinBox({
    title: appName,
    icon: appIcon,
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "27%",
    height: "70%",
    top: 0,
    right: 0,
    bottom: 70,
    left: 0,
    url: "./Assets/legacy/apps/calc/index.html",
  });
  let index = addTaskbarItem(appName, appIcon, wb);
}

function myFunction() {
  var x = document.getElementById("ur").value;
  document.getElementById("demo").innerHTML = x;
  document.getElementById("myIframe").src = x;
}

let chooseFolder = async () => {
  const dirHandle = await window.showDirectoryPicker();
  for await (const entry of dirHandle.values()) {
    let elmnt = document.createElement("div");
    if (entry.kind === "file") {
      const file = await entry.getFile();
      console.log(file.name);
      elmnt.innerHTML = file.name;
      // const text = await file.text();
      // console.log(text);
    }
    if (entry.kind === "directory") {
      /* for file in this directory do something */
      console.log(entry.name);
      elmnt.innerHTML = entry.name;
    }
    document.querySelector(".winbox #files #filelist").appendChild(elmnt);
  }
};
function createRipple(event) {
  const button = event.currentTarget;

  const circle = document.createElement("span");
  const diameter = Math.max(button.clientWidth, button.clientHeight);
  const radius = diameter / 2;

  circle.style.width = circle.style.height = `${diameter}px`;
  circle.style.left = `${event.clientX - button.offsetLeft - radius}px`;
  circle.style.top = `${event.clientY - button.offsetTop - radius}px`;
  circle.classList.add("ripple");

  const ripple = button.getElementsByClassName("ripple")[0];

  if (ripple) {
    ripple.remove();
  }

  button.appendChild(circle);
}

function checkConnection() {
  var statusElement = document.getElementById("status");
  var iconElement = document.getElementById("icon");

  // Check if the navigator is online
  if (navigator.onLine) {
    statusElement.textContent = "Envy is connected to the Internet";
    statusElement.classList.add("connected");
    statusElement.classList.remove("disconnected");
    iconElement.src = "wifi.svg";
  } else {
    statusElement.textContent = "Envy is not connected to the Internet";
    statusElement.classList.add("disconnected");
    statusElement.classList.remove("connected");
    iconElement.src = "nowifi.png";
  }
}

var slider = document.getElementById("myRange");
var output = document.getElementById("demo");

function id1() {
    let appName = "Piskel",
      appIcon = "assets/mtrui/apps/id1/favicon.png";
    let wb = new WinBox({
      title: appName,
      icon: appIcon,
      class: "eclipsemono",
      x: "center",
      y: "center",
      width: "50%",
      height: "50%",
      top: 0,
      right: 0,
      bottom: 70,
      left: 0,
      url: "https://www.piskelapp.com/p/create/sprite",
    });
    let index = addTaskbarItem(appName, appIcon, wb);
  }
  
  function id2() {
    new WinBox({
      title: "Localhost Viewer",
      class: "eclipsemono",
      x: "center",
      y: "center",
      width: "50%",
      height: "50%",
      top: 0,
      right: 0,
      bottom: 70,
      left: 0,
      url: "http://localhost:3000",
    });
  }
  
  function id3() {
    let appName = "Browser",
      appIcon = "assets/mtrui/apps/id3/favicon.png";
    let wb = new WinBox({
      title: appName,
      icon: appIcon,
      class: ["eclipsemono"],
      x: "center",
      y: "center",
      width: "50%",
      height: "50%",
      top: 0,
      right: 0,
      bottom: 70,
      left: 0,
      mount: document.getElementById("TeresaMark").cloneNode(true),
    });
  
    addTaskbarItem(appName, appIcon, wb);
  }
  
  function id4() {
    let appName = "Eaglercraft",
      appIcon = "assets/mtrui/apps/id4/favicon.png";
    let wb = new WinBox({
      title: appName,
      icon: appIcon,
      class: "eclipsemono",
      x: "center",
      y: "center",
      width: "50%",
      height: "50%",
      top: 0,
      right: 0,
      bottom: 70,
      left: 0,
      url: "https://eaglercraft.com/",
    });
    addTaskbarItem(appName, appIcon, wb);
  }
  
  function id5() {
    console.log("id5 no longer exist.");
  }
  
  function id6() {
    let wb = new WinBox({
      title: "Calculator",
      class: "eclipsemono",
      x: "center",
      y: "center",
      width: "27%",
      height: "70%",
      top: 0,
      right: 0,
      bottom: 70,
      left: 0,
      url: "http://freshman-calculator.surge.sh/",
    });
  }
  
  function id7() {
    let appName = "Virtual x86",
      appIcon = "assets/mtrui/apps/id7/favicon.png";
    let wb = new WinBox({
      title: appName,
      icon: appIcon,
      class: "eclipsemono",
      x: "center",
      y: "center",
      width: "27%",
      height: "70%",
      top: 0,
      right: 0,
      bottom: 70,
      left: 0,
      url: "https://copy.sh/v86/",
    });
    addTaskbarItem(appName, appIcon, wb);
  }
  
  function id8() {
    let appName = "Notepad",
      appIcon = "assets/mtrui/apps/id8/favicon.png";
    let wb = new WinBox({
      title: appName,
      icon: appIcon,
      class: "eclipsemono",
      x: "center",
      y: "center",
      width: "27%",
      height: "70%",
      top: 0,
      right: 0,
      bottom: 70,
      left: 0,
      url: "https://zen.unit.ms/",
    });
    addTaskbarItem(appName, appIcon, wb);
  }
  
  function id9() {
    let appName = "Maps (Beta)",
      appIcon = "icons/maps.webp";
  
    let wb = new WinBox({
      title: appName,
      icon: appIcon,
      class: ["no-title", "eclipsemono"],
      x: "center",
      y: "center",
      width: "50%",
      height: "50%",
      top: 0,
      right: 0,
      bottom: 70,
      left: 0,
      url: "https://maps.google.com/maps?width=100%25&amp;height=200&amp;hl=en&amp;t=&amp;z=1&amp;ie=UTF8&amp;iwloc=B&amp;output=embed",
    });
  
    let index = addTaskbarItem(appName, appIcon, wb);
  }
  
  function id10() {
    let appName = "bClock",
      appIcon = "assets/mtrui/apps/placeholder/favicon.png";
    let wb = new WinBox({
      title: appName,
      icon: appIcon,
      class: "eclipsemono",
      x: "center",
      y: "center",
      width: "25%",
      height: "75%",
      top: 0,
      right: 0,
      bottom: 70,
      left: 0,
      url: "./assets/apps/bClock.html",
    });
    let index = addTaskbarItem(appName, appIcon, wb);
  }
  
  function id12() {
    let appName = "PCSXJS",
      appIcon = "assets/mtrui/apps/placeholder/favicon.png";
    let wb = new WinBox({
      title: appName,
      icon: appIcon,
      class: "eclipsemono",
      x: "center",
      y: "center",
      width: "25%",
      height: "75%",
      top: 0,
      right: 0,
      bottom: 70,
      left: 0,
      url: "https://tjwei.github.io/pcsxjs/",
    });
    let index = addTaskbarItem(appName, appIcon, wb);
  }
  
  
  function id13() {
    let appName = "Envy Mobile Shell",
      appIcon = "assets/mtrui/apps/id12/favicon.png";
    let wb = new WinBox({
      title: appName,
      icon: appIcon,
      class: "eclipsemono",
      x: "center",
      y: "center",
      width: "25%",
      height: "75%",
      top: 0,
      right: 0,
      bottom: 70,
      left: 0,
      url: "./apps/mobile/index.html",
    });
    let index = addTaskbarItem(appName, appIcon, wb);
  }
  console.log("[EWBL] eWinBox layer loaded");