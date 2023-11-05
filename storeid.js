function id1() {
  new WinBox({
    title: "Piskel for Envy",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    url: "https://www.piskelapp.com/p/create/sprite",
  });
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
    bottom: 45,
    left: 0,
    url: "http://localhost:3000",
  });
}

function id3() {
  let appName = "Aspen Browser",
    appIcon = "icons/aspen.png";
  let wb = new WinBox({
    title: appName,
    icon: "icons/aspen.png",
    class: ["eclipsemono"],
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    mount: document.getElementById("TeresaMark").cloneNode(true),
  });

  addTaskbarItem(appName, appIcon, wb);
}

function id4() {
  new WinBox({
    title: "Eaglercraft",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    url: "https://eaglercraft.com/",
  });
}

function id5() {
  new WinBox({
    title: "Resent Client",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "50%",
    height: "50%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    url: "https://reslauncher.vercel.app/",
  });
}

function id6() {
  new WinBox({
    title: "Calculator",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "27%",
    height: "70%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    url: "http://freshman-calculator.surge.sh/",
  });
}

function id7() {
  new WinBox({
    title: "Virtual x86",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "27%",
    height: "70%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    url: "https://copy.sh/v86/",
  });
}

function id8() {
  new WinBox({
    title: "Notepad Legacy",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "27%",
    height: "70%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    url: "https://zen.unit.ms/",
  });
}

function id9() {
  let appName = "Maps (Beta)",
    appIcon = "https://static.vecteezy.com/system/resources/previews/016/716/478/original/google-maps-icon-free-png.png";

  function DS_Login() {
   new WinBox({
    title: "DS Login",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "30%",
    height: "70%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    url: "https://login.davidstudios.uk",
  });
  }

  function id10() {
   new WinBox({
    title: "Word Game",
    class: "eclipsemono",
    x: "center",
    y: "center",
    width: "36%",
    height: "65%",
    top: 0,
    right: 0,
    bottom: 45,
    left: 0,
    url: "https://wordgame.davidstudios.uk/?src=envywbos&iframe=true",
  });
  }

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
    url: "https://maps.google.com/maps?width=100%25&amp;height=200&amp;hl=en&amp;q=1+()&amp;t=&amp;z=1&amp;ie=UTF8&amp;iwloc=B&amp;output=embed",
  });

  let index = addTaskbarItem(appName, appIcon, wb);
}
