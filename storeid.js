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
