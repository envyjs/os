var content = document.createElement("div");
content.innerHTML = `
<div class="watermark" style="top: 20px">
<p>Developer mode is enabled. Software integrity cannot be verified.<p>
</div>
`;
document.getElementById("userland").appendChild(content);