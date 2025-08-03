async function login(u, p) {
    return await fetch("https://zeon.dev/api/public/login", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        // Add any additional headers needed for authentication or other purposes
      },
      body: JSON.stringify({
        // Add your request data in the appropriate format
        username: u,
        password: p,
      }),
    })
      .then((response) => response.json())
      .then((data) => {
        if (data.status !== 200) return data;

        if (!sessionStorage.getItem("userData")) {
          sessionStorage.setItem("userData", JSON.stringify(data.info));
          checkUserData();
          return data;
        } else {
          return data;
        }
      })
      .catch((error) => {
        // Handle any errors that occur during the request
        console.error("the error occurred", error);
      });
  }

  function logout() {
    sessionStorage.removeItem("userData");
    checkUserData();
  }

  const stockUserData = {
    username: "User",
    pfp: "./assets/user-avatar.svg",
    id: -1,
    email: null,
    onlineAccount: false,
  };

  let currentUserData = {
    username: "User",
    pfp: "./assets/user-avatar.svg",
    id: -1,
    email: null,
    onlineAccount: sessionStorage.getItem("userData") !== null,
  };

  function checkUserData() {
    if (sessionStorage.getItem("userData") !== null) {
      try {
        const data = JSON.parse(sessionStorage.getItem("userData"));
        if (data.user && data.pfp && data.id && data.email) {
          currentUserData.username = data.user;
          currentUserData.pfp = data.pfp.replace("/", "https://zeon.dev/");
          currentUserData.id = data.id;
          currentUserData.email = data.email;
          currentUserData.onlineAccount = true;
        }
      } catch (e) {}
    } else {
      Object.assign(currentUserData, stockUserData);
    }
  }

let loginGUI = async () => {
    await login(document.querySelector(".winbox #zuser").value, document.querySelector(".winbox #zpass").value)
    // alert("Logged in!\n" + JSON.stringify(sessionStorage.getItem("userData")))
    if(sessionStorage.getItem("userData") === null) {
      new WinBox({
        title: " ",
        class: ["no-max", "no-min", "no-title", "eclipsemono"],
        x: "center",
        y: "center",
        width: "30%",
        height: "20%",
        header: 0,
        top: 0,
        right: 0,
        bottom: 45,
        left: 0,
        modal: true,
        mount: document.getElementById("zeonerr").cloneNode(true),
      });
    } else {
      var zwb = new WinBox({
        title: " ",
        class: ["no-max", "no-min", "no-title", "eclipsemono"],
        x: "center",
        y: "center",
        width: "30%",
        height: "27%",
        header: 0,
        top: 0,
        right: 0,
        bottom: 45,
        left: 0,
        modal: true,
        mount: document.getElementById("zeonsuccess").cloneNode(true),
      });
      zwb.body.querySelectorAll("#zwelcomename").forEach(zw => { zw.innerHTML = `Hi, ${JSON.parse(sessionStorage.getItem("userData")).user}!`; });
    }
    
  }

let oobelogin = async () => {
    await login(document.querySelector(".winbox #zuser").value, document.querySelector(".winbox #zpass").value)
    if(sessionStorage.getItem("userData") === null) {
      new WinBox({
        title: " ",
        class: ["no-max", "no-min", "no-title", "eclipsemono"],
        x: "center",
        y: "center",
        width: "25%",
        height: "20%",
        header: 0,
        top: 0,
        right: 0,
        bottom: 45,
        left: 0,
        modal: true,
        mount: document.getElementById("zeonerr").cloneNode(true),
      });
    } else {
      var zwb = new WinBox({
        title: " ",
        class: ["no-max", "no-min", "no-title", "eclipsemono"],
        x: "center",
        y: "center",
        width: "25%",
        height: "27%",
        header: 0,
        top: 0,
        right: 0,
        bottom: 45,
        left: 0,
        modal: true,
        mount: document.getElementById("zeonsuccess").cloneNode(true),
      });
      zwb.body.querySelectorAll("#zwelcomename").forEach(zw => { zw.innerHTML = `Hi, ${JSON.parse(sessionStorage.getItem("userData")).user}!`; });
    }
    finishzeonsetup()
  }


let loadLoginStatus = () => {
  console.log(sessionStorage.getItem("userData") !== undefined | null, sessionStorage.getItem("userData"))
  document.querySelector(".winbox #loginStatus").innerHTML = sessionStorage.getItem("userData") === null ? "Not logged in!" : "Logged in!"
}
