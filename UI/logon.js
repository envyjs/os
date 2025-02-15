var content = document.createElement("div");
content.innerHTML = `
<div id="loginbg">
<div id="login">
    <div id="userlogintile" style="height: 200px; width: 250px; border: 1px solid #fff">
        <img src="./Assets/Kot.jpg" height="40px" style="z-index: 99 !important; margin-left: 10px; margin-top: 10px">
        <h2 style="line-height: 10px; margin-left: 10px">User</h2>
    </div>
    <div id="userlogintile2" style="height: 200px; width: 250px; border: 1px solid #fff">
    <img src="./Assets/Kot.jpg" height="40px" style="z-index: 99 !important; margin-left: 10px; margin-top: 10px">
    <h2 style="line-height: 10px; margin-left: 10px">User</h2>
    </div>
</div>
<button style="position: absolute; bottom: 10px; left: 50%; -webkit-transform: translate(-50%, -50%); transform: translate(-50%, -50%);" onclick="loadScript('./UI/desktop.js');">Start guest session</button>
</div>
`;
document.getElementById("userland").appendChild(content);

// <img src="./Assets/Wave.jpg" style="filter: blur(4px);">