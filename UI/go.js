var content = document.createElement("div");
content.innerHTML = `
<div id="goguide">
<img src="../Assets/goguide/back.svg"></img>
<img src="../Assets/envy.svg"></img>
<img src="../Assets/goguide/search.svg"></img>
</div>
`;
document.getElementById("userland").appendChild(content);
loadScript('./Registry/Local/Envy/DefaultShell/Go.js');
