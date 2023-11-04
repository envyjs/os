// todo
// none atm
let hasClicked = false;
document.addEventListener("click", (e) => {
  if (!hasClicked) {
    hasClicked = true;
    document.querySelector("audio").play();
  }
});

// if (/Mobi/.test(navigator.userAgent) === true)
//  location.replace("mobile/index.html");

function SearchQuery(val) {
  document.getElementById("searchButton").href =
    "https://www.bing.com/search?q=" + val;
}

function buttonClick() {
  document.getElementById("searchBox").value = "";
}

document.onclick = hideMenu;
document.oncontextmenu = rightClick;

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
      height: "45px",
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
      backgroundColor: "#2f2f2f",
      border: "1px solid #2f2f2f",
      userSelect: "none",
      borderRadius: "6px",
      marginRight: "8px",
      display: "flex",
      alignItems: "center",
      gap: "8px",
      flexShrink: "0",
    })
    .appendMany(
      new Html("img")
        .style({ width: "28px", height: "28px" })
        .attr({ src: icon }),
      new Html("span").text(name)
    )
    .appendTo(taskbarBar);

  if (winboxRef !== null) {
    winboxRef.onclose = function () {
      removeTaskbarIem(index);
    };
    winboxRef.onfocus = function () {
      element.styleJs({
        backgroundColor: "#2f2f2f",
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

new WinBox({
  title: "Welcome to Envy",
  class: ["no-min", "no-max", "no-close", "eclipsemono"],
  x: "center",
  y: "center",
  width: "50%",
  height: "75%",
  top: 0,
  right: 0,
  bottom: 45,
  left: 0,
  modal: true,
  mount: document.getElementById("teresaoobe").cloneNode(true),
});

function showInvidious() {
  let appName = "YouTube",
    appIcon =
      "https://cdn3.iconfinder.com/data/icons/social-network-30/512/social-06-512.png";
  let wb = new WinBox({
    title: appName,
    icon: appIcon,
    icon: "icons/invidious.png",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    url: "https://ivp.cherries.to/",
  });

  addTaskbarItem(appName, appIcon, wb);
}

function showNotepad() {
  new WinBox({
    title: "Notepad",
    icon: "icons/Win-Notepad.png",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 45,
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
    bottom: 45,
    left: 0,
    url: "https://copy.sh/v86/",
  });
}

function showDiscordWindow() {
  new WinBox({
    title: "Discord",
    icon: "icons/discross.png",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    url: "https://corsproxy.io/?https://www.discord.com/app');",
  });
}

function showotherSearch() {
  new WinBox({
    title: "Envy search",
    class: ["no-min", "no-max", "no-title", "eclipsemono"],
    x: "left",
    y: "bottom",
    width: "45%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    mount: document.getElementById("teresasearch").cloneNode(true),
  });
}

function showWeather() {
  let appName = "Weather",
    appIcon = "icons/wttr.png";
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
    bottom: 45,
    left: 0,
    url: "https://wttr.in/",
  });
  addTaskbarItem(appName, appIcon, wb);
}

function showTelegram() {
  let appName = "JS/Linux",
    appIcon = "icons/term.png";

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
    bottom: 45,
    left: 0,
    url: "https://bellard.org/jslinux/vm.html?url=alpine-x86.cfg&mem=245",
  });

  addTaskbarItem(appName, appIcon, wb);
}

function showmoreTelegram() {
  new WinBox({
    title: "test",
    icon: "icons/telegram_450x450.png",
    class: ["no-min", "eclipsemono"],
    x: "center",
    y: "center",
    width: "50%",
    height: "80%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    url: "https://bellard.org/jslinux/vm.html?url=win2k.cfg&mem=512&graphic=1&w=800&h=600",
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
    bottom: 45,
    left: 0,
    url: location.href,
  });
}

function showVidstest() {
  new WinBox({
    title: "VidsTest",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    url: "apps/vidstest.html",
  });
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
    bottom: 45,
    left: 0,
    mount: document.getElementById("teresadual").cloneNode(true),
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
    mount: document.getElementById("teresalock").cloneNode(true),
  });
}

function setWallpaper() {
  localStorage.setItem("bg", document.querySelector(".winbox #newid").value);
  document.body.style.backgroundImage =
    'url("' + document.querySelector(".winbox #newid").value + '")';
}

function dark() {
  localStorage.setItem("bg", "wp.png");
  document.body.style.backgroundImage = 'url("' + "wp.png" + '")';
  document.querySelector(".bar").style.backgroundColor = "#000000";
  setWinboxColor("#000000");
  document.documentElement.setAttribute("data-theme", "dark");
}

function light() {
  localStorage.setItem("bg", "wp2.png");
  document.body.style.backgroundImage = 'url("' + "wp2.png" + '")';
  document.querySelector(".bar").style.backgroundColor = "#e6e6e6";
  setWinboxColor("#e6e6e6");
  // add data-theme to html
  document.documentElement.setAttribute("data-theme", "light");
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
    bottom: 45,
    left: 0,
    url: "https://example.com",
  });
}

function summongamehub() {
  new WinBox({
    title: "Gaming",
    icon: "icons/game.png",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 45,
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
    bottom: 45,
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
    bottom: 45,
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
    bottom: 45,
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
    bottom: 45,
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
    bottom: 45,
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
    bottom: 45,
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
    bottom: 45,
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
    bottom: 45,
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
    bottom: 45,
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
    bottom: 45,
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
    bottom: 45,
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
    bottom: 45,
    left: 0,
    url: "https://zeon.dev/signup",
  });
}

function aspenplus() {
  let appName = "Aspen Network",
    appIcon = "icons/aspen.png";

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
    bottom: 45,
    left: 0,
    mount: document.getElementById("aspenplus").cloneNode(true),
  });

  let index = addTaskbarItem(appName, appIcon, wb);
}

function flipify() {
  let appName = "Aspen Flipify",
    appIcon = "icons/aspen.png";

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
    bottom: 45,
    left: 0,
    url: "https://envyjs.github.io/flipify",
  });

  let index = addTaskbarItem(appName, appIcon, wb);
}

function showClock() {
  new WinBox({
    title: "Clock",
    icon: "icons/clock.png",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "15%",
    height: "14%",
    top: 0,
    right: 0,
    bottom: 45,
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
    bottom: 45,
    left: 0,
    mount: document.getElementById("log").cloneNode(true),
  });
}

function shownet() {
  new WinBox({
    title: "About Aspen Network",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "center",
    y: "center",
    width: "20%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    mount: document.getElementById("net").cloneNode(true),
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
    bottom: 45,
    left: 0,
    mount: document.getElementById("more").cloneNode(true),
  });
}

function showSettingsWindow() {
  let appName = "Settings",
    appIcon = "icons/setting.png";

  const wb = new WinBox({
    title: appName,
    icon: appIcon,
    class: ["no-max", "eclipsemono"],
    x: "center",
    y: "center",
    width: "30%",
    height: "30%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    mount: document.getElementById("cc").cloneNode(true),
  });

  addTaskbarItem(appName, appIcon, wb);
}

function showAboutWindow() {
  let appName = "About this device",
    appIcon = "icons/info.png";
  let wb = new WinBox({
    title: appName,
    class: ["no-max", "no-min", "eclipsemono"],
    x: "center",
    y: "center",
    width: "25%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    mount: document.getElementById("about").cloneNode(true),
  });

  addTaskbarItem(appName, appIcon, wb);
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
    bottom: 45,
    left: 0,
    mount: document.getElementById("ccdisplay").cloneNode(true),
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
    bottom: 45,
    left: 0,
    mount: document.getElementById("ccaccess").cloneNode(true),
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
    bottom: 45,
    left: 0,
    mount: document.getElementById("ccbackup").cloneNode(true),
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
    bottom: 45,
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
    bottom: 45,
    left: 0,
    mount: document.getElementById("ccuser").cloneNode(true),
  });
  loadLoginStatus()
}

function showBooWindow() {
  new WinBox({
    title: " ",
    class: ["no-max", "no-min", "no-title", "eclipsemono"],
    x: "left",
    y: "bottom",
    width: "7%",
    height: "60%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    mount: document.getElementById("Teresa").cloneNode(true),
  });
}

function showstore() {
  let appName = "Envy Store",
    appIcon = "icons/shop.png";
  let wb = new WinBox({
    title: appName,
    icon: appIcon,
    class: ["eclipsemono"],
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    mount: document.getElementById("TeresaZwei").cloneNode(true),
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
    bottom: 45,
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
    bottom: 45,
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
    bottom: 45,
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
    height: "40%",
    header: 0,
    top: 0,
    right: 0,
    bottom: 45,
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
    bottom: 45,
    left: 0,
    modal: true,
    mount: document.getElementById("resetinit").cloneNode(true),
  });
  localStorage.clear()
  location.reload()
}

function timeUpdate() {
  // 24-hour time example
  let x = new Date();
  let hours = x.getHours().toString().padStart(2, "0"); // 18
  let minutes = x.getMinutes().toString().padStart(2, "0"); // 37
  let seconds = x.getSeconds().toString().padStart(2, "0"); // 01
  let timeString = `${hours}:${minutes}:${seconds}`; // 18:37:01
  let element = document.getElementById("clock"); // assuming <... id="clock" ...>
  element.innerText = timeString;
}
timeUpdate();
setInterval(timeUpdate, 1000); // 1000ms -> 1 second delay

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
    bottom: 45,
    left: 0,
    mount: document.getElementById("widgallery").cloneNode(true),
  });
}

document.addEventListener("DOMContentLoaded", () => {
  if (localStorage.getItem("bg") === undefined || !localStorage.getItem("bg")) {
    localStorage.setItem("bg", "wp.png");
  }
  document.body.style.backgroundImage =
    'url("' + localStorage.getItem("bg") + '")';
});

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
